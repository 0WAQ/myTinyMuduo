
project(example)

# 宏
macro(ADDEXAM FILENAME)
    file(GLOB CPP_LIST ${FILENAME}/*.cpp)
    add_executable(${FILENAME} ${CPP_LIST})
    target_link_libraries(${FILENAME} PRIVATE mymuduo)
    add_dependencies(${FILENAME} mymuduo)
    add_custom_command(TARGET ${FILENAME}
                    POST_BUILD

                    COMMAND cp -r ${PROJECT_BINARY_DIR}/${FILENAME} ${PROJECT_SOURCE_DIR}/../bin/
    )
endmacro()

# 添加客户端
ADDEXAM(ChatClient)
ADDEXAM(TestClient)

# 添加Echo服务器
ADDEXAM(EchoServer)