PartsBasedDetector
==================

This project implements a Parts Based Detector in C++, described in the following paper:

    Yi Yang, Deva Ramanan, "Articulated Pose Estimation with Flexible Mixtures-of-Parts," CVPR 2011

Dependencies:
-------------
The project has the following dependencies:

 - OpenCV  REQUIRED (for image processing)
 - CMake   REQUIRED (for building)
 - Doxygen OPTIONAL (for documentation)
 - OpenMP  OPTIONAL (for multithreading)
 - ROS     OPTIONAL (for publishing detections on a ROS topic)
 - ECTO    OPTIONAL (for building and ECTO cell)

Building:
---------
The project can be built in one of two modes:

 - A standalone binary (for testing functionality)
 - A shared library    (for use in existing applications)

To configure the project, set the options at the top of CMakeLists.txt
To build the project, follow the normal cmake routine from the root folder:

 1. mkdir build
 2. cd build
 3. cmake ..
 4. make

Detecting:
----------
To run the detector, please consult the Mainpage of the docs, or src/demo.cpp. Both contain examples of how the detector can be initialised and run.

Learning:
---------
The learning code is currently only in Octave/Matlab. This is because the detector supports a number of learning schema, and porting all of these to C++ is not practical at this time. Please consult the README within the matlab/ directory for instructions on training a model

This package is developed and maintained by Hilton Bristow, Willow Garage
