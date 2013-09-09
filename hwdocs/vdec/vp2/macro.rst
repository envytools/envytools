.. _pvp2-macro:

===========================
VP2 command macro processor
===========================

.. contents::


Introduction
============

The VP2 macro processor is a small programmable processor that can emit vector
processor commands when triggered by special commands from xtensa. All vector
commands first go through the macro processor, which checks whether they're
in macro command range, and either passes them down to vector processor, or
interprets them itself, possibly launching a macro and submitting other vector
commands. It is one of the four major blocks making up the PVP2 engine.

The macro processor has:

- 64-bit VLIW opcodes, controlling two separate execution paths, one
  primarily for processing/emitting commands, the other for command
  parameters
- dedicated code RAM, 512 64-bit words in size
- 32 * 32-bit word LUT data space, RW by host and RO by the macro code
- 6 32-bit global [not banked] GPRs visible to macro code and host [$g0-$g5]
- 8 32-bit banked GPRs visible to macro code and host, meant for passing
  parameters - one bank is writable by the param commands, the other is in
  use by macro code at any time [$p0-$p7]
- 3 1-bit predicates, with conditional execution [$p1-$p3]
- instruction set consisting of bit operations, shifts, and 16-bit addition
- no branch/loop capabilities
- a 32-bit command path accumulator [$cacc]
- a 32-bit data path accumulator [$dacc]
- a 7-bit LUT address register [$lutidx]
- 15-bit command, 32-bit data, and 8-bit high data registers for command
  submission [$cmd, $data, $datahi]
- 64-entry input command FIFO
- 2-entry output command FIFO
- a single hardware breakpoint


.. _pvp2-io-macro:

MMIO registers
==============

The macro processor registers occupy 0x00f600:0x00f700 range in BAR0 space,
corresponding to 0x2c000:0x2e000 range in PVP2's XLMI space. They are:

=============== ================= ============= ==============
XLMI            MMIO              Name          Description
=============== ================= ============= ==============
0x2c000         0x00f600          CONTROL       :ref:`master control <pvp2-io-macro-control>`
0x2c100         0x00f608          STATUS        :ref:`detailed status <pvp2-io-macro-status>`
0x2c180         0x00f60c          IDLE          :ref:`a busy/idle status <pvp2-io-macro-idle>`
0x2c200         0x00f610          INTR_EN       :ref:`interrupt enable <pvp2-io-macro-intr>`
0x2c280         0x00f614          INTR          :ref:`interrupt status <pvp2-io-macro-intr>`
0x2c300         0x00f618          BREAKPOINT    :ref:`breakpoint address and enable <pvp2-io-macro-breakpoint>`
0x2c800:0x2c880 0x00f640          LUT[0:32]     :ref:`the LUT data <pvp2-io-macro-lut>`
0x2c880:0x2c8a0 0x00f644          PARAM_A[0:8]  :ref:`$p bank A <pvp2-io-macro-param>`
0x2c900:0x2c920 0x00f648          PARAM_B[0:8]  :ref:`$p bank B <pvp2-io-macro-param>`
0x2c980:0x2c9a0 0x00f64c          GLOBAL[0:8]   :ref:`$g registers <pvp2-io-macro-global>`
0x2cb80         0x00f65c          PARAM_SEL     :ref:`$p bank selection switch <pvp2-io-macro-param>`
0x2cc00         0x00f660          RUNNING       :ref:`code execution in progress switch <pvp2-io-macro-running>`
0x2cc80         0x00f664          PC            :ref:`program counter <pvp2-io-macro-pc>`
0x2cd00         0x00f668          DATAHI        :ref:`$datahi register <pvp2-io-macro-sreg>`
0x2cd80         0x00f66c          LUTIDX        :ref:`$lutidx register <pvp2-io-macro-sreg>`
0x2ce00         0x00f670          CACC          :ref:`$cacc register <pvp2-io-macro-sreg>`
0x2ce80         0x00f674          CMD           :ref:`$cmd register <pvp2-io-macro-sreg>`
0x2cf00         0x00f678          DACC          :ref:`$dacc register <pvp2-io-macro-sreg>`
0x2cf80         0x00f67c          DATA          :ref:`$data register <pvp2-io-macro-sreg>`
0x2d000         0x00f680          IFIFO_DATA    :ref:`input FIFO data <pvp2-io-macro-fifo>`
0x2d080         0x00f684          IFIFO_ADDR    :ref:`input FIFO command <pvp2-io-macro-fifo>`
0x2d100         0x00f688          IFIFO_TRIGGER :ref:`input FIFO manual read/write trigger <pvp2-io-macro-fifo>`
0x2d180         0x00f66c          IFIFO_SIZE    :ref:`input FIFO size limitter <pvp2-io-macro-fifo>`
0x2d200         0x00f670          IFIFO_STATUS  :ref:`input FIFO status <pvp2-io-macro-fifo>`
0x2d280         0x00f674          OFIFO_DATA    :ref:`output FIFO data <pvp2-io-macro-fifo>`
0x2d300         0x00f678          OFIFO_ADDR    :ref:`output FIFO command & high data <pvp2-io-macro-fifo>`
0x2d380         0x00f67c          OFIFO_TRIGGER :ref:`output FIFO manual read/write trigger <pvp2-io-macro-fifo>`
0x2d400         0x00f680          OFIFO_SIZE    :ref:`output FIFO size limitter <pvp2-io-macro-fifo>`
0x2d480         0x00f684          OFIFO_STATUS  :ref:`output FIFO status <pvp2-io-macro-fifo>`
0x2d780         0x00f6bc          CODE_SEL      :ref:`selects high or low part of code RAM for code window <pvp2-io-macro-code>`
0x2d800:0x2e000 0x00f6c0:0x00f700 CODE          :ref:`a 256-word window to code space <pvp2-io-macro-code>`
=============== ================= ============= ==============


