.. _tesla-isa:

==============
Tesla CUDA ISA
==============

.. contents::


Introduction
============

This file deals with description of Tesla CUDA instruction set.  CUDA stands
for Completely Unified Device Architecture and refers to the fact that all
types of shaders (vertex, geometry, fragment, and compute) use nearly the
same ISA and execute on the same processors (called streaming
multiprocessors).

The Tesla CUDA ISA is used on Tesla generation GPUs (G8x, G9x, G200, GT21x,
MCP77, MCP79, MCP89).  Older GPUs have separate ISAs for vertex and fragment
programs.  Newer GPUs use Fermi, Kepler2, or Maxwell ISAs.

Variants
--------

There are seversal variants of Tesla ISA (and the corresponding
multiprocessors).  The features added to the ISA after the first iteration
are:

- breakpoints [G84:]
- new barriers [G84:]
- atomic operations on g[] space [G84:]
- load from s[] instruction [G84:]
- lockable s[] memory [G200:]
- double-precision floating point [G200 *only*]
- 64-bit atomic add on g[] space [G200:]
- vote instructions [G200:]
- D3D10.1 additions [GT215:]:
  - $sampleid register (for sample shading)
  - texprep cube instruction (for cubemap array access)
  - texquerylod instruction
  - texgather instruction
- preret and indirect bra instructions [GT215:]?

.. todo:: check variants for preret/indirect bra

.. _tesla-warp:

Warps and thread types
----------------------

Programs on Tesla MPs are executed in units called "warps".  A warp is a group
of 32 individual threads executed together.  All threads in a warp share common
instruction pointer, and always execute the same instruction, but have
otherwise independent state (ie. separate register sets).  This doesn't
preclude independent branching: when threads in a warp disagree on a branch
condition, one direction is taken and the other is pushed onto a stack for
further processing.  Each of the divergent execution paths is tagged with
a "thread mask": a bitmask of threads in the warp that satisfied (or not)
the branch condition, and hence should be executed.  The MP does no work
(and modifies no state) for threads not covered by the current thread mask.
Once the first path reaches completion, the stack is popped, restoring target
PC and thread mask for the second path, and execution continues.

Depending on warp type, the threads in a warp may be related to each other or
not.  There are 4 warp types, corresponding to 4 program types:

- vertex programs: executed once for each vertex submitted to the 3d pipeline.
  They're grouped into warps in a rather uninteresting way.  Each thread has
  read-only access to its vertex' input attributes and write-only access to
  its vertex' output attributes.

- geometry programs: if enabled, executed once for each geometry primitive
  submitted to the 3d pipeline.  Also grouped into warps in an uninteresting
  way.  Each thread has read-only access to input attributes of its primitive's
  vertices and per-primitive attributes.  Each thread also has write-only
  access to output vertex attributes and instructions to emit a vertex and
  break the output primitive.

- fragment programs: executed once for each fragment rendered by the 3d
  pipeline.  Always dispatched in groups of 4, called quads, corresponding
  to aligned 2x2 squares on the screen (if some of the fragments in the square
  are not being rendered, the fragment program is run on them anyway, and its
  result discarded).  This grouping is done so that approximate screen-space
  derivatives of all intermediate results can be computed by exchanging data
  with other threads in the quad.  The quads are then grouped into warps in
  an uninteresting way.  Each thread has read-only access to interpolated
  attribute data and is expected to return the pixel data to be written
  to the render output surface.

- compute programs: dispatched in units called blocks.  Blocks are submitted
  manually by the user, alone or in so-called grids (basically big 2d arrays
  of blocks with identical parameters).  The user also determines how many
  threads are in a block.  The threads of a block are sequentially grouped into
  warps.  All warps of a block execute in parallel on a single MP, and have
  access to so-called shared memory.  Shared memory is a fast per-block area of
  memory, and its size is selected by the user as part of block configuration.
  Compute warps also have random R/W access to so-called global memory areas,
  which can be arbitrarily mapped to card VM by the user.

Registers
---------

The registers in Tesla ISA are:

