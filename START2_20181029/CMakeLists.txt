cmake_minimum_required(VERSION 3.9)
project(START2_20181029 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)

link_libraries(/home/waterlemon/jpeg/libjpeg-turbo-2.0.1/a64-64bit/a64/libjpeg.a)
link_libraries(/home/waterlemon/jpeg/libjpeg-turbo-2.0.1/a64-64bit/a64/libturbojpeg.a)
link_libraries(pthread)

add_executable(START2_20181029 main.c PrinterControl.c Jpeg.c Scan.c LightAdjustment.c Correction.c Update.c KernelLed.c KernelSpi.c KernelVideo.c ColorRemap.c)