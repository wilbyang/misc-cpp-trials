#include <fmt/core.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

int main()
{
    fmt::print("=== FFmpeg Installation Verification ===\n\n");

    // Print FFmpeg version information
    fmt::print("FFmpeg library versions:\n");
    fmt::print("  libavcodec version: {}.{}.{}\n", 
               LIBAVCODEC_VERSION_MAJOR,
               LIBAVCODEC_VERSION_MINOR,
               LIBAVCODEC_VERSION_MICRO);
    fmt::print("  libavformat version: {}.{}.{}\n",
               LIBAVFORMAT_VERSION_MAJOR,
               LIBAVFORMAT_VERSION_MINOR,
               LIBAVFORMAT_VERSION_MICRO);
    fmt::print("  libavutil version: {}.{}.{}\n",
               LIBAVUTIL_VERSION_MAJOR,
               LIBAVUTIL_VERSION_MINOR,
               LIBAVUTIL_VERSION_MICRO);
    fmt::print("  libswscale version: {}.{}.{}\n\n",
               LIBSWSCALE_VERSION_MAJOR,
               LIBSWSCALE_VERSION_MINOR,
               LIBSWSCALE_VERSION_MICRO);

    // Print FFmpeg configuration
    fmt::print("FFmpeg configuration: {}\n\n", avutil_configuration());

    // List some available codecs
    fmt::print("Sample of available codecs:\n");
    void* opaque = nullptr;
    const AVCodec* codec = nullptr;
    int count = 0;
    while ((codec = av_codec_iterate(&opaque)) != nullptr && count < 10) {
        fmt::print("  [{}] {} - {}\n", 
                   codec->type == AVMEDIA_TYPE_VIDEO ? "VIDEO" : 
                   codec->type == AVMEDIA_TYPE_AUDIO ? "AUDIO" : "OTHER",
                   codec->name,
                   codec->long_name ? codec->long_name : "N/A");
        count++;
    }

    fmt::print("\n✓ FFmpeg installation verified successfully!\n");

    return 0;
}
