.. _pfuse:

==========================
PFUSE: Configuration fuses
==========================

.. contents::

Introduction
============

PFUSE contains factory-set configuration values that cannot be overridden by
software later on. The name comes from the fact that once the Configuration
has been set, it is possible to avoid further writes by blowing the power supply
or the data lines of the the write circuitry.

.. _pfuse-mmio:

MMIO registers
==============

PFUSE's MMIO range is 0x021000-0x022000.

MMIO 0x21144: TPC_DISABLE_MASK
  The TPC disable mask.

MMIO 0x21148: PART_DISABLE_MASK
  The PART disable mask.

MMIO 0x211a0: TEMP_CAL_SLOPE_MUL_OFFSET
  An offset added to the slope calibration value of the internal temperature
  sensor (int8_t). :ref:`See PTHERM for more information <ptherm>`

MMIO 0x211a4: TEMP_CAL_OFFSET_MUL_OFFSET
  An offset added to the offset calibration value of the internal temperature
  sensor (int16_t). :ref:`See PTHERM for more information <ptherm>`

MMIO 0x211a8: TEMP_CAL_OK
  Should the internal temperature sensor be used? Set to 1 if so, 0 otherwise.
