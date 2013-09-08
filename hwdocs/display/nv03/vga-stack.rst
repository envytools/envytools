.. _vga-stack:

=========
VGA stack
=========

.. contents::


Introduction
============

A dedicated RAM made of 0x200 8-bit cells arranged into a hw stack. NFI what
it is for, apparently related to VGA. Present on NV41+ cards.


.. _pbus-mmio-vga-stack:
.. _pdisplay-mmio-vga-stack:
.. _nv03-cr-vga-stack:
.. _nv50-cr-vga-stack:

MMIO registers
==============

On NV41:NV50, the registers are located in PBUS area:

- 001380 VAL
- 001384 CTRL
- 001388 CONFIG
- 00138c SP

They are also aliased in the VGA CRTC register space:

- CR90 VAL
- CR91 CTRL

On NV50+, the registers are located in PDISPLAY.VGA area:

- 619e40 VAL
- 619e44 CTRL
- 619e48 CONFIG
- 619e4c SP

And aliased in VGA CRTC register space, but in a different place:

- CRA2 VAL
- CRA3 CTRL


Description
===========

The stack is made of the following data:

- an array of 0x200 bytes [the actual stack]
- a write shadow byte, WVAL [NV50+ only]
- a read shadow byte, RVAL [NV50+ only]
- a 10-bit stack pointer [SP]
- 3 config bits:
  - push mode: auto or manual
  - pop mode: auto or manual
  - manual pop mode: read before pop or read after pop
- 2 sticky error bits:
  - stack underflow
  - stack overflow

The stack grows upwards. The stack pointer points to the cell that would be
written by a push. The valid values for stack pointer are thus 0-0x200, with
0 corresponding to an empty stack and 0x200 to a full stack. If stack is ever
accessed at position >= 0x200 [which is usually an error], the address wraps
modulo 0x200.

There are two major modes the stack can be operated in: auto mode and manual
mode. The mode settings are independent for push and pop accesses - one can
use automatic pushes and manual pops, for example. In automatic mode, the
read/write access to the VAL register automatically performs the push/pop
operation. In manual mode, the push/pop needs to be manually triggered in
addition to accessing the VAL reg. For manual pushes, the push should be
triggered after writing the value. For pops, the pop should be triggered
before or after reading the value, depending on selected manual pop mode.

The stack also keeps track of overflow and underflow errors. On NV41:NV50,
while these error conditions are detected, the offending access is still
executed [and the stack pointer wraps]. On NV50+, the offending access is
discarded. The error status is sticky. On NV41:NV50, it can only be cleared
by poking the CONFIG register clear bits. On NV50+, the overflow status
is cleared by executing a pop, and the underflow status is cleared by
executing a push.


Stack access registers
======================

The stack data is read or written through the VAL register:

MMIO 0x001380 / CR 0x90: VAL [NV41:NV50]
MMIO 0x619e40 / CR 0xa2: VAL [NV50-]
  Accesses a stack entry. A write to this register stored the low 8 bits
  of written data as a byte to be pushed. If automatic push mode is set,
  the value is pushed immediately. Otherwise, it is pushed after PUSH_TRIGGER
  is set. A read from this register returns popped data [causing a pop in
  the process if automatic pop mode is set]. If manual read-before-pop mode
  is in use, the returned byte is the byte that the next POP_TRIGGER would
  pop. In manual pop-before-read, it is the byte that the last POP_TRIGGER
  popped.

The CTRL register is used to manually push/pop the stack and check its status:

MMIO 0x001384 / CR 0x91: CTRL [NV41:NV50]
MMIO 0x619e44 / CR 0xa3: CTRL [NV50-]
  - bit 0: PUSH_TRIGGER - when written as 1, executes a push. Always reads as 0.
  - bit 1: POP_TRIGGER - like above, for pop.
  - bit 4: EMPTY - read-only, reads as 1 when SP == 0.
  - bit 5: FULL - read-only, reads as 1 when SP >= 0x200.
  - bit 6: OVERFLOW - read-only, the sticky overflow error bit
  - bit 7: UNDERFLOW - read-only, the sticky underflow error bit


= Stack configuration registers =

To configure the stack, the CONFIG register is used:

MMIO 0x001388: CONFIG [NV41:NV50]
MMIO 0x619e48: CONFIG [NV50-]
  - bit 0: PUSH_MODE - selects push mode [see above]

    - 0: MANUAL
    - 1: AUTO

  - bit 1: POP_MODE - selects pop mode [see above]

    - 0: MANUAL
    - 1: AUTO

  - bit 2: MANUAL_POP_MODE - for manual pop mode, selects manual pop submode.
    Unused for auto pop mode.

    - 0: POP_READ - pop before read
    - 1: READ_POP - read before pop

  - bit 6: OVERFLOW_CLEAR [NV41:NV50] - when written as 1, clears CTRL.OVERFLOW
    to 0. Always reads as 0.
  - bit 7: UNDERFLOW_CLEAR [NV41:NV50] - like above, for CTRL.UNDERFLOW

The stack pointer can be accessed directly by the SP register:

MMIO 0x00138c: SP [NV41:NV50]
MMIO 0x619e4c: SP [NV50-]
  The stack pointer. Only low 10 bits are valid.


Internal operation
==================

NV41:NV50 VAL write::

	if (SP >= 0x200)
		CTRL.OVERFLOW = 1;
	STACK[SP] = val;
	if (CONFIG.PUSH_MODE == AUTO)
		PUSH();

NV41:NV50 PUSH::

	SP++;

NV41:NV50 VAL read::

	if (SP == 0)
		CTRL.UNDERFLOW = 1;
	if (CONFIG.POP_MODE == AUTO) {
		POP();
		res = STACK[SP];
	} else {
		if (CONFIG.MANUAL_POP_MODE == POP_READ)
			res = STACK[SP];
		else
			res = STACK[SP-1];
	}

NV41:NV50 POP::

	SP--;

NV50+ VAL write::

	WVAL = val;
	if (CONFIG.PUSH_MODE == AUTO)
		PUSH();

NV50+ PUSH::

	if (SP >= 0x200)
		CTRL.OVERFLOW = 1;
	else
		STACK[SP++] = WVAL;
	CTRL.UNDERFLOW = 0;

NV50+ VAL read::

	if (CONFIG.POP_MODE == AUTO) {
		POP();
		res = RVAL;
	} else {
		if (CONFIG.MANUAL_POP_MODE == POP_READ || SP == 0)
			res = RVAL;
		else
			res = STACK[SP-1];
	}

NV50+ POP::

	if (SP == 0)
		CTRL.UNDERFLOW = 1;
	else
		RVAL = STACK[--SP];
	CTRL.OVERFLOW = 0;
