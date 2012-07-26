###############################################################################
# The Barbeque RTLib Test Application
################################################################################

# Define CMake compatibility version
cmake_minimum_required (VERSION 2.6 FATAL_ERROR)

# The name of this project.
project (BbqOpenCVDemo)

################################################################################
# Installation items
################################################################################

set (BBQUE_OPENCV_DEMO_PATH_BINS    "usr/bin")
set (BBQUE_OPENCV_DEMO_PATH_RECIPES "etc/bbque/recipes")
set (BBQUE_OPENCV_DEMO_PATH_DOCS    "usr/share/bbque/bbque-demoapp")


################################################################################
# Common initialization settings
################################################################################

# Load additional CMake modules
list (APPEND CMAKE_MODULE_PATH
	"${CMAKE_INSTALL_PREFIX}/usr/share/bbque/.cmake")
list (APPEND CMAKE_MODULE_PATH
	"${CMAKE_INSTALL_PREFIX}/share/OpenCV")
list (APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}/.cmake")

# Make sure the compiler can find include files
include_directories (
	${PROJECT_BINARY_DIR}/include
	${PROJECT_SOURCE_DIR}/include
)

message ( STATUS )
message ( STATUS "Checking dependencies..." )

# Setup the source tree versioning number
include (GetGitRevisionDescription)
git_describe(GIT_SHA1)
set (VERSION "${GIT_SHA1}")


################################################################################
# Build version specific configurations
################################################################################
# Choose the type of build, options are:
#    None Debug Release RelWithDebInfo MinSizeRel

# Set default release version to RELEASE
if (NOT CMAKE_BUILD_TYPE)
	set (CMAKE_BUILD_TYPE RelWithDebInfo)
endif (NOT CMAKE_BUILD_TYPE)

# Options common to all build version
set (CMAKE_CXX_FLAGS "-std=c++0x -Wall -lrt -DBBQUE_APP")

# These are used to clean-up unsed RTLib symbols
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--gc-sections")

# Options for build version: DEBUG
set (CMAKE_CXX_FLAGS_DEBUG "-g -Wextra -pedantic -DDEBUG")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set (BBQUE_DEBUG ON)
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

# Options for build version: DESKTOP RELEASE
set (CMAKE_CXX_FLAGS_RELEASE "-O3")

# Options for build version: EMBEDDED RELEASE
set (CMAKE_CXX_FLAGS_MINSIZEREL "-Os")


################################################################################
# Installation locations
################################################################################

# set default install prefix to project root directory
if (CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
	set (CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/out" )
endif (CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")


################################################################################
# Lookup for library dependencies
################################################################################

# Force usage of BOSP provided BOOST installation
set (Boost_NO_SYSTEM_PATHS ON)
set (BOOST_ROOT ${CMAKE_INSTALL_PREFIX})
set (BOOST_INCLUDEDIR ${CMAKE_INSTALL_PREFIX})

# Check for required libs and packages (headers + lib)
find_package(Boost 1.45.0 REQUIRED program_options)
find_package(OpenCV REQUIRED)

# Add compilation dependencies
include_directories(
	${Boost_INCLUDE_DIRS}
)

# Add linking dependencies
link_directories(
	${Boost_LIBRARY_DIRS}
)

################################################################################
# Subdirectories analysis
################################################################################

# Recurse into project subfolders
add_subdirectory (src)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/recipes/"
	DESTINATION "${BBQUE_OPENCV_DEMO_PATH_RECIPES}"
	FILES_MATCHING PATTERN "*.recipe")

################################################################################
# Doxygen Documentation
################################################################################
# check if doxygen is even installed
find_package(Doxygen)
if (DOXYGEN_FOUND STREQUAL "NO")
	message(FATAL_ERROR "Doxygen not found. "
		"Please get a copy http://www.doxygen.org")
endif (DOXYGEN_FOUND STREQUAL "NO")
# prepare doxygen configuration file
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
	${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
# add doxygen as target
add_custom_target (doxygen ${DOXYGEN_EXECUTABLE}
	${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
# cleanup $build/api-doc on "make clean"
set_property(DIRECTORY APPEND PROPERTY
		     ADDITIONAL_MAKE_CLEAN_FILES api-doc)
# add doxygen as dependency to doc-target
get_target_property (DOC_TARGET doc TYPE)
if (NOT DOC_TARGET)
	add_custom_target(doc)
endif ()
add_dependencies (doc doxygen)
# install HTML API documentation and manual pages
set (DOC_PATH "${BBQUE_OPENCV_DEMO_PATH_DOCS}")
install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/api-doc/html
	DESTINATION ${BBQUE_OPENCV_DEMO_PATH_DOCS} OPTIONAL)
# install man pages into packages, scope is now project root..
install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/api-doc/man/man3
	DESTINATION ${BBQUE_OPENCV_DEMO_PATH_DOCS} OPTIONAL)


################################################################################
# Configuration resume
################################################################################

# display status message for important variables
message ( STATUS )
message ( STATUS "=====[ BBQ-OpenCV Demo - Building System Configuration ]"
                 "=================" )
message ( STATUS "OpenCV Demo Version....... ${VERSION}" )
message ( STATUS "Build type............ ${CMAKE_BUILD_TYPE}" )
message ( STATUS "Installation prefix... ${CMAKE_INSTALL_PREFIX}" )
message ( STATUS "   DemoApp bin........ <prefix>/${BBQUE_OPENCV_DEMO_PATH_BINS}" )
message ( STATUS "   Recipes............ <prefix>/${BBQUE_OPENCV_DEMO_PATH_RECIPES}" )
message ( STATUS "   Documentation...... <prefix>/${BBQUE_OPENCV_DEMO_PATH_DOCS}" )
message ( STATUS "Using RTLib........... ${BBQUE_RTLIB_LIBRARY}" )
message ( STATUS "Boost library......... ${Boost_LIBRARY_DIRS}" )
message ( STATUS "Using OpenCV.......... ${OpenCV_VERSION}" )
message ( STATUS "   Headers............ ${OpenCV_INCLUDE_DIRS}" )
message ( STATUS "   Libs............... ${OpenCV_LIB_DIR}" )
message ( STATUS )
message ( STATUS "Default values could be changes at command line, with:")
message ( STATUS "  cmake -D<Variable>=<Value>" )
message ( STATUS "or use the '-i' cmake flag to run in interactive mode" )
message ( STATUS "======================================================"
                 "==========================" )
message(STATUS)

