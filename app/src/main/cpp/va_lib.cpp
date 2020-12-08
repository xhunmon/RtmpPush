#include <jni.h>
#include <string>
#include "core/Pusher.h"

Pusher *pPusher = nullptr;


extern "C"
JNIEXPORT void JNICALL
Java_com_xhunmon_rtmppush_core_Pusher_native_1init(JNIEnv *env, jobject thiz, jint fps,
                                                   jint bit_rate, jint sample_rate, jint channels) {
    pPusher = new Pusher(fps, bit_rate, sample_rate, channels);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_xhunmon_rtmppush_core_Pusher_native_1prepare(JNIEnv *env, jobject thiz, jstring path_) {

    const char *path = env->GetStringUTFChars(path_, 0);
    char *url = new char[strlen(path) + 1];
    strcpy(url, path);
    if (!pPusher) {
        return;
    }
    pPusher->prepare(url);
    env->ReleaseStringUTFChars(path_, path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_xhunmon_rtmppush_core_Pusher_native_1stop(JNIEnv *env, jobject thiz) {
    if (!pPusher) {
        return;
    }
    pPusher->stop();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_xhunmon_rtmppush_core_Pusher_native_1audioPush(JNIEnv *env, jobject thiz,
                                                        jbyteArray bytes) {

    if (!pPusher || !pPusher->isReadyPushing) {
        return;
    }
    jbyte *data = env->GetByteArrayElements(bytes, NULL);
    pPusher->encodeAudioData(data);
    env->ReleaseByteArrayElements(bytes, data, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_xhunmon_rtmppush_core_Pusher_native_1videoDataChange(JNIEnv *env, jobject thiz, jint width,
                                                              jint height) {
    if (!pPusher) {
        return;
    }
    pPusher->videoDataChange(width,height);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_xhunmon_rtmppush_core_Pusher_native_1videoPush(JNIEnv *env, jobject thiz,
                                                        jbyteArray bytes) {

    if (!pPusher || !pPusher->isReadyPushing) {
        return;
    }
    jbyte *data = env->GetByteArrayElements(bytes, NULL);
    pPusher->encodeVideoData(data);
    env->ReleaseByteArrayElements(bytes, data, 0);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_xhunmon_rtmppush_core_Pusher_native_1audioGetSamples(JNIEnv *env, jobject thiz) {
    if (!pPusher) {
        return -1;
    }
    return pPusher->audioGetSamples();
}