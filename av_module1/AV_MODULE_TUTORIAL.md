# av_module 音视频通话 SDK 实现与使用教程

## 1. 设计目标

- 高内聚、低耦合：所有实现细节封装于模块内部，外部只需操作 `AvEngine`。
- 可扩展：后续可无缝集成人脸识别、录制、截图等功能，互不干扰。
- 商业级架构：公共 API 与私有实现严格分离，便于维护和升级。
- 跨平台策略：当前主力支持 Linux（Allwinner T113-S3），依赖 jrtplib、ALSA。

## 2. 目录结构

```text
av_module/
├── av_module.pri           # qmake 构建配置，include 到你的 .pro
├── include/                # 仅对外头文件
│   ├── av_types.h          # CallState/AvConfig 配置与状态
│   ├── av_engine.h         # AvEngine 门面类（唯一对外接口）
│   └── video_widget.h      # 视频显示控件
└── src/                    # 内部实现（外部无需关心）
    ├── av_engine.cpp / av_engine_p.h
    ├── call_state_machine.h/.cpp
    ├── signaling_protocol.h
    ├── signaling_client.h/.cpp
    ├── rtp_session.h/.cpp
    ├── video_pipeline.h/.cpp
    ├── audio_pipeline.h/.cpp
    ├── pcmu_codec.h/.cpp
    ├── v4l2_capturer.h/.cpp
    ├── alsa_capturer.h/.cpp
    └── alsa_player.h/.cpp
```

## 3. 快速集成

### 3.1 qmake 引入

在 `llfcchat.pro` 中加入：

```pro
include(av_module/av_module.pri)
```

### 3.2 依赖环境（Linux）

- `libjrtplib-dev`
- `libasound2-dev`
- 摄像头设备（默认 `/dev/video0`）

### 3.2.1 上游源码地址变更（Codeberg）

`JThread` 已声明迁移到 Codeberg，建议优先使用 Codeberg 源下载源码。

使用 git 克隆：

```bash
git clone https://codeberg.org/j0r1/JThread.git
git clone https://codeberg.org/j0r1/JRTPLIB.git
```

如果你所在环境 `git` 不方便，也可以使用 `wget` 下载源码快照：

```bash
wget -O JThread.tar.gz https://codeberg.org/j0r1/JThread/archive/master.tar.gz
wget -O JRTPLIB.tar.gz https://codeberg.org/j0r1/JRTPLIB/archive/master.tar.gz
tar -xzf JThread.tar.gz
tar -xzf JRTPLIB.tar.gz
```

说明：

- 源码快照方式不包含 `.git` 历史，适合直接编译安装。
- 若需可复现构建，建议用 `git clone` 并记录 `git rev-parse HEAD`。

### 3.3 头文件使用

```cpp
#include <av_engine.h>
#include <video_widget.h>
```

### 3.4 启动信令服务端（联调用）

项目内已提供参考服务端实现：

- `av_module/signaling_server/signaling_server.py`

启动命令：

```bash
cd av_module/signaling_server
python3 signaling_server.py --host 0.0.0.0 --port 9000
```

客户端配置需与服务端端口一致：

- `cfg.signalingHost = "<服务端IP>"`
- `cfg.signalingPort = 9000`

## 4. 对外 API（AvEngine）

`include/av_engine.h` 是模块的主要入口。

### 生命周期

- `setConfig(const AvConfig&)`
- `start()`
- `stop()`
- `isRunning()`

### 呼叫控制

- `call(const QString &peerId)`
- `accept()`
- `reject(const QString &reason)`
- `hangup()`
- `callState()`

### 查询在线用户

- `queryPeerList()`

### 核心信号

- `incomingCall(const QString &peerId)`
- `callStateChanged(CallState)`
- `localVideoFrame(const QImage&)`
- `remoteVideoFrame(const QImage&)`
- `peerListReceived(const QStringList&)`
- `error(const QString&)`
- `logMessage(const QString&)`

## 5. 最小可运行示例

```cpp
#include <QApplication>
#include <QVBoxLayout>
#include <QWidget>
#include <av_engine.h>
#include <video_widget.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    auto *layout = new QVBoxLayout(&window);
    auto *localView = new avcall::VideoWidget;
    auto *remoteView = new avcall::VideoWidget;
    layout->addWidget(localView);
    layout->addWidget(remoteView);

    avcall::AvEngine engine;

    avcall::AvConfig cfg;
    cfg.userId = "alice";
    cfg.signalingHost = "192.168.1.100";
    cfg.signalingPort = 9000;
    cfg.rtpPort = 5004;
    cfg.videoDevice = "/dev/video0";
    engine.setConfig(cfg);

    QObject::connect(&engine, &avcall::AvEngine::localVideoFrame,
                     localView, &avcall::VideoWidget::setFrame);
    QObject::connect(&engine, &avcall::AvEngine::remoteVideoFrame,
                     remoteView, &avcall::VideoWidget::setFrame);

    QObject::connect(&engine, &avcall::AvEngine::incomingCall,
                     &window, [&](const QString &) {
        engine.accept();
    });

    QObject::connect(&engine, &avcall::AvEngine::error,
                     &window, [](const QString &msg) {
        qWarning() << "av error:" << msg;
    });

    engine.start();
    window.show();

    // 发起呼叫示例（可放在按钮点击里）
    // engine.call("bob");

    const int rc = app.exec();
    engine.stop();
    return rc;
}
```

