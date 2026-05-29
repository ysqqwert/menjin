#ifndef AV_MODULE_PCMU_CODEC_H
#define AV_MODULE_PCMU_CODEC_H

#include <QByteArray>

namespace avcall {

/// G.711 μ-law codec with 16 kHz ↔ 8 kHz sample-rate conversion.
class PcmuCodec
{
public:
    /// 16 kHz PCM-16-LE → 8 kHz PCMU (downsample 2:1, encode)
    static QByteArray encode(const QByteArray &pcm16le);

    /// 8 kHz PCMU → 16 kHz PCM-16-LE (decode, upsample 1:2)
    static QByteArray decode(const QByteArray &pcmu);

private:
    static unsigned char linearToUlaw(short sample);
    static short ulawToLinear(unsigned char ulaw);
};

} // namespace avcall

#endif // AV_MODULE_PCMU_CODEC_H
