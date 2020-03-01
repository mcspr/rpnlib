# $ mkdir build
# $ cd build
# $ cmake ../
# $ cmake --build .
# $ ./runner


cmake_minimum_required(VERSION 3.5)
project(runner VERSION 1 LANGUAGES C CXX)

# required for esp8266 host mocking
set(COMMON_FLAGS
    -DCORE_MOCK
    -DHOST_MOCK=1
    -DLWIP_IPV6=0
)

# base for test/piotest
add_library(unity STATIC
    ${UNITY}/unity_fixture_stubs.c
    ${UNITY}/unity.c
)
target_include_directories(unity PUBLIC
    ${UNITY}
)

# try to hack esp8266 host test layer
# - we need to specify bunch of things that the original Makefile does
# - there are a lot of cross-dependencies, we need to include a lot of .cpp files here
add_library(esp8266 STATIC
    ${ESP8266}/tests/host/common/Arduino.cpp
    ${ESP8266}/tests/host/common/ArduinoMain.cpp
    ${ESP8266}/tests/host/common/ArduinoMainUdp.cpp
    ${ESP8266}/tests/host/common/MockUART.cpp
    ${ESP8266}/tests/host/common/MockTools.cpp
    ${ESP8266}/tests/host/common/MockEsp.cpp
    ${ESP8266}/tests/host/common/UdpContextSocket.cpp
    ${ESP8266}/tests/host/common/user_interface.cpp
    ${ESP8266}/tests/host/common/noniso.c
    ${ESP8266}/cores/esp8266/core_esp8266_noniso.cpp
    ${ESP8266}/cores/esp8266/WString.cpp
    ${ESP8266}/cores/esp8266/HardwareSerial.cpp
    ${ESP8266}/cores/esp8266/Print.cpp
    ${ESP8266}/cores/esp8266/Stream.cpp
)
target_include_directories(esp8266 PUBLIC
    ${ESP8266}/tests/host/common/
    ${ESP8266}/tests/host
    ${ESP8266}/tools/sdk/lwip2/include
    ${ESP8266}/tools/sdk/include
    ${ESP8266}/cores/esp8266/
)
target_compile_options(esp8266 BEFORE PRIVATE
    "SHELL:-include ${ESP8266}/tests/host/common/mock.h"
    "SHELL:-include ${ESP8266}/tests/host/common/c_types.h"
)
target_compile_options(esp8266 PUBLIC
    ${COMMON_FLAGS}
    -Wl,--defsym,_FS_start=0x40300000
    -Wl,--defsym,_FS_end=0x411FA000
    -Wl,--defsym,_FS_page=0x100
    -Wl,--defsym,_FS_block=0x2000
    -Wl,--defsym,_EEPROM_start=0x411fb000
)

# our library source (can probably add as *.cpp + *.c)
add_library(rpnlib STATIC
    src/fs_math.c
    src/rpnlib_fmath.cpp
    src/rpnlib_variable.cpp
    src/rpnlib_stack.cpp
    src/rpnlib_operators.cpp
    src/rpnlib_value.cpp
    src/rpnlib.cpp
)
target_include_directories(rpnlib PUBLIC
    src/
)
target_compile_options(rpnlib PUBLIC
    ${COMMON_FLAGS}
    -DRPNLIB_ADVANCED_MATH
)
target_compile_options(rpnlib PRIVATE
    -Wall
)

# small repl test
add_executable(runner runner.cpp)
target_compile_options(runner PRIVATE
    ${COMMON_FLAGS}
)

set_target_properties(runner PROPERTIES COMPILE_FLAGS -g)
set_target_properties(rpnlib PROPERTIES COMPILE_FLAGS -g)

target_link_libraries(rpnlib esp8266)
target_link_libraries(runner rpnlib)

# like `pio test`, but without `pio`
add_executable(piotest test/piotest/main.cpp)
target_link_libraries(piotest unity rpnlib)
target_compile_options(piotest PRIVATE
    ${COMMON_FLAGS}
)
set_target_properties(piotest PROPERTIES COMPILE_FLAGS -g)

# try out one example
#add_executable(example-time examples/time/time.cpp)
#set_target_properties(example-time PROPERTIES LINKER_LANGUAGE CXX)
#target_link_libraries(example-time rpnlib)
#target_compile_options(example-time PRIVATE
#    ${COMMON_FLAGS}
#)
#set_target_properties(example-time PROPERTIES COMPILE_FLAGS -g)
