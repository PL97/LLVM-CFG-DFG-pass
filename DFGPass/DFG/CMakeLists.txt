add_library(LLVMDFG MODULE
    # List your source files here.
    DFG.cpp
)

SET(CMAKE_CXX_FLAGS "-Wall -fno-rtti")