- up to 128 32-bit GPRs per thread: $r0-$r127.  These registers are used for
  all calculations (with the exception of some address calculations), whether
  integer or floating-point.

  The amount of available GPRs per thread is chosen by the user as part of MP
  configuration, and can be selected per program type.  For example, if
  the user enables 16 registers, $r0-$r15 will be usable and $r16-$r127 will be
  forced to 0.  Since the MP has a rather limitted amount of storage for GPRs,
  this configuration parameter determines how many active warps will fit
  simultanously on an MP.
  
  If a 16-bit operation is to be performed, each GPR from $r0-$r63 range can
  be treated as a pair of 16-bit registers: $rXl (low half of $rX) and $rXh
  (high part of $rX).
  
  If a 64-bit operation is to be performed, any naturally aligned pair of GPRs
  can be treated as a 64-bit register: $rXd (which has the low half in $rX and
  the high half in $r(X+1), and X has to even). Likewise, if a 128-bit
  operation is to be performed, any naturally aligned group of 4 registers
  can be treated as a 128-bit registers: $rXq. The 32-bit chunks are assigned
  to $rX..(X+3) in order from lowest to highest.

- 4 16-bit address registers per thread: $a1-$a4, and one additional register
  per warp ($a7).  These registers are used for addressing all memory spaces
  except global memory (which uses 32-bit addressing via $r register file).
  In addition to the 4 per-thread registers and 1 per-warp register, there's
  also $a0, which is always equal to 0.

  .. todo:: wtf is up with $a7?

- 4 4-bit condition code registers per thread: $c0-$c3.  These registers
  can be optionally set as a result of some (mostly arithmetic) instructions
  and are made of 4 individual bits:

  - bit 0: Z - zero flag.  For integer operations, set when the result is equal
    to 0.  For floating-point operations, set when the result is 0 or NaN.

  - bit 1: S - sign flag.  For integer operations, set when the high bit of
    the result is equal to 1.  For floating-point operations, set when
    the result is negative or NaN.

  - bit 2: C - carry flag.  For integer addition, set when there is a carry out
    of the highest bit of the result.

  - bit 3: O - overflow flag.  For integer addition, set when the true
    (infinite-precision) result doesn't fit in the destination (considered to
    be a signed number).

- A few read-only 32-bit special registers, $sr0-$sr8:

  - $sr0 aka $physid: when read, returns the physical location of the current
    thread on the GPU:

    - bits 0-7: thread index (inside a warp)
    - bits 8-15: warp index (on an MP)
    - bits 16-19: MP index (on a TPC)
    - bits 20-23: TPC index

  - $sr1 aka $clock: when read, returns the MP clock tick counter.

    .. todo:: a bit more detail?

  - $sr2: always 0?

    .. todo:: perhaps we missed something?

  - $sr3 aka $vstride: attribute stride, determines the spacing between
    subsequent attributes of a single vertex in the input space. Useful
    only in geometry programs.

    .. todo:: seems to always be 0x20. Is it really that boring, or does
       MP switch to a smaller/bigger stride sometimes?

  - $sr4-$sr7 aka $pm0-$pm3: :ref:`MP performance counters <g80-mp-pm>`.

  - $sr8 aka $sampleid [GT215:]: the sample ID. Useful only in fragment
    programs when sample shading is enabled.

Memory
------

The memory spaces in Tesla ISA are:

- C[]: code space.  24-bit, byte-oriented addressing.  The only way to access
  this space is by executing code from it (there's no "read from code space"
  instruction).  There is one code space for each program type, and it's mapped
  to a 16MB range of VM space by the user.  It has three levels of cache
  (global, TPC, MP) that need to be manually flushed when its contents are
  modified by the user.

- c0[]-c15[]: const spaces.  16-bit byte-oriented addressing.  Read-only and
  accessible from any program type in 8, 16, and 32-bit units.  Like C[], it
  has three levels of cache.  Each of the 16 const spaces of each program type
  can be independently bound to one of 128 global (per channel) const buffers.
  In turn, each of the const buffers can be independently bound to a range of
  VM space (with length divisible by 256) or disabled by the user.

- l[]: local space.  16-bit, byte-oriented addressing.  Read-write and
  per-thread, accessible from any program type in 8, 16, 32, 64, and 128-bit
  units.  It's directly mapped to VM space (although with heavy address
  mangling), and hence slow.  Its per-thread length can be set to any power
  of two size between 0x10 and 0x10000 bytes, or to 0.

- a[]: attribute space.  16-bit byte-oriented addressing.  Read-only,
  per-thread, accessible in 32-bit units only and only available in vertex
  and geometry programs.  In vertex programs, contains input vertex
  attributes.  In geometry programs, contains pointers to vertices in p[]
  space and per-primitive attributes.

- p[]: primitive space.  16-bit byte oriented addressing.  Read-only, per-MP,
  available only from geometry programs, accessed in 32-bit units.  Contains
  input vertex attributes.

