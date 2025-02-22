
project(mymuduo)

# 获取文件列表
file(GLOB CPP_LIST "*.cpp")

# 构建静态库
add_library(${PROJECT_NAME} SHARED ${CPP_LIST})

# 项目的头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/../include)

target_link_libraries(${PROJECT_NAME} PUBLIC -lpthread)

# 设置静态库的搜索路径, 属性为PUBLIC
target_include_directories(${PROJECT_NAME} PUBLIC
                           ${PROJECT_SOURCE_DIR}/../include
)

add_custom_command(TARGET ${PROJECT_NAME}
                    POST_BUILD

                    COMMAND cp -r ${PROJECT_BINARY_DIR}/libmymuduo.so ${PROJECT_SOURCE_DIR}/../lib/
)