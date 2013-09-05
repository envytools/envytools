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

  - PVLD, the variable length decoder			`<../vdec/vp3/pvld.txt>`_
  - PVDEC, the video decoder				`<../vdec/vp3/pvdec.txt>`_
  - PPPP, the video post-processor			`<../vdec/vp3/pppp.txt>`_

- The VP3 cryptographic engine [NV98, NVAA, NVAC]:

  - PCRYPT3, the cryptographic engine			`<../vdec/vp3/pcrypt3.txt>`_

- The NVA3:NVE4 copy engines:

  - PCOPY0 [NVA3:NVE4]					`<../fifo/pcopy.txt>`_
  - PCOPY1 [NVC0:NVE4]					`<../fifo/pcopy.txt>`_

- The NVA3+ daemon engines:

  - PDAEMON [NVA3+]					`<../pm/pdaemon.txt>`_
  - PDISPLAY.DAEMON [NVD9+]				`<../display/nv50/pdisplay-daemon.txt>`_

- The NVC0 PGRAPH CTXCTL engines:

  - PGRAPH.CTXCTL					`<../graph/nvc0-ctxctl/intro.txt>`_
  - PGRAPH.GPC[*].CTXCTL				`<../graph/nvc0-ctxctl/intro.txt>`_

- The video compositing engine, PVCOMP [NVAF:NVC0]	`<../vdec/pvcomp.txt>`_
- The unknown NVD9 PUNK1C3 engine			`<../display/nv50/punk1c3.txt>`_
- The H.264 encoding engine, PVENC [NVE4+]		`<../vdec/pvenc.txt>`_

.. todo:: figure out PUNK1C3
