.. _pgraph-xf:

===========
XF overview
===========

.. contents::


Introduction
============

XF is a PGRAPH unit responsible for processing vertices before they are sent
to the rasterizer.  It first appeared on NV10 -- before that, there was no
transform engine, and the user supplied raw vertex data directly to the
rasterizer.  On G80, it has been replaced with unified shader architecture.
Curiously, it has also been transplanted for use on pre-Kepler Tegra GPUs.

The following versions of XF exist:

1. NV10: the original incarnation of XF.  It is accompanied by the lighting
   engine, LT.  Together, they perform fixed-function transform & lighting
   on incoming vertices.  Supported features:

   - computes eye-space, clip-space and window-space position
   - can transform via a weighted combination of two matrices
   - supports several texgen modes:

     - eye linear
     - object linear
     - sphere map
     - reflection map
     - normal map
     - emboss map

   - performs texture matrix multiplication
   - performs lighting calculations, making final primary and secondary colors
     out of position, normal, and input colors.  Infinite, local, and spot
     lights are supported.
   - computes or passes the fog coordinate, with radial or planar distance
     calculations
   - computes the point size based on distance
   - all of the above can be disabled in favor of a simple bypass mode

2. NV15: Bugfix version of NV10.

3. NV20: Introduces support for programmability, aka vertex shaders.
   If enabled, fixed function processing is disabled, and XF instead performs
   operations according to a user-provided program.  Other features include:

   - 16 input attributes that can be arbitrarily assigned when in programmable
     mode
   - two-sided lighting is supported -- all lighting calculations can be
     performed twice, with different parameters, outputing two sets of
     primary and secondary colors.
   - weighting supports up to 4 matrices and 4 weights
   - 4 sets of output texture coordinates are supported, and each set now
     includes 4 components.
   - more flexibility in light material specification (every material property
     can be independently assigned to primary or secondary color)

4. NV25: Includes two XF units on GPU, for double processing power.  Also has some
   minor changes in context layout.

5. NV30:

   - fixed-function viewport transform can now be performed in addition to
     programmable processing, avoding the need to include it in program
     manually
   - some fixed-function geometric calculations have been moved from LT to XF,
     for greater precision
   - a new Rankine ISA (a proper superset of Kelvin ISA), featuring:

     - condition code register and conditional execution
     - branching and subroutine calls
     - two address registers, which are now 4-component vectors
     - transcendential functions with reasonable precision
     - some minor new instructions
     - "take absolute value" modifier on all sources
     - bumped code and const memory size

   - Kelvin ISA is supported as a compatibility mode, by converting
     instructions to the new format as they are uploaded
   - 8 sets of output texture coordinates are supported
   - changed ordering of input and output attributes
   - up to 6 clip distances can be output by user program, or computed by
     fixed-function hardware
   - bypass mode has been removed (but can be trivally emulated by a simple
     vertex program)
   - to prevent infinite loops, a configurable timeout was added

6. NV34: Minor revision, removing support for alternate light attenuation mode

7. NV40:

   - LT is no longer present, and all fixed-function work is now performed on
     the main XF engine
   - Kelvin ISA is no longer supported
   - Rankine ISA is supported as a compatibility mode
   - a new Curie ISA is introduced, which is not a proper superset of the
     previous two:

     - limitted texture lookup capability (only unfiltered linear 2D FP32
       textures are supported)
     - second condition code register
     - address registers can be pushed/popped on the stack
     - indirect addressing for inputs and outputs
     - saturation modifier on outputs

   - programs are stored internally in a special native ISA which is a proper
     superset of both Rankine and Curie ISAs
   - flexible mapping of output array to atttributes
   - XF state is now specified by pipeline bundles, like most other pipeline
     state -- XFMODE is gone
   - individual XF units are now called VPEs are are more independent of each
     other

8. NV41: Rankine compatibility has been removed:

   - the fixed-function mode is completely gone
   - Rankine ISA is no longer supported
   - Curie ISA is now used directly as the native ISA

9. NV43: Shortened XFCTX from 0x220 words to 0x1d4 words.

