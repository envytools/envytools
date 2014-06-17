.. _pdaemon-status-user:
.. _pdaemon-io-user:

====================
User busy indication
====================

To enable the microcode to set the "PDAEMON is busy" flag without actually
making any PDAEMON subunit perform computation, bit 4 of the falcon status
register is connected to a dummy unit whose busy status is controlled directly
by the user:

MMIO 0x420 / I[0x10800]: USER_BUSY
  Read/write, only bit 0 is valid. If set, falcon status line 4 or 5 [USER] is
  set to 1 [busy], otherwise it's set to 0 [idle].

.. todo:: what could possibly use PDAEMON's busy status?
