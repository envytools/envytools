.. _fifo-intro:

=============
FIFO overview
=============

.. contents::


Introduction
============

Commands to most of the engines are sent through a special engine called PFIFO.
PFIFO maintains multiple fully independent command queues, known as "channels"
or "FIFO"s. Each channel is controlled through a "channel control area", which
is a region of MMIO [pre-GF100] or VRAM [GF100+]. PFIFO intercepts all accesses
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

.. todo:: check if it still holds on GF100

On NV1:NV4, the only engine that PFIFO controls is PGRAPH, the main 2d/3d
engine of the card. In addition, PFIFO can submit commands to the SOFTWARE
pseudo-engine, which will trigger an interrupt for every submitted method.

The engines that PFIFO controls on NV4:GF100 are:

== =========== =========================== =================================================== 
Id Present on  Name                        Description                                        
== =========== =========================== =================================================== 
0  all         SOFTWARE                    Not really an engine, causes interrupt for each
                                           command, can be used to execute driver functions
                                           in sync with other commands.
1  all         :ref:`PGRAPH <graph-intro>` Main engine of the card: 2d, 3d, compute.
2  NV31:G98    :ref:`PMPEG <pmpeg>`        The PFIFO interface to VPE MPEG2 decoding engine.
   G200:MCP77
3  NV40:G84    :ref:`PME <me-fifo>`        VPE motion estimation engine.
4  NV41:G84    :ref:`PVP1 <pvp1>`          VPE microcoded vector processor.
4  VP2         :ref:`PVP2 <pvp2>`          xtensa-microcoded vector processor.
5  VP2         :ref:`PCIPHER <pcipher>`    AES cryptography and copy engine.
6  VP2         :ref:`PBSP <pbsp>`          xtensa-microcoded bitstream processor.
2  VP3-        :ref:`PPPP <pppp>`          falcon-based video post-processor.
4  VP3-        :ref:`PPDEC <ppdec>`        falcon-based microcoded video decoder.
5  VP3         :ref:`PSEC <psec>`          falcon-based AES crypto engine. On VP4, merged into PVLD.
6  VP3-        :ref:`PVLD <pvld>`          falcon-based variable length decoder.
3  GT215-      :ref:`PCOPY <pcopy>`        falcon-based memory copy engine.
5  MCP89:GF100 :ref:`PVCOMP <pvcomp>`      falcon-based video compositing engine.
== =========== =========================== =================================================== 

The engines that PFIFO controls on GF100- are:

===== ===== ===== ===== ===== =========== =========================== ===================================================
Id    Id    Id    Id    Id    Present on  Name                        Description
GF100 GK104 GK208 GK20A GM107
===== ===== ===== ===== ===== =========== =========================== ===================================================
1f    1f    1f    1f    1f    all         SOFTWARE                    Not really an engine, causes interrupt for each
                                                                      command, can be used to execute driver functions
                                                                      in sync with other commands.
0     0     0     0     0     all         :ref:`PGRAPH <graph-intro>` Main engine of the card: 2d, 3d, compute.
1     1     1     ?     \-    GF100:GM107 :ref:`PPDEC <ppdec>`        falcon-based microcoded picture decoder.
2     2     2     ?     \-    GF100:GM107 :ref:`PPPP <pppp>`          falcon-based video post-processor.
3     3     3     ?     \-    GF100:GM107 :ref:`PVLD <pvld>`          falcon-based variable length decoder.
4,5   \-    \-    \-    \-    GF100:GK104 :ref:`PCOPY <pcopy>`        falcon-based memory copy engines.
\-    6     5     ?     2     GK104:      :ref:`PVENC <pvenc>`        falcon-based H.264 encoding engine.
\-    4,5.7 4,-.6 ?     4,-.5 GK104:      :ref:`PCOPY <pcopy>`        Memory copy engines.
\-    \-    \-    ?     1     GM107:      :ref:`PVDEC <pvdec>`        falcon-based unified video decoding engine
\-    \-    \-    ?     3     GM107:      :ref:`PSEC <psec>`          falcon-based AES crypto engine, recycled
===== ===== ===== ===== ===== =========== =========================== ===================================================