10. NV44: unknown changes from NV43.

11. Tegra: derived from Curie, but not much known.


Structure and operation
=======================

The XF complex is in the main pipeline after the IDX complex (for Kelvin,
this means after the FD unit) and before the VTX complex (aka the
post-transform cache).  It is made of the following parts:

1. IDX2XF: Input interface from the IDX complex (for Kelvin, from the FD
   unit).  XF receives all sorts of commands here.

2. XF2VTX: Output interface to the VTX complex.  XF outputs processed vertices
   and passthrough data here.  On Celsius, also used to implement state readback
   for context switching.  Note that no commands are emitted on this interface
   -- VTX instead takes commands directly from the IDX complex by a side FIFO
   (IDX2VTX) that bypasses the XF complex.  Data will only be consumed from here
   by VTX when it's told what to expect via the IDX2VTX interface.

3. VAB: vertex attribute buffer.  Serves as assembly space for data received on
   the IDX2XF interface.  Has one 128-bit slot for every input vertex attribute,
   plus one extra "passthrough" slot for assembling state updates.  Data goes
   from here to IBUF or XFPR.

4. XFMODE [NV10:NV40] or bundle [NV40:] storage: Remembers the control bits
   for the whole XF complex.

5. One or more VPEs, which do the main load of vertex processing.  Each one
   has:

   1. XFPR [NV20:]: RAM containing user programs.  Before NV40, shared
      between all VPEs.
   2. XFCTX: RAM containing parameters for fixed-function processing and user
      programs.  Made of 4-element vectors of 32-bit floats.  Before NV40,
      shared between all VPEs.
   3. Several copies of input/output buffers (6 copies on NV10:NV40, ??? on
      NV40:), one for each inflight vertex:

      1. IBUF: contains input attributes of the vertex
      2. TBUF: contains output attributes of the vertex
         (at least the subset computed before LT).
      3. WBUF [NV10:NV30]: contains outputs to be consumed by the LT unit
         for lighting calculations, made of 3-element vectors of 22-bit
         floating-point numbers.
      4. VBUF [NV10:NV30]: a second buffer like WBUF.
      5. UBUF [NV30:NV40]: like WBUF/VBUF on earlier GPUs, but now contains
         5-element vectors.
      6. STPOS [NV20:NV40]: a shadow copy of the first output attribute.
      7. SIPOS [NV25:NV40?]: a shadow copy of the first input attribute ???

   4. XFREG: Temporary register file.
   5. Control unit -- contains PC, condition code, address registers, call
      stack, and fixed-function program sequencer.  Can control processing
      of up to 3 vertices at a time, in SMT fashion.
   6. MLU: the multiplication execution unit.  Can do 4 32-bit floating-point
      multiplies every cycle.
   7. ALU: the addition execution unit.  Can do 3 [NV10:NV20] or 4 [NV20:]
      32-bit 2-input floating-point sums, or a single 4-input sum every cycle.
      Can also do comparisons and other simple operations.
   8. ILU: the inverse execution unit.  Can do one approximate reciprocal or
      reciprocal square root per two cycles.  On NV20:, can also do
      low-precision exponential and logarithm approximations.
   9. MFU [NV30:]: the multi-function unit.  Can compute EX2, LG2, SIN, COS
      with reasonable precision.

6. The LT unit [NV10:NV40], computing final vertex colors in fixed-function
   mode (as well as point size and fog before NV30).  Uses a lower-precision
   22-bit floating point format.  Made of:

   1. LTCTX: RAM containing parameters for fixed-function processing
      (like XFCTX).  Made of 3-element vectors of 22-bit floats.
      On NV25:NV30, split into two RAMs: LTCTX_A and LTCTX_B.
   2. Control unit -- steps through the LT microcode, processing up to 3
      vertices at a time in SMT fashion.
   3. MLU: can perform 3 float multiplications per cycle.
   4. ALU: can perform 3 float additions or one 3-input sum per cycle.
   5. MAC0 and MAC2: perform scalar float multiply-accumulate operations.
      On NV30:, MAC0 can only do accumulate (no multiplication).
   6. LTC0 (for MAC0) [NV10:NV30] and LTC2 (for MAC2): RAMs containing
      multiplication factors for the MACs.  Made of 22-bit floats.
   7. LTC1 (for MAC0) and LTC3 (for MAC2): RAMs containing additive factors
      for the MACs.  Made of 22-bit floats.
   8. ILU: performs very approximate reciprocal, reciprocal square, and some
      misc operations.
   9. LTREG: the temporary register file.

