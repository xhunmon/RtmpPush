/**
 * description:   <br>
 * @author 秦城季
 * @date 2020/11/12
 */

#include <android/log.h>

#define TAG "va_native"

#ifndef CLOSE_DEBUG
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG,__VA_ARGS__)
#else
#define LOGD(...) /*printEmpty(__VA_ARGS__)*/
#endif

//宏函数
#define DELETE(obj) if(obj){ delete obj; obj = 0; }