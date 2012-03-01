
#----- Check for the required Libraries
find_package(BbqRTLib REQUIRED)

#----- Add compilation dependencies
include_directories(${BBQUE_RTLIB_INCLUDE_DIR})

#----- Add "BbqRTLibTestApp" target application
set(BBQUE_OPENCV_DEMO_SRC ocvdemo ocvdemo_exc)
add_executable(bbque-ocvdemo ${BBQUE_OPENCV_DEMO_SRC})

#----- Linking dependencies
target_link_libraries(
	bbque-ocvdemo
	${OpenCV_LIBS}
	${Boost_LIBRARIES}
	${BBQUE_RTLIB_LIBRARY}
)

# Use link path ad RPATH
set_property(TARGET bbque-ocvdemo PROPERTY
	PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

#----- Install the OpenCV Demo
install (TARGETS bbque-ocvdemo RUNTIME
	DESTINATION ${BBQUE_OPENCV_DEMO_PATH_BINS})