The VAB, XFCTX, XFPR, LTCTX, and LTC* RAMs need to be context-switched.
On Celsius, this is done via the readback functionality.  On Kelvin and
Rankine, they can be accessed via the RDI interface (done automatically
by the hardware context switch).  On Curie, they can be context-switched
by the context microcode.

All input/output and computation is performed on 32-bit or 22-bit floats --
vertex attributes read from different formats are converted by IDX, and
output attributes that require different formats are converted by VTX.
The 32-bit floats are in IEEE single-precision format with some minor
modifications:

- denormals are not supported (and are considered equal to 0).
- there is no distinction between QNaNs and SNaNs (since there are no traps
  in XF, all NaNs are effectively quiet).  Whenever a NaN is created,
  the value ``0x7fffffff`` is used.

The 22-bit float format is used by computations in the LT unit, and works
like the 32-bit float format with the low 10 bits cut off (and assumed to
be 0).

.. todo:: NV25, NV30 have RAMs unaccounted for.

.. todo:: Curie still has switchable RAMs unaccounted for.


IDX2XF: the command interface
=============================

IDX2XF is the input interface to XF.  IDX (or FD on Kelvin) can perform the
following operations here:

- write command: contains a 4-bit command type, an address (10 to 14-bit,
  depending on GPU) and a 32-bit or 64-bit payload.
  Depending on the address, can update a piece of XF state, request
  a data passthru to XF2VTX interface, or start a vertex state program.
- read command [Celsius only]: contains a command type and an address, like
  a write command.  Requests a readback of a piece of state to the XF2VTX
  interface.  Used to implement context switching (badly), not used otherwise.
- vertex trigger: starts processing a vertex, which will be output on
  the XF2VTX interface when fully processed.

The addresses for commands are usually constructed as follows:

- bits 0-1: always 0 (ie. all addresses are word-aligned).
- bits 2-3: select a 32-bit word in a 128-bit vector.  0 is the highest word
  (or the X component), while 3 is the lowest word (or the W component).
- bits 4-9 [NV10:NV20], 4-11 [NV20:NV30], 4-12 [NV30:NV40], or 4-13 [NV40:]:
  select the 128-bit vector in a space.

Read commands always target a 32-bit word, which will be read and delivered
to XF2VTX interface.  If the address is not valid for reading, XF will ignore
the read command and deliver nothing to VTX.  This will cause VTX to hang,
in turn hanging FE3D, the PCI bus, the CPU, and the whole machine.  Don't do
that.

Write commands can target a 32-bit word, or an aligned pair of 32-bit words.
Since XF internal paths are mostly 128-bit wide, several write commands
are usually needed to perform a single operation.  Thus, for most commands,
writing to words 0-2 merely store the payload in the VAB passthrough
slot, while writing to word 3 completes the 128-bit vector in
the VAB and send it downstream.

Note that XF is, in many ways, a big-endian creature (though not consistently
so).  Since most of the GPU follows little-endian design, this leads to
things looking reversed in many places (in particular, when RDI is accessed).
You have been warned...

The following command types exist:

- 0x0: NOP.  Writes store the payload in VAB passthrough slot and do nothing.
  Not readable.
- 0x1: VAB.  Writes or reads VAB words.  Used by IDX to upload input vertex
  attributes.
- 0x2: XFPR [NV20:].  Writes program instructions to the XFPR RAM (possibly
  with ISA encoding conversion), assembling them in VAB.
- 0x4: PARAM [NV20:NV41?].  Writes the VAB passthrough slot, does nothing
  else.  Used together with RUN command to pass a parameter to a vertex
  state program.
