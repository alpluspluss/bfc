# bfc: A Brainfuck Compiler from Scratch

bfc is a fully-made-from-scratch with its custom ARM64 code generation backend. bfc is developed as a testament to my learning of ARM64 instructions encoding and decoding.

The codebase is written with simplicity in mind with a fair share of comments in most files. Readers should have some ideas about basic instruction encoding as well as how a typical compiler optimizes your code. 

## Status

bfc is able to compile all Brainfuck instructions into native binary with naive optimizations enabled.

### Essentials

- [X] Tokenizing
- [X] Parsing
- [X] IR generation
- [X] Naive optimization (#1 pass)
- [ ] Peephole optimization (#2 pass)
- [X] Native ARM64 instruction encoding
- [ ] Object file generation

bfc will not be implementing other related toolchains such as linker, assembler, or anything that is beyond the scope of a "compiler" does.

## Building

### Requirements

Before building bfc, there are several required dependencies:

- GNU Make
- Clang (both Apple and LLVM works)

### Optional

To ensure the generated binary is compiled correctly, you may want to install `capstone` to disassemble the machine code for correctness. You can download capstone by simply typing:

```shell
pip install capstone
```