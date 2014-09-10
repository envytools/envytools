.. _g80-gpio:

====================
G80:GF119 GPIO lines
====================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


.. _g80-gpio-intr:

Interrupts
==========

.. todo:: write me


G80 GPIO NVIO specials
======================

This list applies to G80.

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


G84 GPIO NVIO specials
======================

This list applies to G84:G94.

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


G94 GPIO NVIO specials
======================

This list applies to G94:GT215.

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


GT215 GPIO NVIO specials
========================

This list applies to GT215:GF119.

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
