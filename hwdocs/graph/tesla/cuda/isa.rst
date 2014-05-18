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

  - $sr4-$sr7 aka $pm0-$pm3: :ref:`MP performance counters <nv50-mp-pm>`.

  - $sr8 aka $sampleid [NVA3-]: the sample ID. Useful only in fragment
    programs when sample shading is enabled.

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
  to 0x4000 bytes.  On NVA0+, has a locked access feature: every warp can have
  one locked location in s[], and all other warps will block when trying
  to access this location.  Load with lock and store with unlock instructions
  can thus be used to implement atomic operations.

- g0[]-g15[]: global spaces.  32-bit byte-oriented addressing.  Read-write,
  available only from compute programs, accessible in 8, 16, 32, 64, and
  128-bit units.  Each global space can be configured in either linear or 2d
  mode.  When in linear mode, a global space is simply mapped to a range of VM
  memory.  When in 2d mode, low 16 bits of gX[] address are the x coordinate,
  and high 16 bits are the y coordinate.  The global space is then mapped to
  a tiled 2d surface in VM space.  On NV84+, some atomic operations on global
  spaces are supported.

.. todo:: when no-one's looking, rename the a[], p[], v[] spaces to something
   sane.

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

====== ====== ======== =================
word 0 word 0 word 1   instruction type
bit 0  bit 1  bits 0-1
====== ====== ======== =================
0      0      \-       short normal
0      1      \-       short control
1      0      0        long normal
1      0      1        long normal with ``join``
1      0      2        long normal with ``exit``
1      0      3        long immediate
1      1      any      long control
====== ====== ======== =================

.. todo:: you sure of control instructions with non-0 w1b0-1?

Long instructions can only be stored on addresses divisible by 8 bytes (ie.
on even word address).  In other words, short instructions usually have to
be issued in pairs (the only exception is when a block starts with a short
instruction on an odd word address).  This is not a problem, as all short
instructions have a long equivalent.

Long normal instructions can have a ``join`` or ``exit`` instruction tacked on.
In this case, the extra instruction is executed together with the main
instruction.

The instruction group is determined by the opcode fields:

- word 0 bits 28-31: primary opcode field
- word 1 bits 29-31: secondary opcode field (long normal instructions only)

The exact instruction of an instruction group is determined by group-specific
encoding.

Predicates
----------

Most long normal and long control instructions can be predicated. A predicated
instruction is only executed if a condition, computed based on a selected $c
register, evaluates to 1. The instruction fields involved in predicates are:

- word 1 bits 7-11: predicate
- word 1 bits 12-13: $c register to use

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

Other fields
------------

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


Conventions
===========

::

    S(x): 31th bit of x for 32-bit x, 15th for 16-bit x.
    SEX(x): sign-extension of x
    ZEX(x): zero-extension of x


Instructions
============

mov
---

::

  [lanemask] mov b32/b16 DST SRC

  lanemask assumed 0xf for short and immediate versions.

    if (lanemask & 1 << (laneid & 3)) DST = SRC;

  Short:    0x10000000 base opcode
        0x00008000 0: b16, 1: b32
        operands: S*DST, S*SRC1/S*SHARED

  Imm:      0x10000000 base opcode
        0x00008000 0: b16, 1: b32
        operands: L*DST, IMM

  Long:     0x10000000 0x00000000 base opcode
        0x00000000 0x04000000 0: b16, 1: b32
        0x00000000 0x0003c000 lanemask
        operands: LL*DST, L*SRC1/L*SHARED

mov (from $c)
-------------

::

  mov DST COND

  DST is 32-bit $r.

    DST = COND;

  Long:     0x00000000 0x20000000 base opcode
        operands: LDST, COND

MOV to $c
---------

::

  mov CDST SRC

  SRC is 32-bit $r. Yes, the 0x40 $c write enable flag in second word is
  actually ignored.

    CDST = SRC;

  Long:     0x00000000 0xa0000000 base opcode
        operands: CDST, LSRC1

MOV from $a
-----------

