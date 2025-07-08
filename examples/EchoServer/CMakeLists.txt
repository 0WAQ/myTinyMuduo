
project(examples/EchoServer)

set(EXECUTABLE_OUTPUT_PATH ${EXECUTABLE_OUTPUT_ROOT_PATH}/EchoServer)

add_executable(server EchoServer.cpp)
add_executable(client EchoClient.cpp)

target_link_libraries(server PRIVATE mymuduo)
target_link_libraries(client PRIVATE mymuduo)

target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/mymuduo/include)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/mymuduo/include)