- o[]: output space.  16-bit byte-oriented addressing.  Write-only, per-thread.
  Available only from vertex and geometry programs, accessed in 32-bit units.
  Contains output vertex attributes.

- v[]: varying space.  16-bit byte-oriented addressing.  Read-only, available
  only from fragment programs, accessed in 32-bit units.  Contains interpolated
  input vertex attributs.  It's a "virtual" construct: there are really three
  words stored in MP for each v[] word (base, dx, dy) and reading from v[]
  space will calculate the value for the current fragment by evaluating
  the corresponding linear function.

- s[]: shared space.  16-bit byte-oriented addressing.  Read-write, per-block,
  available only from compute programs, accessible in 8, 16, and 32-bit units.
  Length per block can be selected by user in 0x40-byte increments from 0
  to 0x4000 bytes.  On G200+, has a locked access feature: every warp can have
  one locked location in s[], and all other warps will block when trying
  to access this location.  Load with lock and store with unlock instructions
  can thus be used to implement atomic operations.

- g0[]-g15[]: global spaces.  32-bit byte-oriented addressing.  Read-write,
  available only from compute programs, accessible in 8, 16, 32, 64, and
  128-bit units.  Each global space can be configured in either linear or 2d
  mode.  When in linear mode, a global space is simply mapped to a range of VM
  memory.  When in 2d mode, low 16 bits of gX[] address are the x coordinate,
  and high 16 bits are the y coordinate.  The global space is then mapped to
  a tiled 2d surface in VM space.  On G84+, some atomic operations on global
  spaces are supported.

.. todo:: when no-one's looking, rename the a[], p[], v[] spaces to something
   sane.

Other execution state and resources
-----------------------------------

There's also a fair bit of implicit state stored per-warp for control flow:

- 22-bit PC (24-bit address with low 2 bits forced to 0): the current address
  in C[] space where instructions are executed.

- 32-bit active thread mask: selects which threads are executed and which are
  not.  If a bit is 1 here, instructions will be executed for the given thread.

- 32-bit invisible thread mask: useful only in fragment programs.  If a bit is
  1 here, the given thread is unused, or corresponds to a pixel on the screen
  which won't be rendered (ie. was just launched to fill a quad).  Texture
  instructions with "live" flag set won't be run for such threads.

- 32*2-bit thread state: stores state of each thread:

  - 0: active or branched off
  - 1: executed the brk instruction
  - 2: executed the ret instruction
  - 3: executed the exit instruction

- Control flow stack.  The stack is made of 64-bit entries, with the following
  fields:

  - PC
  - thread mask
  - entry type:

    - 1: branch
    - 2: call
    - 3: call with limit
    - 4: prebreak
    - 5: quadon
    - 6: joinat

.. todo:: discard mask should be somewhere too?

.. todo:: call limit counter

Other resources available to CUDA code are:

- $t0-$t129: up to 130 textures per 3d program type, up to 128 for compute
  programs.

- $s0-$s17: up to 18 texture samplers per 3d program type, up to 16 for compute
  programs.  Only used if linked texture samplers are disabled.

- Up to 16 barriers.  Per-block and available in compute programs only.
  A barrier is basically a warp counter: a barrier can be increased or waited
  for.  When a warp increases a barrier, its value is increased by 1.  If
  a barrier would be increased to a value equal to a given warp count, it's
  set to 0 instead.  When a barrier is waited for by a warp, the warp is
  blocked until the barrier's value is equal to 0.

.. todo:: there's some weirdness in barriers.


Instruction format
==================

Instructions are stored in C[] space as 32-bit little-endian words.  There
are short (1 word) and long (2 words) instructions.  The instruction type
can be distinguished as follows:

======== ======== =================
word 0   word 1   instruction type
bits 0-1 bits 0-1
======== ======== =================
0        \-       short normal
1        0        long normal
1        1        long normal with ``join``
1        2        long normal with ``exit``
1        3        long immediate
2        \-       short control
3        any      long control
======== ======== =================

.. todo:: you sure of control instructions with non-0 w1b0-1?

Long instructions can only be stored on addresses divisible by 8 bytes (ie.
on even word address).  In other words, short instructions usually have to
be issued in pairs (the only exception is when a block starts with a short
instruction on an odd word address).  This is not a problem, as all short
instructions have a long equivalent.  Attempting to execute a non-aligned
long instruction results in UNALIGNED_LONG_INSTRUCTION decode error.

Long normal instructions can have a ``join`` or ``exit`` instruction tacked on.
In this case, the extra instruction is executed together with the main
instruction.

The instruction group is determined by the opcode fields:

- word 0 bits 28-31: primary opcode field
- word 1 bits 29-31: secondary opcode field (long instructions only)

Note that only long immediate and long control instructions always have the
secondary opcode equal to 0.

The exact instruction of an instruction group is determined by group-specific
encoding.  Attempting to execute an instruction whose primary/secondary opcode
doesn't map to a valid instruction group results in ILLEGAL_OPCODE decode
error.

Other fields
------------

Other fields used in instructions are quite instruction-specific. However,
some common bitfields exist. For short normal instructions, these are:

- bits 0-1: 0 (select short normal instruction)
- bits 2-7: destination
- bit 8: modifier 1
- bits 9-14: source 1
- bit 15: modifier 2
- bits 16-21: source 2
- bit 22: modifier 3
- bit 23: source 2 type
- bit 24: source 1 type
- bit 25: $a postincrement flag
- bits 26-27: address register
- bits 28-31: primary opcode

For long immediate instructions:

- word 0:

  - bits 0-1: 1 (select long non-control instruction)
  - bits 2-7: destination
  - bit 8: modifier 1
  - bits 9-14: source 1
  - bit 15: modifier 2
  - bits 16-21: immediate low 6 bits
  - bit 22: modifier 3
  - bit 23: unused
  - bit 24: source 1 type
  - bit 25: $a postincrement flag
  - bits 26-27: address register
  - bits 28-31: primary opcode

- word 1:

  - bits 0-1: 3 (select long immediate instruction)
  - bits 2-27: immediate high 26 bits
  - bit 28: unused
  - bits 29-31: always 0

For long normal instructions:

- word 0:

  - bits 0-1: 1 (select long non-control instruction)
  - bits 2-8: destination
  - bits 9-15: source 1
  - bits 16-22: source 2
  - bit 23: source 2 type
  - bit 24: source 3 type
  - bit 25: $a postincrement flag
  - bits 26-27: address register low 2 bits
  - bits 28-31: primary opcode

- word 1:

  - bits 0-1: 0 (no extra instruction), 1 (``join``), or 2 (``exit``)
  - bit 2: address register high bit
  - bit 3: destination type
  - bits 4-5: destination $c register
  - bit 6: $c write enable
  - bits 7-11: predicate
  - bits 12-13: source $c register
  - bits 14-20: source 3
  - bit 21: source 1 type
  - bits 22-25: c[] space index
  - bit 26: modifier 1
  - bit 27: modifier 2
  - bit 28: unused
  - bits 29-31: secondary opcode

Note that short and long immediate instructions have 6-bit source/destination
fields, while long normal instructions have 7-bit ones.  This means only half
the registers can be accessed in such instructions ($r0-$r63, $r0l-$r31h).

For long control instructions:

- word 0:

  - bits 0-1: 3 (select long control instruction)
  - bits 9-24: code address low 18 bits
  - bits 28-31: primary opcode

- word 1:

  - bit 6: modifier 1
  - bits 7-11: predicate
  - bits 12-13: source $c register
  - bits 14-19: code address high 6 bits

.. todo:: what about other bits? ignored or must be 0?

Note that many other bitfields can be in use, depending on instruction.  These
are just the most common ones.

Whenever a half-register ($rXl or $rXh) is stored in a field, bit 0 of that
field selects high or low part (0 is low, 1 is high), and bits 1 and up select
$r index.  Whenever a double register ($rXd) is stored in a field, the index
of the low word register is stored.  If the value stored is not divisible by 2,
the instruction is illegal.  Likewise, for quad registers ($rXq), the lowest
word register is stored, and the index has to be divisible by 4.

Predicates
----------

Most long normal and long control instructions can be predicated. A predicated
instruction is only executed if a condition, computed based on a selected $c
register, evaluates to 1. The instruction fields involved in predicates are:

- word 1 bits 7-11: predicate field - selects a boolean function of the $c
  register
- word 1 bits 12-13: $c source field - selects the $c register to use

The predicates are:

