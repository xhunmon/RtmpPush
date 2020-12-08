cmake_minimum_required(VERSION 3.4.1)
#预编译宏
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_CRYPTO"  )
#所有源文件放入 rtmp_source 变量
file(GLOB rtmp_source *.c)
#编译静态库
add_library(rtmp STATIC ${rtmp_source} )