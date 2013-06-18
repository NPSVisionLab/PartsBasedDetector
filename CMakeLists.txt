

project(dpm)
# dpm CMakeLists.txt
cmake_minimum_required(VERSION 2.8.5)

# -----------------------------------------------
# USER DEFINED VARIABLES
# -----------------------------------------------
option(BUILD_WITH_DPM_INCLUIDNG_EXECUTABLE "Build as executable to test functionality"                     ON)
option(BUILD_WITH_DPM_INCLUIDNG_OPENMP      "Build with OpenMP support for multithreading"                  OFF)
# COMPILER FLAGS
# -----------------------------------------------

# include all warning types
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wno-unused-parameter -pedantic")

# set the build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
        "Choose the type of build. Options are: None, Debug, Release, RelWithDebInfo, MinSizeRel"
    FORCE)
endif()

# add OpenMP support
if (BUILD_WITH_DPM_INCLUIDNG_OPENMP)
    find_package(OpenMP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${OpenMP_C_FLAGS}")
endif()

# add vectorization support
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -msse4.1")

# use highest level of optimization in Release mode
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -O3")
endif ()

# -----------------------------------------------
# DEPENDENCIES
# -----------------------------------------------
# find the dependencies
#include(cmake/FindEigen3.cmake)
find_package(Boost COMPONENTS system filesystem signals REQUIRED)
find_package(OpenCV REQUIRED)
#find_package(Eigen)
#find_package(Eigen REQUIRED)
#include_directories(${EIGEN_INCLUDE_DIRS})
#add_definitions(-DEIGEN_USE_NEW_STDVECTOR
#                -DEIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET)

# include the dependencies
include_directories(SYSTEM  ${OPENCV_INCLUDE_DIRS}     
							${Boost_INCLUDE_DIRS}
)

include_directories(include)


# -----------------------------------------------
# SUBDIRECTORIES
# -----------------------------------------------
# add the main executable/library
ADD_SUBDIRECTORY(src)
#ADD_SUBDIRECTORY(lib)
ADD_SUBDIRECTORY(detectorService)


