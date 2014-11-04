
project(DatasetIO C)

set(SOURCES
	"create.c"
	"read.c"
	"update.c"
	"delete.c"
)

find_package(Sqlite3 REQUIRED)
find_library(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIR} ${SQLITE3_INCLUDE_DIR})

if(CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(CMAKE_BUILD_TYPE STREQUAL "")

set(CMAKE_C_FLAGS_RELEASE "-Wall -Wextra -fomit-frame-pointer -O3 -DNDEBUG")
add_definitions ("-Wall -pedantic")

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} ${SQLITE3_LIBRARY})

