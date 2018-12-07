package com.example.a12207.rtmpreceive;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {
    public static final String MY_TAG = "wumiao";
    private EditText pass_word;
    private Button button;
    public SurfaceView stream;
    public SurfaceHolder surfaceViewHolder;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
        System.loadLibrary("avcodec");
        System.loadLibrary("avfilter");
        System.loadLibrary("avformat");
        System.loadLibrary("avutil");
        System.loadLibrary("swresample");
        System.loadLibrary("swscale");
        System.loadLibrary("fdk-aac");

    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Log.i(MY_TAG,"onCreate");
        // Example of a call to a native method
        stream = (SurfaceView) findViewById(R.id.streamView);
        surfaceViewHolder = stream.getHolder();
        surfaceViewHolder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        VideoPlayer.FFmpegTest(surfaceViewHolder.getSurface());
                        //System.out.println("我会执行");
                    }
                }).start();
                //获取文件路径，这里将文件放置在手机根目录下
                //FFmpegTest();\
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        VideoPlayer.Render();
                    }
                }).start();
            }
            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {    }
            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {    }
        });


        int addResult = 666;
        String stringResult = String.valueOf(addResult);
        System.out.println(addResult);
        //FFmpegTest();
        Button startButton = (Button) this.findViewById(R.id.start);



    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    public native int CAdd(int a, int b);

    //public static native int FFmpegTest();
}
