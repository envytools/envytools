.. _nv50-gpio:

====================
NV50:NVD9 GPIO lines
====================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


.. _nv50-gpio-intr:

Interrupts
==========

.. todo:: write me


NV50 GPIO NVIO specials
=======================

This list applies to NV50.

===== ========== =======
Line  Output     Input
===== ========== =======
0     PWM_0
----- ---------- -------
1     \-
----- ---------- -------
2     \-
----- ---------- -------
3     tag 0x42?
----- ------------------
4     SLI_SENSE_0?
----- ------------------
5     \-
----- ---------- -------
6     \-
----- ---------- -------
7     \-         PTHERM_INPUT_0
----- ---------- -------
8     \-         PTHERM_INPUT_2
----- ---------- -------
9     related to e1bc and PTHERM?
----- ------------------
10    \-
----- ---------- -------
11    SLI_SENSE_1?
----- ------------------
12    tag 0x43?
----- ------------------
13    tag 0x0f?
----- ------------------
14    \-
===== ========== =======


NV84 GPIO NVIO specials
=======================

This list applies to NV84:NV94.

===== =============== =======
Line  Output          Input
===== =============== =======
4     PWM_0
----- --------------- -------
8     THERM_SHUTDOWN? PTHERM_INPUT_0
----- --------------- -------
9     PWM_1           PTHERM_INPUT_1
----- --------------- -------
11    SLI_SENSE_0?
----- -----------------------
12                    PTHERM_INPUT_2
----- --------------- -------
13    tag 0x0f?
----- -----------------------
14    SLI_SENSE_1?
===== =======================


NV94 GPIO NVIO specials
=======================

This list applies to NV94:NVA3.

===== =============== =======
Line  Output          Input
===== =============== =======
1                     AUXCH_HPD_0
4     PWM_0
8     THERM_SHUTDOWN? PTHERM_INPUT_0
9     PWM_1           PTHERM_INPUT_1
12                    PTHERM_INPUT_2
15                    AUXCH_HPD_2
20                    AUXCH_HPD_1
21                    AUXCH_HPD_3
===== =============== =======


NVA3 GPIO NVIO specials
=======================

This list applies to NVA3:NVD9.

===== =============== =======
Line  Output          Input
===== =============== =======
1                     AUXCH_HPD_0
3     SLI_SENSE?
----- -----------------------
8     THERM_SHUTDOWN? PTHERM_INPUT_0
9     PWM_1           PTHERM_INPUT_1
11    SLI_SENSE?
----- -----------------------
12                    PTHERM_INPUT_2
15                    AUXCH_HPD_2
16    PWM_0
19                    AUXCH_HPD_1
21                    AUXCH_HPD_3
22    tag 0x42?
----- -----------------------
23    tag 0x0f?
----- -----------------------
[any]                 FAN_TACH
===== =============== =======
