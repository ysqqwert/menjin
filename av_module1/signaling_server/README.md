# av_module 信令服务端（参考实现）

这是一个与 `av_module` 客户端协议对齐的最小信令服务端实现，使用 Python3 标准库（无第三方依赖）。

## 支持能力

- 用户注册：`register`
- 在线列表：`list_clients`
- 呼叫转发：`call`
- 接听转发：`accept`
- 拒绝转发：`reject`
- 挂断转发：`hangup`
- 异常断线时自动通知对端 `hangup`

## 运行方式

```bash
cd av_module/signaling_server
python3 signaling_server.py --host 0.0.0.0 --port 9000
```

客户端侧配置：

- `AvConfig.signalingHost = "<服务器IP>"`
- `AvConfig.signalingPort = 9000`

## 协议要点

- 传输：TCP
- 编码：UTF-8
- 帧格式：每行一条 JSON（以 `\n` 结尾）

示例：

```json
{"type":"register","client_id":"alice"}
{"type":"call","to":"bob","rtp_port":5004}
```

## 说明

- `accept` 转发时，服务端使用连接来源 IP 作为 `peer_ip`，减少客户端自报 IP 不准确的问题。
- 当前实现适合单机联调和内网测试；若用于生产，建议增加鉴权、超时清理、TLS、日志落盘、限流等能力。
