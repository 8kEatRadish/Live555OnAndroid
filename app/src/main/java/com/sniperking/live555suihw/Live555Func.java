package com.sniperking.live555suihw;

public class Live555Func {
    static {
        System.loadLibrary("suihwlive555");
    }

    public static native String getUrl();

    public static native String start(String fileName);
}
