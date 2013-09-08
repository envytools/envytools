.. _falcon-intro:

============
Introduction
============

falcon is a class of general-purpose microprocessor units, used in multiple
instances on nvidia GPUs starting from NV98. Originally developed as the
controlling logic for VP3 video decoding engines as a replacement for xtensa
used on VP2, it was later used in many other places, whenever a microprocessor
of some sort was needed.

A single falcon unit is made of:

- the core microprocessor with its code and data SRAM [see :ref:`falcon-proc`]
- an IO space containing control registers of all subunits, accessible from
  the host as well as from the code running on the falcon microprocessor [see
  :ref:`falcon-io`]
- common support logic:

  - interrupt controller [see :ref:`falcon-intr`]
  - periodic and watchdog timers [see :ref:`falcon-timer`]
  - scratch registers for communication with host [see :ref:`falcon-io-scratch`]
  - PCOUNTER signal output [see :ref:`falcon-perf`]
  - some unknown other stuff

- optionally, FIFO interface logic, for falcon units used as PFIFO engines and
  some others [see :ref:`falcon-fifo`]
- optionally, common memory interface logic [see :ref:`falcon-memif`]. However,
  some engines have their own type of memory interface.
- optionally, a cryptographic AES coprocessor. A falcon unit with such
  coprocessor is called a "secretful" unit. [see :ref:`falcon-crypt`]
- any unit-specific logic the microprocessor is supposed to control

.. todo:: figure out remaining circuitry

The base falcon hardware comes in three different revisions:

- version 0: used on NV98, NVAA, NVAC
- version 3: used on NVA3+, adds a crude VM system for the code segment,
  edge/level interrupt modes, new instructions [division, software traps,
  bitfield manipulation, ...], and other features
- version 4: used on NVD9+ for some engines [others are still version 3]:
  adds support for 24-bit code addressing, debugging and ???

.. todo:: figure out v4 new stuff

The falcon units present on nvidia cards are:

- The VP3/VP4/VP5 engines [NV98 and NVAA+]:

  - :ref:`PVLD <pvld-falcon>`, the variable length decoder
  - :ref:`PVDEC <pvdec-falcon>`, the video decoder
  - :ref:`PPPP <pppp-falcon>`, the video post-processor

- The VP3 cryptographic engine [NV98, NVAA, NVAC]:

  - :ref:`PCRYPT3 <pcrypt3-falcon>`, the cryptographic engine

- The NVA3:NVE4 copy engines:

  - :ref:`PCOPY[0] <pcopy-falcon>` [NVA3:NVE4]
  - :ref:`PCOPY[1] <pcopy-falcon>` [NVC0:NVE4]

- The NVA3+ daemon engines:

  - :ref:`PDAEMON [NVA3+] <pdaemon-falcon>`
  - :ref:`PDISPLAY.DAEMON [NVD9+] <pdisplay-daemon-falcon>`
  - :ref:`PUNK1C3 [NVD9+] <punk1c3-falcon>`

- The NVC0 PGRAPH CTXCTL engines:

  - PGRAPH.CTXCTL					`<../graph/nvc0-ctxctl/intro.txt>`_
  - PGRAPH.GPC[*].CTXCTL				`<../graph/nvc0-ctxctl/intro.txt>`_

- :ref:`PVCOMP <pvcomp-falcon>`, the video compositing engine [NVAF:NVC0]
- :ref:`PVENC <pvenc-falcon>`, the H.264 encoding engine [NVE4+]
