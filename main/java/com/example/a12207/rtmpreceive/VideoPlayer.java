package com.example.a12207.rtmpreceive;

public class VideoPlayer {
    static {
        System.loadLibrary("native-lib");
        System.loadLibrary("avcodec");
        System.loadLibrary("avfilter");
        System.loadLibrary("avformat");
        System.loadLibrary("avutil");
        System.loadLibrary("swresample");
        System.loadLibrary("swscale");
        System.loadLibrary("fdk-aac");
        System.loadLibrary("yuv");
    }
    public static native int FFmpegTest(Object surface);

    public static native int Render();

}
