.. _pcopy:
.. _pcopy-falcon:
.. _pcopy-io:
.. _pcopy-mmio:
.. _pcopy-intr:
.. _nv50-pfifo:
.. _nv50-pfifo-vm:
.. _nv50-pfifo-bg:

PFIFO: command submission to execution engines
==============================================

Contents:

.. toctree::

::

    FIFO, user perspective
    [**** ] fifo/intro.txt - FIFO overview
    [     ] fifo/pio.txt - PIO submission to FIFOs
    [**** ] fifo/dma-pusher.txt - DMA submission to FIFOs on NV04+
    [***  ] fifo/puller.txt - handling of submitted commands by FIFO
    [***  ] fifo/classes.txt - List and overview of object classes


    PFIFO, kernel perspective
    [**   ] fifo/nv01-pfifo.txt - NV01:NV04 PFIFO engine
    [     ] fifo/nv04-pfifo.txt - NV04:NV50 PFIFO engine
    [     ] fifo/nv50-pfifo.txt - NV50:NVC0 PFIFO engine
    [     ] fifo/nvc0-pfifo.txt - NVC0+ PFIFO engine

    Other FIFO engines:
    [*    ] fifo/pcopy.txt - PCOPY copying engine
