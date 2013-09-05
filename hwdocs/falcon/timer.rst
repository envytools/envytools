.. _falcon-timer:

======
Timers
======

.. contents::


Introduction
============

Time and timer-related registers are the same on all falcon engines, except
PGRAPH CTXCTLs which lack PTIMER access.

You can:

- Read PTIMER's clock
- Use a periodic timer: Generate an interrupt periodically
- Use a watchdog/one-shot timer: Generate an interrupt once in the future

Also note that the CTXCTLs have another watchdog timer on their own - see
`<../graph/nvc0-ctxctl/intro.txt>`_ for more information.


.. _falcon-intr-periodic:
.. _falcon-io-periodic:

Periodic timer
==============

All falcon engines have a periodic timer. This timer generates periodic interrupts
on interrupt line. The registers controlling this timer are:

MMIO 0x020 / I[0x00800]: PERIODIC_PERIOD
    A 32-bit register defining the period of the periodic timer, minus 1.
MMIO 0x024 / I[0x00900]: PERIODIC_TIME
    A 32-bit counter storing the time remaining before the tick.
MMIO 0x028 / I[0x00a00]: PERIODIC_ENABLE
    bit 0: Enable the periodic timer. If 0, the counter doesn't change and no
    interrupts are generated.

When the counter is enabled, PERIODIC_TIME decreases by 1 every clock cycle.
When PERIODIC_TIME reaches 0, an interrupt is generated on line 0 and the
counter is reset to PERIODIC_PERIOD.

Operation (after each falcon core clock tick)::

	if (PERIODIC_ENABLE) {
		if (PERIODIC_TIME == 0) {
			PERIODIC_TIME = PERIODIC_PERIOD;
			intr_line[0] = 1;
		} else {
			PERIODIC_TIME--;
			intr_line[0] = 0;
		}
	} else {
		intr_line[0] = 0;
	}


.. _falcon-io-ptimer:

= PTIMER access =

The falcon engines other than PGRAPH's CTXCTLs have PTIMER's time registers
aliased into their IO space. aliases are:

MMIO 0x02c / I[0x00b00]: TIME_LOW
    Alias of PTIMER's TIME_LOW register [MMIO 0x9400]
MMIO 0x030 / I[0x00c00]: TIME_HIGH
    Alias of PTIMER's TIME_HIGH register [MMIO 0x9410]

Both of these registers are read-only. See :ref:`ptimer` for more information
about PTIMER.


.. _falcon-intr-watchdog:
.. _falcon-io-watchdog:

Watchdog timer
==============

Apart from a periodic timer, the falcon engines also have an independent one-shot
timer, also called watchdog timer. It can be used to set up a single interrupt
in near future. The registers are:

MMIO 0x034 / I[0x00d00]: WATCHDOG_TIME
    A 32-bit counter storing the time remaining before the interrupt.
MMIO 0x038 / I[0x00e00]: WATCHDOG_ENABLE
    bit 0: Enable the watchdog timer. If 0, the counter doesn't change and no
    interrupts are generated.

A classic use of a watchdog is to set it before calling a sensitive function by
initializing it to, for instance, twice the usual time needed by this function to
be executed.

In falcon's case, the watchdog doesn't reboot the µc. Indeed, it is very similar to
the periodic timer. The differences are:

- it generates an interrupt on line 1 instead of 0.
- it needs to be reset manually

Operation (after each falcon core clock tick)::

	if (WATCHDOG_ENABLE) {
		if (WATCHDOG_TIME == 0) {
			intr_line[1] = 1;
		} else {
			WATCHDOG_TIME--;
			intr_line[1] = 0;
		}
	} else {
		intr_line[1] = 0;
	}