.. _pvp2-io-macro-control:
.. _pvp2-io-macro-status:
.. _pvp2-io-macro-idle:

Control and status registers
============================

.. todo:: write me


.. _pvp2-io-macro-intr:

Interrupts
==========

.. todo:: write me


.. _pvp2-io-macro-fifo:

FIFOs
=====

.. todo:: write me


.. _pvp2-cmd-macro:

Commands
========

The macro processor processes commands in 0xc000-0xdfff range from the input
FIFO, passing down all other commands directly to the output FIFO [provided
that no macro is executing at the moment]. The macro processor commands are:

========== =================== =================
Command    Name                Description
========== =================== =================
0xc000+i*4 MACRO_PARAM[0:8]    :ref:`write to $p host register bank <pvp2-cmd-macro-param>`
0xc020+i*4 MACRO_GLOBAL[0:8]   :ref:`write to $g registers <pvp2-cmd-macro-global>`
0xc080+i*4 MACRO_LUT[0:32]     :ref:`write to given LUT entry <pvp2-cmd-macro-lut>`
0xc100     MACRO_EXEC          :ref:`execute a macro <pvp2-cmd-macro-exec>`
0xc200     MACRO_DATAHI        :ref:`write to $datahi register <pvp2-cmd-macro-datahi>`
0xd000+i*4 MACRO_CODE[0:0x400] :ref:`upload half of a code word <pvp2-cmd-macro-code>`
========== =================== =================


Execution state and registers
=============================

.. _pvp2-io-macro-code:
.. _pvp2-cmd-macro-code:

Code RAM
--------

The code RAM contains 512 opcodes. Opcodes are 64 bits long and are accessible
by the host as pairs of 32-bit words. Code may be read or written using MMIO
window:

BAR0 0x00f6bc / XLMI 0x2d780: CODE_SEL
  1-bit RW register. Writing 0 selects code RAM entries 0:0x100 to be mapped to
  the CODE window, writing 1 selects code RAM entries 0x100:0x200.

BAR0 0x00f6c0 + (i >> 5) * 4 [index i & 0x1f] / XLMI 0x2d800 + i * 4, i < 0x200: CODE[i]
  The code window. Reading or writing CODE[i] is equivalent to reading or
  writing low [if i is even] or high [if i is odd] 32 bits of code RAM cell
  i >> 1 | CODE_SEL << 8.

