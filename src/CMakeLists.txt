
# Make an automatic library - will be static or dynamic based on user setting
add_library(network network.c "${PROJECT_SOURCE_DIR}/include/network.h")
add_library(logging logging.c "${PROJECT_SOURCE_DIR}/include/logging.h")
add_library(structures structures.c "${PROJECT_SOURCE_DIR}/include/structures.h")

# We need this directory, and users of our library will need it too
target_include_directories(network PUBLIC ../include)
target_include_directories(logging PUBLIC ../include)
target_include_directories(structures PUBLIC ../include)

# All users of this library will need at least C 11
target_compile_features(network PUBLIC c_std_11)
target_compile_features(logging PUBLIC c_std_11)
target_compile_features(structures PUBLIC c_std_11)

# IDEs should put the headers in a nice place
#source_group(
#		TREE "${PROJECT_SOURCE_DIR}/include"
#		PREFIX "Header Files"
#		FILES ${HEADER_LIST}
#)

