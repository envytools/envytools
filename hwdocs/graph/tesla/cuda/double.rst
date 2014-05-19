.. _tesla-double:

============================================
Double precision floating point instructions
============================================

.. contents::


Introduction
============

.. todo:: write me


Addition: dadd
==============

.. todo:: write me


Multiplication: dmul
====================

.. todo:: write me


Fused multiply+add: dfma
========================

.. todo:: write me

::


  fma f64 DST SRC1 SRC2 SRC3

    Fused multiply-add, with no intermediate rounding.


Min/max: dmin, dmax
===================

.. todo:: write me

::

  min f64 DST SRC1 SRC2
  max f64 DST SRC1 SRC2

    Sets DST to the smaller/larger of two SRC1 operands. If one operand is NaN,
    DST is set to the non-NaN operand. If both are NaN, DST is set to NaN.


Comparison: dset
================

.. todo:: write me

::

  set [CDST] DST <cmpop> f64 SRC1 SRC2

    Does given comparison operation on SRC1 and SRC2. DST is set to 0xffffffff
    if comparison evaluats true, 0 if it evaluates false. if used, CDST.SZ are
    set according to DST.