They can also be written in pipelined manner by the MACRO_CODE command:

VP command 0xd000 + i * 4, i < 0x400: MACRO_CODE[i]
  Write the parameter to low [if i is even] or high [if i is odd] 32 bits of
  code RAM cell i >> 1. If a macro is currently executing, execution of this
  command is blocked until it finishes. Valid only on macro input FIFO.


.. _pvp2-io-macro-running:
.. _pvp2-io-macro-pc:
.. _pvp2-io-macro-breakpoint:
.. _pvp2-cmd-macro-exec:

Execution control
-----------------

.. todo:: write me


.. _pvp2-io-macro-param:
.. _pvp2-cmd-macro-param:

Parameter registers
-------------------

Parameter registers server dual purpose: they're meant for passing parameters
to macros, but can also be used as GPRs by the code. There are two banks of
parameter registers, bank A and bank B. Each bank contains 8 32-bit registers.
At any time, one of the banks is in use by the macro code, while the other can
be written by the host via MACRO_PARAM commands for next macro execution. Each
time a macro is launched, the bank assignments are swapped. The current
assignment is controlled by the PARAM_SEL register:

BAR0 0x00f65c / XLMI 0x2cb80: PARAM_SEL
  1-bit RW register. Can be set to one of:

  - 0: CODE_A_CMD_B - bank A is in use by the macro code, commands will write
    to bank B
  - 1: CODE_B_CMD_A - bank B is in use by the macro code, commands will write
    to bank A

  This register is toggled on every MACRO_EXEC command execution.

The parameter register banks can be accessed through MMIO registers:

BAR0 0x00f644 [index i] / XLMI 0x2c880 + i * 4, i < 8: PARAM_A[i]
BAR0 0x00f648 [index i] / XLMI 0x2c900 + i * 4, i < 8: PARAM_B[i]
  These MMIO registers are mapped straight to corresponding parameter
  registers.
 
The bank not currently in use by code can also be written by MACRO_PARAM
commands:

VP command 0xc000 + i * 4, i < 8: MACRO_PARAM[i]
  Write the command data to parameter register i of the bank currently
  not assigned to the macro code. Execution of this command won't wait for
  the current macro execution to finish. Valid only on macro input FIFO.

The parameter registers are visible to the macro code as GPR registers 0-7.


.. _pvp2-io-macro-global:
.. _pvp2-cmd-macro-global:

Global registers
----------------

There are 6 normal global registers, $g0-$g5. They are simply 32-bit GPRs for
use by macro computations. There are also two special global pseudo-registers,
$g6 and $g7.

$g6 is the LUT readout register. Any attempt to read from it will read from
the LUT entry selected by $lutidx register. Any attempt to write to it will
be ignored.

$g7 is the special predicate register, $pred. Its 4 low bits are mapped to the
four predicates, $p0-$p3. Any attempt to read from this register will read
the predicates, and fill high 28 bits with zeros. Any attempt to write this
register will write the predicates.

$p0 is always forced to 1, while $p1-$p3 are writable. The predicates are used
for conditional execution in macro code. In addition to access through $pred,
the predicates can also be written by macro code individually as a result of 
various operations.

All 8 global registers are accessible through MMIO and the command stream:

BAR0 0x00f64c [index i] / XLMI 0x2c980 + i * 4, i < 8: GLOBAL[i]
  These registers are mapped straight to corresponding global registers.

VP command 0xc020 + i * 4, i < 8: MACRO_GLOBAL[i]
  Write the command data to global register i. If a macro is currently
  executing, execution of this command is blocked until it finishes. Valid
  only on macro input FIFO.

The global registers are visible to the macro code as GPR registers 8-15.


.. _pvp2-io-macro-sreg:
.. _pvp2-cmd-macro-datahi:

Special registers
-----------------

In addition to the GPRs, the macro code can use 6 special registers. There are
4 special registers belonging to the command execution path, identified by
a 2-bit index:

- 0: $cacc, command accumulator
- 1: $cmd, output command register
- 2: $lutidx, LUT index
- 3: $datahi, output high data register

