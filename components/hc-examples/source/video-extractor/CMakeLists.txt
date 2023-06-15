cmake_minimum_required(VERSION 3.10.0)
project(extractor)
set(CMAKE_BUILD_TYPE "release")

set(FFMPEG_DIR "/media/ps/data/home/weiwen.liu/Download/ffmpeg-4.4.1/")
include_directories(${PROJECT_SOURCE_DIR}
		${FFMPEG_DIR}
                    )

link_directories(${PROJECT_SOURCE_DIR}
                    )
add_executable(${PROJECT_NAME} extractor.c)
#set(FFMPEG_LIB  avfilter avdevice avformat avcodec swscale avutil swresample)
set(FFMPEG_LIB  avformat avcodec avutil swresample)
message("${FFMPEG_LIB}")
target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIB} -lm -pthread )
