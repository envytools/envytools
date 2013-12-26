.. _nv50-vram:

==================================
NV50:NVC0 VRAM structure and usage
==================================

.. contents::


Introduction
============

The basic structure of NV50 memory is similiar to other card generations
and is described in :ref:`vram`.

There are two sub-generations of NV50 memory controller: the original NV50 one
and the NVA3 one. The NV50 memory controller was designed for DDR2 and GDDR3
memory. It's split into several [1-8] partitions, each of them having 64-bit
memory bus. The NVA3 memory controller added support for DDR3 and GDDR5
memory and split the partitions into two subpartitions, each of them having
32-bit memory bus.

On NV50, the combination of DDR2/GDDR3 [ie. 4n prefetch] memory with 64-bit
memory bus results in 32-byte minimal transfer size. For that reason, 32-byte
units are called sectors. On NVA3, DDR3/GDDR5 [ie. 8n prefetch] memory with
32-bit memory bus gives the same figure.

Next level of granularity for memory is 256-byte blocks. For tiled surfaces,
blocks correspond directly to small tiles. Memory is always assigned
to partitions in units of whole blocks - all addresses in a block will stay
in a single partition. Also, format dependent memory address reordering is
applied within a block.

The final fixed level of VRAM granularity is a 0x10000-byte [64kiB] large
page. While NV50 VM supports using smaller page sizes for VRAM, certain
features [compression, long partition cycle] should only be enabled on
per-large page basis.

Apart from VRAM, the memory controller uses so-called tag RAM, which is used
for compression. Compression is a feature that allows a memory block to be
stored in a more efficient manner [eg. using 2 sectors instead of the normal
8] if its contents are sufficiently regular. The tag RAM is used to store
the compression information for each block: whether it's compressed, and if
so, in what way. Note that compression is only meant to save memory bandwidth,
not memory capacity: the sectors saved by compression don't have to be
transmitted over the memory link, but they're still assigned to that block and
cannot be used for anything else. The tag RAM is allocated in units of tag
cells, which have varying size depending on the partition number, but always
correspond to 1 or 2 large pages, depending on format.

VRAM is addressed by 32-bit linear addresses. Some memory attributes affecting
low-level storage are stored together with the linear address in the page
tables [or linear DMA object]. These are:

- storage type: a 7-bit enumerated value that describes the memory purpose
  and low-level storage within a block, and also selects whether normal
  or alternative bank cycle is used
- compression mode: a 2-bit field selecting whether the memory is:
  
  - not compressed,
  - compressed with 2 tag bits per block [1 tag cell per large page], or
  - compressed with 4 tag bits per block [2 tag cells per large page]

- compression tag cell: a 12-bit index into the available tag memory, used
  for compressed memory
- partition cycle: a 1-bit field selecting whether the short [1 block] or long
  [4 blocks] partition cycle is used

The linear addresses are transformed in the following steps:

1. The address is split into the block index [high 24 bits], and the offset
   inside the block [low 8 bits].
2. The block index is transformed to partition id and partition block index.
   The process depends on whether the storage type is tiled or linear and
   the partition cycle selected. If compression is enabled, the tag cell
   index is also translated to partition tag bit index.
3. [NVA3+ only] The partition block index is translated into subpartition
   ID and subpartition block index. If compression is enabled, partition tag
   bit index is also translated to subpartition tag bit index.
4. [Sub]partition block index is split into row/bank/column fields.
5. Row and bank indices are transformed according to the bank cycle. This
   process depends on whether the storage type selects the normal or alternate
   bank cycle.
6. Depending on storage type and the compression tag contents, the offset in
   the block may refer to varying bytes inside the block, and the data may
   be transformed due to compression. When the required transformed block
   offsets have been determined, they're split into the remaining low column
   bits and offset inside memory word.


Partition cycle
===============

.. todo:: write me


Tag memory addressing
---------------------

.. todo:: write me


Subpartition cycle
==================

.. todo:: write me


Row/bank/column split
=====================

.. todo:: write me


Bank cycle
==========

.. todo:: write me


Storage types
=============

.. todo:: write me
