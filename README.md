# Neander Assembler

This project is a lexer and assembler for an assembly language based on the Neander project. The Neander project is a simple educational computer architecture used to teach the basics of computer organization and assembly language programming.

## Description

The assembler reads an assembly source file, processes it to generate a list of variables and instructions, and then outputs a binary file representing the assembled program. The assembler supports basic operations such as loading, storing, adding, and logical operations, as well as jump instructions.

## Files

- main.c: The main source file containing the assembler logic.
- main.h: Header file with definitions and function declarations.
- example.asm: Example assembly source file.
- Makefile: Makefile to build and run the project.

## Instructions to Run

1. **Clone the repository:**
   ```sh
   git clone <repository_url>
   cd neander-assembler-c
   ```

2. **Build the project:**
   ```sh
   make
   ```

3. **Run the assembler:**
   ```sh
   make run
   ```

   This will assemble the example.asm file and generate a binary file named binary.mem.

4. **Clean the build files:**
   ```sh
   make clean
   ```

## Example

The example.asm file contains a simple assembly program:

```asm
; Comentário
; x = a + b

.DATA

A DB 50
B DB 10
X DB ?

.CODE
.ORG 0
LDA A        ; a
ADD B        ; b
STA X        ; x
HLT
```

Running the assembler on this file will generate a binary file binary.mem representing the assembled program.

## References

- [Neander Project](https://www.inf.ufrgs.br/arq/wiki/doku.php?id=neander)