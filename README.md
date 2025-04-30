# Computer System Architecture project
simulating a fictional processor design and architecture using C

The project involves implementing a Von Neumann architecture processor with a specific instruction set and pipelined execution.

Key Components
Memory Architecture
Type: Von Neumann (shared program and data memory)

Size: 2048 × 32 bits (word-addressable)

Addresses 0-1023: program instructions

Addresses 1024-2047: data

Registers: 33 registers (31 GPRs, 1 zero register, 1 PC)

Instruction Set Architecture
12 instructions divided into 3 types (R, I, J)

32-bit fixed instruction size

Includes arithmetic (ADD, SUB, MUL), logical (AND, XORI), control flow (JEQ, JMP), and memory operations (MOVR, MOVM)

Pipeline Implementation
5-stage pipeline (IF, ID, EX, MEM, WB)

Special constraints:

IF and MEM cannot run in parallel (memory conflict)

Fixed timing pattern (fetch every 2 cycles)

Clock cycle formula: 7 + ((n-1)*2) for n instructions

Implementation Requirements
Assembly Parser: Read instructions from text file and store in memory

Pipeline Simulation: Implement the 5-stage pipeline with the specified timing constraints

Detailed Logging: Print pipeline status, register/memory changes after each cycle

Final State Output: Display all registers and memory contents after execution