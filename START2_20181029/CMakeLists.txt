cmake_minimum_required(VERSION 3.9)
project(START2_20181029 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_COMPILER arm-linux-gcc)
#set(CMAKE_C_COMPILER /home/waterlemon/arm-linux/cqa64_linux_qt5.3.2/lichee/out/sun50iw1p1/dragonboard/common/buildroot/external-toolchain_32/bin/arm-linux-gnueabi-gcc)

link_libraries(/usr/local/libjpeg-turbo_arm/lib32/libjpeg.a)
link_libraries(/usr/local/libjpeg-turbo_arm/lib32/libturbojpeg.a)
link_libraries(pthread)

add_executable(START2_20181029 main.c PrinterControl.c Jpeg.c Scan.c LightAdjustment.c Correction.c Update.c KernelLed.c KernelSpi.c KernelVideo.c ColorRemap.c)