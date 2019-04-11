# LLVM-CFG-DFG-pass
Simple passes for CFG and DFG analysis

## quick start
We have already bulid the file, so you cna use it directly as long as the following requirements are satisified:
+ you pc has already installed llvm & clang
+ graphviz installed on your pc (use ***sudo apt-get install graphviz graphviz-doc*** if you do not possess one)


use the bash command to test our passes
```bash
  sh testDFGPass.sh
  sh testCFGPass.sh
```
the result is based on the c code: ***test.c***

<img align = "center" width = '300' height = '300' href = 'CFG.png'>
<div align = "center">fig1: CFG for test.c</div>


<img align = "center" width = '300' height = '300' href = 'DFG.png'>
<div align = "center">fig1: DFG for test.c</di>
