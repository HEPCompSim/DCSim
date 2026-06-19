cmake_minimum_required(VERSION 3.12)
message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

project(DCSim CXX)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_definitions("-Wall -Wno-unused-variable -Wno-unused-private-field")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

# Find Boost
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
find_package(Boost COMPONENTS program_options regex REQUIRED)

# Find SimGrid
if (DEFINED SimGrid_PATH)
    if (NOT EXISTS "${SimGrid_PATH}")
        message(FATAL_ERROR "The specified SimGrid_PATH doesn't exit")
    else()
        file(GLOB LIBSIMGRID_FILES
                "${SimGrid_PATH}/lib/libsimgrid.so"
                "${SimGrid_PATH}/lib/libsimgrid.dylib"
        )
        if (NOT LIBSIMGRID_FILES)
            message(FATAL_ERROR "The specified SimGrid_PATH doesn't seem to contain a valid SimGrid installation")
        else()
            set(CMAKE_PREFIX_PATH ${SimGrid_PATH} ${CMAKE_PREFIX_PATH})
        endif()
    endif()
endif()
find_package(SimGrid REQUIRED)

# Find SimGrid's FS module
if (DEFINED FSMOD_PATH)
    if (NOT EXISTS "${FSMOD_PATH}")
        message(FATAL_ERROR "The specified FSMOD_PATH doesn't exit")
    else()
        file(GLOB LIBFSMOD_FILES
                "${FSMOD_PATH}/liblibfsmod.so"
                "${FSMOD_PATH}/lib/libfsmod.dylib"
        )
        if (NOT LIBFSMOD_FILES)
            message(FATAL_ERROR "The specified FSMOD_PATH doesn't seem to contain a valid FSMod installation")
        else()
            set(CMAKE_PREFIX_PATH ${FSMOD_PATH} ${CMAKE_PREFIX_PATH})
        endif()
    endif()
endif()
find_package(FSMod REQUIRED)

# Find WRENCH
if (DEFINED WRENCH_PATH)
    if (NOT EXISTS "${WRENCH_PATH}")
        message(FATAL_ERROR "The specified WRENCH_PATH doesn't exit")
    else()
        file(GLOB LIBWRENCH_FILES
                "${WRENCH_PATH}/lib/libwrench.a"
        )
        if (NOT LIBWRENCH_FILES)
            message(FATAL_ERROR "The specified WRENCH_PATH doesn't seem to contain a valid WRENCH installation")
        else()
            set(CMAKE_PREFIX_PATH ${WRENCH_PATH} ${CMAKE_PREFIX_PATH})
        endif()
    endif()
endif()
find_package(WRENCH REQUIRED)


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