There are also 2 special registers belonging to the data execution path,
identified by a 1-bit index:

- 0: $dacc, data accumulator
- 1: $data, output data register

The $cacc and $dacc registers are 32-bit and can be read back by the macro
code, and so are usable for general purpose computations.

The $cmd, $data, and $datahi registers are write-only by the macro code, and
their contents are submitted to the macro output FIFO when a submit opcode
is executed. $data is 32-bit, $datahi is 8-bit, mapping to bits 0-7 of written
values. $cmd is 15-bit, mapping to bits 2-16 of written values. The $datahi
register is also used to fill the high data bits in output FIFO whenever
a command is bypassed from the input FIFO.

The $lutidx register is 5-bit and write-only by the macro code. It maps to
bits 0-4 of written values. Its value selects the LUT entry visible in $g6
pseudo-register.

All 6 special registers can be accessed through MMIO, and the $datahi register
can be additionally set by a command:

MMIO 0x00f668 / XLMI 0x2cd00: DATAHI
MMIO 0x00f66c / XLMI 0x2cd80: LUTIDX
MMIO 0x00f670 / XLMI 0x2ce00: CACC
MMIO 0x00f674 / XLMI 0x2ce80: CMD
MMIO 0x00f678 / XLMI 0x2cf00: DACC
MMIO 0x00f67c / XLMI 0x2cf80: DATA
  These registers map directly to corresponding special registers. For $cacc,
  $dacc, and $data, all bits are valid. For $cmd, bits 2-16 are valid.
  For $lutidx, bits 0-4 are valid. For $datahi, bits 0-7 are valid. Remaining
  bits are forced to 0.

VP command 0xc200: MACRO_DATAHI
  Sets $datahi to low 8 bits of the command data.  If a macro is currently
  executing, execution of this command is blocked until it finishes. Valid
  only on macro input FIFO.


.. _pvp2-io-macro-lut:
.. _pvp2-cmd-macro-lut:

The LUT
-------

The LUT is a small indexable RAM that's read-only by the macro code, but
freely writable by the host. It's made of 32 32-bit words. The LUT entry
selected by $lutidx register can be read by macro code simply by reading from
the $g6 pseudo-register. The LUT can be accessed by the host through MMIO
and the command stream:

BAR0 0x00f640 [index i] / XLMI 0x2c800 + i * 4, i < 32: LUT[i]
  These registers are mapped straight to corresponding LUT entries.

VP command 0xc080 + i * 4, i < 32: MACRO_LUT[i]
  Write the command data to LUT entry i. If a macro is currently executing,
  execution of this command is blocked until it finishes. Valid only on macro
  input FIFO.


Opcodes
=======

The code opcodes are 64 bits long. They're divided in several major parts:

- bits 0-2: conditional execution predicate selection.

  - bits 0-1: PRED, the predicate to use [selected from $p0-$p3]
  - bit 2: PNOT, selects whether the predicate is negated before use.

- bit 3: EXIT, exit flag
- bit 4: SUBMIT, submit flag
- bits 5-30: command opcode
- bits 31-32: PDST, predicate destination [selected from $p0-$p3]
- bits 33-63: data opcode

When a macro is launched, opcodes are executed sequentially from the macro
start address until an opcode with the exit flag set is executed. An opcode
is executed as follows:

1. If the SUBMIT bit is set, the current values of $cmd, $data, $datahi are
   sent to the output FIFO.
2. Conditional execution status is determined: the predicate selected by
   PRED is read. If PNOT is set to 0, conditional execution will be enabled
   if the predicate is set to 1. Otherwise [PNOT set to 1], conditional
   execution will be enabled if the predicate is set to 0.  Unconditional
   opcodes are simply opcodes using non-negated predicate $p0 [PRED = 0,
   PNOT = 0].
3. If the SUBMIT bit is set, conditional execution is enabled, and
   ($cmd & 0x1fe80) == 0xb000 [ie. the submitted command was in 0xb000-0xb07c
   or 0xb100-0xb17c ranges, correnspoding to vector processor param commands],
   $cmd is incremented by 4. This enables submitting several parameters in
   a row without having to update the $cmd register.
