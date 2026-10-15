cmake_minimum_required(VERSION 3.13)

project(battery_percent VERSION 2.0.0 LANGUAGES C)

add_library(battery_percent src/battery_percent.c)
add_library(battery_percent::battery_percent ALIAS battery_percent)
target_include_directories(battery_percent PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
set_target_properties(battery_percent PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
    if(BUILD_TESTING)
        add_executable(test_battery_percent tests/test_battery_percent.c)
        target_link_libraries(test_battery_percent PRIVATE battery_percent)
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(battery_percent PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
            target_compile_options(test_battery_percent PRIVATE -Wall -Wextra -Werror)
        endif()
        add_test(NAME test_battery_percent COMMAND test_battery_percent)
    endif()
endif()

install(TARGETS battery_percent EXPORT battery_percent_targets ARCHIVE DESTINATION lib)
install(FILES include/battery_percent.h DESTINATION include)
