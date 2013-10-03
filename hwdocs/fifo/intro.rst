.. _fifo-intro:

=============
FIFO overview
=============

Introduction
============

Commands to most of the engines are sent through a special engine caled PFIFO.
PFIFO maintains multiple fully independent command queues, known as "channels"
or "FIFO"s. Each channel is controlled through a "channel control area", which
is a region of MMIO [pre-NVC0] or VRAM [NVC0+]. PFIFO intercepts all accesses
to that area and acts upon them.

PFIFO internally does time-sharing between the channels, but this is
transparent to the user applications. The engines that PFIFO controls are also
aware of channels, and maintain separate context for each channel.

The context-switching ability of PFIFO depends on card generation. Since NV40,
PFIFO is able to switch between channels at essentially any moment. On older
cards, due to lack of backing storage for the CACHE, a switch is only possible
when the CACHE is empty. The PFIFO-controlled engines are, however, much worse
at switching: they can only switch between commands. While this wasn't a big
problem on old cards, since the commands were guaranteed to execute in finite
time, introduction of programmable shaders with looping capabilities made it
possible to effectively hang the whole GPU by launching a long-running shader.

.. todo:: check if it still holds on NVC0

The engines that PFIFO controls are:

+-----+----------+-----------+---------------------------------------------------+
|idx  |   name   |   cards   | description                                       |
+=====+==========+===========+===================================================+
|0/1f |          |           | Not really an engine, causes interrupt for each   |
|     | SOFTWARE |    all    | command, can be used to execute driver functions  |
|     |          |           | in sync with other commands.                      |
+-----+----------+-----------+---------------------------------------------------+
|1/0  | PGRAPH   |    all    | Main engine of the card: 2d, 3d, compute.         |
|     |          |           | [see :ref:`graph-intro`]                          |
+-----+----------+-----------+---------------------------------------------------+
|2/-  | PMPEG    | NV31:NV98 | The PFIFO interface to VPE MPEG2 decoding engine. |
|     |          |           | [see :ref:`pmpeg`]                                |
+-----+----------+-----------+---------------------------------------------------+
|3/-  | PME      | NV40:NV84 | VPE motion estimation engine [see :ref:`pme`]     |
+-----+----------+-----------+---------------------------------------------------+
|4/-  | PVP1     | NV41:NV84 | VP1 microcoded video processor. [see :ref:`pvp1`] |
+-----+----------+-----------+---------------------------------------------------+
|4/-  | PVP2     |           | VP2 video decoding engines: video processor and   |
+-----+----------+           | bitstream processor. Microcoded through embedded  |
|6/-  | PBSP     | NV84:NV98 | xtensa cores. [see :ref:`pvp2`, :ref:`pbsp`]      |
+-----+----------+           +---------------------------------------------------+
|5/-  | PCRYPT2  |           | AES cryptography and copy engine. [:ref:`pcrypt2`]|
+-----+----------+-----------+---------------------------------------------------+
|2/2  | PPPP     |           | VP3 falcon-microcoded video decoding engines:     |
+-----+----------+           | picture, post-processor, video processor,         |
|4/1  | PVDEC    | NV98:.... | bitstream processor. [see :ref:`pvld`,            |
+-----+----------+           | :ref:`pvdec`, :ref:`pppp`]                        |
|6/3  | PVLD     |           |                                                   |
+-----+----------+-----------+---------------------------------------------------+
|     |          |           | falcon-microcoded engine with AES crypto          |
|5/-  | PCRYPT3  | NV98:NVA3 | coprocessor. On NVA3+, the crypto powers were     |
|     |          |           | instead merged into PVLD. [see :ref:`pcrypt3`]    |
+-----+----------+-----------+---------------------------------------------------+
|5/-  | PVCOMP   | NVAF:NVC0 | falcon-microcoded video compositing engine        |
|     |          |           | [see :ref:`pvcomp`]                               |
+-----+----------+-----------+---------------------------------------------------+
|3/4,5|          |           | falcon-microcoded engine, meant to copy stuff from|
|     | PCOPY    | NVA3:.... | point A to point B. Comes in two copies on NVC0+, |
|     |          |           | three copies on NVE4+ [see :ref:`pcopy`]          |
+-----+----------+-----------+---------------------------------------------------+
|-/6  | PVENC    | NVE4:.... | falcon-microcoded H.264 encoding engine           |
|     |          |           | [see :ref:`pvenc`]                                |
+-----+----------+-----------+---------------------------------------------------+

idx is the FIFO engine id. The first number is the id for pre-nvc0 cards, the
second is for nvc0+ cards.

This file deals only with the user-visible side of the PFIFO. For kernel-side
programming, see :ref:`nv01-pfifo`, :ref:`nv04-pfifo`, :ref:`nv50-pfifo`,
or :ref:`nvc0-pfifo`.

.. note:: NVC0 information can still be very incomplete / not exactly true.


Overall operation
=================

The PFIFO can be split into roughly 4 pieces:

- PFIFO pusher: collects user's commands and injects them to
- PFIFO CACHE: a big queue of commands waiting for execution by
- PFIFO puller: executes the commands, passes them to the proper engine,
  or to the driver.
- PFIFO switcher: ticks out the time slices for the channels and saves/
  restores the state of the channel between PFIFO registers and RAMFC
  memory.

A channel consists of the following:

- channel mode: PIO [NV01:NVC0], DMA [NV04:NVC0], or IB [NV50:NVC0]
- PFIFO DMA pusher state [DMA and IB channels only] [see :ref:`DMA pusher <dma-pusher>`]
- PFIFO CACHE state: the commands already accepted but not yet executed
- PFIFO puller state [see :ref:`DMA puller <puller>`]
- RAMFC: area of VRAM storing the above when channel is not currently active
  on PFIFO [not user-visible]
- RAMHT [pre-NVC0 only]: a table of "objects" that the channel can use. The
  objects are identified by arbitrary 32-bit handles, and can be DMA objects
  [see :ref:`nv03-dmaobj`, :ref:`nv04-dmaobj`, :ref:`nv50-vm`] or
  engine objects [see :ref:`DMA puller <puller>` and engine documentation]. On pre-NV50
  cards, individual objects can be shared between channels.
- vspace [NV50+ only]: A hierarchy of page tables that describes the virtual
  memory space visible to engines while executing commands for the channel.
  Multiple channels can share a vspace. [see :ref:`nv50-vm`,
  :ref:`nvc0-vm`]
- engine-specific state

Channel mode determines the way of submitting commands to the channel. PIO
mode is available on pre-NVC0 cards, and involves poking the methods directly
to the channel control area. It's slow and fragile - everything breaks down
easily when more than one channel is used simultanously. Not recommended. See
:ref:`pio` for details. On NV01:NV40, all channels support PIO mode. On
NV40:NV50, only first 32
channels support PIO mode. On NV50:NVC0
only channel 0 supports PIO mode.

.. todo:: check PIO channels support on NV40:NV50

NV01 PFIFO doesn't support any DMA mode. There's apparently a PDMA engine that
could be DMA command submission, but nobody bothers enough to figure out how
it works.

NV03 PFIFO introduced a hacky DMA mode that requires kernel assistance for
every submitted batch of commands and prevents channel switching while stuff
is being submitted. See :ref:`nv01-pfifo` for details.

NV04 PFIFO greatly enhanced the DMA mode and made it controllable directly
through the channel control area. Thus, commands can now be submitted by
multiple applications simultanously, without coordination with each other
and without kernel's help. DMA mode is described in :ref:`dma-pusher`.

NV50 introduced IB mode. IB mode is a modified version of DMA mode that,
instead of following a single stream of commands from memory, has the ability
to stitch together parts of multiple memory areas into a single command stream
- allowing constructs that submit commands with parameters pulled directly from
memory written by earlier commands. IB mode is described along with DMA mode in
:ref:`dma-pusher`.

NVC0 rearchitected the whole PFIFO, made it possible to have up to 3 channels
executing simultanously, and introduced a new DMA packet format.

The commands, as stored in CACHE, are tuples of:

- subchannel: 0-7
- method: 0-0x1ffc [really 0-0x7ff] pre-NVC0, 0-0x3ffc [really 0-0xfff] NVC0+
- parameter: 0-0xffffffff
- submission mode [NV10+]: I or NI

Subchannel identifies the engine and object that the command will be sent to.
The subchannels have no fixed assignments to engines/objects, and can be
freely bound/rebound to them by using method 0. The "objects" are individual
pieces of functionality of PFIFO-controlled engine. A single engine can expose
any number of object types, though most engines only expose one.

The method selects an individual command of the object bound to the selected
subchannel, except methods 0-0xfc which are special and are executed directly
by the puller, ignoring the bound object. Note that, traditionally, methods
are treated as 4-byte addressable locations, and hence their numbers are
written down multiplied by 4: method 0x3f thus is written as 0xfc. This is
a leftover from PIO channels. In the documentation, whenever a specific method
number is mentioned, it'll be written pre-multiplied by 4 unless specified
otherwise.

The parameter is an arbitrary 32-bit value that accompanies the method.

The submission mode is I if the command was submitted through increasing DMA
packet, or NI if the command was submitted through non-increasing packet. This
information isn't actually used for anything by the card, but it's stored in
the CACHE for certain optimisation when submitting PGRAPH commands.

Method execution is described in detail in :ref:`DMA puller <puller>` and engine-specific
documentation.

Pre-NV11, PFIFO treats everything as little-endian. NV11 introduced big-endian
mode, which affects pushbuffer/IB reads and semaphores. On NV11:NV50 cards,
the endian can be selected per channel via big_endian flag. On NV50+ cards,
PFIFO endianness is a global switch.

.. todo:: look for NVC0 PFIFO endian switch

The channel control area endianness is not affected by the big_endian flag or
NV50+ PFIFO endianness switch. Instead, it follows the PMC MMIO endian switch.

.. todo:: is it still true for NVC0, with VRAM-backed channel control area?
