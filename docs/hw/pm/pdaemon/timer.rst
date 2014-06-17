.. _pdaemon-timer:
.. _pdaemon-intr-timer:
.. _pdaemon-io-timer:
.. _pdaemon-io-timer-intr:

=========
The timer
=========

Aside from the usual :ref:`falcon timers <falcon-timer>`, PDAEMON has its own
timer. The timer can be configured as either one-shot or periodic, can run
on either daemon clock or PTIMER clock divided by 64, and generates
interrupts. The following registers deal with the timer:

MMIO 0x4e0 / I[0x13800]: TIMER_START
  The 32-bit count the timer starts counting down from. Read/write. For
  periodic mode, the period will be equal to TIMER_START+1 source cycles.

MMIO 0x4e4 / I[0x13900]: TIMER_TIME
  The current value of the timer, read only. If TIMER_CONTROL.RUNNING is set,
  this will decrease by 1 on every rising edge of the source clock. If such
  rising edge causes this register to become 0, the TIMER_INTR bit 8 [TIMER]
  is set. The behavior of rising edge when this register is already 0 depends
  on the timer mode: in ONESHOT mode, nothing will happen. In PERIODIC mode,
  the timer will be reset to the value from TIMER_START. Note that interrupts
  won't be generated if the timer becomes 0 when copying the value from
  TIMER_START, whether caused by starting the timer or beginning a new PERIODIC
  period. This means that using PERIODIC mode with TIMER_START of 0 will never
  generate any interrupts.

MMIO 0x4e8 / I[0x13a00]: TIMER_CTRL
  - bit 0: RUNNING - when 0, the timer is stopped, when 1, the timer is runinng.
    Setting this bit to 1 when it was previously 0 will also copy the
    TIMER_START value to TIMER_TIME.

  - bit 4: SOURCE - selects the source clock

    - 0: DCLK - daemon clock, effectively timer decrements by 1 on every daemon
      cycle
    - 1: PTIMER_B5 - PTIMER time bit 5 [ie. bit 10 of TIME_LOW]. Since timer
      decrements by 1 on every rising edge of the clock, this effectively
      decrements the counter on every 64th PTIMER clock.

  - bit 8: MODE - selects the timer mode

    - 0: ONESHOT - timer will halt after reaching 0
    - 1: PERIODIC - timer will restart from TIMER_START after reaching 0

MMIO 0x680 / I[0x1a000]: TIMER_INTR
  - bit 8: TIMER - set whenever TIMER_TIME becomes 0 except by a copy from
    TIMER_START, write 1 to this bit to clear it. When this and bit 8
    of TIMER_INTR_EN are set at the same time, falcon interrupt line #14 [TIMER]
    is asserted.

MMIO 0x684 / I[0x1a100]: TIMER_INTR_EN
  - bit 8: TIMER - when set, timer interupt delivery to falcon interrupt line 14 is
    enabled.
