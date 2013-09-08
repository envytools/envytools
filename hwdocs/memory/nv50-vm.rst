.. _nv50-vm:

===================
NV50 virtual memory
===================

.. contents::

0. Introduction
1. VM users
2. Channels
3. DMA objects
4. Page tables
5. TLB flushes
6. User vs supervisor accesses
7. Storage types
8. Compression modes
9. VM faults
10. BAR1, BAR3, and the PEEPHOLE


Introduction
============

NV50 generation cards feature an MMU that translates user-visible logical
addresses to physical ones. The translation has two levels: DMA objects,
which behave like x86 segments, and page tables. The translation involves
the following address spaces:

- physical addresses: 40-bit physical address + target specifier, which
  can be VRAM, SYSRAM_SNOOP, or SYSRAM_NOSNOOP. They can refer to:

  - VRAM: 32-bit physical addresses - high 8 bits are ignored - on-board
    memory of the card. Supports LSR and compression. See :ref:`nv50-vram`
  - SYSRAM: 40-bit physical addresses - accessing this space will cause
    the card to invoke PCIE read/write transactions to the given bus
    address, allowing it to access system RAM or other PCI devices' memory.
    SYSRAM_SNOOP uses normal PCIE transactions, SYSRAM_NOSNOOP uses PCIE
    transactions with the "no snoop" bit set.

  Mostly, physical addresses are a result of logical address translation,
  but some memory areas are specified directly by their physical addresses.
- 12-bit tag addresses: select a cell in hidden compression tag RAM, used
  for compressed areas of VRAM. See :ref:`nv50-comp`
- virtual addresses: 40-bit virtual address + channel descriptor address,
  specifies an address that will be looked up in the page tables of the
  relevant channel. Virtual addresses are always a result of logical address
  translation and can never be specified directly.
- logical addresses: 40-bit logical address + channel descriptor address +
  DMAobj address. Specifies an address that will be translated by the
  relevant DMAobj, and then by the page tables if DMAobj says so. All
  addresses appearing in FIFO command streams are logical addresses, or
  eventually translated to logical addresses

The VM's job is to translate a logical address into its associated data:

- physical address
- target: VRAM, SYSRAM_SNOOP, or SYSRAM_NOSNOOP
- read-only flag
- supervisor-only flag
- storage type: a special value that selects the internal structure of
  contained data and enables more efficient accesses by increasing cache
  locality
- compression mode: if set, write accesses will attempt to compress the
  written data and, if successful, write only a fraction of the original
  write size to memory and mark the tile as compressed in the hidden
  tag memory. Read accesses will transparently uncompress the data. Can only
  be used on VRAM.
- compression tag address: the address of tag cell to be used if compression
  is enabled. Tag memory is addressed by "cells". Each cell is actually
  0x200 tag bits. For SINGLE compression mode, every 0x10000 bytes of
  compressed VRAM require 1 tag cell. For DOUBLE compression mode, every
  0x10000 bytes of VRAM require 2 tag cells.

.. todo:: list incomplete

A VM access can also end unsuccessfully due to multiple reasons, like a non
present page. When that happens, a VM fault is triggered. The faulting access
data is stored, and fault condition is reported to the requesting engine.
Consequences of a faulted access depend on the engine.


VM users
========

VM is used by several clients, which are identified by VM client id:

==== ========== =========== ================ ============
Id   Present on Name        Engines          Description
==== ========== =========== ================ ============
0    all        STRMOUT     PGRAPH           :ref:`PGRAPH streamout writes <nv50-pgraph-strmout>`
3    all        DISPATCH    PGRAPH           :ref:`PGRAPH context save/restore, NOTIFY, QUERY, COND, and m2mf <nv50-pgraph-dispatch>`
4    all        PFIFO_WRITE PFIFO, PFIFO_BG, :ref:`non-blocking write accesses by PFIFO, for FIFOs and BARs <nv50-pfifo-vm>`
                            PEEPHOLE, BAR   
5    all        CCACHE      PGRAPH           :ref:`PGRAPH c[], code, TIC, and TSC accesses <nv50-pgraph-ccache>`
6    VP1-VP2    PVPE        PMPEG, PME, PVP1 :ref:`VM front-end for PMPEG + PME [NV50 only] + PVP1 [NV50 only] <pvpe>`
6    VP3        PPPP        PPPP             :ref:`pppp`
7    all        CLIPID      PGRAPH           :ref:`PGRAPH window clip id reads/writes <nv50-pgraph-clipid>`
8    all        PFIFO_READ  PFIFO, PFIFO_BG  :ref:`reads by PFIFO, for FIFOs and BARs <nv50-pfifo-vm>`
                            PEEPHOLE, BAR   
