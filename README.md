

# Live555OnAndroid

#### 介绍
移植Live555到Android，用nkd交叉编译live555，Android java JNI调用建立RTSP Server，实现本地局域网h264视频流播放。

#### Live555介绍
Live555是一个跨平台的流媒体解决方案，以C++为开发语言，实现了RTSP包括服务器-客户端的整套结构，并且支持H.264, H.265, MPEG, AAC等多种视频和音频编码。

live555的核心模块：

RTSP服务器和客户端的交互流程：

Live555的流媒体模块基本分为Source和Sink两大部分，当然他们也有一个共同的基类Medium。对服务器来说，Source为数据来源，Sink为数据输出，视频数据就通过MediaSource传递给MediaSink，最终通过RTPInterface网络传输给客户端，通过完成自己的ServerMediaSubsession和MediaSource来实现将需要直播的H.264编码数据传递给live555，以实现RTSP直播。

服务端用到的模块以及继承关系：

#### RTSP协议简介

RTSP全称实时流协议（Real Time Streaming Protocol），它是一个网络控制协议，设计用于娱乐、会议系统中控制流媒体服务器。RTSP用于在希望通讯的两端建立并控制媒体会话，客户端通过发出VCR-style命令如play、record和pause等来实时控制媒体流。

RTSP处理流时会根据端点间可用带宽大小，将大数据切割成小分组，以使得客户端可以在播放一个分组的同时，可以解压第二个甚至同时下载第三个分组。这样一来，用户将不会感觉到数据间存在停顿。至于RTSP的特性，则主要体现在如下方面：

1. 多服务器兼容 ：媒体流可来自不同服务器
2. 可协商 ：客户端和服务器可协商feature支持程度
3. HTTP亲和性 ：尽可能重用HTTP概念，包括认证、状态码、解析等
4. 易解析 ：HTML或MIME解析器均可在RTSP中适用
5. 易扩展 ：新的方法或参数甚至协议本身均可添加或定制
6. 防火墙亲和性 ：传输层或应用层防火墙均可被协议较好处理
7. 服务器控制 ：控制概念易于理解，服务器不允许向客户端传输不能被客户端关闭的流
8. 多场景适用 ：RTSP提供帧级别精度，适用于更多媒体应用场景

RTSP组合使用了可靠传输协议TCP（控制）和高效传输协议UDP（内容）来串流内容给用户，即文件开始传输而客户端不用等待整个文件内容抵达就开始播放。其实不仅仅是点播服务，RTSP还支持传输不能以传统方式下载后播放的直播内容。

RTSP也并不负责数据传输，通常（非必须）是通过RTP（Real-time Transport Protocol）来完成，而RTP中，又通过RTCP负责完成同步、QOS管理等功能。具体应用中，它们三者的关系如下图所示：

RTSP：主要负责提供像播放、暂停、快进等操作，它负责定义具体的控制消息、操作方法、状态码等，此外还描述了与RTP间的交互操作。

RTP：传输音频/视频数据，如果是PLAY，Server发送到Client端，如果是RECORD，可以由Client发送到Server。整个RTP协议由两个密切相关的部分组成：RTP数据协议和RTP控制协议（RTCP）。

RTCP：服务质量的监视与反馈、媒体间的同步，以及多播组中成员的标识。在RTP会话期 间，各参与者周期性地传送RTCP包。RTCP包中含有已发送的数据包的数量、丢失的数据包的数量等统计资料，因此，各参与者可以利用这些信息动态地改变传输速率，甚至改变有效载荷类型。RTP和RTCP配合使用，它们能以有效的反馈和最小的开销使传输效率最佳化，因而特别适合传送网上的实时数据。

RTSP协议的方法以及报文规则请自行百度，这里便不再叙述。


#### 自行编译

1.  JNI头文件以及实现cpp放在jni/live/live555suihw文件夹下面，可以按照JNI规则自行修改。
2.  在JNI目录下面执行ndk-build命令，编译成so库。
3.  把so库放到项目libs里面，run即可得到RTSP服务。

#### 使用说明

run项目后，在局域网下通过流媒体播放器（VLC等）播放视频url。

#### JNI本地方法实现

```cpp
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
```



#### 移植中碰到的坑

在ndk交叉编译的时候出现类似与下图的error：

开始的时候以为是const标记的变量重新被赋值，通过查看源码发现是不同类型的赋值

GroupsockHelper.cpp

```cpp
Boolean socketLeaveGroupSSM(UsageEnvironment& /*env*/, int socket,
			    netAddressBits groupAddress,
			    netAddressBits sourceFilterAddr) {
  if (!IsMulticastAddress(groupAddress)) return True; // ignore this case

  struct ip_mreq_source imr;
#ifdef __ANDROID__
    imr.imr_multiaddr = groupAddress;
    imr.imr_sourceaddr = sourceFilterAddr;
    imr.imr_interface = ReceivingInterfaceAddr;
#else
    imr.imr_multiaddr = groupAddress;
    imr.imr_sourceaddr = sourceFilterAddr;
    imr.imr_interface = ReceivingInterfaceAddr;
#endif
  if (setsockopt(socket, IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP,
		 (const char*)&imr, sizeof (struct ip_mreq_source)) < 0) {
    return False;
  }

  return True;
}
```

```cpp
typedef u_int32_t netAddressBits;
```

其中netAddressBits的类型为u_int32_t。

struct ip_mreq_source:

```cpp
struct ip_mreq_source {
  struct  in_addr imr_multiaddr;  /* IP multicast address of group */
  struct  in_addr imr_sourceaddr; /* IP address of source */
  struct  in_addr imr_interface;  /* local IP address of interface */
};
```

in_addr.h:

```cpp
/** An integral type representing an IPv4 address. */
typedef uint32_t in_addr_t;

/** A structure representing an IPv4 address. */
struct in_addr {
  in_addr_t s_addr;
};
```

修改后ndk交叉编译成功。

```cpp
#ifdef __ANDROID__
    imr.imr_multiaddr.s_addr = groupAddress;
    imr.imr_sourceaddr.s_addr = sourceFilterAddr;
    imr.imr_interface.s_addr = ReceivingInterfaceAddr;
#else
```



#### 后续待研究方向

1. 传输高码率视频的时候会被live555内部默认帧解析buffer限制。
2. 内存拷贝占用问题。
3. 在android上面直接传输camera数据问题。