- 0x5: PASSTHRU.  Passes its payload through VAB, IBUF and TBUF to the XF2VTX
  interface.  This command is used by IDX along with the BUNDLE command on
  the IDX2VTX interface to send bundles to the VTX complex.  Using it
  without the accompanying IDX2VTX command will desync and hang VTX, so don't
  do that.  Not readable.
- 0x6: RUN [NV20:NV41?].  Starts execution of a vertex state program, copying
  its parameter from the passthrough VAB slot to the IBUF.  Meant to be used
  with the PARAM command.  The low bits of the payload contain starting PC
  of the vertex state program.
- 0x7: MODE [NV10:NV40].  Assembles a vector and sends it to the internal
  XFMODE storage.  Not readable.
- 0x8: XTRA [NV30:NV41].  Assembles a vector and sends it to the extra XFPR
  RAM slots.
- 0x9: XFCTX.  Assembles a vector and sends it through IBUF to XFCTX.
  Readable.
- 0xa: LTCTX.  Assembles a vector and sends it through IBUF and WBUF/VBUF to
  LTCTX.  Readable.
- 0xb: LTC0 [NV10:NV30].  Goes  through IBUF and WBUF/VBUF to LTC0.  Readable.
- 0xc: LTC1.  Goes  through IBUF and WBUF/VBUF/UBUF to LTC1.  Readable.
- 0xd: LTC2.  Likewise.
- 0xe: LTC3.  Likewise.
- 0xf: SYNC.  Performs a full XF barrier -- waits for all pending vertices
  to be processed before processing any more commands.  Not readable.

XF commands will be emitted by IDX in the following circumstances:

- whenever vertex data is submitted by any means (through vertex buffers,
  inline data, or immediate mode), the corresponding VAB write command will
  be sent to XF.
- whenever a bundle command is processed by IDX, the bundle will be submitted
  as payload in the PASSTHRU command, and a corresponding bundle token will
  be emitted on IDX2VTX interface.
- a "submit XF command" IDX command is received on the FE2IDX interface,
  either from method execution or from the PIPE MMIO register.

.. todo:: None of the above is certain on Curie.


IDX command wrapping
--------------------

The FE can submit commands to XF by wrapping them in IDX commands and sending
them on the FE2IDX interface.  When IDX sees such a wrapped command, it will
be unwrap it at the last stage of processing and emit it on the IDX2XF
interface.  This functionality is used by FE when executing methods that
update XF context, and can be used by the driver directly through the PIPE
MMIO register as well.

On Celsius, the wrapped command addresses are:

- bits 0-9: XF address
- bits 10-13: XF command type
- bit 14: set to 1 (identifies wrapped XF command).

On Kelvin:

- bits 0-11: XF address
- bits 12-15: XF command type
- bit 16: set to 1.

On Rankine:

- bits 0-12: XF address
- bits 13-16: XF command type
- bit 17: set to 1.

On Curie:

- bits 0-13: XF address
- bits 14-17: XF command type
- target code: set to 3?

.. todo:: Figure out how this works on Curie.


VAB: vertex assembly buffer
===========================

VAB is the front gate to the XF complex.  Its purpose is twofold:

1. Keeping track of the last submitted value of every input vertex attribute,
   whether it comes from immediate data, inline data, or vertex buffer.
2. Assembling 32-bit or 64-bit input words into 128-bit vectors [NV10:NV40].

Whenever IDX signals that a vertex is to be processed, the contents of the
VAB (except for the passthrough slot) are copied to an IBUF slot for
processing, and data for the next vertex can be loaded to the VAB while
XF is working on the previous one(s) in IBUF.


Celsius
-------

On Celsius, VAB is made of 8 128-bit vectors, which are in turn made of 4
32-bit words.  The first 7 vectors correspond more or less to the first 7
vertex attributes recognized by IDX, while the last one is special:

- 0: OPOS, the object position.
- 1: COL0, the primary color.  The X, Y, Z, W components correspond to
  R, G, B, A components of the color.
