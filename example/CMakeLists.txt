
project(example)

# 宏
macro(ADDEXAMPLE FILENAME)
    file(GLOB CPP_LIST ${FILENAME}/*.cpp)
    add_executable(${FILENAME} ${CPP_LIST})
    target_link_libraries(${FILENAME} PRIVATE mymuduo)
    add_dependencies(${FILENAME} mymuduo)
endmacro(ADDEXAMPLE FILENAME)

# 添加Echo服务器
ADDEXAMPLE(EchoServer)