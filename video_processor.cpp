#include <emscripten/bind.h>
#include <iostream>
#include <string>
extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavfilter/avfilter.h>
    #include <libavfilter/buffersink.h>
    #include <libavfilter/buffersrc.h>
}
std::string process_video(const std::string& input_path, const std::string& output_path) {
    av_register_all();
    avfilter_register_all();

    AVFormatContext* format_context = avformat_alloc_context();
    if (avformat_open_input(&format_context, input_path.c_str(), nullptr, nullptr) != 0) {
        return "Error opening video file";
    }

    if (avformat_find_stream_info(format_context, nullptr) < 0) {
        return "Error finding stream info";
    }

    AVCodec* codec = nullptr;
    AVCodecContext* codec_context = nullptr;
    int video_stream_index = -1;

    for (unsigned int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            codec = avcodec_find_decoder(format_context->streams[i]->codecpar->codec_id);
            codec_context = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(codec_context, format_context->streams[i]->codecpar);
            avcodec_open2(codec_context, codec, nullptr);
            break;
        }
    }

    if (video_stream_index == -1) {
        return "Error finding video stream";
    }

    AVFilterGraph* filter_graph = avfilter_graph_alloc();
    AVFilterContext* buffersrc_ctx = nullptr;
    AVFilterContext* buffersink_ctx = nullptr;
    AVFilterInOut* inputs = avfilter_inout_alloc();
    AVFilterInOut* outputs = avfilter_inout_alloc();

    const AVFilter* buffersrc = avfilter_get_by_name("buffer");
    const AVFilter* buffersink = avfilter_get_by_name("buffersink");
    const AVFilter* scale = avfilter_get_by_name("scale");
    const AVFilter* format = avfilter_get_by_name("format");

    char args[512];
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             codec_context->width, codec_context->height, codec_context->pix_fmt,
             codec_context->time_base.num, codec_context->time_base.den,
             codec_context->sample_aspect_ratio.num, codec_context->sample_aspect_ratio.den);

    avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, nullptr, filter_graph);
    avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", nullptr, nullptr, filter_graph);

    AVPixelFormat pix_fmts[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE };
    av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);

    AVFilterContext* scale_ctx = nullptr;
    AVFilterContext* format_ctx = nullptr;

    avfilter_graph_create_filter(&scale_ctx, scale, "scale", "640:480", nullptr, filter_graph);
    avfilter_graph_create_filter(&format_ctx, format, "format", "gray", nullptr, filter_graph);

    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    avfilter_link(buffersrc_ctx, 0, scale_ctx, 0);
    avfilter_link(scale_ctx, 0, format_ctx, 0);
    avfilter_link(format_ctx, 0, buffersink_ctx, 0);

    avfilter_graph_parse_ptr(filter_graph, "null", &inputs, &outputs, nullptr);
    avfilter_graph_config(filter_graph, nullptr);

    AVFrame* frame = av_frame_alloc();
    AVFrame* filt_frame = av_frame_alloc();
    AVPacket packet;

    while (av_read_frame(format_context, &packet) >= 0) {
        if (packet.stream_index == video_stream_index) {
            avcodec_send_packet(codec_context, &packet);
            while (avcodec_receive_frame(codec_context, frame) >= 0) {
                if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                    break;
                }
                while (av_buffersink_get_frame(buffersink_ctx, filt_frame) >= 0) {
                    // Process the filtered frame (e.g., save to file)
                    av_frame_unref(filt_frame);
                }
                av_frame_unref(frame);
            }
        }
        av_packet_unref(&packet);
    }

    av_frame_free(&frame);
    av_frame_free(&filt_frame);
    avfilter_graph_free(&filter_graph);
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);

    return "Video processed successfully";
}

EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::function("process_video", &process_video);
}
