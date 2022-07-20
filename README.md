# Syntactic-C (or `cyn`)
> **:warning:** Notice
> 
> Syntactic is my playground for learning compiler construction. !!Use at your own risk!!

Syntactic is a programming language that extends the C programming language by adding some syntax sugar.
The idea is that once complete, we should be able to transpile syntactic-c sources to C and compile with any C-compiler
the developer prefers.

The short name for the language is **`cyn`** pronounced `seen` (from **`syn`-tactic).

## Syntactic-C Virtual Machine (or `cynvm`)

`cynvm` is a simple register based virtual machine capable of executing the Syntactic-C binary code specification.

### Syntactic-C Virtual Machine Instruction Set

#### Halt (or `halt`)
This instruction unconditional stops the virtual machine. This should be used to exit the executing program

Field | Value
----  | -----
Mnemonic | `halt`
Code | `0x00`
Arguments | `0`
C-Macro |  `cHALT`


