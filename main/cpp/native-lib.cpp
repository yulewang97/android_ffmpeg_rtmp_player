#include <jni.h>
#include <iostream>
#include <string>
#include <cassert>
#include <android/log.h>
#include <dlfcn.h>
#include <time.h>
#include "player.h"
#include <android/native_window.h>
#include <android/native_window_jni.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include <libavutil/pixfmt.h>
}

using namespace std;

GlobalContext global_context;
PacketQueue * packetQueue;
bool initFinished = false;



extern "C" JNIEXPORT jstring JNICALL
Java_com_example_a12207_rtmpreceive_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT jint
Java_com_example_a12207_rtmpreceive_MainActivity_CAdd(
        JNIEnv* env,
        jobject obgj,jint a, jint b ) {
    av_register_all();
    cout << "whereiam" << endl;
    return a+b;
}

extern "C" JNIEXPORT jint
Java_com_example_a12207_rtmpreceive_VideoPlayer_FFmpegTest(
        JNIEnv *env,
        jobject obgj, jobject surface) {

    FFLOGI("%s", "抓包线程");
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    int	i, videoindex;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVPacket *packet;
    int ret;
    int got_picture = 0;
    pthread_t thread;

    char filepath[] = "rtmp://192.168.1.177:1935/live/live";
    //rtmp://59.78.0.57:1935/live
    //rtsp://192.168.1.123:8554/


    global_context.nativeWindow = ANativeWindow_fromSurface(env, surface);

    av_register_all();
    avformat_network_init();
    avcodec_register_all();

    AVDictionary* opts = NULL;
    //av_dict_set(&opts, "analyzeduration", "0", 0);
    //av_dict_set(&opts,"probesize","409600",0);
    //av_dict_set(&opts, "timeout", "10000000", 0); // 设置timeout，为微秒。一共5秒
    //av_dict_set(&opts, "buffer_size", "10240000", 0);
    av_dict_set(&opts,"framerate","15",0);



    ret = avformat_open_input(&pFormatCtx, filepath, 0, &opts);
    //FFLOGE("%d", av_open_input_file(&pFormatCtx, filepath, NULL,0,NULL));


    if(ret != 0){
        char errorbuf[1024] = {0};
        av_make_error_string(errorbuf, 1024, ret);
        FFLOGE("%s,%d,%s", "无法打开输入视频文件", ret, errorbuf);

    }

    //FFLOGI("%s", "视频长度：");

    ret = avformat_find_stream_info(pFormatCtx, NULL);



    videoindex = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoindex = i;
            break;
        }

    //pCodecCtx = pFormatCtx->streams[videoindex]->codec;
    global_context.vcodec_ctx = pFormatCtx->streams[videoindex]->codec;
    //global_context.vcodec_ctx->thread_count = 2;

    pCodec = avcodec_find_decoder(global_context.vcodec_ctx->codec_id);
    avcodec_open2(global_context.vcodec_ctx, pCodec, NULL);


    global_context.vstream = pFormatCtx->streams[videoindex];
    global_context.vcodec = pCodec;


    packet = (AVPacket *)av_malloc(sizeof(AVPacket));

    initFinished = true;

    while (av_read_frame(pFormatCtx, packet) >= 0)
    {

        if (packet->stream_index == videoindex)
        {
            packet_queue_put(&global_context.video_queue, packet);
            //ret = avcodec_send_packet(pCodecCtx, packet);
            //got_picture = avcodec_receive_frame(pCodecCtx, pFrame);
        }

        //av_packet_unref(packet);
        av_free_packet(packet);

    }

    return 0;


}

extern "C" JNIEXPORT jint
Java_com_example_a12207_rtmpreceive_VideoPlayer_Render(
        JNIEnv *env,
        jobject obgj) {
    AVPacket* packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    int frameFinished;
    AVFrame *pFrame,*pFrameYUV, *pFrameRGB;
    FFLOGI("%s", "显示线程");
    ANativeWindow_Buffer windowBuffer;

    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();
    pFrameRGB = av_frame_alloc();
    while(!initFinished){

    }
    unsigned char *rgbBuffer=(unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGBA, global_context.vcodec_ctx->width, global_context.vcodec_ctx->height,1));

    //
    int ret, got_picture;

    //需要注意的地方
    avpicture_fill((AVPicture *)pFrameRGB,rgbBuffer,AV_PIX_FMT_RGBA,global_context.vcodec_ctx->width, global_context.vcodec_ctx->height);

    int h;
    int videoWidth = global_context.vcodec_ctx->width;
    int videoHeight = global_context.vcodec_ctx->height;
    ANativeWindow_setBuffersGeometry(global_context.nativeWindow,  videoWidth, videoHeight, WINDOW_FORMAT_RGBX_8888);

    for (;;) {

        if (global_context.quit) {
            av_log(NULL, AV_LOG_ERROR, "video_thread need exit. \n");
            break;
        }

        if (global_context.pause) {
            continue;
        }
        //FFLOGI("%s", "开始前");
        if (packet_queue_get(&global_context.video_queue, packet) <= 0) {
            // means we quit getting packets
            continue;
        }

        //FFLOGI("%s,%d", "开始后",packet->size);
        clock_t start1 = clock();
        avcodec_decode_video2(global_context.vcodec_ctx, pFrame, &frameFinished,
                              packet);
        clock_t timeElapsed1 = (clock() - start1);
        FFLOGI("%s,%d", "解码", timeElapsed1);

        //FFLOGI("%s,%d", "失败与否",frameFinished);
        if (frameFinished) {

            ANativeWindow_lock(global_context.nativeWindow, &windowBuffer, 0);
            clock_t start = clock();
            libyuv::I420ToABGR((const uint8*)pFrame->data[0], pFrame->linesize[0],
                               (const uint8*)pFrame->data[1], pFrame->linesize[1],
                               (const uint8*)pFrame->data[2], pFrame->linesize[2],
                               (uint8*)pFrameRGB->data[0], videoWidth * 4,
                               videoWidth, videoHeight);
            clock_t timeElapsed = (clock() - start);
            FFLOGI("%s,%d", "耗时", timeElapsed);
            FFLOGI("%s,%d", "包号", pFrame->pts);

            uint8_t *dst = (uint8_t *) windowBuffer.bits;
            int dstStride = windowBuffer.stride * 4;
            uint8_t *src = (uint8_t *) (pFrameRGB->data[0]);
            int srcStride = pFrameRGB->linesize[0];

            //FFLOGI("%s,%d", "目标长度：",windowBuffer.stride);

            //clock_t start = clock ();
            for (h = 0; h < videoHeight; h++) {
                memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
            }

            ANativeWindow_unlockAndPost(global_context.nativeWindow);
        }

        //av_free_packet(packet);
        //usleep(10000);
    }

    return 0;
}