## 6. 内部架构解读

### 6.1 AvEngine + Pimpl

- `AvEngine` 为外部门面，接口稳定。
- `AvEngine::Private`（`av_engine_p.h`）聚合状态机、信令、RTP、音视频管线。
- 外部永远不直接依赖内部实现，降低 ABI/API 变更风险。

### 6.2 呼叫状态机（CallStateMachine）

合法流转：

- `Idle -> Calling`（发起呼叫）
- `Idle -> Ringing`（收到来电）
- `Calling -> Connected`（对方接听）
- `Ringing -> Connected`（本端接听）
- `Connected -> Idle`（挂断）

状态机保证不会出现“未响铃先接听”等非法状态。

### 6.3 信令层（SignalingClient + signaling_protocol）

- TCP 长连接 + JSON 行协议（每条消息以 `\n` 分隔）。
- `signaling_protocol.h` 负责统一构造消息，避免字符串散落。
- 支持消息：`register`、`call`、`accept`、`reject`、`hangup`、`list_clients`。

### 6.4 RTP 层（RtpSession）

- 依赖 jrtplib。
- 单会话复用音视频：
  - 视频：PT=26（JPEG）
  - 音频：PT=0（PCMU）
- 定时 `poll()` 拉取收包并分发到各 pipeline。

### 6.5 视频管线（VideoPipeline）

- 采集：`V4L2Capturer`
- 发送：将 MJPEG 帧封装 RTP 发出
- 接收：重组/解码后发 `remoteFrameReady(QImage)`

### 6.6 音频管线（AudioPipeline）

- 采集：`AlsaCapturer`
- 编码：`PcmuCodec`（G.711 u-law）
- 发送：RTP 音频包
- 接收：解码后经 `AlsaPlayer` 播放

## 7. 典型呼叫时序

### 主叫

1. `start()`
2. 向信令服务器 `register`
3. `call(peerId)`
4. 收到 `accept`
5. 设置 RTP 对端地址
6. `startMedia()`，音视频开始流动

### 被叫

1. `start()` 并 `register`
2. 收到 `call`
3. 进入 `Ringing`，发出 `incomingCall`
4. `accept()`
5. 回 `accept`（携带本地 IP + RTP 端口）
6. `startMedia()`

## 8. 常见错误与排查

### 无法启动

- 检查 `userId` 是否为空（会触发 `error("userId is empty")`）。
- 检查 RTP 端口是否被占用（`rtp.start()` 失败）。
- 检查信令地址是否可达。

### 有信令无画面

- 检查摄像头设备路径（默认 `/dev/video0`）。
- 检查对端 `peer_ip/rtp_port` 是否正确。
- 检查防火墙或端口策略。

### 有信令无声音

- 检查 ALSA 设备名（默认 `default`）。
- 确认采样率/声道数一致。
- 检查音频线程是否正常启动。

## 9. 扩展建议

### 人脸识别接入

推荐新建独立模块（例如 `face_module/`），通过订阅 `localVideoFrame` 或直接接入视频管线输入帧，不要把识别逻辑塞进 `AvEngine`。

### 录制功能

- 视频：在发送前或接收后保存 MJPEG/H264 流。
- 音频：在编码前 PCM 或解码后 PCM 旁路写文件。

### 会议能力

当前是一对一模型；会议可在信令层扩展多方会话管理，并在 RTP 层支持多 SSRC 管理。

## 10. 为什么 public 头文件只放 include/

- `include/` 只保留稳定 API，使用方看到的是“产品接口”。
- `src/` 头文件是内部细节，可随版本调整，不影响外部调用方。
- 这是 SDK 常见的“稳定边界”实践，能显著降低升级成本。

## 11. 一句话总结

`av_module` 采用“门面 API + 状态机 + 信令 + RTP + 双媒体管线”的分层架构，既能快速落地一对一通话，也为后续高级能力扩展预留了清晰边界。

## 12. 交叉编译与移植到 Linux 开发板

下面给出一套通用流程，适用于 ARM Linux 开发板（如 T113 系列）。核心原则是：

- 先准备好可用的交叉工具链。
- 使用开发板 rootfs 作为 sysroot。
- 先交叉编译第三方库（JThread、JRTPLIB）到同一 sysroot。
- 再用同一套工具链交叉编译 Qt 客户端。

### 12.1 准备环境变量

示例（请按你的工具链路径替换）：

```bash

export SYSROOT=~/t113/Tina-Linux/out/t113-zhiwan_v1/staging_dir/target/rootfs


export CC=arm-openwrt-linux-gcc
export CXX=arm-openwrt-linux-g++
export AR=arm-openwrt-linux-ar
export LD=arm-openwrt-linux-ld
export STRIP=arm-openwrt-linux-strip
```

