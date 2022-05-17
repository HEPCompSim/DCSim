cmake_minimum_required(VERSION 3.2)
message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

project(%s)

add_definitions("-Wall -Wno-unused-variable -Wno-unused-private-field")

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

find_package(SimGrid REQUIRED)
find_package(Boost COMPONENTS program_options REQUIRED)


# include directories for dependencies and WRENCH libraries
include_directories(src/ ${SimGrid_INCLUDE_DIR}/include /usr/local/include /opt/local/include /usr/local/include/wrench ${Boost_INCLUDE_DIR})

# source files
set(SOURCE_FILES
        src/SimpleExecutionController.h
        src/SimpleExecutionController.cpp
        src/SimpleSimulator.h
        src/SimpleSimulator.cpp
        src/JobSpecification.h
        src/LRU_FileList.h
        src/MonitorAction.h
        src/MonitorAction.cpp
        src/computation/CacheComputation.h
        src/computation/CacheComputation.cpp
        src/computation/StreamedComputation.h
        src/computation/StreamedComputation.cpp
        src/computation/CopyComputation.h
        src/computation/CopyComputation.cpp
        )

# test files
set(TEST_FILES
        src/JobSpecification.h
        src/MonitorAction.h
        src/computation/CacheComputation.h
        src/computation/StreamedComputation.h
        src/computation/CopyComputation.h
        src/LRU_FileList.h
        src/SimpleSimulator.h
        )

# wrench library and dependencies
find_library(WRENCH_LIBRARY NAMES wrench)
find_library(PUGIXML_LIBRARY NAMES pugixml)
find_library(GTEST_LIBRARY NAMES gtest)
find_library(SimGrid_LIBRARY NAMES simgrid)

# generating the executable
add_executable(sgbatch-sim ${SOURCE_FILES})

if (ENABLE_BATSCHED)
target_link_libraries(sgbatch-sim
                       ${WRENCH_LIBRARY}
                       ${SimGrid_LIBRARY}
                       ${Boost_LIBRARIES}
                       ${PUGIXML_LIBRARY}
                      -lzmq )
else()
target_link_libraries(sgbatch-sim
                       ${WRENCH_LIBRARY}
                       ${SimGrid_LIBRARY}
                       ${Boost_LIBRARIES}
                       ${PUGIXML_LIBRARY}
                      )
endif()

install(TARGETS sgbatch-sim DESTINATION bin)

# generating unit tests
add_executable(unit_tests EXCLUDE_FROM_ALL
        ${SOURCE_FILES}
        ${TEST_FILES}
        )
target_link_libraries(unit_tests
        ${GTEST_LIBRARY} wrench -lpthread -lm
        )