- 2: COL1F, the secondary color and fog coordinate.  The first three
  components (X, Y, Z) correspond to R, G, B components of the secondary
  color, while component W corresponds to the fog factor.
- 3: TXC0, the texture 0 coordinates.
- 4: TXC1, the texture 1 coordinates.
- 5: NRML, the normal.  Component W is effectively unused.
- 6: WGHT, the weight (used for transform matrix interpolation), stored
  in component X.  Components Y, Z, W are effectively unused.
- 7: PASS, the passthrough slot, used to assemble full vectors for commands
  other than VAB.


Kelvin and up
-------------

On Kelvin and Rankine, VAB is made of 17 128-bit vectors:

- 0-15: Generic input vertex attributes, corresponding directly to the ones
  used by IDX.
- 16: PASS, the passthrough slot.

On Curie, VAB is made of 16 128-bit vectors, corresponding directly to the
input vertex attributes (there is no passthrough slot).

If the fixed function transformation is used on Kelvin, the input attributes
have the following interpretation:

- 0: OPOS.
- 1: WGHT, a vector of up to 4 weights used for transform matrix interpolation.
- 2: NRML (only X, Y, Z are used).
- 3: COL0.
- 4: COL1 (only X, Y, Z are used).
- 5: FOGC, the fog coordinate (only X is used).
- 6-8: not used.
- 9-12: TXC0-TXC3, the texture coordinates.
- 13-15: not used.

On Rankine and Curie, the interpretation for fixed-function is:

- 0: OPOS.
- 1: WGHT.
- 2: NRML.
- 3: COL0.
- 4: COL1.
- 5: FOGC.
- 6-7: not used.
- 8-15: TXC0-TXC7.


The passthrough slot
--------------------

The passthrough slot is used by commands that upload data into XF (other than
VAB commands) to assemble the full 128-bit value from 32-bit or 64-bit pieces.
All write commands of the relevant types write their payload to the
corresponding 32-bit component (or component pair) of the passthrough slot,
then (on the final component, or for some commands, on any component) send
the value of the whole passthrough slot downstream.  This includes the
following commands:

- NOP (though the written data is ignored in this case)
- SYNC (data is likewise ignored)
- XFPR
- PARAM (merely gathers the components, does not send them anywhere)
- RUN (doesn't write the slot, merely reads the value left by the PARAM
  command)
- PASSTHRU
- XTRA
- MODE
- XFCTX
- LTC*

.. todo:: How are things assembled on Curie?


RDI access
----------

On Kelvin and Rankine, VAB can be accessed through RDI as space ``0x15``.
This space is made of 128-bit little-endian quaadwords.  When writing,
a complete 128-bit quadword must be written at once, or data will be damaged.
Note that the 32-bit words inside quadwords are effectively in reverse order
wrt IDX2XF commands (since IDX2XF transfers the high word as word 0).  In
other words:

- bits 0-31 (RDI address 0x0 modulo 0x10): W component, IDX2XF word 3
- bits 32-63 (RDI address 0x4 modulo 0x10): Z component, IDX2XF word 2
- bits 64-95 (RDI address 0x8 modulo 0x10): Y component, IDX2XF word 1
- bits 96-127 (RDI address 0xc modulo 0x10): X component, IDX2XF word 0


VAB command
-----------

The VAB command (type 0x1) can be sent by IDX to write or read VAB slots.
To simplify writing attributes shorter than 4 components, the write command
has some special behavior.

On Celsius, the write command works like this:

1. If component X or Y of slots 0, 1, 3, or 4 (OPOS, COL0, TXC*) is being
   written:

   1. On NV15 and up, set component Y to 0.
   2. Set component Z to 0.
   3. Set component W to 0x3f800000 (1.0f).

2. Set the selected component(s) of the selected slot to the submitted
   value(s).

On Kelvin and up, the write command works like this:

1. If component X of any slot other than the passthrough one is being written:

   1. Set component Y to 0.
   2. Set component Z to 0.
   3. Set component W to 0x3f800000 (1.0f).

2. Set the selected component(s) of the selected slot to the submitted
   value(s).
