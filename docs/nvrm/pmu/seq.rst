.. _falcon-seq:

=================
SEQ Scripting ISA
=================

.. contents::

Introduction
============

NVIDIA uses PDAEMON for power-management related functions, including DVFS. For this
they extended the firmware, PMU, with a scripting language called seq. Scripts are
uploaded through :ref:`falcon data I/O <falcon-io-data>`.

.. _falcon-seq-isa:

SEQ conventions
===============

Operations are represented as 32-bit opcodes, follwed by 0 or more 32-bit parameters.
The opcode is encoded as follows:

- Bit 0-7: operation
- Bit 31-16: total operation length in 32-bit words (# parameters + 1)

A script ends with 0x0. In the pseudo-code in the rest of this document, the following conventions hold:

- $r3 is reserved as the script program counter, aliased pc
- op aliases \*pc & 0xffff
- params aliases (\*pc & 0xffff0000) >> 16
- param[] points to the first parameter, the first word after \*pc
- PMU reserves 0x5c bytes on the stack for general usage, starting at sp+0x24
- scratch[] is a pointer to scratchpad memory from 0x3e0 onward.

Stack layout
------------

=========== ======== =============== ============================================
address     Type     Alias           Description
=========== ======== =============== ============================================
0x00-0x20   u32[9]                   Callers $r[0]..$r[8]
0x24        u32      \*packet.data    Pointer to data structure
0x2a        u16      in_words        Number of words in the program.
0x2c        u32      \*in_end         Pointer to the end of the program
0x30        u32      insn_len        Length of the currently executed instruction
0x54        u32      \*crtc_vert       &(PDISPLAY.CRTC_STAT[0].VERT)+crtc_off
0x58        u32      crtc_off        Offset for current CRTC from PDISPLAY[0]
0x5c        u32      \*in_start       Pointer to the start of the program
0x62        u16      word_exit
0x64        u32      timestamp
=========== ======== =============== ============================================

Scratch layout
--------------

======= =========== ============================================================
Type    Name        Description
======= =========== ============================================================
u8      out_words   Size of the ``out`` memory section, in 32-bit units
u24                 Unused, padding
u32     \*out_start Pointer to the ``out`` memory section
u8      flag_eq     1 if compare val_last == param
u8      flag_lt     1 if compare val_last < param
u16                 Unused, padding
u32     val_last    Holds the register last read or written. Can be set manually
u32     reg_last    The value last read or written. Can be set manually
u32     val_ret     Holds a return value written back to sp[80] after successful
                    execution
======= =========== ============================================================

.. _falcon-seq-opcodes:

Opcodes
=======

XXX: Gaps are all sorts of exit routines. Not clear how the exit procedure works
wrt status propagation.

========= ========= =================================================================
Opcode    Params    Description
========= ========= =================================================================
0x00      1         :ref:`SET last value <falcon-seq-op-set-last>`
0x01      1         :ref:`SET last register <falcon-seq-op-set-last>`
0x02      1         :ref:`OR last value <falcon-seq-op-or-last>`
0x03      1         :ref:`OR last register <falcon-seq-op-or-last>`
0x04      1         :ref:`AND last value <falcon-seq-op-and-last>`
0x05      1         :ref:`AND last register <falcon-seq-op-and-last>`
0x06      1         :ref:`ADD last value <falcon-seq-op-add-last>`
0x07      1         :ref:`ADD last register <falcon-seq-op-add-last>`
0x08      1         :ref:`SHIFT last value <falcon-seq-op-shift-last>`
0x09      1         :ref:`SHIFT last register <falcon-seq-op-shift-last>`
0x0a      0         :ref:`READ last register <falcon-seq-op-read-last>`
0x0b      1         :ref:`READ last register <falcon-seq-op-read-last>`
0x0c      1         :ref:`READ last register <falcon-seq-op-read-last>`
0x0d      0         :ref:`WRITE last register <falcon-seq-op-write-last>`
0x0e      1         :ref:`WRITE last register <falcon-seq-op-write-last>`
0x0f      1         :ref:`WRITE last register <falcon-seq-op-write-last>`
0x10      0         :ref:`EXIT <falcon-seq-op-exit>`
0x11      0         :ref:`EXIT <falcon-seq-op-exit>`
0x12      0         :ref:`EXIT <falcon-seq-op-exit>`
0x13      1         :ref:`WAIT <falcon-seq-op-wait>`
0x14      2         :ref:`WAIT STATUS <falcon-seq-op-wait-status>`
0x15      2         :ref:`WAIT BITMASK last <falcon-seq-op-wait-bitmask-last>`
0x16      1         :ref:`EXIT <falcon-seq-op-exit>`
0x17      1         :ref:`COMPARE last value <falcon-seq-op-cmp>`
0x18      1         :ref:`BRANCH EQ <falcon-seq-op-branch-eq>`
0x19      1         :ref:`BRANCH NEQ <falcon-seq-op-branch-neq>`
0x1a      1         :ref:`BRANCH LT <falcon-seq-op-branch-lt>`
0x1b      1         :ref:`BRANCH GT <falcon-seq-op-branch-gt>`
0x1c      1         :ref:`BRANCH <falcon-seq-op-branch>`
0x1d      0         :ref:`IRQ_DISABLE <falcon-seq-op-irq-disable>`
0x1e      0         :ref:`IRQ_ENABLE <falcon-seq-op-irq-enable>`
0x1f      1         :ref:`AND last value, register <falcon-seq-op-and-ind-last>`
0x20      1         :ref:`FB PAUSE/RESUME <falcon-seq-op-fb>`
0x21      2n        :ref:`SET register(s) <falcon-seq-op-reg-set>`
0x22      1         :ref:`WRITE OUT last value <falcon-seq-op-write-out-last>`
0x23      1         :ref:`WRITE OUT indirect last value <falcon-seq-op-write-out-last>`
0x24      2         :ref:`WRITE OUT <falcon-seq-op-write-out>`
0x25      2         :ref:`WRITE OUT indirect <falcon-seq-op-write-out>`
0x26      1         :ref:`READ OUT last value <falcon-seq-op-read-out-last>`
0x27      1         :ref:`READ OUT indirect last value <falcon-seq-op-read-out-last>`
0x28      1         :ref:`READ OUT last register <falcon-seq-op-read-out-last-reg>`
0x29      1         :ref:`READ OUT indirect last register <falcon-seq-op-read-out-last-reg>`
0x2a      2         :ref:`ADD OUT <falcon-seq-op-add-out>`
0x2b      1         :ref:`COMPARE OUT <falcon-seq-op-cmp-out>`
0x2c      1         :ref:`OR last value, register <falcon-seq-op-or-ind-last>`
0x2d      2         XXX: Display-related
0x2e      1         :ref:`WAIT <falcon-seq-op-wait>`
0x2f      0         :ref:`EXIT <falcon-seq-op-exit>`
0x30      1         :ref:`OR OUT last value <falcon-seq-op-or-out>`
0x31      1         :ref:`OR OUT indirect last value <falcon-seq-op-or-out>`
0x32      1         :ref:`AND OUT last value <falcon-seq-op-and-out>`
0x33      1         :ref:`AND OUT indirect last value <falcon-seq-op-and-out>`
0x34      1         :ref:`WRITE OUT TIMESTAMP <falcon-seq-op-write-out-ts>`
0x35      1         :ref:`WRITE OUT TIMESTAMP indirect <falcon-seq-op-write-out-ts>`
0x38      0         NOP
0x3b      1         :ref:`ADD last value, OUT <falcon-seq-op-add-last-out>`
0x3c      1         :ref:`ADD last value, OUT indirect <falcon-seq-op-add-last-out>`
other     0         :ref:`EXIT <falcon-seq-op-exit>`
========= ========= =================================================================

.. _falcon-seq-op-mem:

Memory
======

.. _falcon-seq-op-set-last:

SET last
--------
Set the last register/value in scratch memory.

Opcode:
        0x00
        0x01
Parameters:
        1
Operation:
    ::

        scratch[3 + (op & 1)] = param[0];

.. _falcon-seq-op-read-last:

READ last register
------------------
Do a read of the last register and/or a register/offset given by parameter 1,
and write back to the last value.

Opcode:
        0x0a
        0x0b
        0x0c
Parameters:
        0/1
Operation:
    ::

        reg = 0;
        if(op == 0xa || op == 0xc)
                reg += scratch->reg_last;
        if(op == 0xb || op == 0xc)
                reg += param[0];
        
        scratch->val_last = mmrd(reg);

.. _falcon-seq-op-write-last:

WRITE last register
-------------------
Do a write to the last register and/or a register/offset given by parameter 1
of the last value.

Opcode:
        0x0d
        0x0e
        0x0f
Parameters:
        0/1
Operation:
    ::

        reg = 0;
        if(op == 0xd || op == 0xf)
                reg += scratch->reg_last;
        if(op == 0xe || op == 0xf)
                reg += param[0];
        
        mmwr_seq(reg, scratch->val_last);

SET register(s)
---------------
For each register/value pair, this operation performs a (locked) register write.
through 

Opcode:
        0x21
Parameters:
        2n for n > 0
Operation:
    ::

        IRQ_DISABLE;
        for (i = 0; i < params; i += 2) {
                mmwr_unlocked(param[i],param[i+1]);
        }
        IRQ_ENABLE;
        scratch->reg_last = param[i-2];
        scratch->val_last = param[i-1];

.. _falcon-seq-op-write-out-last:

WRITE OUT last value
--------------------
Write a word to the OUT memory section, offset by the first parameter. For
indirect read, the parameter points to an 8-bit value describing the offset of 
the address to write to.

Opcode:
        0x22
        0x23
Parameters:
        1
Operation:
    ::

        if (!out_start)
                exit(pc);
        idx = $param[0].u08;
        if (idx >= out_words.u08)
                exit(pc);
        
        /* Indirect */
        if (op & 0x1) {
                idx = out_start[idx];
                if (idx >= out_words.u08)
                        exit(pc);
        }
        
        out_start[idx] = scratch->val_last;

.. _falcon-seq-op-write-out:

WRITE OUT
---------
Write a word to the OUT memory section, offset by the first parameter. For
indirect read, the parameter points to an 8-bit value describing the offset of
the address to write to.

Opcode:
        0x24
        0x25
Parameters:
        2
Operation:
    ::

        if (!out_start)
                exit(pc);
        idx = $param[0].u08;
        if (idx >= out_words.u08)
                exit(pc);
        
        /* Indirect */
        if (op & 0x1) {
                idx = out_start[idx];
                if (idx >= out_words.u08)
                        exit(pc);
        }
        
        out_start[idx] = param[1];

.. _falcon-seq-op-read-out-last:

READ OUT last value
----------------------
Read a word from the OUT memory section, into the val_last location. Parameter is
the offset inside the out page. For indirect read, the parameter points to an
8-bit value describing the offset of the read out value.

Opcode:
        0x26
        0x27
Parameters:
        1
Operation:
    ::

        if (!out_start)
                exit(pc);
        idx = $param[0].u08;
        if (idx >= out_words.u08)
                exit(pc);

        /* Indirect */
        if (op & 0x1) {
                idx = out_start[idx];
                if (idx >= out_words.u08)
                        exit(pc);
        }
        
        scratch->val_last = out_start[idx];

.. _falcon-seq-op-read-out-last-reg:

READ OUT last register
----------------------
Read a word from the OUT memory section, into the reg_last location. Parameter is
the offset inside the out page. For indirect read, the parameter points to an
8-bit value describing the offset of the read out value.

Opcode:
        0x28
        0x29
Parameters:
        1
Operation:
    ::

        if (!out_start)
                exit(pc);
        idx = $param[0].u08;
        if (idx >= out_words.u08)
                exit(pc);

        /* Indirect */
        if (op & 0x1) {
                idx = out_start[idx];
                if (idx >= out_words.u08)
                        exit(pc);
        }
        
        scratch->reg_last = out_start[idx];

.. _falcon-seq-op-write-out-ts:

WRITE OUT TIMESTAMP
-------------------
Write the current timestamp to the OUT memory section, offset by the first
parameter. For indirect read, the parameter points to an 8-bit value describing
the offset of the address to write to.

Opcode:
        0x34
        0x35
Parameters:
        2
Operation:
    ::

        if (!out_start)
                exit(pc);
        idx = $param[0].u08;
        if (idx >= out_words.u08)
                exit(pc);
        
        /* Indirect */
        if (op & 0x1) {
                idx = out_start[idx];
                if (idx >= out_words.u08)
                        exit(pc);
        }
        
        call_timer_read(&value)
        out_start[idx] = value;

.. _falcon-seq-op-arith:

Arithmetic
==========
.. _falcon-seq-op-or-last:

OR last
-------
OR the last register/value in scratch memory.

Opcode:
        0x02
        0x03
Parameters:
        1
Operation:
    ::

        scratch[3 + (op & 1)] |= param[0];

.. _falcon-seq-op-and-last:

AND last
--------
AND the last register/value in scratch memory.

Opcode:
        0x04
        0x05
Parameters:
        1
Operation:
    ::

        scratch[3 + (op & 1)] &= param[0];

.. _falcon-seq-op-add-last:

ADD last
--------
ADD the last register/value in scratch memory.

Opcode:
        0x06
        0x07
Parameters:
        1
Operation:
    ::

        scratch[3 + (op & 1)] += param[0];
        
.. _falcon-seq-op-shift-last:

SHIFT-left last
---------------
Shift the last register/value in scratch memory to
the left, negative parameter shifts right.

Opcode:
        0x08
        0x09
Parameters:
        1
Operation:
    ::
    
        if(param[0].s08 >= 0) {
                scratch[3 + (op & 1)] <<= sex($param[0].s08);
                break;
        } else {
                scratch[3 + (op & 1)] >>= -sex($param[0].s08);
                break;
        }

.. _falcon-seq-op-and-ind-last:

AND last value, register
------------------------
AND the last value with value read from register.

Opcode:
        0x1f
Parameters:
        1
Operation:
    ::

        scratch->val_last &= mmrd(param[0]);

.. _falcon-seq-op-add-out:

ADD OUT
--------
ADD an immediate value to a value in the OUT memory region.

Opcode:
        0x2a
Parameters:
        2
Operation:
    ::

        if (!out_start)
                exit(pc);
        idx = param[0];
        if (idx >= out_len)
                exit(pc);

        out_start[idx] += param[1];

.. _falcon-seq-op-or-ind-last:

OR last value, register
-----------------------
OR the last value with value read from register

Opcode:
        0x2c
Parameters:
        1
Operation:
    ::

        scratch->val_last |= mmrd(param[0]);
        
.. _falcon-seq-op-or-out:

OR OUT last value
-----------------
OR the contents of last_val with a value in the OUT memory region.

Opcode:
        0x30
        0x31
Parameters:
        1
Operation:
    ::

        if (!out_start)
                exit(pc);
        idx = param[0];
        if (idx >= out_len)
                exit(pc);

        /* Indirect */
        if (op & 0x1) {
                idx = out_start[idx];
                if (idx >= out_words.u08)
                        exit(pc);
        }

        out_start[idx] |= scratch->val_last;

.. _falcon-seq-op-add-last-out:

ADD last value, OUT
-------------------
Add a value in OUT to val_last.

Opcode:
        0x3b
        0x3c
Parameters:
        1
Operation:
    ::

        if (!out_start)
                exit(pc);
        idx = param[0];
        if(idx >= out_len)
                exit(pc);

        /* Indirect
        if(!op & 0x1) {
                idx = out_start[idx];
                if (idx >= out_words.u08)
                        exit(pc);
        }
        val_last += out_start[idx];

.. _falcon-seq-op-and-out:

AND OUT last value
------------------
AND the contents of last_val with a value in the OUT memory region.

Opcode:
        0x32
        0x33
Parameters:
        1
Operation:
    ::

        if (!out_start)
                exit(pc);
        idx = param[0];
        if (idx >= out_len)
                exit(pc);

        /* Indirect */
        if (op & 0x1) {
                idx = out_start[idx];
                if (idx >= out_words.u08)
                        exit(pc);
        }

        out_start[idx] &= scratch->val_last;

.. _falcon-seq-op-ctrl-flow:

Control flow
============

.. _falcon-seq-op-exit:

EXIT
----
Exit

Opcode:
        0x10..0x12
        0x16
        0x2f
Parameters:
        0/1
Operation:
    ::

        if(op == 0x16)
                exit(param[0].s08);
        else
                exit(-1);
                
.. _falcon-seq-op-cmp:

COMPARE last value
------------------
Compare last value with a parameter. If smaller, set flag_lt. If equal, set
flag_eq.

Opcode:
        0x17
Parameters:
        1
Operation:
    ::

        flag_eq = 0;
        flag_lt = 0;
        
        if(scratch->val_last < param[0])
                flag_lt = 1;
        else if(scratch->val_last == param[0])
                flag_eq = 1;

.. _falcon-seq-op-branch-eq:

BRANCH EQ
---------
When compare resulted in eq flag set, branch to an absolute location in the
program.

Opcode:
        0x18
Parameters:
        1
Operation:
    ::

        if(flag_eq)
                BRANCH param[0];

.. _falcon-seq-op-branch-neq:

BRANCH NEQ
----------
When compare resulted in eq flag unset, branch to an absolute location in the
program.

Opcode:
        0x19
Parameters:
        1
Operation:
    ::

        if(!flag_eq)
                BRANCH param[0];
                
.. _falcon-seq-op-branch-lt:

BRANCH LT
---------
When compare resulted in lt flag unset, branch to an absolute location in the
program.

Opcode:
        0x1a
Parameters:
        1
Operation:
    ::

        if(flag_lt)
                BRANCH param[0];
                
.. _falcon-seq-op-branch-gt:

BRANCH GT
---------
When compare resulted in lt and eq flag unset, branch to an absolute location in
the program.

Opcode:
        0x1b
Parameters:
        1
Operation:
    ::

        if(!flag_lt && !flag_eq)
                BRANCH param[0];

.. _falcon-seq-op-branch:

BRANCH
------
Branch to an absolute location in the program.

Opcode:
        0x1c
Parameters:
        1
Operation:
    ::

        target = param[0].s16;
        if(target >= in_words)
                exit(target);

        word_exit = $r9.s16
        target &= 0xffff;
        target <<= 2;
        pc = in_start + target;

        if(pc >= in_end)
                exit(in_end);

.. _falcon-seq-op-cmp-out:

COMPARE OUT
-----------
Compare word in OUT with a parameter. If smaller, set flag_lt. If equal, set
flag_eq.

Opcode:
        0x2b
Parameters:
        1
Operation:
    ::

        if(!out_start)
                exit(pc);

        idx = param[0];
        if(idx >= out_words.u08)
                exit(pc);
        
        flag_eq = 0;
        flag_lt = 0;
        
        if(out_start[idx] < param[1])
                flag_lt = 1;
        else if(out_start[idx] == param[1])
                flag_eq = 1;

.. _falcon-seq-op-misc:

Miscellaneous
=============

.. _falcon-seq-op-wait:

WAIT
----
Waits for desired number of nanoseconds, synchronous for 0x2e.

Opcode:
        0x13
        0x2e
Parameters:
        1
Operation:
    ::

        if(op == 0x2e)
                mmrd(0);
        call_timer_wait_nf(param[0]);

.. _falcon-seq-op-wait-status:

WAIT STATUS
-----------
Shifts val_ret left by 1 position, and waits until a status bit is set/unset. Sets flag_eq and the LSB of val_ret on success. The second parameter contains the timeout.The first parameter encodes the desired status.

*Old blob*

======== ========================
param[0] Test
======== ========================
0        UNKNOWN(0x01)
1        !UNKNOWN(0x01)
2        FB_PAUSED
3        !FB_PAUSED
4        CRTC0_VBLANK
5        !CRTC0_VBLANK
6        CRTC1_VBLANK
7        !CRTC1_VBLANK
8        CRTC0_HBLANK
9        !CRTC0_HBLANK
10       CRTC1_HBLANK
11       !CRTC1_HBLANK
======== ========================

*New blob*

In newer blobs (like 337.25), bit 16 encodes negation. Bit 8:10 the status type to wait for, and where applicable bit 0 chooses the CRTC.

======== ========================
param[0] Test
======== ========================
0x0      CRTC0_VBLANK
0x1      CRTC1_VBLANK
0x100    CRTC0_HBLANK
0x101    CRTC1_HBLANK
0x300    FB_PAUSED
0x400    PGRAPH_IDLE
0x10000  !CRTC0_VBLANK
0x10001  !CRTC1_VBLANK
0x10100  !CRTC0_HBLANK
0x10101  !CRTC1_HBLANK
0x10300  !FB_PAUSED
0x10400  !PGRAPH_IDLE
======== ========================

Todo:
        Why isn't flag_eq unset on failure?
        Find out switching point from old to new format?
Opcode:
        0x14
Parameters:
        2
Operation *OLD BLOB*:
    ::

        val_ret *= 2;
        test_params[1] = param[0] & 1;
        test_params[2] = I[0x7c4];

        switch ((param[0] & ~1) - 2) {
                default:
                        test_params[0] = 0x01;
                        break;
                case 0:
                        test_params[0] = 0x04;
                        break;
                case 2:
                        test_params[0] = 0x08;
                        break;
                case 4:
                        test_params[0] = 0x20;
                        break;
                case 6:
                        test_params[0] = 0x10;
                        break;
                case 8:
                        test_params[0] = 0x40;
                        break;
        }

        if (call_timer_wait(&input_bittest, test_params, param[1])) {
                flag_eq = 1;
                val_ret |= 1;
        }
        
Operation *NEW BLOB*:
    ::

        b32 func(b32 *) *f;
        unk3ec[2] <<= 1;

        test_params[2] = 0x1f100; // 7c4
        test_params[1] = (param[0] >> 16) & 0x1;

        switch(param[0] & 0xffff) {
        case 0x0:
                test_params[0] = 0x8;
                f = &input_test
                break;
        case 0x1:
                test_params[0] = 0x20;
                f = &input_test
                break;
        case 0x100:
                test_params[0] = 0x10;
                f = &input_test
                break;
        case 0x101:
                test_params[0] = 0x40;
                f = &input_test
                break;
        case 0x300:
                test_params[0] = 0x04;
                f = &input_test
                break;
        case 0x400:
                test_params[0] = 0x400;
                f = &pgraph_test;
                break;
        default:
                f = NULL;
                break;
        }

        if(f && timer_wait(f, param, timeout) != 0) {
                unk3e8 = 1;
                unk3ec[2] |= 1;
        }

.. _falcon-seq-op-wait-bitmask-last:

WAIT BITMASK last
-----------------
Shifts val_ret left by 1 position, and waits until the AND operation of the register pointed in reg_last and the first parameter equals val_last. Sets flag_eq and the LSB of val_ret on success. The first parameter encodes the bitmask to test. The second parameter contains the timeout.

Todo:
        Why isn't flag_eq unset on failure?

Opcode:
        0x15
Parameters:
        2
Operation:
    ::

        b32 seq_cb_wait(b32 parm) {
                return (mmrd(last_reg) & parm) == last_val;
        }

        val_ret *= 2;
        if (call_timer_wait(seq_cb_wait, param[0], param[1]))
                break;

        val_ret |= 1;
        flag_eq = 1;

.. _falcon-seq-op-irq-disable:

IRQ_DISABLE
-----------
Disable IRQs, increment reference counter ``irqlock_lvl``

Opcode:
        0x1f
Parameters:
        1
Operation:
    ::

        interrupt_enable_0 = interrupt_enable_1 = false;
        irqlock_lvl++;

.. _falcon-seq-op-irq-enable:

IRQ_ENABLE
----------
Decrement reference counter ``irqlock_lvl``, enable IRQs if 0.

Opcode:
        0x1f
Parameters:
        1
Operation:
    ::

        if(!irqlock_lvl--)
                interrupt_enable_0 = interrupt_enable_1 = true;
        
.. _falcon-seq-op-fb:

FB PAUSE/RESUME
-----------------------
If parameter 1, disable IRQs on PDAEMON and pause framebuffer (memory),
otherwise resume FB and enable IRQs.

Opcode:
        0x20
Parameters:
        1
Operation:
    ::

        if (param[0]) {
                IRQ_DISABLE;
        
                /* XXX What does this bit do? */
                mmwrs(0x1610, (mmrd(0x1610) & ~3) | 2);
                mmrd(0x1610);

                mmwrs(0x1314, (mmrd(0x1314) & ~0x10001) | 0x10001);

                /* RNN:PDAEMON.INPUT0_STATUS.FB_PAUSED */
                while (!(RD(0x7c4) & 4));
                
                mmwr_seq = &mmwr_unlocked;
        } else {
                mmwrs(0x1314, mmrd(0x1314) & ~0x10001);

                while (RD(0x7c4) & 4);

                mmwrs(0x1610, mmrd(0x1610) & ~0x33);
                IRQ_ENABLE;
                
                mmwr_seq = &mmwrs;
        }

.. _falcon-seq-op-reg-set:
