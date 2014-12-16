SET( APP_NAME DPM_Detector )
PROJECT ( ${APP_NAME} )

WRAP_SLICE(SLICE_FILES
           ${CVAC_ROOT_DIR}/src/Data.ice
           ${CVAC_ROOT_DIR}/src/Services.ice
           )

SET(SOURCE_FILES
    DPMDetectionI.cpp
)

SET(HEADER_FILES
    DPMDetectionI.h
)

SET(INCLUDE_DIRS
    .
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CVAC_INCLUDE_DIR}
    ${ICE_INCLUDE}
    ${CMAKE_CURRENT_BINARY_DIR}#for the generated slice files
)

INCLUDE_DIRECTORIES(${INCLUDE_DIRS})

ADD_LIBRARY( ${APP_NAME} SHARED
            ${SOURCE_FILES}
            ${HEADER_FILES}
            ${SLICE_FILES}
            )

TARGET_LINK_LIBRARIES( ${APP_NAME} 	
    dpm
    utils
    ${ICE_LIBRARIES} ${ICE_BOX_LIBRARY}
    ${OpenCV_LIBS}
    )
          
IF( NOT BUILD_3RDPARTY_PACKAGE )
IF(WIN32)
INSTALL( TARGETS ${APP_NAME}
               DESTINATION bin
               COMPONENT Runtime
             )
ELSE (WIN32)
INSTALL( TARGETS ${APP_NAME}
               DESTINATION lib
               COMPONENT Runtime
             )

ENDIF (WIN32)
ENDIF( NOT BUILD_3RDPARTY_PACKAGE )
