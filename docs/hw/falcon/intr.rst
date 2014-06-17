==========
Interrupts
==========

.. contents::


Introduction
============

falcon has interrupt support. There are 16 interrupt lines on each engine, and
two interrupt vectors on the microprocessor. Each of the interrupt lines can
be independently routed to one of the microprocessor vectors, or to the PMC
interrupt line, if the engine has one. The lines can be individually masked
as well. They can be triggered by hw events, or by the user.

The lines are:

===== ================ ======== ============
Line  v3+ type         Name     Description
===== ================ ======== ============
0     edge             PERIODIC :ref:`periodic timer <falcon-intr-periodic>`
1     edge             WATCHDOG :ref:`watchdog timer <falcon-intr-watchdog>`
2     level            FIFO     :ref:`FIFO data available <falcon-intr-fifo>`
3     edge             CHSW     :ref:`PFIFO channel switch        <falcon-intr-chsw>`
4     edge             EXIT     :ref:`processor stopped <falcon-intr-exit>`
5     edge             ???      [related to falcon+0x0a4]
6-7   edge             SCRATCH  scratch [unused by hw, user-defined]
8-9   edge by default  \-       engine-specific
10-15 level by default \-       engine-specific
===== ================ ======== ============

.. todo:: figure out interrupt 5

Each interrupt line has a physical wire assigned to it. For edge-triggered
interrupts, there's a flip-flop that's set by 0-to-1 edge on the wire or
a write to INTR_SET register, and cleared by writing to INTR_CLEAR register.
For level-triggered interrupts, interrupt status is wired straight to the
input.


.. _falcon-io-intr:
.. _falcon-io-intr-enable:

Interrupt status and enable registers
=====================================

The interrupt and interrupt enable registers are actually visible as
set/clear/status register triples: writing to the set register sets all bits
that are 1 in the written value to 1. Writing to clear register sets them
to 0. The status register shows the current value when read, but cannot be
written.

MMIO 0x000 / I[0x00000]: INTR_SET
MMIO 0x004 / I[0x00100]: INTR_CLEAR
MMIO 0x008 / I[0x00200]: INTR [status]
    A mask of currently pending interrupts. Write to SET to manually trigger
    an interrupt. Write to CLEAR to ack an interrupt. Attempts to SET or CLEAR
    level-triggered interrupts are ignored.

MMIO 0x010 / I[0x00400]: INTR_EN_SET
MMIO 0x014 / I[0x00500]: INTR_EN_CLEAR
MMIO 0x018 / I[0x00600]: INTR_EN [status]
    A mask of enabled interrupts. If a bit is set to 0 here, the interrupt
    handler isn't run if a given interrupt happens [but the INTR bit is still
    set and it'll run once INTR_EN bit is set again].


.. _falcon-io-intr-mode:

Interrupt mode setup
====================

MMIO 0x00c / I[0x00300]: INTR_MODE [v3+ only]
    Bits 0-15 are modes for the corresponding interrupt lines. 0 is edge
    trigered, 1 is level triggered.

    Setting a sw interrupt to level-triggered, or a hw interrupt to mode it
    wasn't meant to be set is likely a bad idea.

    This register is set to 0xfc04 on reset.

.. todo:: check edge/level distinction on v0


.. _falcon-io-intr-route:

Interrupt routing
=================

MMIO 0x01c / I[0x00700]: INTR_ROUTING
  - bits 0-15: bit 0 of interrupt routing selector, one for each interrupt line
  - bits 16-31: bit 1 of interrupt routing selector, one for each interrupt line

  For each interrupt line, the two bits from respective bitfields are put
  together to find its routing destination:

  - 0: falcon vector 0
  - 1: PMC HOST/DAEMON line
  - 2: falcon vector 1
  - 3: PMC NRHOST line [NVC0+ selected engines only]

If the engine has a PMC interrupt line and any interrupt set for PMC irq
delivery is active and unmasked, the corresponding PMC interrupt input line
is active.


.. _falcon-sr-iv:
.. _falcon-flags-ie:
.. _falcon-flags-is:
.. _falcon-intr:

Interrupt delivery
==================

