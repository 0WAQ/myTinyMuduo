project(benchmarks)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin/benchmarks)

find_package(benchmark REQUIRED)

get_target_property(benchmark_defs benchmark::benchmark INTERFACE_COMPILE_DEFINITIONS)
message(STATUS "Benchmark compile definitions: ${benchmark_defs}")

function(add_bench name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE
        mymuduo
        pthread
        benchmark::benchmark
        benchmark::benchmark_main
    )
endfunction(add_bench name)


# 添加性能测试
add_bench(benchmark_Logger)