4. If conditional execution is enabled, the command opcode is executed, and
   the command result, command predicate result, and the C2D intermediate
   value are computed.
5. If conditional execution is enabled, the data opcode is executed, and
   the data result and data predicate result are computed.
6. If conditional execution is enabled, the command and data results are
   written to their destination registers.
7. If the EXIT bit is set, macro execution halts.

Effectively, conditional execution affects all computations [including auto
$cmd increment], but doesn't affect submit and exit opcodes.


Command opcodes
---------------

The command processing path is mainly meant for processing commands and data
going to $lutidx/$datahi register, but can also exchange data with the data
processing path if needed.

The command opcode bitfields are:

- bits 5-9: CBFSTART - bitfield start [CINSRT_R, CINSRT_I, some data ops]
- bits 10-14: CBFEND - bitfield end [CINSRT_R, CINSRT_I, some data ops]
- bits 15-19: CSHIFT - shift count [CINSRT_R]
- bit 20: CSHDIR - shift direction [CINSRT_R]
- bits 15-20: CIMM6 - 6-bit unsigned immediate [CINSRT_I]
- bits 21-22: CSRC2 - selects command source #2 [CINSRT_I, CINSRT_R], one of:

  - 0: ZERO, source #2 is 0
  - 1: CACC, source #2 is current value of $cacc
  - 2: DACC, source #2 is current value of $dacc
  - 3: GPR, source #2 is same as command source #1

- bits 15-22: CIMM8 - 8-bit unsigned immediate [CEXTRADD8]
- bits 5-22: CIMM18 - 18-bit signed immediate [CMOV_I]
- bits 23-26: CSRC1 - selects command source #1 [CINSRT_R, CEXTRADD8,
  DSHIFT_R, DADD16_R]. The command source #1 is the GPR with index selected
  by this bitfield.
- bits 27-28: CDST - the command destination, determines where the command
  result will be written; one of:

  - 0: CACC
  - 1: CMD
  - 2: LUTIDX
  - 3: DATAHI

- bits 29-30: COP - the command operation, one of:

  - 0: CINSRT_R, bitfield insertion with shift, register sources
  - 1: CINSRT_I, bitfield insertion with 6-bit immediate source
  - 2: CMOV_I, 18-bit immediate value load
  - 3: CEXTRADD8, bitfield extraction + 8-bit immediate addition

The command processing path computes four values for further processing:

- the command result, ie. the 32-bit value that will later be written to
  the command destination register
- the command predicate result, ie. the 1-bit value that may later be
  written to the destination predicate
- the C2D value, a 32-bit intermediate result used in some data opcodes
- the command bitfield mask [CBFMASK], a 32-bit value used in some command
  and data opcodes

The command bitfield mask is used by the bitfield insertion operations. It
is computed from the command bitfield start and end as follows::

    if (CBFEND >= CBFSTART) {
        CBFMASK = (2 << CBFEND) - (1 << CBFSTART); // bits CBFSTART-CBFEND are 1
    } else {
        CBFMASK = 0;
    }

Since the CBFEND and CBFSTART fields conflict with CIMM18 field, the data ops
using the command mask should not be used together with the CMOV_I operation.

The CINSRT_R operation has the following semantics::

    if (CSHDIR == 0) /* 0 is left shift, 1 is right logical shift */
        shifted_source = command_source_1 << CSHIFT;
    else
        shifted_source = command_source_1 >> CSHIFT;
    C2D = command_result = (shifted_source & CBFMASK) | (command_source_2 & ~CBFMASK);
    command_predicate_result = (shifted_source & CBFMASK) == 0;

The CINSRT_I operation has the following semantics::

    C2D = command_result = (CIMM6 << CBFSTART & CBFMASK) | (command_source_2 & ~CBFMASK);
    command_predicate_result = 0;

The CMOV_I operation has the following semantics::

    C2D = command_result = sext(CIMM18, 17); /* sign-extend 18-bit immediate to 32 bits */
    command_predicate_result = 0;

