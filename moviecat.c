#define _XOPEN_SOURCE 600 /* for usleep */
#include <unistd.h>
#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

static long flag = 0;
static AVFormatContext *fmt_ctx;
static AVCodecContext *dec_ctx;
AVFilterContext *buffersink_ctx;
AVFilterContext *buffersrc_ctx;
AVFilterGraph *filter_graph;
static int video_stream_index = -1;
static int64_t last_pts = AV_NOPTS_VALUE;

static short repeat_flag = 1;
void sig_handler(int SIG);
static int *bak;

static int getColor(int r, int g, int b)
{
        r = (int)(r*6/256);
        g = (int)(g*6/256);
        b = (int)(b*6/256);
        return ( 16 + (r * 36) + (g * 6) + b);
}
static int open_input_file(const char *filename)
{
    int ret;
    AVCodec *dec;

    if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    /* select the video stream */
    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
        return ret;
    }
    video_stream_index = ret;
    dec_ctx = fmt_ctx->streams[video_stream_index]->codec;

    /* init the video decoder */
    if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
        return ret;
    }

    return 0;
}

static int init_filters(const char *filters_descr)
{
    char args[512];
    int ret;
    AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    AVFilter *buffersink = avfilter_get_by_name("ffbuffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    enum PixelFormat pix_fmts[] = { PIX_FMT_RGBA, PIX_FMT_NONE };
    AVBufferSinkParams *buffersink_params;

    filter_graph = avfilter_graph_alloc();

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
            dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
            dec_ctx->time_base.num, dec_ctx->time_base.den,
            dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
        return ret;
    }

    /* buffer video sink: to terminate the filter chain. */
    buffersink_params = av_buffersink_params_alloc();
    buffersink_params->pixel_fmts = pix_fmts;
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, buffersink_params, filter_graph);
    av_free(buffersink_params);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
        return ret;
    }

    /* Endpoints for the filter graph. */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    if ((ret = avfilter_graph_parse(filter_graph, filters_descr,
                                    &inputs, &outputs, NULL)) < 0)
        return ret;

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        return ret;
    return 0;
}

static void display_picref(AVFilterBufferRef *picref, AVRational time_base)
{
    int x, y;
    uint8_t *p0, *p;
    int64_t delay;

    if (picref->pts != AV_NOPTS_VALUE) {
        if (last_pts != AV_NOPTS_VALUE) {
            /* sleep roughly the right amount of time;
             * usleep is in microseconds, just like AV_TIME_BASE. */
            delay = av_rescale_q(picref->pts - last_pts,
                                 time_base, AV_TIME_BASE_Q);
//            if (delay > 0 && delay < 1000000)
//                usleep(delay*2);
            
        }
        last_pts = picref->pts;
    }

    /* Trivial ASCII grayscale display. */
    p0 = picref->data[0];
    for (y = 0; y < picref->video->h; y++) {
        p = p0;
        for (x = 0; x < picref->video->w; x++)
        {
            int color = (getColor(*(p++), *(p++), *(p++)));
            if(*(bak+y*picref->video->w+x) != color){
                *(bak+y*picref->video->w+x) = color;
                attrset(COLOR_PAIR(color));
                mvprintw(y, x, " ");
            }else{
                flag++;
            }
            p++;
        }
        p0 += picref->linesize[0];
    }
    refresh();
}

int main(int argc, char **argv)
{
    //curses
    int color_pair;
    if(initscr() == NULL){
        fprintf(stderr, "init failure\n");
        exit(EXIT_FAILURE);
    }
    /* start_colorは色属性を使用するときは最初に必ず実行する.
       initscrの直後に実行するのがよい習慣らしい. */
    if(has_colors() == FALSE || start_color() == ERR){
        endwin();
        fprintf(stderr, "This term seems not to having Color\n");
        exit(EXIT_FAILURE);
    }

    if(signal(SIGINT, sig_handler) == SIG_ERR ||
            signal(SIGQUIT, sig_handler) == SIG_ERR){
        fprintf(stderr, "signal failure\n");
        exit(EXIT_FAILURE);
    }
    curs_set(0);

    /* 色のペアを作る */
    color_pair = 1;
    for(color_pair = 1; color_pair < 256; color_pair++){
        init_pair(color_pair, color_pair, color_pair);
    }

    refresh();

    char filter_descr[10000];
    int w, h;
    w = COLS;
    h = LINES;
    bak = malloc(sizeof (int) * LINES*COLS+1);
        sprintf(filter_descr, "scale=%d:%d", w, h);



    int ret;
    AVPacket packet;
    AVFrame frame;
    int got_frame;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        exit(1);
    }

    avcodec_register_all();
    av_register_all();
    avfilter_register_all();

    if ((ret = open_input_file(argv[1])) < 0)
        goto end;
    if ((ret = init_filters(filter_descr)) < 0)
        goto end;

    /* read all packets */
    while (1) {
        AVFilterBufferRef *picref;
        if ((ret = av_read_frame(fmt_ctx, &packet)) < 0)
            break;

        if (packet.stream_index == video_stream_index) {
            avcodec_get_frame_defaults(&frame);
            got_frame = 0;
            ret = avcodec_decode_video2(dec_ctx, &frame, &got_frame, &packet);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error decoding video\n");
                break;
            }

            if (got_frame) {
                frame.pts = av_frame_get_best_effort_timestamp(&frame);

                /* push the decoded frame into the filtergraph */
                if (av_buffersrc_add_frame(buffersrc_ctx, &frame, 0) < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
                    break;
                }

                /* pull filtered pictures from the filtergraph */
                while (repeat_flag) {
                    ret = av_buffersink_get_buffer_ref(buffersink_ctx, &picref, 0);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    if (ret < 0)
                        goto end;

                    if (picref) {
                        display_picref(picref, buffersink_ctx->inputs[0]->time_base);
                        avfilter_unref_bufferp(&picref);
                    }
                }
            }
        }
        av_free_packet(&packet);
    }
end:
    endwin();
    avfilter_graph_free(&filter_graph);
    if (dec_ctx)
        avcodec_close(dec_ctx);
    avformat_close_input(&fmt_ctx);

    if (ret < 0 && ret != AVERROR_EOF) {
        char buf[1024];
        av_strerror(ret, buf, sizeof(buf));
        fprintf(stderr, "Error occurred: %s\n", buf);
        exit(1);
    }

    exit(0);
}
void sig_handler(int SIG)
{
    fprintf(stderr, "Got SIG %d\n", SIG);
    repeat_flag = 0;
}
