#include <jni.h>
#include <string>

#include <com_sniperking_live555suihw_Live555Func.h>
#include <android/log.h>
#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>

#define LOG_TAG "rtsp_server"
#define LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

char* url;

JNIEXPORT jstring JNICALL Java_com_sniperking_live555suihw_Live555Func_getUrl
  (JNIEnv *env, jclass instance){
    if (!url) {
        return NULL;
    }
    return env->NewStringUTF(url);
}

JNIEXPORT jstring JNICALL Java_com_sniperking_live555suihw_Live555Func_start
  (JNIEnv *env, jclass instance, jstring fileName_){
    const char *inputFilename = env->GetStringUTFChars(fileName_, 0);
    FILE *file = fopen(inputFilename, "rb");
    if (!file) {
        LOGE("couldn't open %s", inputFilename);
        exit(1);
    }
    fclose(file);

    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* env_ = BasicUsageEnvironment::createNew(*scheduler);
    UserAuthenticationDatabase* authDB = NULL;
    // Create the RTSP server:
    RTSPServer* rtspServer = RTSPServer::createNew(*env_, 8554, authDB);
    if (rtspServer == NULL) {
        LOGE("Failed to create RTSP server: %s", env_->getResultMsg());
        exit(1);
    }
    char const* descriptionString = "Session streamed by \"testOnDemandRTSPServer\"";
    char const* streamName = "v";
    ServerMediaSession* sms  = ServerMediaSession::createNew(*env_, streamName, streamName, descriptionString);
    sms->addSubsession(H264VideoFileServerMediaSubsession::createNew(*env_, inputFilename, True));
    rtspServer->addServerMediaSession(sms);

    url = rtspServer->rtspURL(sms);
    LOGE("%s stream, from the file %s ",streamName, inputFilename);
    LOGE("Play this stream using the URL: %s", url);
//    delete[] url;

    env->ReleaseStringUTFChars(fileName_, inputFilename);
    env_->taskScheduler().doEventLoop(); // does not return
}