The CEXTRADD8 operation has the following semantics::

    C2D = (command_source_1 & CBFMASK) >> CBFSTART;
    command_result = ((C2D + CIMM8) & 0xff) | (C2D & ~0xff); /* add immediate to low 8 bits of extracted value */
    command_predicate_result = 0;


Data opcodes
------------

The command processing path is mainly meant for processing command data, but
can also exchange data with the command processing path if needed.

The data opcode bitfields are:

- bits 33-37: DBFSTART - bitfield start [DINSRT_R, DINSRT_I, DSEXT]
- bits 38-42: DBFEND - bitfield end [DINSRT_R, DINSRT_I, DSEXT]
- bits 43-47: DSHIFT - shift count and SEXT bit position [DINSRT_R, DSEXT]
- bit 48: DSHDIR - shift direction [DINSRT_R, DSHIFT_R]
- bits 43-48: DIMM6 - 6-bit unsigned immediate [DINSRT_I]
- bits 33-48: DIMM16 - 16-bit immediate [DADD16_I, DLOGOP16_I]
- bit 49: C2DEN - enables double bitfield insertion, using C2D value
  [DINSRT_R, DINSRT_I, DSEXT]
- bit 49: DDSTSKIP - skips DDST write if set [DADD16_I]
- bit 49: DSUB - selects whether DADD16_R operation does an addition or
  substraction
- bits 49-50: DLOGOP - the DLOGOP16_I suboperation, one of:

  - 0: MOV, result is set to immediate
  - 1: AND, result is source ANDed with the immediate
  - 2: OR, result is source ORed with the immediate
  - 3: XOR, result is source XORed with the immediate

- bits 50-51: DSRC2 - selects data source #2 [DINSRT_R, DINSRT_I], one of:

  - 0: ZERO, source #2 is 0
  - 1: CACC, source #2 is current value of $cacc
  - 2: DACC, source #2 is current value of $dacc
  - 3: GPR, source #2 is same as data source #1

- bit 50: DHI2 - selects low or high 16 bits of second operand [DADD16_R]
- bit 51: DHI - selects low or high 16 bits of an operand [DADD16_I,
  DLOGOP16_I, DADD16_R]
- bits 52-55: DSRC1 - selects data source #1 [DINSRT_R, DINSRT_I,
  DADD16_I, DLOGOP16_I, DSHIFT_R, DSEXT, DADD16_R]. The data source #1 is
  the GPR with index selected by this bitfield.
- bits 33-55: DIMM23 - 23-bit signed immediate [DMOV_I]
- bits 56-59: DRDST - selects data GPR destination register. The GPR
  destination is the GPR with index selected by this bitfield. The data
  result will be written here, along with the special register selected
  by DDST.
- bit 60: DDST - the data special register destination, determines where the
  data result will be written (along with DRDST); one of:

  - 0: DACC
  - 1: DATA

- bits 61-63: DOP - the data operation, one of:

  - 0: DINSRT_R, bitfield insertion with shift, register sources
  - 1: DINSRT_I, bitfield insertion with 6-bit immediate source
  - 2: DMOV_I, 23-bit immediate value load
  - 3: DADD16_I, 16-bit addition with immediate
  - 4: DLOGOP16_I, 16-bit logic operation with immediate
  - 5: DSHIFT_R, shift by the value of a register
  - 6: DSEXT, sign extension
  - 7: DADD16_R, 16-bit addition/substraction with register operands

The data processing path computes three values:

- the data result, ie. the 32-bit value that will be written to the data
  destination registers
- the data predicate result, ie. the 1-bit value that will be written to
  the destination predicate
- the skip special destination flag, a 1-bit flag that disables write
  to the data special register if set

Not all data operations produce a predicate result. For ones that don't, the
command predicate result will be output instead.

