# lisp.c - A Lisp interpreter written in C

lisp.c is a Lisp interpreter experiment written entirely in C. It's core is a Lisp AST interpreter but is also contains a simple bytecode VM as well as a runtime bytecode compiler.

Contents:

- `lisp.c`: Interpreter main program. Can be used as an interactive console or a classical file interpreter (see below).
- `tests`: A collections of tests that cover most parts of the interpreter (see below).
- `samples`: Several small test programs, mainly for the lisp.c interpreter. Also contains programs in some other languages written for performance comparisons.


# Features

- AST interpreter
- Lexical scoping and capturing (closures)
- Ability to load additional builtins at runtime via shared objects
- Small bytecode VM which uses one execution stack
- Stack frames are copied to GC heap space if they were captured by one or more lambdas (recursively)
- Bytecode compiler that translates AST lambdas to bytecode
- Largely covered by tests

The [Boehm-Demers-Weiser conservative garbage collector][1] is used right now. Unfortunately the available time did not allow to write an own garbage collector.

[1]: http://www.hpl.hp.com/personal/Hans_Boehm/gc/


# Requirements

- `gcc` compiler
- `make`
- libgc: `libgc1c2` shared library package and `libgc-dev` header package

On an Debian based Linux system (e.g. Ubuntu Linux) the packages can be installed with the following command:

	sudo apt-get install gcc make libgc1c2 libgc-dev

This has been tested on Ubuntu Linux 12.04.


# Getting started

1. Install the required packages (see above)
2. Compile the interpreter and run tests:
  - Enter the main directory (with the lisp.c file in it) using a terminal
  - Execute `make`, the default make recipe will compile the interpreter and run the tests. In case of trouble the interpreter itself can be compiled using the `make lisp` command. See below to run the tests.
3. Run samples or use the interactive console
  - `./lisp samples/fac.l`
  - `./lisp samples/capturing.l`
  - `./lisp samples/fib.l`
  - `./lisp` to enter interactive console, ctrl + D to exit.
  - `./lisp -h` shows a small option help


# Using the lisp.c interpreter

The lisp.c interpreter can be run in different modes:

- As interactive console: `./lisp`
- As file interpreter: `./lisp filename`

When in interactive mode each defined lambda is compiled to bytecode. In file mode the entire file is compiled into one begin statement and then executed.

The bytecode compiler can be disabled by using the `-i` option of the interpreter, e.g. `./lisp -i samples/fib.l` will run the Fibonacci number calculation without the compiler. Alternatively the `__compile_lambdas` variable can be set to `false` to disable compilation of new lambdas: `(set! __compile_lambdas false)`. Note that previously compiled lambdas still work. Only the bytecode compiler is disabled, not the interpreter.

The lisp.c interpreter can also be used for shell scripting via the hash bang line at the start of the file:

	#!/home/steven/projects/lisp.c/lisp

The full path is required and the script needs execute permissions. `samples/fac.l` uses the hash-bang line but the path must be updated to your own location to the `lisp` binary.


# Running the tests

Most parts of the interpreter, compiler and VM are covered by test cases. These can be explicitly run by changing into the `tests` directory and executing `make`.

The tests use a minimalistic test system, therefore the displayed number of tests is more or less equal to the `asserts` of normal test systems (assert already has a different meaning in C). An extra output stream and scanner hat to be written to make input and output testable.

The test cases contain large amout of code. Especially the bytecode interpreter and compiler test cases might be of interest.


# Loading shared objects

A sample shared object can be found in `mod_hello.c`. It uses the init function to define a new builtin atom (named `test`) in the current execution environement. This new builtin just prints "test run".

The shared object can be compiled using `make mod_hello`. To load it into the interpreter:

	$ ./lisp
	> (mod_load "./mod_hello.so")
	nil
	> (test)
	test run
	nil

The "./" in the file name is important. Otherwise `dlopen` will search the system directories for the `so` file. With the "./" in front it searches only in the current directory.

# Performance

To compare the performance between the AST and bytecode interpreter the Fibonacci number calcuation sample was used. It calcuates the 30th Fibonacci number. To measure the CPU time the `time` function of zsh was used (basically the same as the `time` program). The interpreter was compiled with `-O2` settings for optimization.

- `time ./lisp samples/fib.l` → cpu 1.819s
- `time ./lisp -i samples/fib.l` → cpu 3.415s

The bytecode interpreter is about 1.7 times faster.

