.. _fermi-isa:

==============
Fermi CUDA ISA
==============

.. contents::


Introduction
============

This file deals with description of Fermi CUDA instruction set.  CUDA stands
for Completely Unified Device Architecture and refers to the fact that all
types of shaders (vertex, tesselation, geometry, fragment, and compute) use
nearly the same ISA and execute on the same processors (called streaming
multiprocessors).

The Fermi CUDA ISA is used on Fermi (GF1xx) and older Kepler (GK10x) GPUs.
Older (Tesla) CUDA GPUs use the Tesla ISA. Newer Kepler ISAs use the Kepler2
ISA.

Variants
--------

There are two variants of the Fermi ISA: the GF100 variant (used on Fermi
GPUs) and the GK104 variant (used on first-gen Kepler GPUs).  The differences
are:

- GF100:
  - surface access based on 8 bindable slots
- GK104:
  - surface access based on descriptor structures stored in c[]?
  - some new instructions
  - texbar instruction
  - every 8th instruction slot should be filled by a special ``sched``
    instruction that describes dependencies and execution plan for the next
    7 instructions

.. todo:: rather incomplete.

Warps and thread types
----------------------

Like on Tesla, programs are executed in :ref:`warps <tesla-warp>`.

There are 6 program types on Fermi:

- vertex programs
- tesselation control programs
- tesselation evaluation programs
- geometry programs
- fragment programs
- compute programs

.. todo:: and vertex programs 2?

.. todo:: figure out the exact differences between these & the pipeline
   configuration business

Registers
---------

The registers in Fermi ISA are:

- up to 63 32-bit GPRs per thread: $r0-$r62.  These registers are used for
  all calculations, whether integer or floating-point.  In addition, $r63
  is a special register that's always forced to 0.

  The amount of available GPRs per thread is chosen by the user as part of MP
  configuration, and can be selected per program type.  For example, if
  the user enables 16 registers, $r0-$r15 will be usable and $r16-$r62 will be
  forced to 0.  Since the MP has a rather limitted amount of storage for GPRs,
  this configuration parameter determines how many active warps will fit
  simultanously on an MP.
  
  If a 64-bit operation is to be performed, any naturally aligned pair of GPRs
  can be treated as a 64-bit register: $rXd (which has the low half in $rX and
  the high half in $r(X+1), and X has to even). Likewise, if a 128-bit
  operation is to be performed, any naturally aligned group of 4 registers
  can be treated as a 128-bit registers: $rXq. The 32-bit chunks are assigned
  to $rX..(X+3) in order from lowest to highest.

  Unlike Tesla, there is no way to access a 16-bit half of a register.

- 7 1-bit predicate registers per thread: $p0-$p6.  There's also $p7, which is
  always forced to 1.  Used for conditional execution of instructions.

- 1 4-bit condition code register: $c.  Has 4 bits:

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

  Overall, works like one of the Tesla $c0-$c3 registers.

- $flags, a flags register, which is just an alias to $c and $pX registers,
  allowing them to be saved/restored with one mov:

  - bits 0-6: $p0-$p6
  - bits 12-15: $c

- A few dozen read-only 32-bit special registers, $sr0-$sr127:

  - $sr0 aka $laneid: XXX
  - $sr2 aka $nphysid: XXX
  - $sr3 aka $physid: XXX
  - $sr4-$sr11 aka $pm0-$pm7: XXX
  - $sr16 aka $vtxcnt: XXX
  - $sr17 aka $invoc: XXX
  - $sr18 aka $ydir: XXX
  - $sr24-$sr27 aka $machine_id0-$machine_id3: XXX
  - $sr28 aka $affinity: XXX
  - $sr32 aka $tid: XXX
  - $sr33 aka $tidx: XXX
  - $sr34 aka $tidy: XXX
  - $sr35 aka $tidz: XXX
  - $sr36 aka $launcharg: XXX
  - $sr37 aka $ctaidx: XXX
  - $sr38 aka $ctaidy: XXX
  - $sr39 aka $ctaidz: XXX
  - $sr40 aka $ntid: XXX
  - $sr41 aka $ntidx: XXX
  - $sr42 aka $ntidy: XXX
  - $sr43 aka $ntidz: XXX
  - $sr44 aka $gridid: XXX
  - $sr45 aka $nctaidx: XXX
  - $sr46 aka $nctaidy: XXX
  - $sr47 aka $nctaidz: XXX
  - $sr48 aka $swinbase: XXX
  - $sr49 aka $swinsz: XXX
  - $sr50 aka $smemsz: XXX
  - $sr51 aka $smembanks: XXX
  - $sr52 aka $lwinbase: XXX
  - $sr53 aka $lwinsz: XXX
  - $sr54 aka $lpossz: XXX
  - $sr55 aka $lnegsz: XXX
  - $sr56 aka $lanemask_eq: XXX
  - $sr57 aka $lanemask_lt: XXX
  - $sr58 aka $lanemask_le: XXX
  - $sr59 aka $lanemask_gt: XXX
  - $sr60 aka $lanemask_ge: XXX
  - $sr64 aka $trapstat: XXX
  - $sr66 aka $warperr: XXX
  - $sr80 aka $clock: XXX
  - $sr81 aka $clockhi: XXX

