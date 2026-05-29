#include "pcmu_codec.h"

namespace avcall {

namespace {

static const int kBias = 0x84;
static const int kClip = 32635;

// ITU G.711 standard exponent lookup table
static const int kExpLut[256] = {
    0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

} // anon

unsigned char PcmuCodec::linearToUlaw(short sample)
{
    int sign = (sample >> 8) & 0x80;
    if (sign)
        sample = static_cast<short>(-sample);
    if (sample > kClip)
        sample = kClip;

    sample = static_cast<short>(sample + kBias);
    int exponent = kExpLut[(sample >> 7) & 0xFF];
    int mantissa = (sample >> (exponent + 3)) & 0x0F;
    return static_cast<unsigned char>(~(sign | (exponent << 4) | mantissa));
}

short PcmuCodec::ulawToLinear(unsigned char ulaw)
{
    ulaw = static_cast<unsigned char>(~ulaw);
    int sign     = ulaw & 0x80;
    int seg      = (ulaw >> 4) & 0x07;
    int mantissa = ulaw & 0x0F;

    int value = ((mantissa << 3) + kBias) << seg;
    value -= kBias;
    return static_cast<short>(sign ? -value : value);
}

QByteArray PcmuCodec::encode(const QByteArray &pcm16le)
{
    if (pcm16le.size() < 4)
        return QByteArray();

    const int sampleCount = pcm16le.size() / 2;
    QByteArray out;
    out.resize(sampleCount / 2);

    const short *src = reinterpret_cast<const short *>(pcm16le.constData());
    unsigned char *dst = reinterpret_cast<unsigned char *>(out.data());

    int j = 0;
    for (int i = 0; i + 1 < sampleCount; i += 2)
        dst[j++] = linearToUlaw(src[i]);

    return out;
}

QByteArray PcmuCodec::decode(const QByteArray &pcmu)
{
    if (pcmu.isEmpty())
        return QByteArray();

    QByteArray out;
    out.resize(pcmu.size() * 2 * 2); // 8k→16k upsample × 2 bytes/sample

    const unsigned char *src = reinterpret_cast<const unsigned char *>(pcmu.constData());
    short *dst = reinterpret_cast<short *>(out.data());

    int j = 0;
    for (int i = 0; i < pcmu.size(); ++i) {
        const short s = ulawToLinear(src[i]);
        dst[j++] = s;
        dst[j++] = s; // duplicate sample for 8k→16k
    }
    return out;
}

} // namespace avcall