======== ========== ========================== =================
encoding name       description                condition formula
======== ========== ========================== =================
``0x00`` ``never``  always false               0
``0x01`` ``l``      less than                  (S & ~Z) ^ O
``0x02`` ``e``      equal                      Z & ~S
``0x03`` ``le``     less than or equal         S ^ (Z | O)
``0x04`` ``g``      greater than               ~Z & ~(S ^ O)
``0x05`` ``lg``     less or greater than       ~Z
``0x06`` ``ge``     greater than or equal      ~(S ^ O)
``0x07`` ``lge``    ordered                    ~Z | ~S
``0x08`` ``u``      unordered                  Z & S
``0x09`` ``lu``     less than or unordered     S ^ O
``0x0a`` ``eu``     equal or unordered         Z
``0x0b`` ``leu``    not greater than           Z | (S ^ O)
``0x0c`` ``gu``     greater than or unordered  ~S ^ (Z | O)
``0x0d`` ``lgu``    not equal to               ~Z | S
``0x0e`` ``geu``    not less than              (~S | Z) ^ O
``0x0f`` ``always`` always true                1
``0x10`` ``o``      overflow                   O
``0x11`` ``c``      carry / unsigned not below C
``0x12`` ``a``      unsigned above             ~Z & C
``0x13`` ``s``      sign / negative            S
``0x1c`` ``ns``     not sign / positive        ~S
``0x1d`` ``na``     unsigned not above         Z | ~C
``0x1e`` ``nc``     not carry / unsigned below ~C
``0x1f`` ``no``     no overflow                ~O
======== ========== ========================== =================

Some instructions read $c registers directly.  The operand ``CSRC`` refers
to the $c register selected by the $c source field.  Note that, on such
instructions, the $c register used for predicating is necessarily the same
as the input register.  Thus, one must generally avoid predicating instructions
with $c input.

$c destination field
--------------------

Most normal long instructions can optionally write status information about
their result to a $c register.  The $c destination is selected by $c
destination field, located in word 1 bits 4-5, and $c destination enable field,
located in word 1 bit 6.  The operands using these fields are:

- ``FCDST`` (forced condition destination): $c0-$c3, as selected by $c
  destination field.
- ``CDST`` (condition destination):

  - if $c destination enable field is 0, no destination is used (condition
    output is discarded).
  - if $c destination enable field is 1, same as ``FCDST``.

Memory addressing
-----------------

Some instructions can access one of the memory spaces available to CUDA code.
There are two kinds of such instructions:

- Ordinary instructions that happen to be used with memory operands.  They
  have very limitted direct addressing range (since they fit the address in 6
  or 7 bits normally used for register selection) and may lack indirect
  addressing capabilities.
- Dedicated load/store instructions.  They have full 16-bit direct addressing
  range and have indirect addressing capabilities.

The following instruction fields are involved in memory addressing:

- word 0 bit 25: autoincrement flag
- word 0 bits 26-27: $a low field
- word 1 bit 2: $a high field
- word 0 bits 9-16: long offset field (used for dedicated load/store
  instructions)

There are two operands used in memory addressing:

- ``SASRC`` (short address source): $a0-$a3, as selected by $a low field.
- ``LASRC`` (long address source): $a0-$a7, as selected by concatenation of $a
  low and high fields.

Every memory operand has an associated offset field and multiplication factor
(a constant, usually equal to the access size).  Memory operands also come in
two kinds: direct (no $a field) and indirect ($a field used).

For direct operands, the memory address used is simply the value of the offset
field times the multiplication factor.

For indirect operands, the memory address used depends on the value of the
autoincrement flag:

- if flag is 0, memory address used is ``$aX + offset * factor``, where $a
  register is selected by ``SASRC`` (for short and long immediate instructions)
  or ``LASRC`` (for long normal instructions) operand.  Note that using ``$a0``
  with this addressing mode can emulate a direct operand.

- if flag is 1, memory address used is simply ``$aX``, but after the memory
  access is done, the ``$aX`` will be increased by ``offset * factor``.
  Attempting to use ``$a0`` (or ``$a5``/``a6``) with this addressing mode
  results in ILLEGAL_POSTINCR decode error.

.. todo:: figure out where and how $a7 can be used.  Seems to be a decode
   error more often than not...

.. todo:: what address field is used in long control instructions?

Shared memory access
--------------------

Most instructions can use an s[] memory access as the first source operand.
When s[] access is used, it can be used in one of 4 modes:

- 0: ``u8`` - read a byte with zero extension, multiplication factor is 1
- 1: ``u16`` - read a half-word with zero extension, factor is 2
- 2: ``s16`` - read a half-word with sign extension, factor is 2
- 3: ``b32`` - read a word, factor is 4

The corresponding source 1 field is split into two subfields.  The high 2
bits select s[] access mode, while the low 4 or 5 bits select the offset.
Shared memory operands are always indirect operands.  The operands are:

- ``SSSRC1`` (short shared word source 1): use short source 1 field, all modes
  valid.
- ``LSSRC1`` (long shared word source 1): use long source 1 field, all modes
  valid.
