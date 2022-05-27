# linking add prefix lib by itself and suffix .a or .so
add_executable(server server.c)

target_compile_features(server PRIVATE c_std_11)
target_link_libraries(server PRIVATE network logging)
target_link_libraries(server PRIVATE structures)


add_executable(client client.c)
target_compile_features(client PRIVATE c_std_11)
target_link_libraries(client PRIVATE network logging)