The DINSRT_R operation has the following semantics::

    if (DBFEND >= DBFSTART) {
        DBFMASK = (2 << DBFEND) - (1 << DBFSTART); // bits DBFSTART-DBFEND are 1
    } else {
        DBFMASK = 0;
    }
    if (DSHDIR == 0) /* 0 is left shift, 1 is right arithmetic shift */
        shifted_source = data_source_1 << DSHIFT;
    else
        shifted_source = (-1 << 32 | data_source_1) >> DSHIFT;
    data_result = (data_source_2 & ~DBFMASK) | (shifted_source & DBFMASK);
    if (C2DEN)
        data_result = (data_result & ~CBFMASK) | (C2D & CBFMASK);
    data_predicate_result = (shifted_source & DBFMASK) == 0;
    skip_special_destination = false;

The DINSRT_I operation has the following semantics::

    if (DBFEND >= DBFSTART) {
        DBFMASK = (2 << DBFEND) - (1 << DBFSTART); // bits DBFSTART-DBFEND are 1
    } else {
        DBFMASK = 0;
    }
    data_result = (data_source_2 & ~DBFMASK) | (DIMM6 << DBFSTART & DBFMASK);
    if (C2DEN)
        data_result = (data_result & ~CBFMASK) | (C2D & CBFMASK);
    data_predicate_result = command_predicate_result;
    skip_special_destination = false;

The DMOV_I operation has the following semantics::

    data_result = sext(DIMM23, 22); /* sign-extend 23-bit immediate to 32 bits */
    data_predicate_result = command_predicate_result;
    skip_special_destination = false;

The DADD16_I operation has the following semantics::

    sum = ((data_source_1 >> (16 * DHI)) + DIMM16) & 0xffff;
    data_result = (data_source_1 & ~(0xffff << (16 * DHI))) | sum << (16 * DHI);
    data_predicate_result = sum >> 15 & 1;
    skip_special_destionation = DDSTSKIP;

The DLOGOP16_I operation has the following semantics::

    src = (data_source_1 >> (16 * DHI)) & 0xffff;
    switch (DLOGOP) {
        case MOV: res = DIMM16; break;
        case AND: res = src & DIMM16; break;
        case OR: res = src | DIMM16; break;
        case XOR: res = src ^ DIMM16; break;
    }
    data_result = (data_source_1 & ~(0xffff << (16 * DHI))) | res << (16 * DHI);
    data_predicate_result = (res == 0);
    skip_special_destination = false;

The DSHIFT_R operation has the following semantics::

    shift = command_source_1 & 0x1f;
    if (DSHDIR == 0) /* 0 is left shift, 1 is right arithmetic shift */
        data_result = data_source_1 << shift;
    else
        data_result = (-1 << 32 | data_source_1) >> shift;
    data_predicate_result = command_predicate_result;
    skip_special_destination = false;

The DSEXT operation has the following semantics::

    bfstart = max(DBFSTART, DSHIFT);
    if (DBFEND >= bfstart) {
        DBFMASK = (2 << DBFEND) - (1 << bfstart); // bits bfstart-DBFEND are 1
    } else {
        DBFMASK = 0;
    }
    sign = data_source_2 >> DSHIFT & 1;
    data_result = (data_source_2 & ~DBFMASK) | (sign ? DBFMASK : 0);
    if (C2DEN)
        data_result = (data_result & ~CBFMASK) | (C2D & CBFMASK);
    data_predicate_result = sign;
    skip_special_destination = false;

The DADD16_R operation has the following semantics::

    src1 = (data_source_1 >> (16 * DHI)) & 0xffff;
    src2 = (command_source_1 >> (16 * DHI2)) & 0xffff;
    if (DSUB == 0)
        sum = (src1 + src2) & 0xffff;
    else
        sum = (src1 - src2) & 0xffff;
    data_result = (data_source_1 & ~(0xffff << (16 * DHI))) | sum << (16 * DHI);
    data_predicate_result = sum >> 15 & 1;
    skip_special_destionation = false;


Destination write
-----------------

Once both command and data processing is done, the results are written to the
destination registers, as follows:

- command_result is written to command special register selected by CDST.
- data_result is written to data special register selected by DDST, unless
  skip_special_destionation is true.
- data_result is written to GPR selected by DRDST. This can be effectively
  disabled by setting DRDST to $g6.
- data_predicate_result is written to predicate selected by PDST. This can
  be effectively disabled by setting PDST to $p0.