- ``SSHSRC1`` (short shared halfword source 1): use short source 1 field, valid
  modes ``u8``, ``u16``, ``s16``.
- ``LSHSRC1`` (long shared halfword source 1): use long source 1 field, valid
  modes ``u8``, ``u16``, ``s16``.
- ``SSUHSRC1`` (short shared unsigned halfword source 1): use short source 1
  field, valid modes ``u8``, ``u16``.
- ``LSUHSRC1`` (long shared unsigned halfword source 1): use long source 1
  field, valid modes ``u8``, ``u16``.
- ``SSSHSRC1`` (short shared signed halfword source 1): use short source 1
  field, valid modes ``u8``, ``s16``.
- ``LSSHSRC1`` (long shared signed halfword source 1): use long source 1
  field, valid modes ``u8``, ``s16``.
- ``LSBSRC1`` (long shared byte source 1): use long source 1 field, only ``u8``
  mode valid.

Attempting to use ``b32`` mode when it's not valid (because source 1 has
16-bit width) results in ILLEGAL_MEMORY_SIZE decode error.  Attempting to use
``u16``/``s16`` mode that is invalid because the sign is wrong results in
ILLEGAL_MEMORY_SIGN decode error.  Attempting to use mode other than ``u8`` for
``cvt`` instruction with u8 source results in ILLEGAL_MEMORY_BYTE decode error.

Destination fields
------------------

Most short and long immediate instructions use the short destination field for 
selecting instruction destination.  The field is located in word 0 bits 2-7.
There are two common operands using that field:

- ``SDST`` (short word destination): GPR $r0-$r63, as selected by the short
  destination field.
- ``SHDST`` (short halfword destination): GPR half $r0l-$r31h, as selected
  by the short destination field.

Most normal long instructions use the long destination field for selecting
instruction destination.  The field is located in word 0 bits 2-8.  This
field is usually used together with destination type field, located in word
1 bit 3.  The common operands using these fields are:

- ``LRDST`` (long register word destination): GPR $r0-$r127, as selected by
  the long destination field.
- ``LRHDST`` (long register halfword destination): GPR half $r0l-$r63h,
  as selected by the long destination field.
- ``LDST`` (long word destination):

  - if destination type field is 0, same as ``LRDST``.
  - if destination type field is 1, and long destination field is equal to 127,
    no destination is used (ie. operation result is discarded).  This is used
    on instructions that are executed only for their $c output.
  - if destination type field is 1, and long destination field is not equal to
    127, o[] space is written, as a direct memory operand with long
    destination field as the offset field and multiplier factor 4.

  .. todo:: verify the 127 special treatment part and direct addressing

- ``LHDST`` (long halfword destination):

  - if destination type field is 0, same as ``LRHDST``.
  - if destination type field is 1, and long destination field is equal to 127,
    no destination is used (ie. operation result is discarded).
  - if destination type field is 1, and long destination field is not equal to
    127, o[] space is written, as a direct memory operand with long
    destination field as the offset field and multiplier factor 2.  Since
    o[] can only be written with 32-bit accesses, the address is rounded down
    to a multiple of 4, and the 16-bit result is duplicated in both low and
    high half of the 32-bit value written in o[] space.  This makes it pretty
    much useless.

- ``LDDST`` (long double destination): GPR pair $r0d-$r126d, as selected by
  the long destination field.

- ``LQDST`` (long quad destination): GPR quad $r0q-$r124q, as selected by
  the long destination field.

Short source fields
-------------------

.. todo:: write me

Long source fields
------------------

.. todo:: write me

Opcode map
----------

