.. _tesla-float:

===========================
Floating point instructions
===========================

.. contents::


Introduction
============

.. todo:: write me


.. _tesla-opg-fadd:
.. _tesla-opg-short-fadd:
.. _tesla-opg-imm-fadd:

Addition: fadd
==============

.. todo:: write me

::

  add [sat] rn/rz f32 DST SRC1 SRC2

    Adds two floating point numbers together.


.. _tesla-opg-fmul:
.. _tesla-opg-short-fmul:
.. _tesla-opg-imm-fmul:

Multiplication: fmul
====================

.. todo:: write me

::

  mul [sat] rn/rz f32 DST SRC1 SRC2

    Multiplies two floating point numbers together


.. _tesla-opg-fmad:
.. _tesla-opg-short-fmad:
.. _tesla-opg-imm-fmad:

Multiply+add: fmad
==================

.. todo:: write me

::

  add f32 DST mul SRC1 SRC2 SRC3

    A multiply-add instruction. With intermediate rounding. Nothing
    interesting. DST = SRC1 * SRC2 + SRC3;


.. _tesla-opg-fmin:
.. _tesla-opg-fmax:

Min/max: fmin, fmax
===================

.. todo:: write me

::

  min f32 DST SRC1 SRC2
  max f32 DST SRC1 SRC2

    Sets DST to the smaller/larger of two SRC1 operands. If one operand is NaN,
    DST is set to the non-NaN operand. If both are NaN, DST is set to NaN.


.. _tesla-opg-fset:

Comparison: fset
================

.. todo:: write me

::

  set [CDST] DST <cmpop> f32 SRC1 SRC2

    Does given comparison operation on SRC1 and SRC2. DST is set to 0xffffffff
    if comparison evaluats true, 0 if it evaluates false. if used, CDST.SZ are
    set according to DST.


.. _tesla-opg-fslct:

Selection: fslct
================

.. todo:: write me

::

  slct b32 DST SRC1 SRC2 f32 SRC3

    Sets DST to SRC1 if SRC3 is positive or 0, to SRC2 if SRC3 negative or NaN.
