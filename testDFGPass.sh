#！/bin/bash                                                                    

clang -S -O0 -emit-llvm test.c -o test.ll
opt -load DFGPass/build/DFG/libLLVMDFG.so -DFGPass <test.ll> /dev/null
dot -Tpng main.dot -o main.png
eog main.png


