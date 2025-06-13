
project(example)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin/example)

function(add_example name)
    add_executable(${name} ${name}/${name}.cpp)
    target_link_libraries(${name} PRIVATE mymuduo)
    add_dependencies(${name} mymuduo)
endfunction()

add_example(EchoClient)
add_example(EchoServer)