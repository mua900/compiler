cmake_minimum_required(VERSION 3.5)

project(compiler)

add_executable(compiler
        main.cpp
        common.cpp
        log.cpp
        parser.cpp
        token.cpp
        expr.cpp
        type.cpp
        typechecker.cpp
        stmt.cpp
        resolve.cpp
        sema.cpp
        c_emitter.cpp
        ir.cpp
        bytecode.cpp
        bytecode_emitter.cpp

        graph.cpp    # utility
)

# maybe bundle this up seperately in the future
# add_executable(graph    # utility
#       token.cpp
#       expr.cpp
#       common.cpp
#       log.cpp
#       type.cpp
#       graph.cpp
#)

add_compile_options(-rdynamic)

set(CMAKE_BUILD_TYPE Debug)
add_definitions(-DDEBUG)
