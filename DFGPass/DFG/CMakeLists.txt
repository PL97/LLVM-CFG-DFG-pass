add_library(LLVMDFG MODULE
    # List your source files here.
    DFG.cpp
    graph.h
)

SET(CMAKE_CXX_FLAGS "-Wall -fno-rtti")