::

  mov DST AREG

  DST is 32-bit $r. Setting flag normally used for autoincrement mode doesn't
  work, but still causes crash when using non-writable $a's.

    DST = AREG;

  Long:     0x00000000 0x40000000 base opcode
        0x02000000 0x00000000 crashy flag
        operands: LDST, AREG

SHL to $a
---------

::

  shl ADST SRC SHCNT

  SRC is 32-bit $r.

    ADST = SRC << SHCNT;

  Long:     0x00000000 0xc0000000 base opcode
        operands: ADST, LSRC1/LSHARED, HSHCNT

ADD from $a to $a
-----------------

::

  add ADST AREG OFFS

  Like mov from $a, setting flag normally used for autoincrement mode doesn't
  work, but still causes crash when using non-writable $a's.

    ADST = AREG + OFFS;

  Long:     0xd0000000 0x20000000 base opcode
        0x02000000 0x00000000 crashy flag
        operands: ADST, AREG, OFFS

MOV from sreg
-------------

::

  mov DST physid    S=0
  mov DST clock     S=1
  mov DST sreg2     S=2
  mov DST sreg3     S=3
  mov DST pm0       S=4
  mov DST pm1       S=5
  mov DST pm2       S=6
  mov DST pm3       S=7

  DST is 32-bit $r.

    DST = SREG;

  Long:     0x00000000 0x60000000 base opcode
        0x00000000 0x0001c000 S
        operands: LDST

Integer ADD family
------------------

::

  add [sat] b32/b16 [CDST] DST SRC1 SRC2        O2=0, O1=0
  sub [sat] b32/b16 [CDST] DST SRC1 SRC2        O2=0, O1=1
  subr [sat] b32/b16 [CDST] DST SRC1 SRC2       O2=1, O1=0
  addc [sat] b32/b16 [CDST] DST SRC1 SRC2 COND      O2=1, O1=1

  All operands are 32-bit or 16-bit according to size specifier.

    b16/b32 s1, s2;
    bool c;
    switch (OP) {
        case add: s1 = SRC1, s2 = SRC2, c = 0; break;
        case sub: s1 = SRC1, s2 = ~SRC2, c = 1; break;
        case subr: s1 = ~SRC1, s2 = SRC2, c = 1; break;
        case addc: s1 = SRC1, s2 = SRC2, c = COND.C; break;
    }
    res = s1+s2+c;  // infinite precision
    CDST.C = res >> (b32 ? 32 : 16);
    res = res & (b32 ? 0xffffffff : 0xffff);
    CDST.O = (S(s1) == S(s2)) && (S(s1) != S(res));
    if (sat && CDST.O)
        if (S(res)) res = (b32 ? 0x7fffffff : 0x7fff);
        else res = (b32 ? 0x80000000 : 0x8000);
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Short/imm:    0x20000000 base opcode
        0x10000000 O2 bit
        0x00400000 O1 bit
        0x00008000 0: b16, 1: b32
        0x00000100 sat flag
        operands: S*DST, S*SRC1/S*SHARED, S*SRC2/S*CONST/IMM, $c0

  Long:     0x20000000 0x00000000 base opcode
        0x10000000 0x00000000 O2 bit
        0x00400000 0x00000000 O1 bit
        0x00000000 0x04000000 0: b16, 1: b32
        0x00000000 0x08000000 sat flag
        operands: MCDST, LL*DST, L*SRC1/L*SHARED, L*SRC3/L*CONST3, COND

Integer short MUL
-----------------

::

  mul [CDST] DST u16/s16 SRC1 u16/s16 SRC2

  DST is 32-bit, SRC1 and SRC2 are 16-bit.

    b32 s1, s2;
    if (src1_signed)
        s1 = SEX(SRC1);
    else
        s1 = ZEX(SRC1);
    if (src2_signed)
        s2 = SEX(SRC2);
    else
        s2 = ZEX(SRC2);
    b32 res = s1*s2;    // modulo 2^32
    CDST.O = 0;
    CDST.C = 0;
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Short/imm:    0x40000000 base opcode
        0x00008000 src1 is signed
        0x00000100 src2 is signed
        operands: SDST, SHSRC/SHSHARED, SHSRC2/SHCONST/IMM

  Long:     0x40000000 0x00000000 base opcode
        0x00000000 0x00008000 src1 is signed
        0x00000000 0x00004000 src2 is signed
        operands: MCDST, LLDST, LHSRC1/LHSHARED, LHSRC2/LHCONST2

