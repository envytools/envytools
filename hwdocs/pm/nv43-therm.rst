.. _nv43-therm:

============================
NV43:NV50 thermal monitoring
============================

.. contents::


Introduction
============

THERM is an area present in PBUS on NV43:NV50 chipsets. This area is reponsible
for temperature monitoring, probably on low-end and middle-range GPUs since
high-end cards have been using LM89/ADT7473 for a long time.
Beside some configuration knobs, THERM can generate IRQs to the HOST when the
temperature goes over a configurable ALARM threshold or outside a configurable
temperature range. This range has been replaced by PTHERM on nv50+ chipsets.

THERM's MMIO range is 0x15b0:0x15c0. There are two major variants of this range:

- NV43:NV47
- NV47:NV50


.. _nv43-therm-mmio:

MMIO register list
==================

======== =========== ========== ============
Address  Present on  Name       Description
======== =========== ========== ============
0x0015b0 all         CFG0       sensor enable / IRQ enable / ALARM configuration
0x0015b4 all         STATUS     sensor state / ALARM state / ADC rate configuration
0x0015b8 non-IGP     CFG1       misc. configuration
0x0015bc all         TEMP_RANGE LOW and HIGH temperature thresholds
======== =========== ========== ============

MMIO 0x15b0: CFG0 [NV43:NV47]
  - bits 0-7: ALARM_HIGH
  - bits 16-23: SENSOR_OFFSET (signed integer)
  - bit 24: DISABLE
  - bit 28: ALARM_INTR_EN

MMIO 0x15b0: CFG0 [NV47:NV50]
  - bits 0-13: ALARM_HIGH
  - bits 16-29: SENSOR_OFFSET (signed integer)
  - bit 30: DISABLE
  - bit 31: ENABLE

MMIO 0x15b4: STATUS [NV43:NV47]
  - bits 0-7: SENSOR_RAW
  - bit 8: ALARM_HIGH
  - bits 25-31: ADC_CLOCK_XXX

.. todo:: figure out what divisors are available

MMIO 0x15b4: STATUS [NV47:NV50]
  - bits 0-13: SENSOR_RAW
  - bit 16: ALARM_HIGH
  - bits 26-31: ADC_CLOCK_DIV
    The division is stored right-shifted 4. The possible division values range
    from 32 to 2016 with the possibility to completely bypass the divider.

MMIO 0x15b8: CFG1 [NV43:NV47]
  - bit 17: ADC_PAUSE
  - bit 23: CONNECT_SENSOR

MMIO 0x15bc: TEMP_RANGE [NV43:NV47]
  - bits 0-7: LOW
  - bits 8-15: HIGH

MMIO 0x15bc: TEMP_RANGE [NV47:NV50]
  - bits 0-13: LOW
  - bits 16-29: HIGH


The ADC clock
=============

The source clock for THERM's ADC is:

- NV43:NV47: the host clock
- NV47:NV50: constant (most likely hclck)

(most likely, since the rate doesn't change when I change the HOST clock)

Before reaching the ADC, the clock source is divided by a fixed divider of 1024
and then by ADC_CLOCK_DIV.

MMIO 0x15b4: STATUS [NV43:NV47]
  - bits 25-31: ADC_CLOCK_DIV

.. todo:: figure out what divisors are available

MMIO 0x15b4: STATUS [NV47:NV50]
  - bits 26-31: ADC_CLOCK_DIV
    The division is stored right-shifted 4. The possible division values range
    from 32 to 2016 with the possibility to completely bypass the divider.

The final ADC clock is:

ADC_clock = source_clock / ADC_CLOCK_DIV

The accuracy of the reading greatly depends on the ADC clock. A clock too fast
will produce a lot of noise. A clock too low may actually produce an offseted
value. The ADC clock rate under 10 kHz is advised, based on limited testing
on a nv4b.

.. todo:: Make sure this clock range is safe on all cards

Anyway, it seems like it is clocked at an acceptable frequency at boot time,
so, no need to worry too much about it.


Reading temperature
===================

Temperature is read from:

MMIO 0x15b4: STATUS [NV43:NV47]
  bits 0-7: SENSOR_RAW
MMIO 0x15b4: STATUS [NV47:NV50]
  bits 0-13: SENSOR_RAW

SENSOR_RAW is the result of the (signed) addition of the actual value read by
the ADC and SENSOR_OFFSET:

MMIO 0x15b0: CFG0 [NV43:NV47]
  - bits 16-23: SENSOR_OFFSET signed

MMIO 0x15b0: CFG0 [NV47:NV50]
  - bits 16-29: SENSOR_OFFSET signed

Starting temperature readouts requires to flip a few switches that are
chipset-dependent:

MMIO 0x15b0: CFG0 [NV43:NV47]
  - bit 24: DISABLE

MMIO 0x15b0: CFG0 [NV47:NV50]
  - bit 30: DISABLE - mutually exclusive with ENABLE
  - bit 31: ENABLE - mutually exclusive with DISABLE

MMIO 0x15b8: CFG1 [NV43:NV47]
  - bit 17: ADC_PAUSE
  - bit 23: CONNECT_SENSOR

Both DISABLE and ADC_PAUSE should be clear. ENABLE and CONNECT_SENSOR should be set.

.. todo:: There may be other switches.


Setting up thresholds and interrupts
====================================


.. _nv43-therm-intr-alarm:

Alarm
-----

THERM features the ability to set up an alarm that will trigger interrupt
PBUS #16 when SENSOR_RAW > ALARM_HIGH. NV43-47 chipsets require ALARM_INTR_EN
to be set in order to get the IRQ. You may need to set bits 0x40001 in 0x15a0
and 1 in 0x15a4. Their purpose has not been understood yet even though they
may be releated to automatic downclocking.

MMIO 0x15b0: CFG0 [NV43:NV47]
  - bits 0-7: ALARM_HIGH
  - bit 28: ALARM_INTR_EN

MMIO 0x15b0: CFG0 [NV47:NV50]
  - bits 0-13: ALARM_HIGH

When SENSOR_RAW > ALARM_HIGH, STATUS.ALARM_HIGH is set.

MMIO 0x15b4: STATUS [NV43:NV47]
  - bit 8: ALARM_HIGH

MMIO 0x15b4: STATUS [NV47:NV50]
  - bit 16: ALARM_HIGH

STATUS.ALARM_HIGH is unset as soon as SENSOR_RAW < ALARM_HIGH, without any
hysteresis cycle.


.. _nv43-therm-intr-range:

Temperature range
-----------------

THERM can check that temperature is inside a range. When the temperature goes
outside this range, an interrupt is sent. The range is defined in the register
TEMP_RANGE where the thresholds LOW and HIGH are set.

MMIO 0x15bc: TEMP_RANGE [NV43:NV47]
  - bits 0-7: LOW
  - bits 8-15: HIGH

MMIO 0x15bc: TEMP_RANGE [NV47:NV50]
  - bits 0-13: LOW
  - bits 16-29: HIGH

When SENSOR_RAW < TEMP_RANGE.LOW, interrupt PBUS #17 is sent.
When SENSOR_RAW > TEMP_RANGE.HIGH, interrupt PBUS #18 is sent.

There are no hyteresis cycles on these thresholds.


Extended configuration
======================

.. todo:: Document reg 15b8
