cmake_minimum_required(VERSION 3.24)

project(A3
        VERSION 0.2.1
        DESCRIPTION "TCP File Transfer w/ FSM"
        LANGUAGES C)

set(CMAKE_C_STANDARD 17)

set(SANITIZE TRUE)

set(SOURCE_DIR src)
set(INCLUDE_DIR include)

set(SOURCE_LIST ${SOURCE_DIR}/main.c
                ${SOURCE_DIR}/client.c
        )
set(HEADER_LIST ${INCLUDE_DIR}/client.h
        )

include_directories(${INCLUDE_DIR})

add_executable(A3 ${SOURCE_MAIN} ${SOURCE_LIST} ${HEADER_LIST})
target_include_directories(A3 PRIVATE include)
target_include_directories(A3 PRIVATE /usr/local/include)
target_link_directories(A3 PRIVATE /usr/local/lib)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_include_directories(A3 PRIVATE /usr/include)
endif ()

set_target_properties(A3 PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR})

find_library(LIBDC_ERROR dc_error REQUIRED)
find_library(LIBDC_ENV dc_env REQUIRED)
find_library(LIBDC_C dc_c REQUIRED)
find_library(LIBDC_POSIX dc_posix REQUIRED)
find_library(LIBDC_UNIX dc_unix REQUIRED)
find_library(LIBDC_UTIL dc_util REQUIRED)
find_library(LIBDC_FSM dc_fsm REQUIRED)
find_library(LIB_CONFIG config REQUIRED)
find_library(LIBDC_APPLICATION dc_application REQUIRED)

target_link_libraries(A3 PUBLIC ${LIBDC_ERROR})
target_link_libraries(A3 PUBLIC ${LIBDC_ENV})
target_link_libraries(A3 PUBLIC ${LIBDC_C})
target_link_libraries(A3 PUBLIC ${LIBDC_POSIX})
target_link_libraries(A3 PUBLIC ${LIBDC_UNIX})
target_link_libraries(A3 PUBLIC ${LIBDC_UTIL})
target_link_libraries(A3 PUBLIC ${LIBDC_FSM})
target_link_libraries(A3 PUBLIC ${LIB_CONFIG})
target_link_libraries(A3 PUBLIC ${LIBDC_APPLICATION})

# Add compiler flags
target_compile_options(A3 PRIVATE
        -Wall              # Enable all compiler warnings
        -Wextra            # Enable extra compiler warnings
        -pedantic          # Enable pedantic mode
        -O2                # Optimization level 2
        -g                 # Generate debug information
        -fPIC              # Generate position-independent code
        # Add more flags as needed
)

set_target_properties(A3 PROPERTIES OUTPUT_NAME "client")
install(TARGETS A3 DESTINATION bin)