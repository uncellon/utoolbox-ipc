set(SERVER_EXAMPLE UTIPCServerExample)

add_executable(${SERVER_EXAMPLE} server.cpp)

target_link_libraries(${SERVER_EXAMPLE} PUBLIC ${PROJECT_NAME})



set(CLIENT_EXAMPLE UTIPCClientExample)

add_executable(${CLIENT_EXAMPLE} client.cpp)

target_link_libraries(${CLIENT_EXAMPLE} PUBLIC ${PROJECT_NAME})