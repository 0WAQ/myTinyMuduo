
project(tests/base)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../../bin/tests/base)

# 查找 GoogleTest
find_package(GTest REQUIRED CONFIG)
include(GoogleTest)

function(add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE
        mymuduo
        GTest::gtest
        pthread
    )
    gtest_discover_tests(${name})
endfunction()


# 添加单元测试
add_test(test_Thread)
add_test(test_ThreadPool)