falcon interrupt delivery is controlled by $iv0, $iv1 registers and ie0, ie1,
is0, is1 $flags bits. $iv0 is address of interrupt vector 0. $iv1 is address
of interrupt vector 1.  ieX are interrupt enable bits for corresponding
vectors. isX are interrupt enable save bits - they store previous status of
ieX bits during interrupt handler execution. Both ieX bits are always cleared
to 0 when entering an interrupt handler.

Whenever there's an active and enabled interrupt set for vector X delivery,
and ieX flag is set, vector X is called::

        $sp -= 4;
        ST(32, $sp, $pc);
        $flags.is0 = $flags.ie0;
        $flags.is1 = $flags.ie1;
        $flags.ie0 = 0;
        $flags.ie1 = 0;
        if (falcon_version >= 4) {
                $flags.unk16 = $flags.unk12;
                $flags.unk1d = $flags.unk1a;
                $flags.unk12 = 0;
        }
        if (vector 0)
                $pc = $iv0;
        else
                $pc = $iv1;


.. _falcon-sr-tv:
.. _falcon-sr-tstatus:
.. _falcon-flags-ta:
.. _falcon-trap:

Trap delivery
=============

falcon trap delivery is controlled by $tv, $tstatus registers and ta $flags
bit. Traps behave like interrupts, but are triggered by events inside the UC.

$tv is address of trap vector. ta is trap active flag. $tstatus is present on
v3+ only and contains information about last trap. The bitfields of $tstatus
are:

- bits 0-19 [or as many bits as required]: faulting $pc
- bits 20-23: trap reason

The known trap reasons are:

====== ============== ============
Reason Name           Description
====== ============== ============
0-3    SOFTWARE       :ref:`software trap <falcon-trap-software>`
8      INVALID_OPCODE :ref:`invalid opcode <falcon-trap-invalid-opcode>`
0xa    VM_NO_HIT      :ref:`page fault - no hit <falcon-trap-vm>`
0xb    VM_MULTI_HIT   :ref:`page fault - multi hit <falcon-trap-vm>`
0xf    BREAKPOINT     :ref:`breakpoint hit <falcon-trap-breakpoint>`
====== ============== ============

Whenever a trapworthy event happens on the uc, a trap is delivered::

        if ($flags.ta) { // double trap?
                EXIT;
        }
        $flags.ta = 1;
        if (falcon_version != 0) // on v0, there's only one possible trap reason anyway [8]
                $tstatus = $pc | reason << 20;
        if (falcon_version >= 4) {
                $flags.is0 = $flags.ie0;
                $flags.is1 = $flags.ie1;
                $flags.unk16 = $flags.unk12;
                $flags.unk1d = $flags.unk1a;
                $flags.ie0 = 0;
                $flags.ie1 = 0;
                $flags.unk12 = 0;
        }
        $sp -= 4;
        ST(32, $sp, $pc);
        $pc = $tv;

.. todo:: didn't ieX -> isX happen before v4?


.. _falcon-isa-iret:

Returning form an interrupt: iret
=================================

Returns from an interrupt handler.

Instructions:
    ==== ======================== =========
    Name Description              Subopcode
    ==== ======================== =========
    iret Return from an interrupt 1
    ==== ======================== =========
Instruction class:
    unsized
Operands:
    [none]
Forms:
    ============= ======
    Form          Opcode
    ============= ======
    [no operands] f8
    ============= ======
Operation:
    ::

        $pc = LD(32, $sp);
        $sp += 4;
        $flags.ie0 = $flags.is0;
        $flags.ie1 = $flags.is1;
        if (falcon_version >= 4) {
                $flags.unk12 = $flags.unk16;
                $flags.unk1a = $flags.unk1d;
        }


.. _falcon-isa-trap:
.. _falcon-trap-software:

Software trap trigger: trap
===========================

Triggers a software trap.

Instructions:
    ====== ======================== ========== =========
    Name   Description              Present on Subopcode
    ====== ======================== ========== =========
    trap 0 software trap #0         v3+ units  8
    trap 1 software trap #1         v3+ units  9
    trap 2 software trap #2         v3+ units  a
    trap 3 software trap #3         v3+ units  b
    ====== ======================== ========== =========
Instruction class:
    unsized
Operands:
    [none]
Forms:
    ============= ======
    Form          Opcode
    ============= ======
    [no operands] f8
    ============= ======
Operation:
    ::

        $pc += oplen; // return will be to the insn after this one
        TRAP(arg);
