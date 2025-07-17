cmake_minimum_required(VERSION 3.12)
message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

project(DCSim)

add_definitions("-Wall -Wno-unused-variable -Wno-unused-private-field")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

find_package(SimGrid REQUIRED)
find_package(WRENCH REQUIRED)
find_package(FSMod REQUIRED)

# Find Boost
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
find_package(Boost COMPONENTS program_options regex REQUIRED)


# include directories for dependencies and WRENCH libraries
include_directories(src/ ${SimGrid_INCLUDE_DIR}/include /usr/local/include /opt/local/include /usr/local/include/wrench ${Boost_INCLUDE_DIR})

# source files
set(SOURCE_FILES
        src/WorkloadExecutionController.h
        src/WorkloadExecutionController.cpp
        src/JobScheduler.h
        src/JobScheduler.cpp
        src/SimpleSimulator.h
        src/SimpleSimulator.cpp
        src/JobSpecification.h
        src/Workload.h
        src/Workload.cpp
        src/Dataset.h
        src/Dataset.cpp
        src/LRU_FileList.h
        src/MonitorAction.h
        src/MonitorAction.cpp
        src/util/DefaultValues.h
        src/util/Utils.h
        src/computation/CacheComputation.h
        src/computation/CacheComputation.cpp
        src/computation/StreamedComputation.h
        src/computation/StreamedComputation.cpp
        src/computation/CopyComputation.h
        src/computation/CopyComputation.cpp
        src/JobScheduler.cpp src/JobScheduler.h)

# test files
set(TEST_FILES
        src/JobSpecification.h
        src/MonitorAction.h
        src/util/DefaultValues.h
        src/util/Utils.h
        src/computation/CacheComputation.h
        src/computation/StreamedComputation.h
        src/computation/CopyComputation.h
        src/LRU_FileList.h
        src/SimpleSimulator.h
        src/WorkloadExecutionController.h
        src/Workload.h
        )

# wrench library and dependencies
find_library(WRENCH_LIBRARY NAMES wrench)
find_library(SimGrid_LIBRARY NAMES simgrid)

# generating the executable
add_library(DCSim SHARED ${SOURCE_FILES})
add_executable(dc-sim ${SOURCE_FILES})

if (ENABLE_BATSCHED)
target_link_libraries(dc-sim
                      DCSim
                      ${WRENCH_LIBRARY}
		      ${FSMOD_LIBRARY}
                      ${SimGrid_LIBRARY}
                      ${Boost_LIBRARIES}
                      -lzmq )
else()
target_link_libraries(dc-sim
                      DCSim
                      ${WRENCH_LIBRARY}
		      ${FSMOD_LIBRARY}
                      ${SimGrid_LIBRARY}
                      ${Boost_LIBRARIES})
endif()

# set_property(TARGET dc-sim PROPERTY CXX_STANDARD 17)

install(TARGETS DCSim DESTINATION lib)
install(TARGETS dc-sim DESTINATION bin)