This file deals only with the user-visible side of the PFIFO. For kernel-side
programming, see :ref:`nv1-pfifo`, :ref:`nv4-pfifo`, :ref:`g80-pfifo`,
or :ref:`gf100-pfifo`.

.. note:: GF100 information can still be very incomplete / not exactly true.


Overall operation
=================

The PFIFO can be split into roughly 4 pieces:

- PFIFO pusher: collects user's commands and injects them to
- PFIFO CACHE: a big queue of commands waiting for execution by
- PFIFO puller: executes the commands, passes them to the proper engine,
  or to the driver.
- PFIFO switcher: ticks out the time slices for the channels and saves /
  restores the state of the channels between PFIFO registers and RAMFC
  memory.

A channel consists of the following:

- channel mode: PIO [NV1:GF100], DMA [NV4:GF100], or IB [G80-]
- PFIFO :ref:`DMA pusher <fifo-dma-pusher>` state [DMA and IB channels only]
- PFIFO CACHE state: the commands already accepted but not yet executed
- PFIFO :ref:`puller <fifo-puller>` state
- RAMFC: area of VRAM storing the above when channel is not currently active
  on PFIFO [not user-visible]
- RAMHT [pre-GF100 only]: a table of "objects" that the channel can use. The
  objects are identified by arbitrary 32-bit handles, and can be DMA objects
  [see :ref:`nv3-dmaobj`, :ref:`nv4-dmaobj`, :ref:`g80-dmaobj`] or
  engine objects [see :ref:`fifo-puller` and engine documentation]. On pre-G80
  cards, individual objects can be shared between channels.
- vspace [G80+ only]: A hierarchy of page tables that describes the virtual
  memory space visible to engines while executing commands for the channel.
  Multiple channels can share a vspace. [see :ref:`g80-vm`,
  :ref:`gf100-vm`]
- engine-specific state

Channel mode determines the way of submitting commands to the channel. PIO
mode is available on pre-GF100 cards, and involves poking the methods directly
to the channel control area. It's slow and fragile - everything breaks down
easily when more than one channel is used simultanously. Not recommended. See
:ref:`fifo-pio` for details. On NV1:NV40, all channels support PIO mode. On
NV40:G80, only first 32 channels support PIO mode. On G80:GF100 only
channel 0 supports PIO mode.

.. todo:: check PIO channels support on NV40:G80

NV1 PFIFO doesn't support any DMA mode.

NV3 PFIFO introduced a hacky DMA mode that requires kernel assistance for
every submitted batch of commands and prevents channel switching while stuff
is being submitted. See :ref:`nv3-pfifo-dma` for details.

NV4 PFIFO greatly enhanced the DMA mode and made it controllable directly
through the channel control area. Thus, commands can now be submitted by
multiple applications simultaneously, without coordination with each other
and without kernel's help. DMA mode is described in :ref:`fifo-dma-pusher`.

G80 introduced IB mode. IB mode is a modified version of DMA mode that,
instead of following a single stream of commands from memory, has the ability
to stitch together parts of multiple memory areas into a single command stream
- allowing constructs that submit commands with parameters pulled directly from
memory written by earlier commands. IB mode is described along with DMA mode in
:ref:`fifo-dma-pusher`.

GF100 rearchitectured the whole PFIFO, made it possible to have up to 3 channels
executing simultaneously, and introduced a new DMA packet format.

The commands, as stored in CACHE, are tuples of:

- subchannel: 0-7
- method: 0-0x1ffc [really 0-0x7ff] pre-GF100, 0-0x3ffc [really 0-0xfff] GF100+
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

Method execution is described in detail in :ref:`DMA puller <fifo-puller>`
and engine-specific documentation.

Pre-NV1A, PFIFO treats everything as little-endian. NV1A introduced big-endian
mode, which affects pushbuffer/IB reads and semaphores. On NV1A:G80 cards,
the endianness can be selected per channel via the big_endian flag. On G80+ cards,
PFIFO endianness is a global switch.

.. todo:: look for GF100 PFIFO endian switch

The channel control area endianness is not affected by the big_endian flag or
G80+ PFIFO endianness switch. Instead, it follows the PMC MMIO endianness switch.

.. todo:: is it still true for GF100, with VRAM-backed channel control area?
