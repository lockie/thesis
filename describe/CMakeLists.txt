
project(Describe CXX)

set(SOURCES
	"describe.c"
	"hog.cpp"
	"phash.cpp"
)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIR} "${CMAKE_SOURCE_DIR}/datasetIO"
	"${CMAKE_SOURCE_DIR}/contrib/pHash/pHash-0.9.6/src"
	"${CMAKE_SOURCE_DIR}/contrib/pHash")

if(CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(CMAKE_BUILD_TYPE STREQUAL "")

set(CMAKE_C_FLAGS_RELEASE "-Wall -Wextra -fomit-frame-pointer -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
add_definitions("-Wall -pedantic")

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} DatasetIO pHash)

