.. _pdaemon-io:

=================
IO register space
=================

On NVA3:NVD9, PDAEMON uses the "classic" falcon addressing scheme: I[] space
addresses are shifted left by 6 wrt the offsets in MMIO window - ie. I[0x12300]
can be accessed through MMIO address 0x10a48c, and registers are usually 0x100
bytes apart in the I[] space and aliased over that 0x100-byte range to be
easily accessible by MMIO. On NVD9+, however, I[] addresses correspond directly
to offsets in MMIO window - I[0x48c] can be accessed through MMIO 0x10a48c.

The following registers/register ranges exist on PDAEMON [first number is MMIO
offset and I[] address on NVD9+, second is I[] address on NVA3:NVD9]:

============ =============== ============ ===================== ===========
Host         Falcon          Present on   Name                  Description
============ =============== ============ ===================== ===========
0x000:0x400  0x00000:0x10000 all          N/A                   :ref:`Falcon registers <falcon-io-common>`
0x404        0x10100         all          SUBENGINE_RESET_TIME  selects how long to keep parts in reset after SUBENGINE_RESET
0x408        0x10200         all          SUBENGINE_RESET_MASK  selects parts affected by SUBENGINE_RESET
0x420        0x10800         all          USER_BUSY - sets the  :ref:`user-controlled busy flag <pdaemon-io-user>`
0x424        0x10900         v3-          ???                   [0/1/0]
0x47c        0x11f00         all          CHSW_REQ              :ref:`requests a channel switch <pdaemon-io-chsw>`
0x484        0x12100         all          ???                   [0/ff07/0]
0x488        0x12200         all          TOKEN_ALLOC           :ref:`allocates a mutex token <pdaemon-io-token>`
0x48c        0x12300         all          TOKEN_FREE            :ref:`frees a mutex token <pdaemon-io-token>`
0x490        0x12400         all          CRC_DATA              :ref:`the data to compute CRC of <pdaemon-io-crc>`
0x494        0x12500         all          CRC_STATE             :ref:`current CRC state <pdaemon-io-crc>`
0x4a0:0x4b0  0x12800:0x12c00 all          FIFO_PUT              :ref:`host to PDAEMON fifo head <pdaemon-io-fifo>`
0x4b0:0x4c0  0x12c00:0x13000 all          FIFO_GET              :ref:`host to PDAEMON fifo tail <pdaemon-io-fifo>`
0x4c0        0x13000         all          FIFO_INTR             :ref:`host to PDAEMON fifo interrupt status <pdaemon-io-fifo-intr>`
0x4c4        0x13100         all          FIFO_INTR_EN          :ref:`host to PDAEMON fifo interrupt enable <pdaemon-io-fifo-intr>`
0x4c8        0x13200         all          RFIFO_PUT             :ref:`PDAEMON to host fifo head <pdaemon-io-rfifo>`
0x4cc        0x13300         all          RFIFO_GET             :ref:`PDAEMON to host fifo tail <pdaemon-io-rfifo>`
0x4d0        0x13400         all          H2D                   :ref:`host to PDAEMON scratch reg <pdaemon-io-h2d>`
0x4d4        0x13500         all          H2D_INTR              :ref:`host to PDAEMON scratch reg interrupt status <pdaemon-io-h2d-intr>`
0x4d8        0x13600         all          H2D_INTR_EN           :ref:`host to PDAEMON scratch reg interrupt enable <pdaemon-io-h2d-intr>`
0x4dc        0x13700         all          D2H                   :ref:`PDAEMON to host scratch reg <pdaemon-io-d2h>`
0x4e0        0x13800         all          TIMER_START           :ref:`timer initial tick count <pdaemon-io-timer>`
0x4e4        0x13900         all          TIMER_TIME            :ref:`timer current remaining tick count <pdaemon-io-timer>`
0x4e8        0x13a00         all          TIMER_CTRL            :ref:`timer control <pdaemon-io-timer>`
0x4f0        0x13c00         all          ???                   [0/f/0, 0/3f/0]
0x4f8        0x13e00         all          ???                   [0/11/0, 0/13/0]
0x500        0x14000         all          COUNTER_SIGNALS       :ref:`idle signal status <pdaemon-io-counter>`
0x504+i*4    0x14100+i*0x100 all          COUNTER_MASK          :ref:`idle counter mask <pdaemon-io-counter>`
0x508+i*4    0x14200+i*0x100 all          COUNTER_COUNT         :ref:`idle counter state <pdaemon-io-counter>`
0x50c+i*4    0x14300+i*0x100 all          COUNTER_MODE          :ref:`idle counter mode <pdaemon-io-counter>`
0x580:0x5c0  0x16000:0x17000 all          MUTEX_TOKEN           :ref:`the current mutex tokens <pdaemon-io-mutex>`
0x5d0:0x5e0  0x17400:0x17800 all          DSCRATCH              :ref:`scratch registers <pdaemon-io-dscratch>`
0x5f0        0x17c00         all          ???                   [0/ffffffff/0]
0x5f4        0x17d00         all          THERM_BYTE_MASK       :ref:`PTHERM register write byte mask <pdaemon-io-therm>`
0x600:0x640  0x18000:0x19000 all          MEMIF                 :ref:`Memory interface <falcon-memif-io>`
0x680        0x1a000         all          TIMER_INTR            :ref:`timer interrupt status <pdaemon-io-timer-intr>`
0x684        0x1a100         all          TIMER_INTR_EN         :ref:`timer interrupt enable <pdaemon-io-timer-intr>`
0x688        0x1a200         all          SUBINTR               :ref:`second-level interrupt status <pdaemon-io-subintr>`
0x68c        0x1a300         all          IREDIR_TRIGGER        :ref:`PMC interrupt redirection trigger <pdaemon-io-iredir>`
0x690        0x1a400         all          IREDIR_STATUS         :ref:`PMC interrupt redirection status <pdaemon-io-iredir>`
0x694        0x1a500         all          IREDIR_TIMEOUT        :ref:`IREDIR_HOST_REQ timeout <pdaemon-io-iredir-timeout>`
0x698        0x1a600         all          IREDIR_ERR_DETAIL     :ref:`IREDIR detailed error status <pdaemon-io-iredir-err>`
0x69c        0x1a700         all          IREDIR_ERR_INTR       :ref:`IREDIR error interrupt state <pdaemon-io-iredir-err>`
0x6a0        0x1a800         all          IREDIR_ERR_INTR_EN    :ref:`IREDIR error interrupt enable <pdaemon-io-iredir-err>`
0x6a4        0x1a900         all          IREDIR_TIMEOUT_ENABLE :ref:`IREDIR_HOST_REQ timeout enable <pdaemon-io-iredir-timeout>`
0x800:0xfe0  0x20000:0x40000 v0-v2        THERM                 :ref:`PTHERM registers <pdaemon-io-therm>`
0xfe0:0x1000 \-              v0-v2        FALCON_HOST           :ref:`Falcon host registers <falcon-io-common>`
\-           0x10000:0x18000 v3-          THERM                 :ref:`PTHERM registers <pdaemon-io-therm>`
============ =============== ============ ===================== ===========

.. todo:: reset doc
.. todo:: unknown v3+ regs at 0x430+
.. todo:: 5c0+
.. todo:: 660+
.. todo:: finish the list

.. note::
    The last 0x20 bytes of THERM range on NVA3:NVD9 aren't accessible by the
    host, since they're hidden by the overlapping falcon host-only control
    registers

    The THERM range on NVD9+ is not accessible at all by the host, since its
    base address is above the end of the MMIO window to falcon's I[] space

    Neither is a problem in practice, since the host can just access the same
    registers via the PTHERM range.
