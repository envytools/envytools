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

Partition cycle is the first address transformation. Its purpose is converting
linear [global] addressing to partition index and per-partition addressing.
The inputs to this process are:

- the block index [ie. bits 8-31 of linear VRAM address]
- partition cycle selected [short or long]
- linear or tiled mode - linear is used when storage type is LINEAR, tiled
  for all other storage types
- partition count in the system [as selected by PBUS HWUNITS register]

The outputs of this process are:

- partition ID
- partition block index

Partition pre-ID and ID adjust are intermediate values in this process.

On NV50 [and NV50 only], there are two partition cycles available: short one
and long one. The short one switches partitions every block, while the long
one switches partitions roughly every 4 blocks. However, to make sure
addresses don't "bleed" between large page bounduaries, long partition cycle
reverts to switching partitions every block near large page bounduaries::

    if partition_cycle == LONG and chipset == NV50:
        # round down to 4 * partition_count multiple
        group_start = block_index / (4 * partition_count) * 4 * partition_count
        group_end = group_start + 4 * partition_count - 1
        # check whether the group is entirely within one large page
        use_long_cycle = (group_start & ~0xff) == (group_end & ~0xff)
    else:
        use_long_cycle = False

On NV84+, long partition cycle is no longer supported - short cycle is used
regardless of the setting.

.. todo:: verify it's really the NV84

When short partition cycle is selected, the partition pre-ID and partition
block index are calculated by simple division. The partition ID adjust is
low 5 bits of partition block index::

    if not use_long_cycle:
        partition_preid = block_index % partition_count
        partition_block_index = block_index / partition_count
        partition_id_adjust = partition_block_index & 0x1f

When long partition cycle is selected, the same calculation is performed,
but with bits 2-23 of block index, and the resulting partition block index
is merged back with bits 0-1 of block index::

    if use_long_cycle:
        quadblock_index = block_index >> 2
        partition_preid = quadblock_index % partition_count
        partition_quadblock_index = quadblock_index / partition_count
        partition_id_adjust = partition_quadblock_index & 0x1f
        partition_block_index = partition_quadblock_index << 2 | (block_index & 3)

Finally, the real partition ID is determined. For linear mode, the partition
ID is simply equal to the partition pre-ID. For tiled mode, the partition ID
is adjusted as follows:

- for 1, 3, 5, or 7-partition GPUs: no change [partition ID = partition pre-ID]
- for 2 or 6-partition GPUs: XOR together all bits of partition ID adjust, then
  XOR the partition pre-ID with the resulting bit to get the partition ID
- for 4-partition GPUs: add together bits 0-1, bits 2-3, and bit 4 of partition
  ID adjust, substract it from partition pre-ID, and take the result modulo 4.
  This is the partition ID.
- for 8-partition GPUs: ???

.. todo:: figure out the 8-partition cycle

In summary::

    if linear or partition_count in [1, 3, 5, 7]:
        partition_id = partition_preid
    elif partition_count in [2, 6]:
        xor = 0
        for bit in range(5):
            xor ^= partition_id_adjust >> bit & 1
        partition_id = partition_preid ^ xor
    elif partition_count == 4:
        sub = partition_id_adjust & 3
        sub += partition_id_adjust >> 2 & 3
        sub += partition_id_adjust >> 4 & 1
        partition_id = (partition_preid - sub) % 4
    elif partition_count == 8:
        # XXX figure it out

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
