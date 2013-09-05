.. _falcon-data:

==========
Data space
==========

.. contents::

.. todo:: document UAS


Introduction
============

Data segment of the falcon is inside the microcontroller itself. Its size can
be determined by looking at :ref:`UC_CAPS register <falcon-io-uc-caps>`, bits
9-16 shifted left by 8.

The segment has byte-oriented addressing and can be accessed in units of 8,
16, or 32 bits. Unaligned accesses are not supported and cause botched reads
or writes.

Multi-byte quantities are stored as little-endian.


.. _falcon-stack:
.. _falcon-sr-sp:

The stack
=========

The stack is also stored in data segment. Stack pointer is stored in $sp
special register and is always aligned to 4 bytes. Stack grows downwards,
with $sp pointing at the last pushed value. The low 2 bits of $sp and bits
higher than what's needed to span the data space are forced to 0.


.. _falcon-data-conventions:

Pseudocode conventions
======================

sz, for sized instructions, is the selected size of operation: 8, 16, or 32.

LD(size, address) returns the contents of size-bit quantity in data segment
at specified address::

        int LD(size, addr) {
                if (size == 32) {
                        addr &= ~3;
                        return D[addr] | D[addr + 1] << 8 | D[addr + 2] << 16 | D[addr + 3] << 24;
                } else if (size == 16) {
                        addr &= ~1;
                        return D[addr] | D[addr + 1] << 8;
                } else { // size == 8
                        return D[addr];
                }
        }

ST(size, address, value) stores the given size-bit value to data segment::

        void ST(size, addr, val) {
                if (size == 32) {
                        if (addr & 1) { // fuck up the written datum as penalty for unaligned access.
                                val = (val & 0xff) << (addr & 3) * 8;
                        } else if (addr & 2) {
                                val = (val & 0xffff) << (addr & 3) * 8;
                        }
                        addr &= ~3;
                        D[addr] = val;
                        D[addr + 1] = val >> 8;
                        D[addr + 2] = val >> 16;
                        D[addr + 3] = val >> 24;
                } else if (size == 16) {
                        if (addr & 1) {
                                val = (val & 0xff) << (addr & 1) * 8;
                        }
                        addr &= ~1;
                        D[addr] = val;
                        D[addr + 1] = val >> 8;
                } else { // size == 8
                        D[addr] = val;
                }
        }


.. _falcon-isa-ld:

Load: ld
========

Loads 8-bit, 16-bit or 32-bit quantity from data segment to register.

Instructions:
     ==== ============================== ================== ====================
     Name Description                    Subopcode - normal Subopcode - with $sp
     ==== ============================== ================== ====================
     ld   Load a value from data segment 8                  0
     ==== ============================== ================== ====================
Instruction class:
    sized
Operands:
    DST, BASE, IDX
Forms:
    =========== ======
    Form        Opcode
    =========== ======
    R1, R2, I8  10
    R2, $sp, I8 34
    R2, $sp, R1 3a
    R3, R2, R1  3c
    =========== ======
Immediates:
    zero-extended
Operation:
    ::

        DST = LD(sz, BASE + IDX * (sz/8));


.. _falcon-isa-st:

Store: st
=========

Stores 8-bit, 16-bit or 32-bit quantity from register to data segment.

Instructions:
    ==== ============================= ================== ====================
    Name Description                   Subopcode - normal Subopcode - with $sp
    ==== ============================= ================== ====================
    st   Store a value to data segment 0                  1
    ==== ============================= ================== ====================
Instruction class:
    sized
Operands:
    BASE, IDX, SRC
Forms:
    =========== ======
    Form        Opcode
    =========== ======
    R2, I8, R1  00
    $sp, I8, R2 30
    R2, 0, R1   38
    $sp, R1, R2 38
    =========== ======
Immediates:
    zero-extended
Operation:
    ::

        ST(sz, BASE + IDX * (sz/8), SRC);


.. _falcon-isa-push:

Push onto stack: push
=====================

Decrements $sp by 4, then stores a 32-bit value at top of the stack.

Instructions:
    ==== ======================= =========
    Name Description             Subopcode
    ==== ======================= =========
    push Push a value onto stack 0
    ==== ======================= =========
Instruction class:
    unsized
Operands:
    SRC
Forms:
    ==== ======
    Form Opcode
    ==== ======
    R2   f9
    ==== ======
Operation:
    ::

        $sp -= 4;
        ST(32, $sp, SRC);


.. _falcon-isa-pop:

Pop from stack: pop
===================

Loads 32-bit value from top of the stack, then incrments $sp by 4.

Instructions:
    ==== =========================== =========
    Name Description                 Subopcode
    ==== =========================== =========
    pop  Pops a value from the stack 0
    ==== =========================== =========
Instruction class:
    unsized
Operands:
    DST
Forms:
    ==== ======
    Form Opcode
    ==== ======
    R2   f2
    ==== ======
Operation:
    ::

        DST = LD(32, $sp);
        $sp += 4;


.. _falcon-isa-addsp:

Adjust stack pointer: add
=========================

Adds a value to the stack pointer.

Instructions:
    ==== ================================= ========================== =====================
    Name Description                       Subopcode - opcodes f4, f5 Subopcode - opcode f9
    ==== ================================= ========================== =====================
    add  Add a value to the stack pointer. 30                         1
    ==== ================================= ========================== =====================
Instruction class:
    unsized
Operands:
    DST, SRC
Forms:
    ======== ======
    Form     Opcode
    ======== ======
    $sp, I8  f4
    $sp, I16 f5
    $sp, R2  f9
    ======== ======
Immediates:
    sign-extended
Operation:
    ::

        $sp += SRC;


.. _falcon-io-data:

Accessing data segment through IO
=================================

On v3+, the data segment is accessible through normal IO space through
index/data reg pairs. The number of available index/data pairs is accessible
by :ref:`UC_CAPS2 register <falcon-io-uc-caps>`. This number is equal to 4
on PDAEMON, 1 on other engines:

MMIO 0x1c0 + i * 8 / I[0x07000 + i * 0x200]: DATA_INDEX
  Selects the place in D[] accessed by DATA reg. Bits:

  - bits 2-15: bits 2-15 of the data address to poke
  - bit 24: write autoincrement flag: if set, every write to corresponding DATA
    register increments the address by 4
  - bit 25: read autoincrement flag: like 24, but for reads

MMIO 0x1c4 + i * 8 / I[0x07100 + i * 0x200]: DATA
  Writes execute ST(32, DATA_INDEX & 0xfffc, value); and increment the address
  if write autoincrement is enabled. Reads return the result of LD(32,
  DATA_INDEX & 0xfffc); and increment if read autoincrement is enabled.

i should be less than DATA_PORTS value from
:ref:`UC_CAPS2 register <falcon-io-uc-caps>`.

On v0, the data segment is instead accessible through the high falcon MMIO
range, see :ref:`falcon-io-upload` for details.
