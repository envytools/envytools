==============
Fermi CUDA ISA
==============

.. contents::


Introduction
============

.. todo:: write me


Notes about scheduling data and dual-issue on NVE4+
===================================================

There should be one "sched instructions" at each 0x40 byte boundary, i.e. one
for each group of 7 "normal" instructions.
For each of these 7 instructions, "sched" containts 1 byte of information:

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
