# $ mkdir build
# $ cd build
# $ cmake ../
# $ cmake --build .
# $ ./runner

cmake_minimum_required(VERSION 3.5)
project(runner VERSION 1 LANGUAGES CXX)

add_library(unixduino STATIC
    ${UNIXHOSTDUINO}/Arduino.cpp
    ${UNIXHOSTDUINO}/Print.cpp
    ${UNIXHOSTDUINO}/StdioSerial.cpp
    ${UNIXHOSTDUINO}/Stream.cpp
    ${UNIXHOSTDUINO}/WString.cpp
    ${UNIXHOSTDUINO}/Wire.cpp
    ${UNIXHOSTDUINO}/avr_stdlib.cpp
    ${UNIXHOSTDUINO}/main.cpp
)

target_include_directories(unixduino PUBLIC
    ${UNIXHOSTDUINO}
)

add_library(rpnlib STATIC
    src/rpnlib_variable.cpp
    src/rpnlib_stack.cpp
    src/rpnlib_operators.cpp
    src/rpnlib_value.cpp
    src/rpnlib.cpp
)
add_executable(runner runner.cpp)

set_target_properties(runner PROPERTIES COMPILE_FLAGS -g)
set_target_properties(rpnlib PROPERTIES COMPILE_FLAGS -g)

target_link_libraries(rpnlib unixduino)
target_link_libraries(runner rpnlib)
