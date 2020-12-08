/**
 * description:   <br>
 * @author 秦城季
 * @date 2020/12/6
 */

#include "Pusher.h"

Pusher::Pusher(int fps, int bitRate, int sampleRate, int channels) {
    audioTask = new AudioTask;
    ((AudioTask *) audioTask)->setDataCallback(callback);
    ((AudioTask *) audioTask)->init(sampleRate, channels);

    videoTask = new VideoTask(fps, bitRate);
    ((VideoTask *) videoTask)->setDataCallback(callback);

    packets.setReleaseCallback(release);
}

void Pusher::encodeAudioData(int8_t *data) {
    ((AudioTask *) audioTask)->encodeData(data);
}

void Pusher::prepare(char *url) {
    this->url = url;
    isReadyPushing = 1;
    packets.setWork(1);
    pthread_create(&pid, 0, pushTask, this);
}

void Pusher::stop() {
    if (!isReadyPushing) {//没有启动过
        return;
    }
    isReadyPushing = 0;
    packets.setWork(0);
//    pthread_join(pid, 0);
    pthread_detach(pid);
}

void Pusher::videoDataChange(int width, int height) {
    ((VideoTask *) videoTask)->dataChange(width, height);
}

void Pusher::encodeVideoData(int8_t *data) {
    ((VideoTask *) videoTask)->encodeData(data);
}

u_long Pusher::audioGetSamples() {
    return ((AudioTask *) audioTask)->getSamples();
}

void callback(RTMPPacket *packet) {
    if (packet) {
        //设置时间戳
        packet->m_nTimeStamp = RTMP_GetTime() - start_time;
        packets.push(packet);
    }
}

void release(RTMPPacket *&packet) {
    if (packet) {
        RTMPPacket_Free(packet);
        delete packet;
        packet = 0;
        LOGD("释放资源。。。。。");
    }
}

/**
 * 线程最后返回0千万别忘记了
 */
void *pushTask(void *args) {
    Pusher *pusher = static_cast<Pusher *>(args);
    char *url = pusher->url;

    RTMP *rtmp = RTMP_Alloc();
    do {
        if (!rtmp) {
            LOGD("alloc rtmp失败~~~~");
            break;
        }
        RTMP_Init(rtmp);
        int ret = RTMP_SetupURL(rtmp, url);
        if (!ret) {
            LOGD("设置地址失败:%s", url);
            break;
        }
        //15s超时时间
        rtmp->Link.timeout = 15;
        RTMP_EnableWrite(rtmp);
        ret = RTMP_Connect(rtmp, 0);
        if (!ret) {
            LOGD("连接服务器:%s", url);
            break;
        }
        ret = RTMP_ConnectStream(rtmp, 0);
        if (!ret) {
            LOGD("连接流:%s", url);
            break;
        }
        //记录一个开始时间
        start_time = RTMP_GetTime();
        //表示可以开始推流了
        //保证第一个数据是 aac解码数据包
//        callback(audioChannel->getAudioTag());
        RTMPPacket *packet = 0;
        LOGD("准备推流~~~~~~~~");
        while (pusher->isReadyPushing) {
            packets.pop(packet);
            if (!packet) {
                continue;
            }
            packet->m_nInfoField2 = rtmp->m_stream_id;
            //发送rtmp包 1：队列
            // 意外断网？发送失败，rtmpdump 内部会调用RTMP_Close
            // RTMP_Close 又会调用 RTMP_SendPacket
            // RTMP_SendPacket  又会调用 RTMP_Close
            // 将rtmp.c 里面WriteN方法的 Rtmp_Close注释掉
            ret = RTMP_SendPacket(rtmp, packet, 1);
            if (!ret) {
                LOGD("发送失败~~~~~~~~");
                break;
            } else {
                LOGD("发送成功：%d", packet->m_nTimeStamp);
            }
        }
    } while (0);
    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }
    packets.clear();
    delete (url);
    LOGD("结束推流~~~~~~~~");
    return 0;
}