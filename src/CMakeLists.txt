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
if (BUILD_WITH_DPM_INCLUIDNG_EXECUTABLE)
    set(SRC_FILES demo.cpp)
    add_executable(${PROJECT_NAME}_DEMO ${SRC_FILES})
    target_link_libraries(${PROJECT_NAME}_DEMO ${LIBS} ${PROJECT_NAME})
    set_target_properties(${PROJECT_NAME}_DEMO PROPERTIES OUTPUT_NAME ${PROJECT_NAME}_DEMO)
    install(TARGETS ${PROJECT_NAME}_DEMO
            RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/bin
    )
endif()
