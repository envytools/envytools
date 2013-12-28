.. _pdaemon-counter:
.. _pdaemon-io-counter:

=============
Idle counters
=============

.. contents::

Introduction
============

PDAEMON's role is mostly about power management. One of the most effective way
of lowering the power consumption is to lower the voltage at which the processor
is powered at. Lowering the voltage is also likely to require lowering the
clocks of the engines powered by this power domain. Lowering the clocks
lowers the performance which means it can only be done to engines that are
under-utilized. This technique is called Dynamic Voltage/Frequency
Scaling (DVFS) and requires being able to read the activity-level/business of
the engines clocked with every clock domains.

PDAEMON could use PCOUNTER to read the business of the engines it needs to
reclock but that would be a waste of counters. Indeed, contrarily to PCOUNTER
that needs to be able to count events, the business of an engine can be polled
at any frequency depending on the level of accuracy wanted. Moreover, doing the
configuration of PCOUNTER both in the host driver and in PDAEMON would likely
require some un-wanted synchronization.

This is most likely why some counters were added to PDAEMON. Those counters are
polling idle signals coming from the monitored engines. A signal is a binary
value that equals 1 when the associated engine is idle, and 0 if it is active.

.. todo:: check the frequency at which PDAEMON is polling

MMIO Registers
==============

On NVA3-NVC0, there were 4 counters while on NVC0+, there are 8 of them. Each
counter is composed of 3 registers, the mask, the mode and the actual count.
There are two counting modes, the first one is to increment the counter every
time every bit of COUNTER_SIGNALS selected by the mask are set. The other mode
only increments when all the selected bits are cleared. It is possible to
set both modes at the same time which results in incrementing at every clock
cycle. This mode is interesting because it allows dedicating a counter to
time-keeping which eases translating the other counters' values to an idling
percentage. This allows for aperiodical polling on the counters without
needing to store the last polling time.

The counters are not double-buffered and are independent. This means every
counters need to be read then reset at roughly the same time if synchronization
between the counters is required. Resetting the counter is done by setting
bit 31 of COUNTER_COUNT.

MMIO 0x500 / I[0x14000]: COUNTER_SIGNALS
  Read-only. Bitfield with each bit indicating the instantenous state of the
  associated engines/blocks. When the bit is set, the engine/block is idle,
  when it is cleared, the engine/block is active.
  - bit 0: GR_IDLE
  - bit 4: PVLD_IDLE
  - bit 5: PVDEC_IDLE
  - bit 6: PPPP_IDLE
  - bit 7: MC_IDLE [NVC0-]
  - bit 8: MC_IDLE [NVA3-NVC0]
  - bit 19: PCOPY0_IDLE
  - bit 20: PCOPY1_IDLE [NVC0-]
  - bit 21: PCOPY2_IDLE [NVE4-]

MMIO 0x504+i*10 / I[0x14100+i*0x400]: COUNTER_MASK
  The mask that will be applied on COUNTER_SIGNALS before applying the logic
  set by COUNTER_MODE.

MMIO 0x508+i*10 / I[0x14100+i*0x400]: COUNTER_COUNT
  - bit 0-30: COUNT
  - bit 31: CLEAR_TRIGGER : Write-only, resets the counter.

MMIO 0x50c+i*10 / I[0x14300+i*0x400]: COUNTER_MODE
  - bit 0: INCR_IF_ALL : Increment the counter if all the masked bits are set
  - bit 1: INCR_IF_NOT_ALL : Increment the counter if all the masked bits are
			     cleared
  - bit 2: UNK2 [NVD9-]
