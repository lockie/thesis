
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(contrib)
add_subdirectory(datasetIO)
add_subdirectory(extractor)
add_subdirectory(dataset)
add_subdirectory(purify)
add_subdirectory(describe)
add_subdirectory(cluster)

