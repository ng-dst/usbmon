cmake_minimum_required(VERSION 3.26)
project(usbmon C)
set(CMAKE_C_STANDARD 23)

set(SVC_NAME "UsbMonitor")

add_definitions(-DSVCNAME=\"${SVC_NAME}\")

add_executable(usbmon main.c src/service.c src/devinfo.c src/usbmon.c src/event.c src/cfg.c)
target_link_libraries(usbmon setupapi -static)