Integer 24-bit MUL
------------------

::

  mul [CDST] DST [high] u24/s24 SRC1 SRC2

  All operands are 32-bit.

    b48 s1, s2;
    if (signed) {
        s1 = SEX((b24)SRC1);
        s2 = SEX((b24)SRC2);
    } else {
        s1 = ZEX((b24)SRC1);
        s2 = ZEX((b24)SRC2);
    }
    b48 m = s1*s2;  // modulo 2^48
    b32 res = (high ? m >> 16 : m & 0xffffffff);
    CDST.O = 0;
    CDST.C = 0;
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Short/imm:    0x40000000 base opcode
        0x00008000 src are signed
        0x00000100 high
        operands: SDST, SSRC/SSHARED, SSRC2/SCONST/IMM

  Long:     0x40000000 0x00000000 base opcode
        0x00000000 0x00008000 src are signed
        0x00000000 0x00004000 high
        operands: MCDST, LLDST, LSRC1/LSHARED, LSRC2/LCONST2

Integer MUL-ADD
---------------

::

  addop [CDST] DST mul u16 SRC1 SRC2 SRC3       O1=0 O2=000 S2=0 S1=0
  addop [CDST] DST mul s16 SRC1 SRC2 SRC3       O1=0 O2=001 S2=0 S1=1
  addop sat [CDST] DST mul s16 SRC1 SRC2 SRC3       O1=0 O2=010 S2=1 S1=0
  addop [CDST] DST mul u24 SRC1 SRC2 SRC3       O1=0 O2=011 S2=1 S1=1
  addop [CDST] DST mul s24 SRC1 SRC2 SRC3       O1=0 O2=100
  addop sat [CDST] DST mul s24 SRC1 SRC2 SRC3       O1=0 O2=101
  addop [CDST] DST mul high u24 SRC1 SRC2 SRC3  O1=0 O2=110
  addop [CDST] DST mul high s24 SRC1 SRC2 SRC3  O1=0 O2=111
  addop sat [CDST] DST mul high s24 SRC1 SRC2 SRC3  O1=1 O2=000

  addop is one of:

  add   O3=00   S4=0 S3=0
  sub   O3=01   S4=0 S3=1
  subr  O3=10   S4=1 S3=0
  addc  O3=11   S4=1 S3=1

  If addop is addc, insn also takes an additional COND parameter. DST and
  SRC3 are always 32-bit, SRC1 and SRC2 are 16-bit for u16/s16 variants,
  32-bit for u24/s24 variants. Only a few of the variants are encodable as
  short/immediate, and they're restricted to DST=SRC3.

    if (u24 || s24) {
        b48 s1, s2;
        if (s24) {
            s1 = SEX((b24)SRC1);
            s2 = SEX((b24)SRC2);
        } else {
            s1 = ZEX((b24)SRC1);
            s2 = ZEX((b24)SRC2);
        }
        b48 m = s1*s2;  // modulo 2^48
        b32 mres = (high ? m >> 16 : m & 0xffffffff);
    } else {
        b32 s1, s2;
        if (s16) {
            s1 = SEX(SRC1);
            s2 = SEX(SRC2);
        } else {
            s1 = ZEX(SRC1);
            s2 = ZEX(SRC2);
        }
        b32 mres = s1*s2;   // modulo 2^32
    }
    b32 s1, s2;
    bool c;
    switch (OP) {
        case add: s1 = mres, s2 = SRC3, c = 0; break;
        case sub: s1 = mres, s2 = ~SRC3, c = 1; break;
        case subr: s1 = ~mres, s2 = SRC3, c = 1; break;
        case addc: s1 = mres, s2 = SRC3, c = COND.C; break;
    }
    res = s1+s2+c;  // infinite precision
    CDST.C = res >> 32;
    res = res & 0xffffffff;
    CDST.O = (S(s1) == S(s2)) && (S(s1) != S(res));
    if (sat && CDST.O)
        if (S(res)) res = 0x7fffffff;
        else res = 0x80000000;
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Short/imm:    0x60000000 base opcode
        0x00000100 S1
        0x00008000 S2
        0x00400000 S3
        0x10000000 S4
        operands: SDST, S*SRC/S*SHARED, S*SRC2/S*CONST/IMM, SDST, $c0

  Long:     0x60000000 0x00000000 base opcode
        0x10000000 0x00000000 O1
        0x00000000 0xe0000000 O2
        0x00000000 0x0c000000 O3
        operands: MCDST, LLDST, L*SRC1/L*SHARED, L*SRC2/L*CONST2, L*SRC3/L*CONST3, COND

