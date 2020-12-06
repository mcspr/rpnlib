# $ mkdir build
# $ cd build
# $ cmake ../ -DESP8266_ARDUINO_CORE_PATH=.. -DUNITY_PATH=..
# $ cmake --build .
# $ ./repl

cmake_minimum_required(VERSION 3.5)
project(host-examples VERSION 1 LANGUAGES C CXX)

# required for esp8266 host mocking
set(COMMON_FLAGS
    -Os
    -g
    -fno-common
    -funsigned-char
    -DCORE_MOCK
    -DHOST_MOCK=1
    -DLWIP_IPV6=0
)
set(RPNLIB_PATH ${CMAKE_SOURCE_DIR}/../../ CACHE FILEPATH "RPNLIB source code repo root")

# base for test/piotest
add_library(unity STATIC
    ${UNITY_PATH}/unity.c
)
target_compile_options(unity BEFORE PRIVATE
    -DUNITY_OUTPUT_COLOR
)
target_include_directories(unity PUBLIC
    ${UNITY_PATH}
)

# try to hack esp8266 host test layer
# - we need to specify bunch of things that the original Makefile does
# - there are a lot of cross-dependencies, we need to include a lot of .cpp files here
add_library(esp8266 STATIC
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/Arduino.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/ArduinoMain.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/ArduinoMainUdp.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/MockUART.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/MockTools.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/MockEsp.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/UdpContextSocket.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/user_interface.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/noniso.c
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/flash_hal_mock.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/spiffs_mock.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/littlefs_mock.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/sdfs_mock.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/ArduinoMainLittlefs.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/ArduinoMainSpiffs.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/cores/esp8266/core_esp8266_noniso.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/cores/esp8266/WString.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/cores/esp8266/HardwareSerial.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/cores/esp8266/Print.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/cores/esp8266/Stream.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/cores/esp8266/FS.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/cores/esp8266/spiffs_api.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/cores/esp8266/spiffs/spiffs_cache.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/cores/esp8266/spiffs/spiffs_check.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/cores/esp8266/spiffs/spiffs_gc.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/cores/esp8266/spiffs/spiffs_hydrogen.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/cores/esp8266/spiffs/spiffs_nucleus.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/libraries/LittleFS/src/LittleFS.cpp
    ${ESP8266_ARDUINO_CORE_PATH}/libraries/LittleFS/src/lfs.c
    ${ESP8266_ARDUINO_CORE_PATH}/libraries/LittleFS/src/lfs_util.c
)
target_include_directories(esp8266 PUBLIC
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/
    ${ESP8266_ARDUINO_CORE_PATH}/tests/host
    ${ESP8266_ARDUINO_CORE_PATH}/tools/sdk/lwip2/include
    ${ESP8266_ARDUINO_CORE_PATH}/tools/sdk/include
    ${ESP8266_ARDUINO_CORE_PATH}/cores/esp8266/
    ${ESP8266_ARDUINO_CORE_PATH}/libraries/LittleFS/src/
    ${ESP8266_ARDUINO_CORE_PATH}/libraries/SPI/
    ${ESP8266_ARDUINO_CORE_PATH}/libraries/ESP8266SdFat/src
)
target_compile_options(esp8266 BEFORE PRIVATE
    "SHELL:-include ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/mock.h"
    "SHELL:-include ${ESP8266_ARDUINO_CORE_PATH}/tests/host/common/c_types.h"
)
target_compile_options(esp8266 PUBLIC
    ${COMMON_FLAGS}
    -DF_CPU=80000000
    -Wl,--defsym,_FS_start=0x40300000
    -Wl,--defsym,_FS_end=0x411FA000
    -Wl,--defsym,_FS_page=0x100
    -Wl,--defsym,_FS_block=0x2000
    -Wl,--defsym,_EEPROM_start=0x411fb000
)

# our library source (can probably add as *.cpp + *.c)
add_library(rpnlib STATIC
    ${RPNLIB_PATH}/src/fs_math.c
    ${RPNLIB_PATH}/src/rpnlib_fmath.cpp
    ${RPNLIB_PATH}/src/rpnlib_operators.cpp
    ${RPNLIB_PATH}/src/rpnlib_stack.cpp
    ${RPNLIB_PATH}/src/rpnlib_value.cpp
    ${RPNLIB_PATH}/src/rpnlib_variable.cpp
    ${RPNLIB_PATH}/src/rpnlib.cpp
)
target_include_directories(rpnlib PUBLIC
    ${RPNLIB_PATH}/src/
)
target_compile_options(rpnlib PUBLIC
    ${COMMON_FLAGS}
    -DRPNLIB_INT_TYPE=int64_t
    -DRPNLIB_UINT_TYPE=uint64_t
    -DRPNLIB_ADVANCED_MATH
)
target_compile_options(rpnlib PRIVATE
    -Wall
)

# small repl test
add_executable(repl repl.cpp)
target_compile_options(repl PRIVATE
    ${COMMON_FLAGS}
)

set_target_properties(repl PROPERTIES COMPILE_FLAGS -g)
set_target_properties(rpnlib PROPERTIES COMPILE_FLAGS -g)

target_link_libraries(rpnlib esp8266)
target_link_libraries(repl rpnlib)

# like `pio test`, but without `pio`
add_executable(piotest ${RPNLIB_PATH}/test/piotest/main.cpp)
target_link_libraries(piotest unity rpnlib)
target_compile_options(piotest PRIVATE
    ${COMMON_FLAGS}
    -Wall
)
set_target_properties(piotest PROPERTIES COMPILE_FLAGS -g)

# try out example code
function(example_build)
    foreach(ARG IN LISTS ARGN)
        configure_file(${RPNLIB_PATH}/examples/${ARG}/${ARG}.ino ${CMAKE_BINARY_DIR}/${ARG}.cpp COPYONLY)
        add_executable(example-${ARG} ${CMAKE_BINARY_DIR}/${ARG}.cpp)
        target_link_libraries(example-${ARG} rpnlib)
        target_compile_options(example-${ARG} PRIVATE
            ${COMMON_FLAGS}
        )
        set_target_properties(example-${ARG} PROPERTIES COMPILE_FLAGS -g)
    endforeach()
endfunction()

# TODO: time example won't run on host, but at least we can try to build it
example_build(basic debug time)
