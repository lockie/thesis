
project(pHash)

set(SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}/pHash-0.9.6")

set(SOURCES
	"${SRCDIR}/src/pHash.cpp"
)

set(CMAKE_THREAD_PREFER_PTHREAD true)
find_package(Threads REQUIRED)
find_package(CImg REQUIRED)

if(CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(CMAKE_BUILD_TYPE STREQUAL "")

set(CMAKE_C_FLAGS_RELEASE "-Wall -Wextra -fomit-frame-pointer -O3 -ffast-math -DNDEBUG")
add_definitions("-DHAVE_CONFIG_H -Dcimg_use_png")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

