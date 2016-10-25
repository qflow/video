cmake_minimum_required(VERSION 3.6)
include(ExternalProject)
project(video)
file(GLOB_RECURSE SRC_LIST
         RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
if(${CMAKE_VS_PLATFORM_NAME} MATCHES "Win32")
set(FFMPEG_DEV_URL https://ffmpeg.zeranoe.com/builds/win32//dev/ffmpeg-3.1.4-win32-dev.zip)
set(FFMPEG_DEV_HASH 761558f05b44594ede26b00f968b9c839881d019)
set(FFMPEG_SHARED_URL https://ffmpeg.zeranoe.com/builds/win32//shared/ffmpeg-3.1.4-win32-shared.zip)
set(FFMPEG_SHARED_HASH a3494d37689ac3920389b4df4fd85ca3b4ebbd56)
endif()
if(${CMAKE_VS_PLATFORM_NAME} MATCHES "x64")
set(FFMPEG_DEV_URL https://ffmpeg.zeranoe.com/builds/win64//dev/ffmpeg-3.1.4-win64-dev.zip)
set(FFMPEG_DEV_HASH db253d861591a35d26377fe6f3d1a77be53881c6)
set(FFMPEG_SHARED_URL https://ffmpeg.zeranoe.com/builds/win64//shared/ffmpeg-3.1.4-win64-shared.zip)
set(FFMPEG_SHARED_HASH 279c3321af92a3fd42f0f98d7f8016e3a5862971)
endif()
    ExternalProject_Add(
        ffmpeg_dev
        PREFIX ffmpeg_dev
        URL ${FFMPEG_DEV_URL}
        URL_HASH SHA1=${FFMPEG_DEV_HASH}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        INSTALL_COMMAND ""
    )
    ExternalProject_Add(
        ffmpeg_shared
        PREFIX ffmpeg_shared
        URL ${FFMPEG_SHARED_URL}
        URL_HASH SHA1=${FFMPEG_SHARED_HASH}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        INSTALL_COMMAND ""
    )
    set(FFMPEG_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/ffmpeg_dev/src/ffmpeg_dev/include)
    message(STATUS "ffmpeg: " ${FFMPEG_INCLUDE_DIRS})
    set(FFMPEG_BIN ${PROJECT_BINARY_DIR}/ffmpeg_shared/src/ffmpeg_shared/bin)
    set(FFMPEG_LIBRARIES avformat avcodec avfilter avutil swscale swresample postproc avdevice)
    link_directories(${PROJECT_BINARY_DIR}/ffmpeg_dev/src/ffmpeg_dev/lib)
endif()
enable_testing()
add_subdirectory(test)
add_library(${PROJECT_NAME} STATIC ${SRC_LIST})
target_compile_options(${PROJECT_NAME} PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest /await>)
include_directories(${FFMPEG_INCLUDE_DIRS})
message(STATUS "FFMPEG Libs: " ${FFMPEG_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${FFMPEG_LIBRARIES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${FFMPEG_INCLUDE_DIRS})
set_property(TARGET ${PROJECT_NAME} PROPERTY FFMPEG_BIN ${FFMPEG_BIN})
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    install(DIRECTORY ${FFMPEG_BIN}/ DESTINATION . COMPONENT video FILES_MATCHING PATTERN "*.dll")
endif()
install (TARGETS ${PROJECT_NAME} DESTINATION . COMPONENT video)