Integer SAD
-----------

::

  sad [CDST] DST u16/s16/u32/s32 SRC1 SRC2 SRC3

  Short variant is restricted to DST same as SRC3. All operands are 32-bit or
  16-bit according to size specifier.

    int s1, s2; // infinite precision
    if (signed) {
        s1 = SEX(SRC1);
        s2 = SEX(SRC2);
    } else {
        s1 = ZEX(SRC1);
        s2 = ZEX(SRC2);
    }
    b32 mres = abs(s1-s2);  // modulo 2^32
    res = mres+s3;      // infinite precision
    CDST.C = res >> (b32 ? 32 : 16);
    res = res & (b32 ? 0xffffffff : 0xffff);
    CDST.O = (S(mres) == S(s3)) && (S(mres) != S(res));
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Short:    0x50000000 base opcode
        0x00008000 0: b16 1: b32
        0x00000100 src are signed
        operands: DST, SDST, S*SRC/S*SHARED, S*SRC2/S*CONST, SDST

  Long:     0x50000000 0x00000000 base opcode
        0x00000000 0x04000000 0: b16, 1: b32
        0x00000000 0x08000000 src sre signed
        operands: MCDST, LLDST, L*SRC1/L*SHARED, L*SRC2/L*CONST2, L*SRC3/L*CONST3

Integer MIN/MAX
---------------

::

  min u16/u32/s16/s32 [CDST] DST SRC1 SRC2
  max u16/u32/s16/s32 [CDST] DST SRC1 SRC2

  All operands are 32-bit or 16-bit according to size specifier.

    if (SRC1 < SRC2) { // signed comparison for s16/s32, unsigned for u16/u32.
        res = (min ? SRC1 : SRC2);
    } else {
        res = (min ? SRC2 : SRC1);
    }
    CDST.O = 0;
    CDST.C = 0;
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Long:     0x30000000 0x80000000 base opcode
        0x00000000 0x20000000 0: max, 1: min
        0x00000000 0x08000000 0: u16/u32, 1: s16/s32
        0x00000000 0x04000000 0: b16, 1: b32
        operands: MCDST, LL*DST, L*SRC1/L*SHARED, L*SRC2/L*CONST2

Integer SET
-----------

::

  set [CDST] DST cond u16/s16/u32/s32 SRC1 SRC2

  cond can be any subset of {l, g, e}.

  All operands are 32-bit or 16-bit according to size specifier.

    int s1, s2; // infinite precision
    if (signed) {
        s1 = SEX(SRC1);
        s2 = SEX(SRC2);
    } else {
        s1 = ZEX(SRC1);
        s2 = ZEX(SRC2);
    }
    bool c;
    if (s1 < s2)
        c = cond.l;
    else if (s1 == s2)
        c = cond.e;
    else /* s1 > s2 */
        c = cond.g;
    if (c) {
        res = (b32?0xffffffff:0xffff);
    } else {
        res = 0;
    }
    CDST.O = 0;
    CDST.C = 0;
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Long:     0x30000000 0x60000000 base opcode
        0x00000000 0x08000000 0: u16/u32, 1: s16/s32
        0x00000000 0x04000000 0: b16, 1: b32
        0x00000000 0x00010000 cond.g
        0x00000000 0x00008000 cond.e
        0x00000000 0x00004000 cond.l
        operands: MCDST, LL*DST, L*SRC1/L*SHARED, L*SRC2/L*CONST2