### 12.2 交叉编译 JThread

```bash
cd JThread
mkdir -p build-arm && cd build-arm

cmake .. \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=arm \
    -DCMAKE_SYSROOT=$SYSROOT \
    -DCMAKE_C_COMPILER=$CC \
    -DCMAKE_CXX_COMPILER=$CXX \
    -DCMAKE_FIND_ROOT_PATH=$SYSROOT \
    -DCMAKE_CXX_FLAGS="-Wno-error=unused-result" \
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
    -DCMAKE_INSTALL_PREFIX=~/jrtplib/build/install


make -j$(nproc)
make  install
```

PATH="/home/ysq/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/home/ysq/arm-t113/toolchain-sunxi-musl/toolchain/bin"

-DJTHREAD_INCLUDE_DIR=~/Desktop/jthread/build_arm/install/include -DJTHREAD_LIBRARY=~/Desktop/jthread/build_arm/install/lib/libjthread.so
cmake ..     -DCMAKE_SYSTEM_NAME=Linux     -DCMAKE_SYSTEM_PROCESSOR=arm     -DCMAKE_CXX_FLAGS="-Wno-error=unused-result"     -DCMAKE_C_COMPILER=$CC     -DCMAKE_CXX_COMPILER=$CXX     -DCMAKE_INSTALL_PREFIX=/home/ysq/Desktop/jrtplib/build_arm/install -DJTHREAD_INCLUDE_DIR=/home/ysq/Desktop/jthread/build_arm/install/include -DJTHREAD_LIBRARY=/home/ysq/Desktop/jthread/build_arm/install/lib/libjthread.so -DCMAKE_DISABLE_FIND_PACKAGE_JThread=ON

### 12.3 交叉编译 JRTPLIB

```bash
cd JRTPLIB
mkdir -p build-arm && cd build-arm

cmake .. \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=arm \
    -DCMAKE_SYSROOT=$SYSROOT \
    -DCMAKE_C_COMPILER=$CC \
    -DCMAKE_CXX_COMPILER=$CXX \
    -DCMAKE_FIND_ROOT_PATH=$SYSROOT \
    -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
    -DCMAKE_IGNORE_PATH=/usr/local;/usr \
    -DCMAKE_CXX_FLAGS="-Wno-error=unused-result" \
    -DCMAKE_INSTALL_PREFIX=/usr

make -j$(nproc)
make DESTDIR=$SYSROOT install
```

### 12.4 交叉编译 llfcchat（qmake）

`av_module.pri` 已支持从 qmake 注入 sysroot：

- `AV_SYSROOT`
- `AV_EXTRA_INCLUDEPATH`
- `AV_EXTRA_LIBPATH`

示例命令：

```bash
cd llfcchat

/path/to/qt-arm/bin/qmake llfcchat.pro \
    "QMAKE_CC=$CC" \
    "QMAKE_CXX=$CXX" \
    "QMAKE_LINK=$CXX" \
    "QMAKE_AR=$AR cqs" \
    "AV_SYSROOT=$SYSROOT"

make -j$(nproc)
```

如果你的库不在 sysroot 默认目录，可额外传参：

```bash
/path/to/qt-arm/bin/qmake llfcchat.pro \
    "AV_SYSROOT=$SYSROOT" \
    "AV_EXTRA_INCLUDEPATH=$SYSROOT/opt/av/include" \
    "AV_EXTRA_LIBPATH=$SYSROOT/opt/av/lib"
```

### 12.5 部署到开发板

至少需要部署：

- 可执行文件（llfcchat）
- `config.ini`
- `static/` 资源目录
- 交叉编译得到的动态库（如 `libjrtp.so*`、`libjthread.so*`，若开发板系统中不存在）

建议在板端设置：

```bash
export LD_LIBRARY_PATH=/usr/lib:/usr/local/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=~/Desktop/jthread/build_arm/install/lib:$LD_LIBRARY_PATH
```

### 12.6 常见问题

- 链接到宿主机库：通常是未使用 sysroot，或 qmake/编译器不匹配。
- 运行时报 `wrong ELF class`：混入了 x86 库，需全部替换为 ARM 版本。
- 头文件找不到 jrtplib3：检查 `AV_SYSROOT` 是否正确，或补 `AV_EXTRA_INCLUDEPATH`。
- 板端无法加载库：检查 `.so` 是否部署到板端并更新 `LD_LIBRARY_PATH`。
- 若出现 `/usr/local/lib/libjthread.so: file not recognized`，说明交叉链接时误用了宿主机库。应确保 `JThread` 先安装到 `$SYSROOT/usr`，并在 CMake 中添加 `-DCMAKE_IGNORE_PATH=/usr/local;/usr`。
- 若工具链提示 `STAGING_DIR not defined`，说明 OpenWrt 工具链环境未补齐。通常可将 `STAGING_DIR` 设为 toolchain 或 SDK 的 `staging_dir` 根目录。
