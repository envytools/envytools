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

.. todo:: more info


MMIO registers
==============

.. space:: 8 pfuse 0x1000 efuses storing GPU options
   0x144 TPC_DISABLE_MASK pfuse-tpc-disable-mask G80:GF100
   0x148 PART_DISABLE_MASK pfuse-part-disable-mask G80:GF100
   0x1a0 TEMP_CAL_SLOPE_MUL_OFFSET pfuse-temp-cal-slope-mul-offset
   0x1a4 TEMP_CAL_OFFSET_MUL_OFFSET pfuse-temp-cal-offset-mul-offset
   0x1a8 TEMP_CAL_OK pfuse-temp-cal-ok

   .. todo:: fill me

.. reg:: 32 pfuse-tpc-disable-mask TPC disable mask

   The TPC disable mask.

.. reg:: 32 pfuse-part-disable-mask PART disable mask

   The PART disable mask.

.. reg:: 32 pfuse-temp-cal-slope-mul-offset slope calibration

   An offset added to the slope calibration value of the internal temperature
   sensor (int8_t). :ref:`See PTHERM for more information <ptherm>`

.. reg:: 32 pfuse-temp-cal-offset-mul-offset slope calibration

   An offset added to the offset calibration value of the internal temperature
   sensor (int16_t). :ref:`See PTHERM for more information <ptherm>`

.. reg:: 32 pfuse-temp-cal-ok internal thermal sensor enable

   Should the internal temperature sensor be used? Set to 1 if so, 0 otherwise.