9    all        VFETCH      PGRAPH           :ref:`PGRAPH vertex array fetch <nv50-pgraph-vfetch>`
0xa  all        TEXTURE     PGRAPH           :ref:`PGRAPH texture fetches <nv50-pgraph-texture>`
0xb  all        ROP         PGRAPH           :ref:`PGRAPH raster output and CUDA global/local memory accesses <nv50-pgraph-rop>`
0xc  VP2        PVP2        PVP2             :ref:`pvp2`
0xc  VP3-       PVDEC       PVDEC            :ref:`pvdec`
0xd  VP2        PBSP        PBSP             :ref:`pbsp`
0xd  VP3-       PVLD        PVLD             :ref:`pvld`
0xe  VP2        PCRYPT2     PCRYPT2          :ref:`pcrypt2`
0xe  VP3        PCRYPT3     PCRYPT3          :ref:`pcrypt3`
0xf  NV84-      PCOUNTER    PCOUNTER         :ref:`pcounter`
0x11 NVA3-      PDAEMON     PDAEMON          :ref:`pdaemon`
0x13 NVA3-      PCOPY       PCOPY            :ref:`pcopy`
0x14 NVAF-      PVCOMP      PVCOMP           :ref:`pvcomp`
==== ========== =========== ================ ============

A related concept is VM engine, which is a group of clients that share TLBs
and stay on the same channel at any single moment. It's possible for a client
to be part of several VM engines. The engines are:

==== ========== =============== ============
Id   Present on Name            Description
==== ========== =============== ============
0    all        PGRAPH          :ref:`nv50-pgraph`
1    VP1        PVP1            :ref:`pvp1`
1    VP2        PVP2            :ref:`pvp2`
1    VP3-       PVDEC           :ref:`pvdec`
4    all        PEEPHOLE        :ref:`peephole`
5    all        PFIFO           :ref:`nv50-pfifo`
6    all        BAR             :ref:`nv50-host-mem`
7    VP1        PME             :ref:`pme`
7    NVAF-      PVCOMP          :ref:`pvcomp`
8    VP1-VP2    PMPEG           :ref:`pmpeg`
8    VP3-       PPPP            :ref:`pppp`
9    VP2        PBSP            :ref:`pbsp`
9    VP3-       PVLD            :ref:`pvld`
0xa  VP2        PCRYPT2         :ref:`pcrypt2`
0xa  VP3        PCRYPT3         :ref:`pcrypt3`
0xb  NV84-      PCOUNTER        :ref:`pcounter`
0xc  NV84-      PFIFO_BG        :ref:`handles background semaphore acquire polling <nv50-pfifo-bg>`
0xd  NVA3-      PCOPY           :ref:`pcopy`
0xe  NVA3-      PDAEMON         :ref:`pdaemon`
==== ========== =============== ============

Client+engine combination doesn't, however, fully identify the source of the
access - to disambiguate that, DMA slot ids are used. The set of DMA slot
ids depends on both engine and client id. The DMA slots are
[engine/client/slot]:

- 0/0/0: PGRAPH STRMOUT
- 0/3/0: PGRAPH context
- 0/3/1: PGRAPH NOTIFY
- 0/3/2: PGRAPH QUERY
- 0/3/3: PGRAPH COND
- 0/3/4: PGRAPH m2mf BUFFER_IN
- 0/3/5: PGRAPH m2mf BUFFER_OUT
- 0/3/6: PGRAPH m2mf BUFFER_NOTIFY
- 0/5/0: PGRAPH CODE_CB
- 0/5/1: PGRAPH TIC
- 0/5/2: PGRAPH TSC
- 0/7/0: PGRAPH CLIPID
- 0/9/0: PGRAPH VERTEX
- 0/a/0: PGRAPH TEXTURE / SRC2D
- 0/b/0-7: PGRAPH RT 0-7
- 0/b/8: PGRAPH ZETA
- 0/b/9: PGRAPH LOCAL
- 0/b/a: PGRAPH GLOBAL
- 0/b/b: PGRAPH STACK
- 0/b/c: PGRAPH DST2D
- 4/4/0: PEEPHOLE write
- 4/8/0: PEEPHOLE read
- 6/4/0: BAR1 write
- 6/8/0: BAR1 read
- 6/4/1: BAR3 write
- 6/8/1: BAR3 read
- 5/8/0: FIFO pushbuf read
- 5/4/1: FIFO semaphore write
- 5/8/1: FIFO semaphore read
- c/8/1: FIFO background semaphore read
- 1/6/8: PVP1 context [NV50:NV84]
- 7/6/4: PME context [NV50:NV84]
- 8/6/1: PMPEG CMD [NV50:NV98 NVA0:NVAA]
- 8/6/2: PMPEG DATA [NV50:NV98 NVA0:NVAA]
- 8/6/3: PMPEG IMAGE [NV50:NV98 NVA0:NVAA]
- 8/6/4: PMPEG context [NV50:NV98 NVA0:NVAA]
- 8/6/5: PMPEG QUERY [NV84:NV98 NVA0:NVAA]
- b/f/0: PCOUNTER record buffer [NV84:NVC0]
- 1/c/0-f: PVP2 DMA ports 0-0xf [NV84:NV98 NVA0:NVAA]
- 9/d/0-f: PBSP DMA ports 0-0xf [NV84:NV98 NVA0:NVAA]
- a/e/0: PCRYPT2 context [NV84:NV98 NVA0:NVAA]
- a/e/1: PCRYPT2 SRC [NV84:NV98 NVA0:NVAA]
- a/e/2: PCRYPT2 DST [NV84:NV98 NVA0:NVAA]
- a/e/3: PCRYPT2 QUERY [NV84:NV98 NVA0:NVAA]
- 1/c/0-7: PVDEC falcon ports 0-7 [NV98:NVA0 NVAA-]
- 8/6/0-7: PPPP falcon ports 0-7 [NV98:NVA0 NVAA-]
- 9/d/0-7: PVLD falcon ports 0-7 [NV98:NVA0 NVAA-]
- a/e/0-7: PCRYPT3 falcon ports 0-7 [NV98:NVA3]
- d/13/0-7: PCOPY falcon ports 0-7 [NVA3-]
- e/11/0-7: PDAEMON falcon ports 0-7 [NVA3-]
- 7/14/0-7: PVCOMP falcon ports 0-7 [NVAF-]

.. todo:: PVP1
.. todo:: PME
.. todo:: Move to engine doc?


Channels
========

All VM accesses are done on behalf of some "channel". A VM channel is just
a memory structure that contains the DMA objects and page directory. VM
channel can be also a FIFO channel, for use by PFIFO and fifo engines and
containing other data structures, or just a "bare" VM channel for use with
non-fifo engines.

A channel is identified by a "channel descriptor", which is a 30-bit number
that points to the base of the channel memory structure:

- bits 0-27: bits 12-39 of channel memory structure physical address
- bits 28-29: the target specifier for channel memory structure
  - 0: VRAM
  - 1: invalid, do not use
  - 2: SYSRAM_SNOOP
  - 3: SYSRAM_NOSNOOP

The channel memory structure contains a few fixed-offset elements, as well
as serving as a container for channel objects, such as DMA objects, that
can be placed anywhere inside the structure. Due to the channel objects
inside it, the channel structure has no fixed size, although the maximal
address of channel objects is 0xffff0. Channel structure has to be aligned
to 0x1000 bytes.

The original NV50 channel structure has the following fixed elements:

- 0x000-0x200: RAMFC [fifo channels only]
- 0x200-0x400: DMA objects for fifo engines' contexts [fifo channels only]
- 0x400-0x1400: PFIFO CACHE [fifo channels only]
- 0x1400-0x5400: page directory

NV84+ cards instead use the following structure:

- 0x000-0x200: DMA objects for fifo engines' contexts [fifo channels only]
- 0x200-0x4200: page directory

The channel objects are specified by 16-bit offsets from start of the channel
structure in 0x10-byte units.


.. _nv50-dmaobj:

DMA objects
===========

The only channel object type that VM subsystem cares about is DMA objects.
DMA objects represent contiguous segments of either virtual or physical
memory and are the first stage of VM address translation. DMA objects can
be paged or unpaged. Unpaged DMA objects directly specify the target space
and all attributes, merely adding the base address and checking the limit.
Paged DMA objects add the base address, then look it up in the page tables.
Attributes can either come from page tables, or be individually overriden
by the DMA object.