Bit operations
--------------

::

  and b32/b16 [CDST] DST [not] SRC1 [not] SRC2      O2=0, O1=0
  or b32/b16 [CDST] DST [not] SRC1 [not] SRC2       O2=0, O1=1
  xor b32/b16 [CDST] DST [not] SRC1 [not] SRC2      O2=1, O1=0
  mov2 b32/b16 [CDST] DST [not] SRC1 [not] SRC2     O2=1, O1=1

  Immediate forms only allows 32-bit operands, and cannot negate second op.

    s1 = (not1 ? ~SRC1 : SRC1);
    s2 = (not2 ? ~SRC2 : SRC2);
    switch (OP) {
        case and: res = s1 & s2; break;
        case or: res = s1 | s2; break;
        case xor: res = s1 ^ s2; break;
        case mov2: res = s2; break;
    }
    CDST.O = 0;
    CDST.C = 0;
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Imm:      0xd0000000 base opcode
        0x00400000 not1
        0x00008000 O2 bit
        0x00000100 O1 bit
        operands: SDST, SSRC/SSHARED, IMM
        assumed: not2=0 and b32.

  Long:     0xd0000000 0x00000000 base opcode
        0x00000000 0x04000000 0: b16, 1: b32
        0x00000000 0x00020000 not2
        0x00000000 0x00010000 not1
        0x00000000 0x00008000 O2 bit
        0x00000000 0x00004000 O1 bit
        operands: MCDST, LL*DST, L*SRC1/L*SHARED, L*SRC2/L*CONST2

Bit shifts
----------

::

  shl b16/b32 [CDST] DST SRC1 SRC2
  shl b16/b32 [CDST] DST SRC1 SHCNT
  shr u16/u32 [CDST] DST SRC1 SRC2
  shr u16/u32 [CDST] DST SRC1 SHCNT
  shr s16/s32 [CDST] DST SRC1 SRC2
  shr s16/s32 [CDST] DST SRC1 SHCNT

    All operands 16/32-bit according to size specifier, except SHCNT. Shift
    counts are always treated as unsigned, passing negative value to shl
    doesn't get you a shr.

        int size = (b32 ? 32 : 16);
    if (shl) {
        res = SRC1 << SRC2; // infinite precision, shift count doesn't wrap.
        if (SRC2 < size) { // yes, <. So if you shift 1 left by 32 bits, you DON'T get CDST.C set. but shift 2 left by 31 bits, and it gets set just fine.
            CDST.C = (res >> size) & 1; // basically, the bit that got shifted out.
        } else {
            CDST.C = 0;
        }
        res = res & (b32 ? 0xffffffff : 0xffff);
    } else {
        res = SRC1 >> SRC2; // infinite precision, shift count doesn't wrap.
        if (signed && S(SRC1)) {
            if (SRC2 < size)
                res |= (1<<size)-(1<<(size-SRC2)); // fill out the upper bits with 1's.
            else
                res |= (1<<size)-1;
        }
        if (SRC2 < size && SRC2 > 0) {
            CDST.C = (SRC1 >> (SRC2-1)) & 1;
        } else {
            CDST.C = 0;
        }
    }
    if (SRC2 == 1) {
        CDST.O = (S(SRC1) != S(res));
    } else {
        CDST.O = 0;
    }
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Long:     0x30000000 0xc0000000 base opcode
        0x00000000 0x20000000 0: shl, 1: shr
        0x00000000 0x08000000 0: u16/u32, 1: s16/s32 [shr only]
        0x00000000 0x04000000 0: b16, 1: b32
        0x00000000 0x00010000 0: use SRC2, 1: use SHCNT
        operands: MCDST, LL*DST, L*SRC1/L*SHARED, L*SRC2/L*CONST2/SHCNT

TBD
---

