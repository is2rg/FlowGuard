# FlowGuard

FlowGuard implements Data-Flow Integrity (DFI) enforcement for C and C++
code. DFIemerges as a prevention mechanism for non-control data attacks, which
exploit vulnera-bilities to overwrite security critical data without subverting
the intended control-flow in theprogram.


## Prototype Details

FlowGuard has been implemented as a plug-in to the popular GCC compiler for C 
and C++ languages. As a prototype, its intended usage is for research purposes
only. It is distributed under the GNU GPL open source license.


## Installation instructions

1. Install dependencies: ```./dependencies.sh```
2. Install GCC version 5.4.0: ```./getgcc.sh```
3. ```cd flowguard; make plugin```


## Usage

The plugin implements the Data-Flow Integrity enforcement in C code. Since this
is just a prototype there might be several bugs.

0. Follow the installation instructions

1. To use this tool we need to generate the static Data-Flow Graph of the
   program, and to instrument the program based on that DFG, to then at runtime
   ensure that the programs follows the statically defined DFG.
   When a program is compiled, the tool generates the DFG and injects the
   required instrumentation. Then, we compile the the runtime library and insert
   this static DFG which is the one that will be checked against at runtime.
   Once the lib is compiled with the static DFG in it, we link the instrumented
   program to the runtime lib.

2. There is a Makefile that automates the compile and link process.
   Running ```make test``` is the easiest way to compile a program, translate
   the DFG into object files, compile the runtime instrumentation lib and link
   the program against the runtime lib.
