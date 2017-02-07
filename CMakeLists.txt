cmake_minimum_required(VERSION 3.5)
include(ExternalProject)
project(video)
file(GLOB_RECURSE SRC_LIST
         RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Caffe REQUIRED)
add_definitions(${Caffe_DEFINITIONS})

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(PkgConfig)
    pkg_check_modules(FFMPEG libavformat libavdevice libavcodec libswscale libavfilter libavutil libswresample libpostproc)
    set(FFMPEG_LIBRARY_DIRS ${FFMPEG_LIBDIR})
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
if(${CMAKE_VS_PLATFORM_NAME} MATCHES "Win32")
set(FFMPEG_DEV_URL https://ffmpeg.zeranoe.com/builds/win32//dev/ffmpeg-3.1.4-win32-dev.zip)
set(FFMPEG_DEV_HASH d4ad2cdc3eba8a3776c38823d4c881d33d47367d)
set(FFMPEG_SHARED_URL https://ffmpeg.zeranoe.com/builds/win32//shared/ffmpeg-3.1.4-win32-shared.zip)
set(FFMPEG_SHARED_HASH d308915d60b27edc427396c941d23a8b6fb66d17)
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
    add_dependencies(${PROJECT_NAME} ffmpeg_dev ffmpeg_shared)
endif()



enable_testing()
add_subdirectory(test)
link_directories(${FFMPEG_LIBRARY_DIRS})
add_library(${PROJECT_NAME} STATIC ${SRC_LIST})
target_compile_options(${PROJECT_NAME} PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest /await>)
target_compile_options(${PROJECT_NAME} PUBLIC $<$<CXX_COMPILER_ID:GNU>:-std=c++14>)
include_directories(${FFMPEG_INCLUDE_DIRS})
message(STATUS "FFMPEG Libs: " ${FFMPEG_LIBRARIES} " FFMPEG paths: " ${FFMPEG_INCLUDE_DIRS} " " ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${FFMPEG_LIBRARIES} ${Caffe_LIBRARIES} stdc++fs)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${FFMPEG_INCLUDE_DIRS} ${Caffe_INCLUDE_DIRS})
set_property(TARGET ${PROJECT_NAME} PROPERTY FFMPEG_BIN ${FFMPEG_BIN})
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    install(DIRECTORY ${FFMPEG_BIN}/ DESTINATION . COMPONENT video FILES_MATCHING PATTERN "*.dll")
endif()
install (TARGETS ${PROJECT_NAME} DESTINATION . COMPONENT video)
