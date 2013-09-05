.. _falcon-proc:

=================
Processor control
=================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


Execution state
===============

The falcon processor can be in one of three states:

- RUNNING: processor is actively executing instructions
- STOPPED: no instructions are being executed, interrupts are ignored
- SLEEPING: no instructions are being executed, but interrupts can restart
  execution

The state can be changed as follows:

======== ======== ==================
From     To       Cause             
======== ======== ==================
any      STOPPED  Reset [non-crypto]
any      RUNNING  Reset [crypto]    
STOPPED  RUNNING  :ref:`Start by UC_CTRL <falcon-io-uc-ctrl>`
RUNNING  STOPPED  :ref:`Exit instruction <falcon-isa-exit>`
RUNNING  STOPPED  :ref:`Double trap <falcon-trap>`
RUNNING  SLEEPING :ref:`Sleep instruction <falcon-isa-sleep>`
SLEEPING RUNNING  :ref:`Interrupt <falcon-intr>`
======== ======== ==================


.. _falcon-intr-exit:

The EXIT interrupt
------------------

Whenever falcon execution state is changed to STOPPED for any reason other
than reset (exit instruction, double trap, or the crypto reset scrubber
finishing), falcon interrupt line 4 is active for one cycle (triggering
the EXIT interrupt if it's set to level mode).


.. _falcon-isa-exit:

Halting microcode execution: exit
---------------------------------

Halts microcode execution, raises EXIT interrupt.

Instructions:
    ==== ======================== =========
    Name Description              Subopcode
    ==== ======================== =========
    exit Halt microcode execution 2
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

        EXIT;


.. _falcon-isa-sleep:

Waiting for interrupts: sleep
-----------------------------

If the :ref:`$flags <falcon-sr-flags>` bit given as argument is set, puts
the microprocessor in sleep state until an unmasked interrupt is received.
Otherwise, is a nop. If interrupted, return pointer will point to start of
the sleep instruction, restarting it if the $flags bit hasn't been cleared.

Instructions:
    ===== ======================== =========
    Name  Description              Subopcode
    ===== ======================== =========
    sleep Wait for interrupts      28
    ===== ======================== =========
Instruction class:
    unsized
Operands:
    FLAG
Forms:
    ============= ======
    Form          Opcode
    ============= ======
    I8            f4
    ============= ======
Operation:
    ::

	if ($flags & 1 << FLAG)
		state = SLEEPING;


.. _falcon-io-uc-ctrl:

Processor execution control registers
-------------------------------------

.. todo:: write me


.. _falcon-isa-movsr:

Accessing special registers: mov
================================

.. todo:: write me


.. _falcon-io-uc-caps:

Processor capability readout
============================

.. todo:: write me
