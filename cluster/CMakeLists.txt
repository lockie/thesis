
project(Cluster CXX)

set(SOURCES
	"cluster.cpp"
)

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} "${CMAKE_SOURCE_DIR}/datasetIO")

if(CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(CMAKE_BUILD_TYPE STREQUAL "")

set(CMAKE_C_FLAGS_RELEASE "-Wall -Wextra -fomit-frame-pointer -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
add_definitions ("-Wall -pedantic")

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} ${Boost_LIBRARIES} DatasetIO)

