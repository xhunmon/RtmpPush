##############################################################################
####   避坑：项目build.gradle版本gradle:4.0.0不能生成动态库，使用3.4.1正常     ######
#############################################################################

#1、添加版本
cmake_minimum_required(VERSION 3.4.1)

# 引入指定目录下的CMakeLists.txt
add_subdirectory(${CMAKE_SOURCE_DIR}/librtmp)
#add_subdirectory(${CMAKE_SOURCE_DIR}/core)
message("AAA没日志输出吗？-2-")
include_directories(include)
#include_directories(${CMAKE_ANDROID_NDK}/sysroot/usr/include/)

file(GLOB core_srcs ${CMAKE_SOURCE_DIR}/core/*.cpp ${CMAKE_SOURCE_DIR}/core/*.h)

#2、编译库，把native-lib.cpp编译成以native-lib为名称的动态库
add_library(
        native-lib
        SHARED
        va_lib.cpp
        ${core_srcs})

#3、设置C++编译参数（CMAKE_CXX_FLAGS是全局变量）
#添加其他预编译库还可以使用这种方式
#使用-L指导编译时库文件的查找路径
#cxx_flags "cxx_flags -L目录"
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/libs/${CMAKE_ANDROID_ARCH_ABI}")

#4、链接到库文件，jni/c/c++可以引入链接到
target_link_libraries(
        native-lib
        rtmp
        x264
        faac
        log)