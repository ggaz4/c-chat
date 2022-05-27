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
	message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory.")
endif ()

# Let's nicely support folders in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# The compiled library code is here
add_subdirectory(src)

# The executable code is here
add_subdirectory(apps)
