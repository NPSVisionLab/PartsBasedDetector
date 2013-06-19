# -----------------------------------------------
# BUILD THE PARTS BASED DETECTOR FROM SOURCE
# -----------------------------------------------
set(SRC_FILES   DepthConsistency.cpp 
                DynamicProgram.cpp
                FileStorageModel.cpp
                HOGFeatures.cpp 
                SpatialConvolutionEngine.cpp
                PartsBasedDetector.cpp 
                SearchSpacePruning.cpp
                StereoCameraModel.cpp
                Visualize.cpp
                nms.cpp
)

set(LIBS        ${Boost_LIBRARIES}
                ${OpenCV_LIBS}
)

# add OpenMP support
if (BUILD_WITH_DPM_INCLUDING_OPENMP)
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

# as a library (always)
if(MSVC)
    # no symbol exported, so make static library instead
    add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
else()
    add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
endif()
target_link_libraries(${PROJECT_NAME} ${LIBS})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
install(TARGETS ${PROJECT_NAME} 
        DESTINATION ${PROJECT_SOURCE_DIR}
)

# as an executable
if (BUILD_WITH_DPM_INCLUDING_EXECUTABLE)
    set(SRC_FILES demo.cpp)
    add_executable(${PROJECT_NAME}_DEMO ${SRC_FILES})
    target_link_libraries(${PROJECT_NAME}_DEMO ${LIBS} ${PROJECT_NAME})
    set_target_properties(${PROJECT_NAME}_DEMO PROPERTIES OUTPUT_NAME ${PROJECT_NAME}_DEMO)
    install(TARGETS ${PROJECT_NAME}_DEMO
            RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/bin
    )
endif()
