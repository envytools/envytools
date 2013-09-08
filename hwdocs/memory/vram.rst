.. _vram:

================
Memory structure
================

.. contents::

While DRAM is often treated as a flat array of bytes, its internal structure
is far more complicated. A good understanding of it is necessary for
high-performance applications like GPUs.

Looking roughly from the bottom up, VRAM is made of:

1. Memory planes of R rows by C columns, with each cell being one bit
2. Memory banks made of 32, 64, or 128 memory planes used in parallel - the
   planes are usually spread across several chips, with one chip containing
   16 or 32 memory planes
3. Memory ranks made of several [2, 4 or 8] memory banks wired together and
   selected by address bits - all banks for a given memory plane reside in
   the same chip
4. Memory subpartitions made of one or two memory ranks wired together and
   selected by chip select wires - ranks behave similarly to banks, but don't
   have to have uniform geometry, and are in separate chips
5. Memory partitions made of one or two somewhat independent subpartitions
6. The whole VRAM, made of several [1-8] memory partitions


Memory planes and banks
=======================

The most basic unit of DRAM is a memory plane, which is a 2d array of bits
organised in so-called columns and rows:

::

         column
    row  0  1  2  3  4  5  6  7
    0    X  X  X  X  X  X  X  X
    1    X  X  X  X  X  X  X  X
    2    X  X  X  X  X  X  X  X
    3    X  X  X  X  X  X  X  X
    4    X  X  X  X  X  X  X  X
    5    X  X  X  X  X  X  X  X
    6    X  X  X  X  X  X  X  X
    7    X  X  X  X  X  X  X  X

    buf  X  X  X  X  X  X  X  X

A memory plane contains a buffer, which holds a whole row. Internally, DRAM is
read/written in row units via the buffer. This has several consequences:

- before a bit can be operated on, its row must be loaded into the buffer,
  which is slow
- after a row is done with, it needs to be written back to the memory array,
  which is also slow
- accessing a new row is thus slow, and even slower when there already is
  an active row
- it's often useful to preemptively close a row after some inactivity time -
  such operation is called "precharging" a bank 
- different columns in the same row, however, can be accessed quickly

Since loading column address itself takes more time than actually accessing
a bit in the active buffer, DRAM is accessed in bursts - a series of accesses
to 1-8 neighbouring bits in the active row. Usually all bits in a burst have
to be located in a single aligned 8-bit group.

The amount of rows and columns in memory plane is always a power of two, and
is measured by the count of row selection and column selection bits [ie. log2
of the row/column count]. There are typically 8-10 column bits and 10-14 row
bits.

The memory planes are organised in banks - groups of some power of two number
of memory planes. The memory planes are wired in parallel, sharing the address
and control wires, with only the data / data enable wires separate. This
effectively makes a memory bank like a memory plane that's composed of
32/64/128-bit memory cells instead of single bits - all the rules that apply
to a plane still apply to a bank, except larger units than a bit are operated
on.

A single memory chip usually contains 16 or 32 memory planes for a single
bank, thus several chips are often wired together to make wider banks.


Memory banks, ranks, and subpartitions
======================================

A memory chip contains several [2, 4, or 8] banks, using the same data wires
and multiplexed via bank select wires. While switching between banks is
slightly slower than switching between columns in a row, it's much faster
than switching between rows in the same bank.

A memory rank is thus made of `(MEMORY_CELL_SIZE / MEMORY_CELL_SIZE_PER_CHIP)`
memory chips.

One or two memory ranks connected via common wires [including data] except
a chip select wire make up a memory subpartition. Switching between ranks
has basically the same performance consequences as switching between banks
in a rank - the only differences are the physical implementation and
the possibility of using different amount of row selection bits for each
rank [though bank count and column count have to match].

The consequences of existence of several banks/ranks:

- it's important to ensure that data accessed together belongs to either
  the same row, or to different banks [to avoid row switching]
- tiled memory layouts are designed so that a tile corresponds roughly to
  a row, and neighbouring tiles never share a bank


Memory partitions and subpartitions
===================================

A memory subpartition has its own DRAM controller on the GPU. 1 or 2
subpartitions make a memory partition, which is a fairly independent entity
with its own memory access queue, own ZROP and CROP units, and own L2 cache
on later cards. All memory partitions taken together with the crossbar logic
make up the entire VRAM logic for a GPU.

All subpartitions in a partition have to be configured identically. Partitions
in a GPU are usually configured identically, but don't have to on newer cards.

The consequences of subpartition/partition existence:

- like banks, different partitions may be utilised to avoid row conflicts for
  related data
- unlike banks, bandwidth suffers if (sub)partitions are not utilised equally
  - load balancing is thus very important


Memory addressing
=================

While memory addressing is highly dependent on chipset family, the basic
approach is outlined here.

The bits of a memory address are, in sequence, assigned to:

- identifying a byte inside a memory cell - since whole cells always have to
  be accessed anyway
- several column selection bits, to allow for a burst
- partition/subpartition selection - in low bits to ensure good load
  balancing, but not too low to keep relatively large tiles in a single
  partition for ROP's benefit
- remaining column selection bits
- all/most of bank selection bits, sometimes a rank selection bit - so that
  immediately neighbouring addresses never cause a row conflict
- row bits
- remaining bank bit or rank bit - effectively allows splitting VRAM into two
  areas, placing color buffer in one and zeta buffer in the other, so that
  there are never row conflicts between them
