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
* 64-bit register virtual machine
* 6 general purpose registers `r0` - `r5`
* Stack pointer register `sp`
* Instruction pointer register `ip`
* Base pointer register `bp`
* Flags register `bp`

### Virtual Machine Memory
When the program boots up, all the memory needed by the virtual machine is allocated. This memory is partitioned into
the following memory regions;
- **Data** - This region is reserved starting from address 0 and stores all data declared by the running program.
- **Heap Allocator** - The heap allocator comes after the data region. This region is used by the heap allocator
to manage heap memory. Writing to this region would cause an **Access Violation** error
- **Heap Memory** - This is heap memory for the virtual machine. All memory allocation request are served from
this region
- **Stack Memory** - Serves as the virtual machines main stack. The stack starts at the top of the RAM and grows
down towards the stack/heap boundary.


### Syntactic-C Virtual Machine Instruction Set

#### Halt (or `halt`)
This instruction unconditional stops the virtual machine. This should be used to exit the executing program

Field | Value
----  | -----
Mnemonic | `halt`
Code | `0x00`
Arguments | `0`
C-Macro |  `cHALT`


