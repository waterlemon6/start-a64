cmake_minimum_required(VERSION 3.9)
project(START2_20180607 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_COMPILER arm-linux-gcc)

add_executable(START2_20180607 main.c KernelInterface.c jb_jtag.c jrunner.c)