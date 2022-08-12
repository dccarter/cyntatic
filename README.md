# Syntactic-C (or `cyn`)
> **:warning:** Notice
> 
> Syntactic is my playground for learning compiler construction. !!Use at your own risk!!

Syntactic is a programming language that extends the C programming language by adding some syntax sugar.
The idea is that once complete, we should be able to transpile syntactic-c sources to C and compile with any C-compiler
the developer prefers.

The short name for the language is **`cyn`** pronounced `seen` (from **`syn`-tactic).

## Syntactic-C Virtual Machine (or `cynvm`)

```c
// ...
u32 hello = VM_code_append_string(&code, "Hello World!\n");    // Add "Hello World!" to data section of the code
// ...
// Program code to write "Hello World" to STDOUT_FILENO
vmCodeAppend(&code,
         cRMEM(rRa(r1), xIMb(u32, hello), dQ),              // get the real memory address of hello
         bncWRITE(xIMa(u32, STDOUT_FILENO),                 // file descriptor to write to
                  rRa(r1),                                  // the real memory address of the string was put in r2
                  xIMa(u32, 13),                            // the size of the string is 13
                  rRa(r2)),                                 // return the number of bytes written to 32
        cHALT());
```

The above code disassembles to the following cyn-assembly code
```asm
00000029: rmem.q r1 16
00000036: push.w 1
00000042: push.q r1
00000044: push.w 13
00000050: push.w 3
00000056: ncall.q 1
00000062: popn.b 1
00000068: pop.w r2
00000070: halt.b
```

The following is the equivalent handwritten cyn-assembly code for the above
```asm
$hello = "Hello World\n"
:main
    rmem r1 hello               // get the real address hello
    push 1                      // push the first argument, the file descriptor to write to (TODO: use @STDOUT_FILENO)
    push r1                     // push second argument, the real address of the string
    push #hello                 // push the third argument, the size of the string
    push 3                      // push the number of arguments
    ncall 0                     // Call the builtin native function bncWrite (TODO: use @write once available)
    popn 2                      // returned values
    halt                        // exit the virtual machine
```

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


