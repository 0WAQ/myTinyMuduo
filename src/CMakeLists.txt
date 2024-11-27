
project(mymuduo)

# 获取文件列表
file(GLOB C_LIST "*.cpp")

# 构建静态库
add_library(${PROJECT_NAME} STATIC ${C_LIST})

# 设置静态库的搜索路径, 属性为PUBLIC
target_include_directories(${PROJECT_NAME} PUBLIC
                           ${PROJECT_SOURCE_DIR}/../include
)