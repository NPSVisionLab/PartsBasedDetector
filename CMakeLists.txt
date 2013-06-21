project(dpm)
# dpm CMakeLists.txt
cmake_minimum_required(VERSION 2.6)

# -----------------------------------------------
# USER DEFINED VARIABLES
# -----------------------------------------------
OPTION(BUILD_WITH_DPM "Enables the projects that depend on the DPM library" OFF)

if (BUILD_WITH_DPM)

    option(BUILD_WITH_DPM_INCLUDING_EXECUTABLE "Build as executable to test functionality"                     ON)
    option(BUILD_WITH_DPM_INCLUDING_OPENMP     "Build with OpenMP support for multithreading"                  OFF)

    # -----------------------------------------------
	# DEPENDENCIES
    # -----------------------------------------------
    # find the dependencies
    #include(cmake/FindEigen3.cmake)
    find_package(Boost COMPONENTS system filesystem signals REQUIRED)
    FIND_PACKAGE( OpenCV 2.4 REQUIRED
      core imgproc highgui ml features2d nonfree objdetect calib3d
      HINTS
      ${CMAKE_SOURCE_DIR}/3rdparty/opencv
    )
    #find_package(Eigen)
    #find_package(Eigen REQUIRED)
    #include_directories(${EIGEN_INCLUDE_DIRS})
    #add_definitions(-DEIGEN_USE_NEW_STDVECTOR
    #                -DEIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET)

    # include the dependencies
    include_directories(SYSTEM  ${OPENCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS}
    )

    include_directories(include)


    # -----------------------------------------------
    # SUBDIRECTORIES
    # -----------------------------------------------
    # add the main executable/library
    ADD_SUBDIRECTORY(src)
    ADD_SUBDIRECTORY(detectorService)
endif()


