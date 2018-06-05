.. _pgraph-xf-isa:

==================
XF instruction set
==================

.. contents::


Introduction
============

XF uses a VLIW instruction set.  Roughly, a single instruction can do all of
the following:

1. Read one IBUF slot.
2. Read one XFCTX slot.
3. Read three source operands:

   - each source can be independently selected from:

     - the value read from the IBUF slot
     - the value read from the XFCTX slot
     - an arbitrary temporary register

   - an arbitrary swizzle can be applied to each source component
   - starting with NV30, each source can be optionally replaced with its
     absolute value
   - each source can be optionally negated

4. Perform one vector operation (using sources #0, #1, and maybe #2) on
   the ALU+MLU.
5. Perform one scalar operation (using source #2) on ILU or SFU.
6. Perform an optional saturation on the results.
7. Write the results (with masking) to temporary registers.
8. Write the results (with masking) to either the output buffers or XFCTX 
   [NV20:NV40].
9. Optionally, end vertex processing (and submit results downstream).

There are 5 instruction sets used by XF:

1. Celsius ISA: used internally by Celsius GPUs as microcode to perform
   the fixed-function processing.  Not accessible in any way from the outside,
   so the encoding will not be described here, but the computation primitives
   are roughly the same as later ISAs and will be described here.
2. Kelvin ISA: used natively by Kelvin GPUs to store the instructions in XFPR
   RAM.  Can be uploaded by the user through the Kelvin classes.  Supported by
   Rankine GPUs in compatibility mode through dynamic translation to Rankine
   ISA.  Corresponds to GL_NV_vertex_program extension.
3. Rankine ISA: used natively by Rankine GPUs and can be uploaded through
   Rankine classes.  Supported by NV40:NV41 in compatibility mode through
   dynamic translation to the combined ISA.  Corresponds to GL_NV_vertex_program2
   extension.  Is a proper superset of the Kelvin ISA.
4. Curie ISA: used natively by NV41:G80 GPUs and can be uploaded through
   Curie classes.  Supported by NV40:NV41 mode through dynamic translation to
   the combined ISA.  Corresponds to GL_NV_vertex_program3 extension.
   Is *not* a proper superset of the Rankine ISA.
5. Combined ISA: used natively by NV40:NV41 GPUs.  Cannot be directly uploaded
   by the user.  Is more or less a sum of Rankine and Curie ISAs.


Program execution environment
=============================

The XF can execute the following kinds of programs:

1. Simple vertex programs.  Started when IDX signals that a full vertex has
   been written to the VAB.  The VAB contents are copied to the IBUF beforehand,
   and when the program is done, outputs will be sent to VTX for further
   processing by the graphics pipeline.  Multiple vertex programs can be
   executing in parallel at a given moment (up to 3 per VPE).  The only effect
   of a simple vertex program is emitting a transformed vertex.
2. Vertex programs with side effects [NV20:NV40].  Started just like simple
   vertex programs (a global mode bit determines whether a simple program or
   a program with side effects is launched), but can write to XFCTX in addition
   to their normal powers, and nothing else can be happening on XF while one
   is running.
3. Vertex state programs [NV20:NV40].  Started by the RUN XF command.  Their
   only input is a single vector submitted beforehand by the PARAM XF command.
   They have no output, and their only possible effect is updating XFCTX.
   Nothing else can be happening on XF while a vertex state program is being
   executed.  Once the program completes, XF moves on to the next input
   command, without submitting anything downstream.

Every program has the following private state while it's executing:

1. IBUF, the input buffer, read only by the program.  On Celsius, is made of
   7 vectors.  On Kelvin and up, is made of 16 vectors.  For vertex programs,
   contains a complete copy of VAB (except the passthrough slot) captured at
   the moment of program start.  For vertex state programs, only the first
   vector is usable, and it contains a copy of VAB passthrough slot (which
   should have been set by XF PARAM command).

2. XFREG, the temporary register file.  Made of 12 vector registers on
   Kelvin, 16 vector registers on Rankine, ??? vector registers on Curie.
   On Celsius, allegedly made of 8 vector registers, but it's impossible to
   tell.

   Starting with Kelvin, the register file is cleared to all-0 between
   executions.  However, this clear is done *after* a program execution, and
   after an XF reset.

3. AREG [NV20:], the address register file.  On Kelvin, this is a single signed
   9-bit integer register (or maybe larger, it's impossible to tell).
   On Rankine, contains 2 vector registers, each made of 4 components,
   where each component is a 10-bit signed integer.  On Curie, is likewise
   made of 2 4-component vector registers, where each component is a ???-bit
   signed integer.

4. CREG [NV30:], the condition register file.  On Rankine, this is a single
   4-component vector register, where each component is a 2-bit condition
   code.  The codes are:

   - U: unordered (result was a NaN)
   - L: less than (result was negative)
   - E: equal (result was a 0)
   - G: greater than (result was positive)

   On Curie, this contains 2 4-component vector registers, with the same
   structure.

5. PC: the program counter.  Basically, a pointer in XFPR RAM.  For vertex
   programs, initialized from the starting PC in XFMODE or XF_PROG bundle.
   For vertex state program, the initial PC is sent as the payload of the
   RUN command.

6. ICNT [NV30:]: the instruction counter.  Counts the number of instructions
   executed by the program so far.  Initialized to 0 on program start.  When
   it hits the timeout value, the program is forcibly stopped.

7. stack [NV30:]: an 8-slot call/return stack.  On Curie, can also be used to
   push and pop address registers.

8. TBUF: the main output buffer.  Write only by the program, contains data
   to be sent to VTX once the program is done.  On Celsius, made of 5 float
   vectors.  On Kelvin and up, made of 16 float vectors.

9. STPOS [NV20:NV40?]: shadow TBUF position.  A single vector register that
   receives a copy of anything written to TBUF slot 0 and can be read back
   by the program.  Used on Kelvin to implement viewport transformation
   transparently wrt user shaders.

10. WBUF [NV10:NV30]: one of the LT output buffers.  Write only by the program,
    contains data to be sent to LT once the program is done.  Made of 17
    3-component vectors of 22-bit floats.  While it can be written by user
    programs, it is only useful for fixed function processing.

11. VBUF [NV10:NV30]: the other LT output buffer.  Like WBUF, except has 13
    entries instead of 17.

12. UBUF [NV30:NV40]: the unified LT output buffer.  Same purpose as WBUF
    and VBUF, but is made of 10 5-component vectors of 22-bit floats.

.. todo:: NV34 (and presumably all Kelvins and Rankines) have SIPOS, which
   is a copy of the first IBUF word with unknown purpose.


In addition, all running programs have access to the following shared
resources:

- mode bits (XFMODE or state bundles): control various aspects of XF operation.
- XFCTX: the context RAM.  Contains state used by fixed-function transform,
  as well as parameters to user-defined programs.  Can be read by all types of
  programs, and can be written by vertex programs with side effects and by
  vertex state programs.
- XFPR [NV20:]: the program code RAM.  Contains the code of user-defined
  programs.
- XTRA [NV30:NV41]: ??? contains 2 vectors of 8 9-bit numbers.
- TIMEOUT [NV30:]: a 16-bit number specifying the maximal number of
  instructions that a single program is allowed to execute.  On Curie,
  this is part of the state bundles, but on Rankine it's a standalone
  piece of state.
- XFTEX [NV40:]: 4 textures with limitted functionality available for sampling
  by programs.


Instruction encoding and storage
================================

User-submitted instructions are stored in the XFPR RAM, which is:

- on Kelvin: a global array of 0x88 92-bit words in Kelvin ISA encoding.
- on Rankine: a global array of 0x118 112-bit words in Rankine ISA encoding.
- on NV40:NV41: a per-VPE array of 0x220 144-bit words in combined ISA encoding.
- on NV41:G80: a per-VPE array of 0x220 127-bit words in Curie ISA encoding.

On NV10:NV41, the XF unit also has instruction ROM with programs for
fixed-function processing, but it is not accessible in any way.

The instruction words are encoded as follows:

======== ======== ======== ======== ==============
Kelvin   Rankine  combined Curie    Field
======== ======== ======== ======== ==============
0        0        0        0        END
1        1        1        1        XFCTX_INDEXED
2        \-       \-       \-       OUT_IS_SCA
3-10     2-10     2-6      2-6      OUT_ADDR
11       11       \-       \-       OUT_TARGET
12-15    \-       \-       \-       OUT_WM
\-       12-15    132-135  \-       OUT_WM_VEC
\-       16-19    128-131  \-       OUT_WM_SCA
\-       \-       7-12     7-12     DST_SCA
24-27    20-23    13-16    13-16    DST_WM_VEC
20-23    112-116  \-       \-       DST
16-19    24-27    17-20    17-20    DST_WM_SCA
\-       \-       111-116  111-116  DST_VEC
28-42    28-42    21-37    21-37    SRC2
43-57    43-57    38-54    38-54    SRC1
58-72    58-72    55-71    55-71    SRC0
73-76    73-76    72-75    72-75    IBUF_ADDR
\-       77       \-       \-       ???
77-84    78-86    76-85    76-85    XFCTX_ADDR
85-88    87-91    86-90    86-90    OP_VEC
89-91    92-96    91-95    91-95    OP_SCA
\-       97-98    96-97    96-97    ASRC_SWZ
\-       99-106   98-105   98-105   CSRC_SWZ
\-       107-109  106-108  106-108  COND_TEST
\-       110      109      109      COND_ENABLE
\-       111      110      110      CDST_WM
\-       117      117      117      SRC0_ABS
\-       118      118      118      SRC1_ABS
\-       119      119      119      SRC2_ABS
\-       120      120      120      ASRC
\-       121      \-       \-       unused?
\-       \-       121      121      CSRCDST
\-       \-       122      122      SAT
\-       \-       123      123      IBUF_INDEXED
\-       \-       124      124      OUT_INDEXED
\-       ?        125      125      CDST_IS_VEC
\-       \-       126      126      OUT_IS_VEC
\-       \-       127      \-       WAS_CURIE
======== ======== ======== ======== ==============

SRC* fields are further subdivided as follows:

======== ======== ======== ======== ==============
Kelvin   Rankine  combined Curie    Field
======== ======== ======== ======== ==============
0-1      0-1      0-1      0-1      SRCx_MUX
2-5      2-5      2-7      2-7      SRCx_REG
6-13     6-13     8-15     8-15     SRCx_SWZ
14       14       16       16       SRCx_NEG
======== ======== ======== ======== ==============

8-bit SWZ fields represent vector swizzles and are made of the following
subfields:

- bits 0-1: W
- bits 2-3: Z
- bits 4-5: Y
- bits 6-7: X


RDI access
----------

.. todo:: write me


Instruction execution
=====================

Reading sources
---------------

.. todo:: write me


Writing outputs
---------------

.. todo:: write me


Output addresses
----------------

.. todo:: write me


Instructions
============

The vector opcodes are:

- 0x00: NOP
- 0x01: MOV
- 0x02: MUL
- 0x03: ADD
- 0x04: MAD
- 0x05: DP3
- 0x06: DPH
- 0x07: DP4
- 0x08: DST [NV20:]
- 0x09: MIN [NV20:]
- 0x0a: MAX [NV20:]
- 0x0b: SLT [NV20:]
- 0x0c: SGE [NV20:]
- 0x0d: ARL [NV20:]
- 0x0e: FRC [NV30:]
- 0x0f: FLR [NV30:]
- 0x10: SEQ [NV30:]
- 0x11: SFL [NV30:]
- 0x12: SGT [NV30:]
- 0x13: SLE [NV30:]
- 0x14: SNE [NV30:]
- 0x15: STR [NV30:]
- 0x16: SSG [NV30:]
- 0x17: ARR [NV30:]
- 0x18: ARA [NV30:]
- 0x19: TXL [NV40:]

The scalar opcodes are:

- 0x00: NOP
- 0x01: MOV
- 0x02: RCP
- 0x03: RCC
- 0x04: RSQ
- 0x05: EXP [NV20:]
- 0x06: LOG [NV20:]
- 0x07: LIT [NV20:]
- 0x08: ??? [NV30:]
- 0x09: BRA [NV30:]
- 0x0a: ??? [NV30:]
- 0x0b: CAL [NV30:]
- 0x0c: RET [NV30:]
- 0x0d: LG2 [NV30:]
- 0x0e: EX2 [NV30:]
- 0x0f: SIN [NV30:]
- 0x10: COS [NV30:]
- 0x11: ??? [NV40:]
- 0x12: ??? [NV40:]
- 0x13: PUSHA [NV40:]
- 0x14: POPA [NV40:]

.. todo:: write me


XFPR command
============

.. todo:: write me


Kelvin -> Rankine ISA conversion
--------------------------------

.. todo:: write me


Rankine -> combined ISA conversion
----------------------------------

.. todo:: write me


Curie -> combined ISA conversion
--------------------------------

.. todo:: write me


Instruction upload methods
==========================

.. todo:: write me
