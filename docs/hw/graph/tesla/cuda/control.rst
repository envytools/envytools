.. _tesla-control:

====================
Control instructions
====================

.. contents::


Introduction
============

.. todo:: write me


Halting program execution: exit
===============================

.. todo:: write me

::

  exit

    Actually, not a separate instruction, just a modifier available on all
    long insns. Finishes thread's execution after the current insn ends.


.. _tesla-opg-bra:

Branching: bra
==============

.. todo:: write me

::

  bra <code target>

    Branches to the given place in the code. If only some subset of threads
    in the current warp executes it, one of the paths is chosen as the active
    one, and the other is suspended until the active path exits or rejoins.


.. _tesla-opg-bra-c:

Indirect branching: bra c[]
===========================

.. todo:: write me


.. _tesla-opg-joinat:

Setting up a rejoin point: joinat
=================================

.. todo:: write me

::

  joinat <code target>

    The arugment is address of a future join instruction and gets pushed
    onto the stack, together with a mask of currently active threads, for
    future rejoining.


Rejoining execution paths: join
===============================

.. todo:: write me

::

  join

    Also a modifier. Switches to other diverged execution paths on the same
    stack level, until they've all reached the join point, then pops off the
    entry and continues execution with a rejoined path.


.. _tesla-opg-prebrk:

Preparing a loop: prebrk
========================

.. todo:: write me

::

  breakaddr <code target>

    Like call, except doesn't branch anywhere, uses given operand as the
    return address, and pushes a different type of entry onto the stack.


.. _tesla-opg-brk:

Breaking out of a loop: brk
===========================

.. todo:: write me

::

  break

    Like ret, except accepts breakaddr's stack entry type, not call's.


.. _tesla-opg-call:

Calling subroutines: call
=========================

.. todo:: write me

::

  call <code target>

    Pushes address of the next insn onto the stack and branches to given place.
    Cannot be predicated.


.. _tesla-opg-ret:

Returning from a subroutine: ret
================================

.. todo:: write me

::

  ret

    Returns from a called function. If there's some not-yet-returned divergent
    path on the current stack level, switches to it. Otherwise pops off the
    entry from stack, rejoins all the paths to the pre-call state, and
    continues execution from the return address on stack. Accepts predicates.


.. _tesla-opg-preret:

Pushing a return address: preret
================================

.. todo:: write me


.. _tesla-opg-trap:
.. _tesla-opg-short-trap:

Aborting execution: trap
========================

.. todo:: write me

::

  trap

    Causes an error, killing the program instantly.


.. _tesla-opg-brkpt:
.. _tesla-opg-short-brkpt:

Debugger breakpoint: brkpt
==========================

.. todo:: write me

::

  brkpt

    Doesn't seem to do anything, probably generates a breakpoint when enabled
    somewhere in PGRAPH, somehow.


.. _tesla-opg-quadon:
.. _tesla-opg-quadpop:

Enabling whole-quad mode: quadon, quadpop
=========================================

.. todo:: write me

::

  quadon

    Temporarily enables all threads in the current quad, even if they were
    disabled before [by diverging, exitting, or not getting started at all].
    Nesting this is probably a bad idea, and so is using any non-quadpop
    control insns while this is active. For diverged threads, the saved PC
    is unaffected by this temporal enabling.

  quadpop

    Undoes a previous quadon command.


.. _tesla-opg-discard:

Discarding fragments: discard
=============================

.. todo:: write me


.. _tesla-opg-bar:

Block thread barriers: bar
==========================

.. todo:: write me

::

  bar sync <barrier number>

    Waits until all threads in the block arrive at the barrier, then continues
    execution... probably... somehow...