::

  interp [cent] [flat] DST v[] [SRC]

    Gets interpolated FP input, optionally multiplying by a given value

  rcp f32 DST SRC
  rsqrt f32 DST SRC
  lg2 f32 DST SRC
  sin f32 DST SRC
  cos f32 DST SRC
  ex2 f32 DST SRC

    Computes a transcendential function of the argument. rcp is 1/x, rsqrt is
    1/sqrt(x). sin, cos, ex2 need arguments preprocessed by appropriate pre
    insn. rcp, rsqrt, lg2 take a float argument directly.

  presin f32 DST SRC
  preex2 f32 DST SRC

    Preprocesses a float argument for use in subsequent sin/cos or ex2
    operation, respectively.

  mov lock CDST DST s[]

    Tries to lock a word of s[] memory and load a word from it. CDST tells
    you if it was successfully locked+loaded, or no. A successfully locked
    word can't be locked by any other thread until it is unlocked.

  mov unlock s[] SRC

    Stores a word to previously-locked s[] word and unlocks it.

  PREDICATE vote any/all CDST

    This instruction doesn't use the predicate field for conditional execution,
    abusing it instead as an input argument. vote any sets CDST to true iff the
    input predicate evaluated to true in any of the warp's active threads.
    vote all sets it to true iff the predicate evaluated to true in all acive
    threads of the current warp.

  set [CDST] DST <cmpop> f32/f64 SRC1 SRC2

    Does given comparison operation on SRC1 and SRC2. DST is set to 0xffffffff
    if comparison evaluats true, 0 if it evaluates false. if used, CDST.SZ are
    set according to DST.

  min f32/f64 DST SRC1 SRC2
  max f32/f64 DST SRC1 SRC2

    Sets DST to the smaller/larger of two SRC1 operands. If one operand is NaN,
    DST is set to the non-NaN operand. If both are NaN, DST is set to NaN.

  cvt <integer dst> <integer src>
  cvt <integer rounding modifier> <integer dst> <float src>
  cvt <rounding modifier> <float dst> <integer src>
  cvt <rounding modifier> <float dst> <float src>
  cvt <integer rounding modifier> <float dst> <float src>

    Converts between formats. For integer destinations, always clamps result
    to target type range.

  add [sat] rn/rz f32 DST SRC1 SRC2

    Adds two floating point numbers together.

  mul [sat] rn/rz f32 DST SRC1 SRC2

    Multiplies two floating point numbers together

  slct b32 DST SRC1 SRC2 f32 SRC3

    Sets DST to SRC1 if SRC3 is positive or 0, to SRC2 if SRC3 negative or NaN.

  quadop f32 <op1> <op2> <op3> <op4> DST <srclane> SRC1 SRC2

    Intra-quad information exchange instruction. Mad as a hatter.
    First, SRC1 is taken from the given lane in current quad. Then
    op<currentlanenumber> is executed on it and SRC2, results get
    written to DST. ops can be add [SRC1+SRC2], sub [SRC1-SRC2],
    subr [SRC2-SRC1], mov2 [SRC2]. srclane can be at least l0, l1,
    l2, l3, and these work everywhere. If you're running in FP, looks
    like you can also use dox [use current lane number ^ 1] and doy
    [use current lane number ^ 2], but using these elsewhere results
    in always getting 0 as the result...

  add f32 DST mul SRC1 SRC2 SRC3

    A multiply-add instruction. With intermediate rounding. Nothing
    interesting. DST = SRC1 * SRC2 + SRC3;

  fma f64 DST SRC1 SRC2 SRC3

    Fused multiply-add, with no intermediate rounding.

  texauto [deriv] live/all <texargs>

    Does a texture fetch. Inputs are: x, y, z, array index, dref [skip all
    that your current sampler setup doesn't use]. x, y, z, dref are floats,
    array index is integer. If running in FP or the deriv flag is on,
    derivatives are computed based on coordinates in all threads of current
    quad. Otherwise, derivatives are assumed 0. For FP, if the live flag
    is on, the tex instruction is only run for fragments that are going to
    be actually written to the render target, ie. for ones that are inside
    the rendered primitive and haven't been discarded yet. all executes
    the tex even for non-visible fragments, which is needed if they're going
    to be used for further derivatives, explicit or implicit.

  texbias [deriv] live/all <texargs>

    Same as texauto, except takes an additional [last] float input specifying
    the LOD bias to add. Note that bias needs to be the same for all threads
    in the current quad executing the texbias insn.

  texlod live/all <texargs>

    Does a texture fetch with given coordinates and LOD. Inputs are like
    texbias, except you have explicit LOD instead of the bias. Just like
    in texbias, the LOD should be the same for all threads involved.

  texsize live/all <texargs>

    Gives you (width, height, depth, mipmap level count) in output, takes
    integer LOD parameter as its only input.

  texfetch live/all <texargs>

    A single-texel fetch. The inputs are x, y, z, index, lod, and are all
    integer.

  emit

    GP-only instruction that emits current contents of $o registers as the
    next vertex in the output primitive and clears $o for some reason.

  restart

    GP-only instruction that finishes current output primitive and starts
    a new one.

  bra <code target>

    Branches to the given place in the code. If only some subset of threads
    in the current warp executes it, one of the paths is chosen as the active
    one, and the other is suspended until the active path exits or rejoins.

  call <code target>

    Pushes address of the next insn onto the stack and branches to given place.
    Cannot be predicated.

  ret

    Returns from a called function. If there's some not-yet-returned divergent
    path on the current stack level, switches to it. Otherwise pops off the
    entry from stack, rejoins all the paths to the pre-call state, and
    continues execution from the return address on stack. Accepts predicates.

  breakaddr <code target>

    Like call, except doesn't branch anywhere, uses given operand as the
    return address, and pushes a different type of entry onto the stack.

  break
  
    Like ret, except accepts breakaddr's stack entry type, not call's.

  quadon

    Temporarily enables all threads in the current quad, even if they were
    disabled before [by diverging, exitting, or not getting started at all].
    Nesting this is probably a bad idea, and so is using any non-quadpop
    control insns while this is active. For diverged threads, the saved PC
    is unaffected by this temporal enabling.

  quadpop

    Undoes a previous quadon command.

  bar sync <barrier number>

    Waits until all threads in the block arrive at the barrier, then continues
    execution... probably... somehow...

  trap

    Causes an error, killing the program instantly.

  joinat <code target>

    The arugment is address of a future join instruction and gets pushed
    onto the stack, together with a mask of currently active threads, for
    future rejoining.

  brkpt
  
    Doesn't seem to do anything, probably generates a breakpoint when enabled
    somewhere in PGRAPH, somehow.
  
  exit

    Actually, not a separate instruction, just a modifier available on all
    long insns. Finishes thread's execution after the current insn ends.

  join

    Also a modifier. Switches to other diverged execution paths on the same
    stack level, until they've all reached the join point, then pops off the
    entry and continues execution with a rejoined path.

-------

::

    Short instructions:

    0x000000fc: S*DST
    0x00000100: flag1
    0x00007e00: S*SRC or S*SHARED
    0x00008000: flag2
    0x003f0000: S*SRC2 or S*CONST
    0x00400000: flag3
    0x00800000: use S*CONST
    0x01000000: use S*SHARED
    0x0e000000: addressing

    Immediate instructions:

    0x00000000000000fc: S*DST
    0x0000000000000100: flag1
    0x0000000000007e00: S*SRC or S*SHARED
    0x0000000000008000: flag2
    0x00000000003f0000: IMMD, low part
    0x0000000000400000: flag3
    0x0000000000800000: -
    0x0000000001000000: use S*SHARED
    0x000000000e000000: addressing

    0x0ffffffc00000000: IMMD, high part

    Long instructions

    0x00000000000001fc: L*DST
    0x000000000000fe00: L*SRC or L*SHARED
    0x00000000007f0000: L*SRC2 or L*CONST2
    0x0000000000800000: use L*CONST2
    0x0000000001000000: use L*CONST3
    0x000000000e000000: addressing

    0x0000000400000000: addressing
    0x0000000800000000: $o DST instead of $r
    0x0000003000000000: $c reg to set
    0x0000004000000000: enable setting that $c.

    0x001fc00000000000: L*SRC3 or L*CONST3
    0x0020000000000000: use L*SHARED
    0x03c0000000000000: c[] space to use
    0x0c00000000000000: misc flags
