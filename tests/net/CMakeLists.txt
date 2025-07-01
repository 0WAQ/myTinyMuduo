
project(tests/net)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../../bin/tests/net)

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
add_test(test_Buffer)
add_test(test_Channel)
add_test(test_Connector)
add_test(test_EventLoop)
add_test(test_EventLoopThread)
add_test(test_EventLoopThreadPool)
add_test(test_TcpConnection)
add_test(test_TcpServer)
add_test(test_TimerQueue)
