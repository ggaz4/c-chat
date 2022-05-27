add_library(network network.c "${PROJECT_SOURCE_DIR}/include/network.h")
add_library(logging logging.c "${PROJECT_SOURCE_DIR}/include/logging.h")
add_library(structures structures.c "${PROJECT_SOURCE_DIR}/include/structures.h")


target_include_directories(network PUBLIC ../include)
target_include_directories(logging PUBLIC ../include)
target_include_directories(structures PUBLIC ../include)


target_compile_features(network PUBLIC c_std_11)
target_compile_features(logging PUBLIC c_std_11)
target_compile_features(structures PUBLIC c_std_11)

