.. _tesla-misc:

=================
Misc instructions
=================

.. contents::


Introduction
============

.. todo:: write me


Data conversion: cvt
====================

.. todo:: write me

::

  cvt <integer dst> <integer src>
  cvt <integer rounding modifier> <integer dst> <float src>
  cvt <rounding modifier> <float dst> <integer src>
  cvt <rounding modifier> <float dst> <float src>
  cvt <integer rounding modifier> <float dst> <float src>

    Converts between formats. For integer destinations, always clamps result
    to target type range.


Attribute interpolation: interp
===============================

.. todo:: write me

::

  interp [cent] [flat] DST v[] [SRC]

    Gets interpolated FP input, optionally multiplying by a given value


Intra-quad data movement: quadop
================================

.. todo:: write me

::

  quadop f32 <op1> <op2> <op3> <op4> DST <srclane> SRC1 SRC2

    Intra-quad information exchange instruction. Mad as a hatter.
    First, SRC1 is taken from the given lane in current quad. Then
    op<currentlanenumber> is executed on it and SRC2, results get
    written to DST. ops can be add [SRC1+SRC2], sub [SRC1-SRC2],
    subr [SRC2-SRC1], mov2 [SRC2]. srclane can be at least l0, l1,
    l2, l3, and these work everywhere. If you're running in FP, looks
    like you can also use dox [use current lane number ^ 1] and doy
    [use current lane number ^ 2], but using these elsewhere results
    in always getting 0 as the result...


Intra-warp voting: vote
=======================

.. todo:: write me

::

  PREDICATE vote any/all CDST

    This instruction doesn't use the predicate field for conditional execution,
    abusing it instead as an input argument. vote any sets CDST to true iff the
    input predicate evaluated to true in any of the warp's active threads.
    vote all sets it to true iff the predicate evaluated to true in all acive
    threads of the current warp.


Vertex stream output control: emit, restart
===========================================

.. todo:: write me

::

  emit

    GP-only instruction that emits current contents of $o registers as the
    next vertex in the output primitive and clears $o for some reason.

  restart

    GP-only instruction that finishes current output primitive and starts
    a new one.


Nop / PM event triggering: nop, pmevent
=======================================

.. todo:: write me
