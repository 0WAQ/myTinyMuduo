
project(example)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin)

# 宏
macro(ADDEXAM FILENAME)
    file(GLOB CPP_LIST ${FILENAME}/*.cpp)
    add_executable(${FILENAME} ${CPP_LIST})
    target_link_libraries(${FILENAME} PRIVATE mymuduo)
    add_dependencies(${FILENAME} mymuduo)
endmacro()

# 添加客户端
ADDEXAM(ChatClient)
ADDEXAM(TestClient)

# 添加Echo服务器
ADDEXAM(EchoServer)