.. list-table:: Opcode map
   :header-rows: 1

   * - Primary opcode
     - short normal
     - long immediate
     - long normal, secondary 0
     - long normal, secondary 1
     - long normal, secondary 2
     - long normal, secondary 3
     - long normal, secondary 4
     - long normal, secondary 5
     - long normal, secondary 6
     - long normal, secondary 7
     - short control
     - long control
   * - ``0x0``
     - \-
     - \-
     - :ref:`ld a[] <tesla-opg-ld-a>`
     - :ref:`mov from $c <tesla-opg-mov-r-c>`
     - :ref:`mov from $a <tesla-opg-mov-r-a>`
     - :ref:`mov from $sr <tesla-opg-mov-r-sr>`
     - :ref:`st o[] <tesla-opg-st-o>`
     - :ref:`mov to $c <tesla-opg-mov-c-r>`
     - :ref:`shl to $a <tesla-opg-shl-a>`
     - :ref:`st s[] <tesla-opg-st-s>`
     - \-
     - :ref:`discard <tesla-opg-discard>`
   * - ``0x1``
     - :ref:`mov <tesla-opg-short-mov>`
     - :ref:`mov <tesla-opg-imm-mov>`
     - :ref:`mov <tesla-opg-mov>`
     - :ref:`ld c[] <tesla-opg-ld-c>`
     - :ref:`ld s[] <tesla-opg-ld-s>`
     - :ref:`vote <tesla-opg-vote>`
     - \-
     - \-
     - \-
     - \-
     - \-
     - :ref:`bra <tesla-opg-bra>`
   * - ``0x2``
     - :ref:`add/sub <tesla-opg-short-add>`
     - :ref:`add/sub <tesla-opg-imm-add>`
     - :ref:`add/sub <tesla-opg-add>`
     - \-
     - \-
     - \-
     - \-
     - \-
     - \-
     - \-
     - \-
     - :ref:`call <tesla-opg-call>`
   * - ``0x3``
     - :ref:`add/sub <tesla-opg-short-add>`
     - :ref:`add/sub <tesla-opg-imm-add>`
     - :ref:`add/sub <tesla-opg-add>`
     - \-
     - \-
     - :ref:`set <tesla-opg-set>`
     - :ref:`max <tesla-opg-max>`
     - :ref:`min <tesla-opg-min>`
     - :ref:`shl <tesla-opg-shl>`
     - :ref:`shr <tesla-opg-shr>`
     - \-
     - :ref:`ret <tesla-opg-ret>`
   * - ``0x4``
     - :ref:`mul <tesla-opg-short-mul>`
     - :ref:`mul <tesla-opg-imm-mul>`
     - :ref:`mul <tesla-opg-mul>`
     - \-
     - \-
     - \-
     - \-
     - \-
     - \-
     - \-
     - \-
     - :ref:`prebrk <tesla-opg-prebrk>`
   * - ``0x5``
     - :ref:`sad <tesla-opg-short-sad>`
     - \-
     - :ref:`sad <tesla-opg-sad>`
     - \-
     - \-
     - \-
     - \-
     - \-
     - \-
     - \-
     - \-
     - :ref:`brk <tesla-opg-brk>`
   * - ``0x6``
     - :ref:`mul+add <tesla-opg-short-mul-add>`
     - :ref:`mul+add <tesla-opg-imm-mul-add>`
     - :ref:`mul+add <tesla-opg-mul-add>`
     - :ref:`mul+add <tesla-opg-mul-add>`
     - :ref:`mul+add <tesla-opg-mul-add>`
     - :ref:`mul+add <tesla-opg-mul-add>`
     - :ref:`mul+add <tesla-opg-mul-add>`
     - :ref:`mul+add <tesla-opg-mul-add>`
     - :ref:`mul+add <tesla-opg-mul-add>`
     - :ref:`mul+add <tesla-opg-mul-add>`
     - \-
     - :ref:`quadon <tesla-opg-quadon>`
   * - ``0x7``
     - :ref:`mul+add <tesla-opg-short-mul-add>`
     - :ref:`mul+add <tesla-opg-imm-mul-add>`
     - :ref:`mul+add <tesla-opg-mul-add>`
     - :ref:`mul+add <tesla-opg-mul-add>`
     - :ref:`mul+add <tesla-opg-mul-add>`
     - :ref:`mul+add <tesla-opg-mul-add>`
     - :ref:`mul+add <tesla-opg-mul-add>`
     - :ref:`mul+add <tesla-opg-mul-add>`
     - :ref:`mul+add <tesla-opg-mul-add>`
     - :ref:`mul+add <tesla-opg-mul-add>`
     - \-
     - :ref:`quadpop <tesla-opg-quadpop>`
   * - ``0x8``
     - :ref:`interp <tesla-opg-short-interp>`
     - \-
     - :ref:`interp <tesla-opg-interp>`
     - \-
     - \-
     - \-
     - \-
     - \-
     - \-
     - \-
     - \-
     - :ref:`bar <tesla-opg-bar>`
   * - ``0x9``
     - :ref:`rcp <tesla-opg-short-rcp>`
     - \-
     - :ref:`rcp <tesla-opg-rcp>`
     - \-
     - :ref:`rsqrt <tesla-opg-rsqrt>`
     - :ref:`lg2 <tesla-opg-lg2>`
     - :ref:`sin <tesla-opg-sin>`
     - :ref:`cos <tesla-opg-cos>`
     - :ref:`ex2 <tesla-opg-ex2>`
     - \-
     - :ref:`trap <tesla-opg-short-trap>`
     - :ref:`trap <tesla-opg-trap>`
   * - ``0xa``
     - \-
     - \-
     - :ref:`cvt i2i <tesla-opg-cvt-i2i>`
     - :ref:`cvt i2i <tesla-opg-cvt-i2i>`
     - :ref:`cvt i2f <tesla-opg-cvt-i2f>`
     - :ref:`cvt i2f <tesla-opg-cvt-i2f>`
     - :ref:`cvt f2i <tesla-opg-cvt-f2i>`
     - :ref:`cvt f2i <tesla-opg-cvt-f2i>`
     - :ref:`cvt f2f <tesla-opg-cvt-f2f>`
     - :ref:`cvt f2f <tesla-opg-cvt-f2f>`
     - \-
     - :ref:`joinat <tesla-opg-joinat>`
   * - ``0xb``
     - :ref:`fadd <tesla-opg-short-fadd>`
     - :ref:`fadd <tesla-opg-imm-fadd>`
     - :ref:`fadd <tesla-opg-fadd>`
     - :ref:`fadd <tesla-opg-fadd>`
     - \-
     - :ref:`fset <tesla-opg-fset>`
     - :ref:`fmax <tesla-opg-fmax>`
     - :ref:`fmin <tesla-opg-fmin>`
     - :ref:`presin/preex2 <tesla-opg-pre>`
     - \-
     - :ref:`brkpt <tesla-opg-short-brkpt>`
     - :ref:`brkpt <tesla-opg-brkpt>`
   * - ``0xc``
     - :ref:`fmul <tesla-opg-short-fmul>`
     - :ref:`fmul <tesla-opg-imm-fmul>`
     - :ref:`fmul <tesla-opg-fmul>`
     - \-
     - :ref:`fslct <tesla-opg-fslct>`
     - :ref:`fslct <tesla-opg-fslct>`
     - :ref:`quadop <tesla-opg-quadop>`
     - \-
     - \-
     - \-
     - \-
     - :ref:`bra c[] <tesla-opg-bra-c>`
   * - ``0xd``
     - \-
     - :ref:`logic op <tesla-opg-imm-logop>`
     - :ref:`logic op <tesla-opg-logop>`
     - :ref:`add $a <tesla-opg-add-a>`
     - :ref:`ld l[] <tesla-opg-ld-l>`
     - :ref:`st l[] <tesla-opg-st-l>`
     - :ref:`ld g[] <tesla-opg-ld-g>`
     - :ref:`st g[] <tesla-opg-st-g>`
     - :ref:`red g[] <tesla-opg-red-g>`
     - :ref:`atomic g[] <tesla-opg-atomic-g>`
     - \-
     - :ref:`preret <tesla-opg-preret>`
   * - ``0xe``
     - :ref:`fmul+fadd <tesla-opg-short-fmul-fadd>`
     - :ref:`fmul+fadd <tesla-opg-imm-fmul-fadd>`
     - :ref:`fmul+fadd <tesla-opg-fmul-fadd>`
     - :ref:`fmul+fadd <tesla-opg-fmul-fadd>`
     - :ref:`dfma <tesla-opg-dfma>`
     - :ref:`dadd <tesla-opg-dadd>`
     - :ref:`dmul <tesla-opg-dmul>`
     - :ref:`dmin <tesla-opg-dmin>`
     - :ref:`dmax <tesla-opg-dmax>`
     - :ref:`dset <tesla-opg-dset>`
     - \-
     - \-
   * - ``0xf``
     - :ref:`texauto/fetch <tesla-opg-short-tex>`
     - \-
     - :ref:`texauto/fetch <tesla-opg-tex>`
     - :ref:`texbias <tesla-opg-texbias>`
     - :ref:`texlod <tesla-opg-texlod>`
     - :ref:`tex misc <tesla-opg-texmisc>`
     - :ref:`texcsaa/gather <tesla-opg-texcsaa>`
     - ???
     - :ref:`emit/restart <tesla-opg-emit>`
     - :ref:`nop/pmevent <tesla-opg-nop>`
     - \-
     - \-


Instructions
============

The instructions are roughly divided into the following groups:

- :ref:`tesla-data`
- :ref:`tesla-int`
- :ref:`tesla-float`
- :ref:`tesla-trans`
- :ref:`tesla-double`
- :ref:`tesla-control`
- :ref:`tesla-texture`
- :ref:`tesla-misc`