.. todo:: figure out and document the SRs

Memory
------

The memory spaces in Fermi ISA are:

- C[]: code space.  The only way to access this space is by executing code
  from it (there's no "read from code space" instruction).  Unlike Tesla,
  the code segment is shared between all program types.  It has three levels
  of cache (global, GPC, MP) that need to be manually flushed when its
  contents are modified by the user.

- c0[] - c17[]: const spaces.  Read-only and accessible from any program type
  in 8, 16, 32, 64, and 128-bit chunks.  Each of the 18 const spaces of each
  program type can be independently bound to a range of VM space (with length
  divisible by 256) or disabled by the user. Cached like C[].

  .. todo:: figure out the semi-special c16[]/c17[].

- l[]: local space.  Read-write and per-thread, accessible from any program
  type in 8, 16, 32, 64, and 128-bit units.  It's directly mapped to VM space
  (although with heavy address mangling), and hence slow.  Its per-thread
  length can be set to any multiple of 0x10 bytes.

- s[]: shared space.  Read-write, per-block, available only from compute
  programs, accessible in 8, 16, 32, 64, and 128-bit units.  Length per block
  can be selected by user.  Has a locked access feature: every warp can have
  one locked location in s[], and all other warps will block when trying
  to access this location.  Load with lock and store with unlock instructions
  can thus be used to implement atomic operations.

  .. todo:: size granularity?

  .. todo:: other program types?

- g[]: global space.  Read-write, accessible from any program type in 8, 16,
  32, 64, and 128-bit units.  Mostly mapped to VM space.  Supports some atomic
  operations.  Can have two holes in address space: one of them mapped to s[]
  space, the other to l[] space, allowing unified addressing for the 3 spaces.

All memory spaces use 32-bit addresses, except g[] which uses 32-bit or 64-bit
addresses.

.. todo:: describe the shader input spaces

Other execution state and resources
-----------------------------------

There's also a fair bit of implicit state stored per-warp for control flow:

.. todo:: describe me

Other resources available to CUDA code are:

- $t0-$t129: up to 130 textures per 3d program type, up to 128 for compute
  programs.

- $s0-$s17: up to 18 texture samplers per 3d program type, up to 16 for compute
  programs.  Only used if linked texture samplers are disabled.

- $g0-$g7: up to 8 random-access read-write image surfaces.

- Up to 16 barriers.  Per-block and available in compute programs only.
  A barrier is basically a warp counter: a barrier can be increased or waited
  for.  When a warp increases a barrier, its value is increased by 1.  If
  a barrier would be increased to a value equal to a given warp count, it's
  set to 0 instead.  When a barrier is waited for by a warp, the warp is
  blocked until the barrier's value is equal to 0.

.. todo:: not true for GK104. Not complete either.


Instruction format
==================

.. todo:: write me


Instructions
============

.. todo:: write me


Notes about scheduling data and dual-issue on GK104+
===================================================

There should be one "sched instructions" at each 0x40 byte boundary, i.e. one
for each group of 7 "normal" instructions.
For each of these 7 instructions, "sched" containts 1 byte of information:

::

    0x00     : no scheduling info, suspend warp for 32 cycles
    0x04     : dual-issue the instruction together with the next one **
    0x20 | n : suspend warp for n cycles before trying to issue the next instruction
               (0 <= n < 0x20)
    0x40     : ?
    0x80     : ?

    ** obviously you can't use 0x04 on 2 consecutive instructions

If latency information is inaccurate and you encounter an instruction where its
dependencies are not yet satisfied, the instruction is re-issued each cycle
until they are.

EXAMPLE
sched 0x28 0x20: inst_issued1/inst_executed = 6/2
sched 0x29 0x20: inst_issued1/inst_executed = 5/2
sched 0x2c 0x20: inst_issued1/inst_executed = 2/2 for
mov b32 $r0 c0[0]
set $p0 eq u32 $r0 0x1

DUAL ISSUE
----------

General constraints for which instructions can be dual-issued:

- not if same dst
- not if both access different 16-byte ranges inside cX[]
- not if any performs larger than 32 bit memory access
- a = b, b = c is allowed
- g[] access can't be dual-issued, ld seems to require 2 issues even for b32
- f64 ops seem to count as 3 instruction issues and can't be dual-issued with anything
 (GeForce only ?)

SPECIFIC (a X b means a cannot be dual-issued with any of b)
mov gpr   X
mov sreg  X  mov sreg
add int   X
shift     X  shift, mul int, cvt any, ins, popc
mul int   X  mul int, shift, cvt any, ins, popc
cvt any   X  cvt any, shift, mul int, ins, popc
ins       X  ins, shift, mul int, cvt any, popc
popc      X  popc, shift, mul int, cvt any, ins
set any   X  set any
logop     X
slct      X
ld l      X  ld l, ld s
ld s      X  ld s, ld l
