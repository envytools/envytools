.. _nv50-vga-mutex:

================
NV50 VGA mutexes
================

.. contents::


Introduction
============

Dedicated mutex support hardware supporting trylock and unlock operations
on 64 mutexes by 2 clients. Present on NV50+ cards.


MMIO registers
==============

On NV50+, the registers are located in PDISPLAY.VGA area:

- 619e80 MUTEX_TRYLOCK_A[0]
- 619e84 MUTEX_TRYLOCK_A[1]
- 619e88 MUTEX_UNLOCK_A[0]
- 619e8c MUTEX_UNLOCK_A[1]
- 619e90 MUTEX_TRYLOCK_B[0]
- 619e94 MUTEX_TRYLOCK_B[1]
- 619e98 MUTEX_UNLOCK_B[0]
- 619e9c MUTEX_UNLOCK_B[1]


Operation
=========

There are 64 mutexes and 2 clients. The clients are called A and B. Each mutex
can be either unlocked, locked by A, or locked by B at any given moment. Each
of the clients has two register sets: TRYLOCK and UNLOCK. Each register set
contains two MMIO registers, one controlling mutexes 0-31, the other mutexes
32-63. Bit i of a given register corresponds directly to mutex i or i+32.

Writing a value to the TRYLOCK register will execute a trylock operation on
all mutexes whose corresponding bit is set to 1. The trylock operation makes
an unlocked mutex locked by the requesting client, and does nothing on
an already locked mutex.

Writing a value to the UNLOCK register will likewise execute an unlock
operation on selected mutexes. The unlock operation makes a mutex locked
by the requesting client unlocked. It doesn't affect mutexes that are
unlocked or locked by the other client.

Reading a value from either the TRYLOCK or UNLOCK register will return 1
for mutexes locked by the requesting client, 0 for unlocked mutexes and
mutexes locked by the other client.

MMIO 0x619e80+i*4, i < 2: MUTEX_TRYLOCK_A
  Writing executes the trylock operation as client A, treating the written
  value as a mask of mutexes to lock. Reading returns a mask of mutexes
  locked by client A. Bit j of the value corresponds to mutex i*32+j.

MMIO 0x619e88+i*4, i < 2: MUTEX_UNLOCK_A
  Like MUTEX_TRYLOCK_A, but executes the unlock operation on write.

MMIO 0x619e90+i*4, i < 2: MUTEX_TRYLOCK_B
  Like MUTEX_TRYLOCK_A, but for client B.

MMIO 0x619e98+i*4, i < 2: MUTEX_UNLOCK_B
  Like MUTEX_UNLOCK_A, but for client B.
