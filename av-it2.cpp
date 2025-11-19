#include <iostream>
#include <memory>
#include <string>
#include <vector>

// 引入 FFmpeg 头文件，必须包裹在 extern "C" 中
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
}

// ==========================================
// 1. RAII 封装部分 (核心技术点)
// ==========================================

struct AVFormatContextDeleter {
    void operator()(AVFormatContext* ctx) const {
        if (ctx) avformat_close_input(&ctx);
    }
};

struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ctx) const {
        if (ctx) avcodec_free_context(&ctx);
    }
};

struct AVPacketDeleter {
    void operator()(AVPacket* pkt) const {
        if (pkt) av_packet_free(&pkt);
    }
};

struct AVFrameDeleter {
    void operator()(AVFrame* frame) const {
        if (frame) av_frame_free(&frame);
    }
};

// 使用 using 定义智能指针类型，简化代码
using FormatCtxPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
using CodecCtxPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
using PacketPtr = std::unique_ptr<AVPacket, AVPacketDeleter>;
using FramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;

// 错误检查辅助函数
void check_err(int ret, const std::string& msg) {
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, err_buf, AV_ERROR_MAX_STRING_SIZE);
        throw std::runtime_error(msg + ": " + std::string(err_buf));
    }
}

// ==========================================
// 2. 主逻辑
// ==========================================

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <video_file_path>" << std::endl;
        return -1;
    }

    const char* infile = argv[1];
    av_log_set_level(AV_LOG_INFO); // 设置 FFmpeg 日志级别

    try {
        // --- 1. 打开文件 (Demuxing) ---
        AVFormatContext* raw_fmt_ctx = nullptr;
        // 注意：avformat_open_input 需要传入二级指针
        int ret = avformat_open_input(&raw_fmt_ctx, infile, nullptr, nullptr);
        check_err(ret, "Failed to open input file");
        
        // 立即接管指针所有权
        FormatCtxPtr fmt_ctx(raw_fmt_ctx);

        // 探测流信息
        ret = avformat_find_stream_info(fmt_ctx.get(), nullptr);
        check_err(ret, "Failed to find stream info");

        // --- 2. 寻找视频流 ---
        int video_stream_index = -1;
        const AVCodec* codec = nullptr;

        // 使用 av_find_best_stream 是更现代的做法
        video_stream_index = av_find_best_stream(fmt_ctx.get(), AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
        check_err(video_stream_index, "Could not find video stream");
        
        std::cout << "Found Video Stream Index: " << video_stream_index 
                  << ", Codec: " << codec->name << std::endl;

        // --- 3. 初始化解码器 (Decoding Context) ---
        CodecCtxPtr codec_ctx(avcodec_alloc_context3(codec));
        if (!codec_ctx) throw std::runtime_error("Failed to allocate codec context");

        // 将流的参数（宽、高、码率等）复制到解码器上下文
        AVStream* stream = fmt_ctx->streams[video_stream_index];
        ret = avcodec_parameters_to_context(codec_ctx.get(), stream->codecpar);
        check_err(ret, "Failed to copy codec params");

        // 打开解码器
        ret = avcodec_open2(codec_ctx.get(), codec, nullptr);
        check_err(ret, "Failed to open codec");

        // --- 4. 准备 Packet 和 Frame ---
        PacketPtr packet(av_packet_alloc());
        FramePtr frame(av_frame_alloc());
        if (!packet || !frame) throw std::runtime_error("Failed to allocate packet/frame");

        std::cout << "Start decoding..." << std::endl;

        // --- 5. 解码循环 (Reading Loop) ---
        int frame_count = 0;
        while (av_read_frame(fmt_ctx.get(), packet.get()) >= 0) {
            
            // 只处理视频流
            if (packet->stream_index == video_stream_index) {
                
                // A. 发送压缩包给解码器 (Send)
                ret = avcodec_send_packet(codec_ctx.get(), packet.get());
                if (ret < 0) {
                    std::cerr << "Error sending packet to decoder" << std::endl;
                    break;
                }

                // B. 循环接收解码后的原始帧 (Receive)
                // 注意：一个 Packet 可能包含多个 Frame，或者需要多个 Packet 才能产出一个 Frame
                while (ret >= 0) {
                    ret = avcodec_receive_frame(codec_ctx.get(), frame.get());
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        // EAGAIN: 需要更多输入
                        // EOF: 流结束
                        break; 
                    } else if (ret < 0) {
                        std::cerr << "Error decoding frame" << std::endl;
                        break; // 真正错误
                    }

                    // --- 成功拿到 Raw Frame (YUV) ---
                    frame_count++;
                    
                    // 这里可以做 sws_scale, 混流, 特效处理...
                    // 简单演示：打印帧信息
                    std::cout << "Frame " << frame_count 
                              << " | pts: " << frame->pts 
                              << " | type: " << av_get_picture_type_char(frame->pict_type) // I, P, B 帧
                              << " | fmt: " << av_get_pix_fmt_name((AVPixelFormat)frame->format)
                              << " | size: " << frame->width << "x" << frame->height
                              << std::endl;
                }
            }

            // 必须：重置 Packet 引用计数，准备读取下一个
            av_packet_unref(packet.get());
        }

        // Flush 解码器 (处理缓冲区中剩余的帧)
        avcodec_send_packet(codec_ctx.get(), nullptr);
        while (avcodec_receive_frame(codec_ctx.get(), frame.get()) >= 0) {
             std::cout << "Flushed Frame..." << std::endl;
        }

        std::cout << "Decoding finished. Total frames: " << frame_count << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    }

    // 离开作用域，unique_ptr 自动调用 Deleter 释放资源 (avformat_close_input 等)
    return 0;
}