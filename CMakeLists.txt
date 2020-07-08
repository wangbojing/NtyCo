cmake_minimum_required(VERSION 2.8)

project(NtyCo)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

include_directories(
	core
)


# 第三方库
set(LIBS crypto pthread ssl)

subdirs(
	core
	sample
)
