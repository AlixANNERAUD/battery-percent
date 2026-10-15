cmake_minimum_required(VERSION 3.13)

project(battery_percent VERSION 2.1.0 LANGUAGES C)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(BATTERY_PERCENT_TOP_LEVEL ON)
else()
    set(BATTERY_PERCENT_TOP_LEVEL OFF)
endif()

option(BATTERY_PERCENT_BUILD_TESTS "Build unit tests" ${BATTERY_PERCENT_TOP_LEVEL})
option(BATTERY_PERCENT_WERROR "Treat warnings as errors" OFF)
option(BATTERY_PERCENT_SANITIZE "Build with address and undefined sanitizers" OFF)
option(BATTERY_PERCENT_COVERAGE "Instrument the library for code coverage" OFF)

if(BATTERY_PERCENT_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_library(battery_percent src/battery_percent.c)
add_library(battery_percent::battery_percent ALIAS battery_percent)
target_include_directories(battery_percent PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
set_target_properties(battery_percent PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(battery_percent PRIVATE -Wall -Wextra -Wpedantic -Wconversion $<$<BOOL:${BATTERY_PERCENT_WERROR}>:-Werror>)
    target_compile_options(battery_percent PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wdocumentation>)
endif()

if(BATTERY_PERCENT_COVERAGE)
    target_compile_options(battery_percent PRIVATE --coverage -O0)
    target_link_options(battery_percent PUBLIC --coverage)
endif()

if(BATTERY_PERCENT_BUILD_TESTS)
    enable_testing()
    add_executable(test_battery_percent tests/test_battery_percent.c)
    target_link_libraries(test_battery_percent PRIVATE battery_percent)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(test_battery_percent PRIVATE -Wall -Wextra $<$<BOOL:${BATTERY_PERCENT_WERROR}>:-Werror>)
    endif()
    add_test(NAME test_battery_percent COMMAND test_battery_percent)
endif()

install(TARGETS battery_percent EXPORT battery_percent_targets ARCHIVE DESTINATION lib)
install(FILES include/battery_percent.h DESTINATION include)
