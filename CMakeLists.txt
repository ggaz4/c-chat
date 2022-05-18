cmake_minimum_required(VERSION 3.11...3.23)
project(
		C_CHAT
		VERSION 0.1
		DESCRIPTION "C Chat Project"
		HOMEPAGE_URL "https://github.com/ggaz4/c-chat"
		LANGUAGES C
)

# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if (EXISTS "${LOC_PATH}")
	message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif ()

# insert CUSTOM FUNCTIONS from a different script
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake-modules/" ${CMAKE_MODULE_PATH})
find_package("CMakeTools")

# Optionally set things like CMAKE_CXX_STANDARD, CMAKE_POSITION_INDEPENDENT_CODE here

# Let's nicely support folders in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Testing only available if this is the main app
# Note this needs to be done in the main CMakeLists
# since it calls enable_testing, which must be in the
# main CMakeLists.
include(CTest)

# The compiled library code is here
add_subdirectory(src)

# The executable code is here
add_subdirectory(apps)

# UNIX, WIN32, WINRT, CYGWIN, APPLE are environment variables as flags set by default system
if (UNIX)
	CMAKETOOLS_PRINT("This is a ${CMAKE_SYSTEM_NAME} System" status)
elseif (WIN32)
	CMAKETOOLS_PRINT("This is a Windows System" status)
elseif (APPLE)
	CMAKETOOLS_PRINT("This is a Mac System" status)
endif ()

CMAKETOOLS_PRINT("${Green}CMAKE_BUILD_TYPE:${ColourReset} ${CMAKE_BUILD_TYPE}" status nosuffix)

if (UNIX)
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		CMAKETOOLS_LIST_VARIABLES(ALL)
	elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
		CMAKETOOLS_LIST_VARIABLES(INFO)
	endif ()
endif ()