To get some perspective the same calculation was also implemented and run in PHP and Ruby:

- `time php samples/fib.php` → cpu 0.859s (PHP 5.3.10, probably a bytecode VM)
- `time ruby1.8 samples/fib.rb` → cpu 1.962s (AST interpreter)
- `time ruby1.9.1 samples/fib.rb` → cpu 0.371s (Bytecode VM)

Therefore the current lisp.c bytecode VM is about as fast as the old Ruby 1.8 AST interpereter which is still used for large amounts of Ruby code. However this is just one test case and one specific algorithm with strong emphasis on function calling overhead.


# Known problems

General problems:

- Comparison (less than and greater than) as well as bit operators are not yet implemented.
- Bytecode interpreter supports variable number of arguments but it's not yet implemented on the Lisp layer.
- Comments generate a nil. That makes them unusable inside of function call argument (e.g. an ";else" inside an if won't work).
- Debugging information (line numbers, etc.) not propagated trough the interpreter, therefore not usable.
- When run as an interactive shell a new bytecode VM is created for each compiled lambda that is executed on the top level.
- Error reporting is done via warnings, e.g. the compiler emits warnings on invalid statements and generates a PUSH_NIL.

Source code:

- The code is in desperate need of refactoring.
- Sightly different coding conventions and array handling schemes have been used over the months.

Performance:

- No symbol table is used right now. All symbol comarisons are done via string comparison.
- The bytecode interpreter is not optimized right now. Several allocations can occur during lambda execution.
- The atoms are not stored on the stack. Therefore each atom allocation adds some overhead.
- Due to complation into different object files most of the code called from the interpreter loop can not be inlined easily (would require link time optimization).

Bytecode VM:

- The bytecode VM is more general than it needs to be for Lisp. The instructions allow combinations and actions that can not be expressed in Lisp (e.g. loading a literal from an outer scope).
- Lambda compilation and execution is more complex due to these extended possibilities. For example it is possible to instantiate a compiled lambda multipe times via the bytecode and each instance individually keeps track of it's stack dependencies and frames. Since lambdas are literals in Lisp and these literals can not be accessed from other scopes this functionality can not be used in Lisp.
- This adds quite some overhead und ugliness to the entire code base (as well as a week of corner case chasing). Unfortunately there was no more time left for a proper cleanup and refactoring.
- Pointers to the bytecode interpreter stack are somehow leaking. Therefore the GC never frees them. This is especially visible in cases with extreme recursion depth (e.g. 1 000 000) as the stack becomes quite large. `valgrind` and `callgrind` go haywire within the gc library functions and just report a seg fault on initialization. The `gc_test.l` sample can be used to provoke that case.


# Language reference

The lisp.c interpreter implements onyl a subset of Lisp. The following functions are supported.

Core functions:

- `(define sym expr)`, no shorthand syntax for lambda definition supported.
- `(set! sym expr)`
- `(if expr expr exp)`
- `(quote expr)`
- `(begin expr ...)`
- `(lambda (args ...) expr ...)`, arguments can be an empty list and multiple expressions in the body are supported (implicitly wrapped into `begin`).

Pair handling:

- `(cons expr expr)`
- `(first expr)`
- `(rest expr)`

Arithmetic (only for number atoms):

- `(+ expr expr)`
- `(- expr expr)`
- `(* expr expr)`
- `(/ expr expr)`

Comparators (only for number atoms):

- `(= expr expr)`

Misc:

- `(mod_load expr)`, `expr` is expected to evaluate to a file name of a shared object. This file is then loaded and its `init` function is called.
- `(print expr)`, evals `expr` and prints the atom value. Works for strings, numbers, nil, true and false.
- `(gc_heap_size)`, returns the current garbage collector heap size as a number atom.


# Bytecode reference

Each bytecode instruction is 64 bits wide (56 bit with 8 bit padding) and consists of the following fields:

- 8 bit op code
- 16 bit offset: Number of scopes to look upwards (e.g. when loading or setting a local variable).

Depending on the instruction the second half is used as follows:

- 32 bit unsigned index: Used to index into literal tables, local variables and arguments
- 32 bit num: Used to embed small numbers directly into the instruction. Larger numbers are added to the literal tabel and then loaded. Also used by CALL instructions for the number of function arguments on the stack.
- 32 bit jump_offset: Offset for jump instructions. It's signed to also support jumping upwards (negative offset), e.g. to compile while loops into bytecode.

The instruction structure can be refactored into 32 bits without imposing to harsh limits. 2^16 lambda arguments and nested scopes are still more than enough. A jump range of 2^15 instructions (32k of code) up and down is also unlikely to cause problems, even for large amouts of generated Lisp code. However no more time was available for that refactoring.

Basic instructions:

- `BC_PUSH_NIL`, `BC_PUSH_TRUE`, `BC_PUSH_FALSE`: These instructions push the corresponding singleton atom on the stack. Instruction properties used: none.
- `BC_PUSH_NUM`: Pushes a numer on top of the stack. The value is stored as part of the instruction. Instruction properties used: num.
- `BC_PUSH_LITERAL`: Pushes an atom from the literal table on top of the stack. Used for all kinds of atoms and larger numbers that don't fit into the instructions num property. Instruction properties used:
  - frame_offset (number of parents to got up for the literal table)
  - index (entry of the literal table to push on the stack)
- `BC_DROP`: Drops the current atom from the stack. Instruction properties used: none.

Argument and local instructions:

- `BC_PUSH_ARG`: Pushes an argument of a compiled lambda on top of the stack. Instruction properties used:
  - frame_offset (number of frames to got up)
  - index (index of the argument)
- `BC_PUSH_VAR`: Pushes the value of a local variable on top of the stack. Instruction properties used:
  - frame_offset (number of frames to got up)
  - index (index of the local variable)
- `BC_SAVE_VAR`: Stores the top of the stack in a local variable. The atom is NOT popped! It's left on the stack as the return value of `define` or `set!`. Instruction properties used:
  - frame_offset (number of frames to got up)
  - index (index of the local variable)

Environment instructions:

- `BC_PUSH_FROM_ENV`: Takes a symbol out of the literal table and searches for a matching entry in the outer definition environment. The found atom is pushes on the stack. frame_offset is not used right now. Don't see a usecase were we access the literal table of a parent lambda. Instruction properties used:
  - index (entry of the literal table where the key symbol is stored)
- `BC_SAVE_ENV`: Stores the top of the stack in the environment of the outermost compiled lambda. The atom is NOT popped! It's left on the stack as the return value of `set!`.
  - index (entry of the literal table where the key symbol is stored)

Function generation and calling instructions:

- `BC_LAMBDA`: Looks up a compiled lambda in the literal table and creates a runtime lambda for it. This associates a compiled lambda (static compiletime stuff) with a reference to the living stacks that are needed for proper argument and local lookups. The result can be seen as an living instance of the compiled lambda. The runtime lambda is pushed on top of the stack. Instruction properties used:
  - frame_offset (number of parents to got up for the literal table)
  - index (entry of the literal table to push on the stack)
- `BC_CALL`: Takes an executable atom (lambda, runtime lambda or buildin) and its arguments from the stack and calls it. The executable atom have to be pushed on the stack first, followed by its arguments. The number of arguments is encoded in the `num` property of the CALL instruction. Instruction properties used:
  - num (number of arguments pushed on the stack)
- `BC_RETURN`: Takes the current top of the stack as return value and cleans up the stack frame of the current function. The result value is then pushed on the stack. Instruction properties used: none.

Branching instructions:

- `BC_JUMP`, `BC_JUMP_IF_FALSE`: Adds `jump_offset` to the current instruction pointer, e.g. skip n instructions. `BC_JUMP` is an unconditional jump, `BC_JUMP_IF_FALSE` pops one value from the stack and jumps only if this value is the false atom. Instruction properties used:
  - jump_offset (value to add to the instruction pointer)

Arithmetic and comparison instructions:

- `BC_ADD`, `BC_SUB`, `BC_MUL`, `BC_DIV`: Pops two values from the stack and performes the matching arithmetic on them (+, -, *, /). The result is pushed on the stack. Instruction properties used: none.
- `BC_EQ`: Pops two values from the stack. If these values are equal the true atom is pushed on the stack, otherwise the false atom is pushed on the stack. Only works with numbers right now. Instruction properties used: none.

Pair handling instructions:

- `BC_CONS`: Pops two values from the stack and builds a new pair out of them. The result is pushed on the stack. Instruction properties used: none.
- `BC_FIRST`, `BC_REST`: Pops one value from the stack and requires it to be a pair. The corresponding component of the pair is then pushed on the stack. Instruction properties used: none.