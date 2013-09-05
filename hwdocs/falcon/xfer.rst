.. _falcon-xfer:

=======================================
Code/data xfers to/from external memory
=======================================

.. contents::


Introduction
============

The falcon has a builtin DMA controller that allows running asynchronous copies
beteween falcon data/code segments and external memory.

An xfer request consists of the following:

- mode: code load [external -> falcon code], data load [external -> falcon data],
  or data store [falcon data -> external]
- external port: 0-7. Specifies which external memory space the xfer should
  use.
- external base: 0-0xffffffff. Shifted left by 8 bits to obtain the base
  address of the transfer in external memory.
- external offset: 0-0xffffffff. Offset in external memory, and for v3+
  code segments, virtual address that code should be loaded at.
- local address: 0-0xffff. Offset in falcon code/data segment where data should
  be transferred. Physical address for code xfers.
- xfer size: 0-6 for data xfers, ignored for code xfers [always effectively
  6]. The xfer copies (4<<size) bytes.
- secret flag: Secret engines code xfers only. Specifies if the xfer should
  load secret code.

.. todo:: one more unknown flag on secret engines

Note that xfer functionality is greatly enhanced on secret engines to cover
copying data to/from crypto registers. See :ref:`falcon-crypt` for details.

xfer requests can be submitted either through special falcon instructions, or
through poking IO registers. The requests are stored in a queue and processed
asynchronously.

A data load xfer copies (4<<$size) bytes from external memory port $port
at address ($ext_base << 8) + $ext_offset to falcon data segment at address
$local_address. external offset and local address have to be aligned to the
xfer size.

A code load xfer copies 0x100 bytes from external memory port $port at address
($ext_base << 8) + $ext_offset to falcon code segment at physical address
$local_address. Right after queuing the transfer, the code page is marked
"busy" and, for v3+, mapped to virtual address $ext_offset. If the secret
flag is set, it'll also be set for the page. When the transfer is finished,
The page flags are set to "usable" for non-secret pages, or "secret" for
secret pages.


.. _falcon-sr-xcbase:
.. _falcon-sr-xdbase:
.. _falcon-sr-xtargets:

xfer special registers
======================

There are 3 falcon special registers that hold parameters for uc-originated xfer
requests. $xdbase stores ext_base for data loads/stores, $xcbase stores
ext_base for code loads. $xtargets stores the ports for various types of
xfer:

- bits 0-2: port for code loads
- bits 8-10: port for data loads
- bits 12-14: port for data stores

The external memory that falcon will use depends on the particular engine.
See `<../graph/nvc0-ctxctl/memif.txt>`_ for NVC0 PGRAPH CTXCTLs,
:ref:`falcon-memif` for the other engines.


.. _falcon-isa-xfer:

Submitting xfer requests: xcld, xdld, xdst
==========================================

These instruction submit xfer requests of the relevant type. ext_base
and port are taken from $xdbase/$xcbase and $xtargets special registers.
ext_offset is taken from first operand, local_address is taken from low 16
bits of second operand, and size [for data xfers] is taken from bits 16-18
of the second operand. Secret flag is taken from $cauth bit 16.

Instructions:
    ==== =========== =========
    Name Description Subopcode
    ==== =========== =========
    xcld code load   4
    xdld data load   5
    xdst data store  6
    ==== =========== =========
Instruction class:
    unsized
Operands:
    SRC1, SRC2
Forms:
    ====== ======
    Form   Opcode
    ====== ======
    R2, R1 fa
    ====== ======
Operation:
    ::

        if (op == xcld)
                XFER(mode=code_load, port=$xtargets[0:2], ext_base=$xcbase,
                        ext_offset=SRC1, local_address=(SRC2&0xffff),
                               secret=($cauth[16:16]));
        else if (op == xdld)
                XFER(mode=data_load, port=$xtargets[8:10], ext_base=$xdbase,
                        ext_offset=SRC1, local_address=(SRC2&0xffff),
                               size=(SRC2>>16));
        else // xdst
                XFER(mode=data_store, port=$xtargets[12:14], ext_base=$xdbase,
                        ext_offset=SRC1, local_address=(SRC2&0xffff),
                               size=(SRC2>>16));


.. _falcon-isa-xwait:

Waiting for xfer completion: xcwait, xdwait
===========================================

These instructions wait until all xfers of the relevant type have finished.

Instructions:
    ====== ======================================== =========
    Name   Description                              Subopcode
    ====== ======================================== =========
    xdwait wait for all data loads/stores to finish 3
    xcwait wait for all code loads to finish        7
    ====== ======================================== =========
Instruction class:
    unsized
Operands:
    [none]
Forms:
    ============= ======
    Form          Opcode
    ============= ======
    [no operands] f8
    ============= ======
Operation:
    ::

        if (op == xcwait)
                while (XFER_ACTIVE(mode=code_load));
        else
                while (XFER_ACTIVE(mode=data_load) || XFER_ACTIVE(mode=data_store));


.. _falcon-io-xfer:

Submitting xfer requests via IO space
=====================================

There are 4 IO registers that can be used to manually submit xfer reuqests.
The request is sent out by writing XFER_CTRL register, other registers have
to be set beforehand.

MMIO 0x110 / I[0x04400]: XFER_EXT_BASE
  Specifies the ext_base for the xfer that will be launched by XFER_CTRL.

MMIO 0x114 / I[0x04500]: XFER_LOCAL_ADDRESS
  Specifies the local_address for the xfer that will be launched by XFER_CTRL.

MMIO 0x118 / I[0x04600]: XFER_CTRL
  Writing requests a new xfer with given params, reading shows the last value
  written + two status flags

  - bit 0: pending [RO]: The last write to XFER_CTRL is still waiting for place
    in the queue. XFER_CTRL shouldn't be written until this bit clears.
  - bit 1: ??? [RO]
  - bit 2: secret flag [secret engines only]
  - bit 3: ??? [secret engines only]
  - bits 4-5: mode
  
    - 0: data load
    - 1: code load
    - 2: data store

  - bits 8-10: size
  - bits 12-14: port

.. todo:: figure out bit 1. Related to 0x10c?

MMIO 0x11c / I[0x04700]: XFER_EXT_OFFSET
  Specifies the ext_offset for the xfer that will be launched by XFER_CTRL.

.. todo:: how to wait for xfer finish using only IO?


.. _falcon-io-xfer-status:

xfer queue status registers
===========================

The status of the xfer queue can be read out through an IO register:

MMIO 0x120 / I[0x04800]: XFER_STATUS
  - bit 1: busy. 1 if any data xfer is pending.
  - bits 4-5: ??? writable
  - bits 16-18: number of data stores pending
  - bits 24-26: number of data loads pending
 
.. todo:: bits 4-5

.. todo:: RE and document this stuff, find if there's status for code xfers