DMA objects are specifid by 16-bit "selectors". In case of fifo engines,
the RAMHT is used to translate from user-visible 32-bit handles to the
selectors [see fifo/puller.txt]. The selector is shifted left by 4 bits
and added to channel structure base to obtain address of DMAobj structure,
which is 0x18 bytes long and made of 32-bit LE words:

word 0:
  - bits 0-15: object class. Ignored by VM, but usually validated by fifo
    engines - should be 0x2 [read-only], 0x3 [write-only], or 0x3d [read-write]
  - bits 16-17: target specifier:
  
    - 0: VM - paged object - the logical address is to be added to the base
      address to obtain a virtual address, then the virtual address should
      be translated via the page tables
    - 1: VRAM - unpaged object - the logical address should be added to the
      base address to directly obtain the physical address in VRAM
    - 2: SYSRAM_SNOOP - like VRAM, but gives SYSRAM address
    - 3: SYSRAM_NOSNOOP - like VRAM, but gives SYSRAM address and uses nosnoop
      transactions
  
  - bits 18-19: read-only flag
  
    - 0: use read-only flag from page tables [paged objects only]
    - 1: read-only
    - 2: read-write
  
  - bits 20-21: supervisor-only flag
  
    - 0: use supervisor-only flag from page tables [paged objects only]
    - 1: user-supervisor
    - 2: supervisor-only
  
  - bits 22-28: storage type. If the value is 0x7f, use storage type from page
     tables, otherwise directly specifies the storage type
  - bits 29-30: compression mode
  
    - 0: no compression
    - 1: SINGLE compression
    - 2: DOUBLE compression
    - 3: use compression mode from page tables
  
  - bit 31: if set, is a supervisor DMA object, user DMA object otherwise

word 1:
  bits 0-31 of limit address
word 2:
  bits 0-31 of base address
word 3:
  - bits 0-7: bits 32-39 of base address
  - bits 24-31: bits 32-39 of limit address
word 4:
  - bits 0-11: base tag address
  - bits 16-27: limit tag address
word 5:
  - bits 0-15: compression base address bits 16-31 [bits 0-15 are forced to 0]
  - bits 16-19: ??? [XXX: figure this out]

First, DMA object selector is compared with 0. If the selector is 0,
NULL_DMAOBJ fault happens. Then, the logical address is added to the base
address from DMA object. The resulting address is compared with the limit
address from DMA object and, if larger or equal, DMAOBJ_LIMIT fault happens.
If DMA object is paged, the address is looked up in the page tables, with
read-only flag, supervisor-only flag, storage type, and compression mode
optionally overriden as specified by the DMA object. Otherwise, the address
directly becomes the physical address. For compressed unpaged VRAM objects,
the tag address is computed as follows:

- take the computed VRAM physical address and substract compression base
  address from it. if result is negative, force compression mode to none
- shift result right by 16 bits
- add base tag address to the result
- if result <= limit tag addres, this is the tag address to use. Else,
  force compression mode to none.

Places where DMA objects are bound, that is MMIO registers or FIFO methods,
are commonly called "DMA slots".

Most engines cache the most recently bound DMA object. To flush the caches,
it's usually enough to rewrite the selector register, or resubmit the selector
method.

It should be noted that many engines require the DMA object's base address
to be of some specific alignment. The alignment depends on the engine and
slot.

The fifo engine context dmaobjs are a special set of DMA objects worth
mentioning. They're used by the fifo engines to store per-channel state
while given channel is inactive on the relevant engine. Their size and
structure depend on the engine. They have fixed selectors, and hence reside
at fixed positions inside the channel structure. On the original NV50, the
objects are:

======== ======= =======
Selector Address Engine
======== ======= =======
0x0020   0x00200 :ref:`PGRAPH <nv50-pgraph>`
0x0022   0x00220 :ref:`PVP1 <pvp1>`
0x0024   0x00240 :ref:`PME <pme>`
0x0026   0x00260 :ref:`PMPEG <pmpeg>`
======== ======= =======

On NV84+ cards, they are:

======== ======= ========== =======
Selector Address Present on Engine
======== ======= ========== =======
 0x0002  0x00020 all        :ref:`PGRAPH <nv50-pgraph>`
 0x0004  0x00040 VP2        :ref:`PVP2 <pvp2>`
 0x0004  0x00040 VP3-       :ref:`PVDEC <pvdec>`
 0x0006  0x00060 VP2        :ref:`PMPEG <pmpeg>`
 0x0006  0x00060 VP3-       :ref:`PPPP <pppp>`
 0x0008  0x00080 VP2        :ref:`PBSP <pbsp>`
 0x0008  0x00080 VP3-       :ref:`PVLD <pvld>`
 0x000a  0x000a0 VP2        :ref:`PCRYPT2 <pcrypt2>`
 0x000a  0x000a0 VP3        :ref:`PCRYPT3 <pcrypt3>`
 0x000a  0x000a0 NVAF-      :ref:`PVCOMP <pvcomp>`
 0x000c  0x000c0 NVA3-      :ref:`PCOPY <pcopy>`
======== ======= ========== =======


Page tables
===========

If paged DMA object is used, the virtual address is further looked up in page
tables. The page tables are two-level. Top level is 0x800-entry page
directory, where each entry covers 0x20000000 bytes of virtual address space.
The page directory is embedded in the channel structure. It starts at offset
0x1400 on the original NV50, at 0x200 on nv84+. Each page directory entry, or
PDE, is 8 bytes long. The PDEs point to page tables and specify the page table
attributes. Each page table can use either small or large pages. Small pages
are 0x1000 bytes long, large pages are 0x10000 bytes long. For small-page
page tables, the size of page table can be artificially limitted to cover
only 0x2000, 0x4000, or 0x8000 pages instead of full 0x20000 pages - the
pages over this limit will fault. Large-page page tables always cover full
0x2000 entries. Page tables of both kinds are made of 8-byte page table
entries, or PTEs.

The PDEs are made of two 32-bit LE words, and have the following format:

word 0:

- bit 0: page table present
- bit 1: page size

  - 0: large pages
  - 1: small pages

- bits 2-3: target specifier for the page table itself

  - 0: VRAM
  - 1: invalid, do not use
  - 2: SYSRAM_SNOOP
  - 3: SYSRAM_NOSNOOP

- bit 4: ??? [XXX: figure this out]
- bits 5-6: page table size [small pages only]

  - 0: 0x20000 entries [full]
  - 1: 0x8000 entries
  - 2: 0x4000 entries
  - 3: 0x2000 entries

- bits 12-31: page table physical address bits 12-31

word 1:

- bits 32-39: page table physical address bits 32-39

The page table start address has to be aligned to 0x1000 bytes.

The PTEs are made of two 32-bit LE words, and have the following format:

word 0:

- bit 0: page present
- bits 1-2: ??? [XXX: figure this out]
- bit 3: read-only flag
- bits 4-5: target specifier

  - 0: VRAM
  - 1: invalid, do not use
  - 2: SYSRAM_SNOOP
  - 3: SYSRAM_NOSNOOP

- bit 6: supervisor-only flag
- bits 7-9: log2 of contig block size in pages [see below]
- bits 12-31: bits 12-31 of physical address [small pages]
- bits 16-31: bits 16-31 of physical address [large pages]

word 1:

- bits 32-39: bits 32-39 of physical address
- bits 40-46: storage type
- bits 47-48: compression mode
- bits 49-60: compression tag address

Contig blocks are a special feature of PTEs used to save TLB space. When 2^o
adjacent pages starting on 2^o page aligned bounduary map to contiguous
physical addresses [and, if appropriate, contiguous tag addresses] and have
identical other attributes, they can be marked as a contig block of order o,
where o is 0-7. To do this, all PTEs for that range should have bits 7-9 set
equal to o, and physical/tag address fields set to the physical/tag address
of the *first* page in the contig block [ie. all PTEs belonging to contig
block should be identical]. The starting physical address need not be aligned
to contig block size, but virtual address has to be.


TLB flushes
===========

The page table contents are cached in per-engine TLBs. To flush TLB contents,
the TLB flush register 0x100c80 should be used:

MMIO 0x100c80:
  - bit 0: trigger. When set, triggers the TLB flush. Will auto-reset to 0
    when flush is complete.
  - bits 16-19: VM engine to flush

A flush consists of writing engine << 16 | 1 to this register and waiting
until bit 0 becomes 0. However, note that NV86 PGRAPH has a bug that can
result in a lockup if PGRAPH TLB flush is initiated while PGRAPH is running,
see graph/nv50-pgraph.txt for details.


User vs supervisor accesses
===========================

.. todo:: write me


Storage types
=============

.. todo:: write me


Compression modes
=================

.. todo:: write me


VM faults
=========

.. todo:: write me
