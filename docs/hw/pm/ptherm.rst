.. _ptherm:

================================================
NV50+'s Thermal and power monitoring and capping
================================================

.. contents::

Introduction
============

On modern hardware, the thermal dissipation of a GPU can be high-enough to
damage itself. The role of PTHERM is to monitor the temperature nd react to
overheating by warning the host through an IRQ and/or by automatically lowering
the clocks of PGRAPH.

Clock rate modulation
=====================

PTHERM has the ability to reduce PGRAPH's clock when it sees fit. This is done
by alternating PGRAPH's clock between the original clock and a divided one.
It is also possible to modulate the ratio of time spent on the original and the
divided clock. This ratio is a 8 bit value where 0 means always use the divided
clock and 0xff means always use the original clock.

However, the exact frequency versus ratio isn't linear (although it could be a
simple model for it). Here is the actual function when using the greatest
divider:

  y = 2.399e7 + 3.492e5*x + 870.6*x^3 + 0.03008*x^5 - 4.159e-5*x^6 -
  7.851*x^4 - 2.989e4*x^2

In average, this allows generating any clock between the original clock and the
clock divided by the greatest divider (16).

This clock rate modulation mechanism has been named FSMR (Frequency-Switching
Modulation Rate) for the lack of a better name. The FSRM can be triggered by
different temperature and power thresholds. Each thresholds can usually
configure the ratio and the divider with different values to stay in the
temperature budget while reducing performance as little as possible.

The state of the FSRM is exposed

The power capping mechanism can adjust the ratio on the fly and will be
further explained later on.

Temperature reading
===================

The temperature can either be read from the internal sensor or an external
sensor (LM89/LM99/ADT7475/...).

External sensors are exposed to PTHERM through three GPIOs indicating different
temperature levels. The meaning of each temperature-level/GPIO should be read
from the GPIO table in the VBIOS. Usually, only two temperature thresholds are
exposed, ALARM and ALERT. The other one is reserved for a power-related issue.

NV50
----

On NV50, temperature monitoring is very rudimentary and inspired from earlier
designs. The internal temperature exposed is the voltage at the pins of the
internal diode. Conversion to a temperature in degrees Celsius should be done
by the host driver.

NV84+
----

On NV84+ GPUs, the internal sensor has a higher resolution as it returns the
temperature with a 0.5°C accuracy. It is factory-calibrated but it can be
overridden by the software. The calibration is composed of an offset and a slope:

  temp = raw_temp * slope + offset

The raw temperature is the voltage read by the Analog-to-Digital Converter (ADC)
at the pins of the internal diode. The resolution of the ADC is 15 bits and its
clock comes from the card's crystal (usually 27 MHz) and can then be divided by
ADC_CLOCK_DIV. The default value seems to be a safe one to use.

.. todo:: check the possible dividers

The temperature calibration values have a fixed precision and are represented
like that with slope_div fixed to 16384 and offset_div fixed to 2:
  temp = raw_temp * (slope / slope_div) + (offset / offset_div)

The base calibration values are stored in SENSOR_CALIB_0. Those values aren't
suitable for all the cards and require a factory-set offset that is stored in
PFUSE. The final hardware calibration value can be read in SENSOR_HW_CALIB_0.

It is possible to override the hardware calibration by setting bits 0 and 1
of SENSOR_CALIB_0 and by setting the wanted calibation values in
SENSOR_SW_CALIB.

The calibrated value, expressed in degrees Celsius, are exposed by TEMP_HIGH
for the integer part of the temperature, and by TEMP_LOW for the .5°C precision.
Reading TEMP_HIGH freezes TEMP_LOW so as an atomic read of the temperature is
possible.

Finally, on nv94+, it is possible to force a temperature to any calibrated
temperature wanted, with a precision of 1°C. This is useful for checking
PTHERM's behaviour on different temperature scenarios.

Temperature management
======================

NV50
----

On NV50, temperature management is again very rudimentary. It allows specifying
3 temperature thresholds. Critical, High and Low.

An activation delay may be set on thresholds to prevent them from oscillating
between the active and inactive state. The state will be considered active if
the temperature stays above (or below, if reversed) for longer than a delay.
The delay is configured as a number of clock cycles to wait (up to 127). It is
possible to divide the clock by 16, 256, 4096 or 65536. The delay time is given
by the following program:

  double clock_hz = (1000000.0 / (16.0 * pow(16, div)));
  double time_delay = (1.0 / clock_hz) * cycles * 0x7f * 1e9;

For each of these thresholds, it is possible to require IRQs to be sent to the
host when the temperature reaches any of the thresholds. It is also possible
to specify if we want the IRQ when the temperature rises past the threshold,
falls bellow it or both.

It is also possible to specify use the FSRM when reaching a temperature
threshold. However, only the divisor can be changed depending on the threshold
as all the temperature-related thresholds need to share the same FSRM ratio.

.. todo:: verify the priorities of each threshold (if two thresholds are active
at the same time, which one is considered as being active?)

NV84+
-----

.. todo:: write me

Power reading
=============

.. todo:: write me

Power management
================
.. todo:: write me


MMIO registers
==============

.. space:: 8 ptherm 0x800 thermal sensor

   .. todo:: write me


.. _ptherm-intr:

Interrupts
==========

.. todo:: write me
