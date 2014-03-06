.. _fifo-dma-pusher:

===============================
DMA submission to FIFOs on NV04
===============================

.. contents:: 


Introduction
============

There are two modes of DMA command submission: The NV04-style DMA mode and IB
mode.

Both of them are based on a conception of "pushbuffer": an area of memory that
user fills with commands and tells PFIFO to process. The pushbuffers are then
assembled into a "command stream" consisting of 32-bit words that make up
"commands". In NV04-style DMA mode, the pushbuffer is always read linearly and
converted directly to command stream, except when the "jump", "return", or
"call" commands are encountered. In IB mode, the jump/call/return commands are
disabled, and command stream is instead created with use of an "IB buffer".
The IB buffer is a circular buffer of (base,length) pairs describing areas of
pushbuffer that will be stitched together to create the command stream. NV04-
style mode is available on NV04:NVC0, IB mode is available on NV50+.

.. todo:: check for NV04-style mode on NVC0

In both cases, the command stream is then broken down to commands, which get
executed. For most commands, the execution consists of storing methods into
CACHE for execution by the puller.


Pusher state
============

The following data makes up the DMA pusher state:

====== ================ ===== ===========================================
type   name             cards description
====== ================ ===== ===========================================
dmaobj dma_pushbuffer   :NVC0 [#S]_ the pushbuffer and IB DMA object
b32    dma_limit        :NVC0 [#S]_ [#O]_ pushbuffer size limit
b32    dma_put          all   pushbuffer current end address
b32    dma_get          all   pushbuffer current read address
b11/12 dma_state.mthd   all   Current method
b3     dma_state.subc   all   Current subchannel
b24    dma_state.mcnt   all   Current method count
b32    dcount_shadow    NV05: number of already-processed methods in cmd
bool   dma_state.ni     NV10+ Current command's NI flag
bool   dma_state.lenp   NV50+ [#I]_ Large NI command length pending
b32    ref              NV10+ reference counter [shared with puller]
bool   subr_active      NV1A+ [#O]_ Subroutine active
b32    subr_return      NV1A+ [#O]_ subroutine return address
bool   big_endian       11:50 [#S]_ pushbuffer endian switch
bool   sli_enable       NV50+ [#S]_ SLI cond command enabled
b12    sli_mask         NV50+ [#S]_ SLI cond mask
bool   sli_active       NV40+ SLI cond currently active
bool   ib_enable        NV50+ [#S]_ IB mode enabled
bool   nonmain          NV50+ [#I]_ non-main pushbuffer active
b8     dma_put_high     NV50+ extra 8 bits for dma_put
b8     dma_put_high_rs  NV50+ dma_put_high read shadow
b8     dma_put_high_ws  NV50+ [#O]_ dma_put_high write shadow
b8     dma_get_high     NV50+ extra 8 bits for dma_get
b8     dma_get_high_rs  NV50+ dma_get_high read shadow
b32    ib_put           NV50+ [#I]_ IB current end position
b32    ib_get           NV50+ [#I]_ IB current read position
b40    ib_address       NV50+ [#S]_ [#I]_ IB address
b8     ib_order         NV50+ [#S]_ [#I]_ IB size
b32    dma_mget         NV50+ [#I]_ main pushbuffer last read address
b8     dma_mget_high    NV50+ [#I]_ extra 8 bits for dma_mget
bool   dma_mget_val     NV50+ [#I]_ dma_mget valid flag
b8     dma_mget_high_rs NV50+ [#I]_ dma_mget_high read shadow
bool   dma_mget_val_rs  NV50+ [#I]_ dma_mget_val read shadow
====== ================ ===== ===========================================

.. [#S] means that this part of state can only be modified by kernel intervention
       and is normally set just once, on channel setup.
.. [#O] means that state only applies to NV04-style mode,
.. [#I] means that state only applies to IB mode.


Errors
======

On pre-NVC0, whenever the DMA pusher encounters problems, it'll raise a
DMA_PUSHER error. There are 6 types of DMA_PUSHER errors:

== ================= ============================================
id name              reason
== ================= ============================================
1  CALL_SUBR_ACTIVE  call command while subroutine active
2  INVALID_MTHD      attempt to submit a nonexistent special method
3  RET_SUBR_INACTIVE return command while subroutine inactive
4  INVALID_CMD       invalid command
5  IB_EMPTY          attempt to submit zero-length IB entry
6  MEM_FAULT         failure to read from pushbuffer or IB
== ================= ============================================

Apart from pusher state, the following values are available on NV05+ to aid
troubleshooting:

- dma_get_jmp_shadow: value of dma_get before the last jump
- rsvd_shadow: the first word of last-read command
- data_shadow: the last-read data word

.. todo:: verify those

.. todo:: determine what happens on NVC0 on all imaginable error conditions


.. _fifo-user-mmio-dma:

Channel control area
====================

The channel control area is used to tell card about submitted pushbuffers.
The area is at least 0x1000 bytes long, though it can be longer depending
on the card generation. Everything in the area should be accessed as 32-bit
integers, like almost all of the MMIO space. The following addresses are
usable:

==== === ============= =================================================
addr R/W name          description
==== === ============= =================================================
0x40 R/W DMA_PUT       dma_put, only writable when not in IB mode
0x44  R  DMA_GET       dma_get
0x48  R  REF           ref
0x4c R/W DMA_PUT_HIGH  dma_put_high_rs/ws, only writable when not in IB
0x50 R/W ???           NVC0+ only
0x54  R  DMA_CGET      [#O]_ nv40+ only, connected to subr_return when
                       subroutine active, dma_get when inactive.
0x58  R  DMA_MGET      dma_mget
0x5c  R  DMA_MGET_HIGH dma_mget_high_rs, dma_mget_val_rs
0x60  R  DMA_GET_HIGH  dma_get_high_rs
0x88  R  IB_GET        [#I]_ ib_get
0x8c R/W IB_PUT        [#I]_ ib_put
==== === ============= =================================================

The channel control area is accessed in 32-bit chunks, but on nv50+, DMA_GET,
DMA_PUT and DMA_MGET are effectively 40-bit quantities. To prevent races, the
high parts of them have read and write shadows. When you read the address
corresponding to the low part, the whole value is atomically read. The low
part is returned as the result of the read, while the high part is copied
to the corresponding read shadow where it can be read through a second access
to the other address. DMA_PUT also has a write shadow of the high part - when
the low part address is written, it's assembled together with the write shadow
and atomically written.

To summarise, when you want to read full DMA_PUT/GET/MGET, first read the low
part, then the high part. Due to the shadows, the value thus read will be
correct. To write the full value of DMA_PUT, first write the high part, then
the low part.

Note, however, that two different threads reading these values simultanously
can interfere with each other. For this reason, the channel control area
shouldn't ever be accessed by more than one thread at once, even for reading.

On NV04:NV40 cards, the channel control area is in BAR0 at address 0x800000 +
0x10000 * channel ID. On NV40, there are two BAR0 regions with channel control
areas: the old-style is in BAR0 at 0x800000 + 0x10000 * channel ID, supports
channels 0-0x1f, can do both PIO and DMA submission, but does not
have DMA_CGET when used in DMA mode. The new-style area is in BAR0 at 0xc0000
+ 0x1000 * channel ID, supports only DMA mode, supports all channels, and has
DMA_CGET. On NV50 cards, channel 0 supports PIO mode and has channel control
area at 0x800000, while channels 1-126 support DMA mode and have channel
control areas at 0xc00000 + 0x2000 * channel ID. On NVC0, the channel control
areas are accessed through selectable addresses in BAR1 and are backed by VRAM
or host memory - see :ref:`NVC0+ PFIFO <nvc0-pfifo>` for more details.

.. todo:: check channel numbers


NV04-style mode
===============

In NV04-style mode, whenever dma_get != dma_put, the card read a 32-bit word
from the pushbuffer at the address specified by dma_get, increments dma_get
by 4, and treats the word as the next word in the command stream. dma_get
can also move through the control flow commands: jump [sets dma_get to param],
call [copies dma_get to subr_return, sets subr_active and sets dma_get to
param], and return [unsets subr_active, copies subr_return to dma_get]. The
calls and returns are only available on NV1A+ cards.

The pushbuffer is accessed through the dma_pushbuffer DMA object. On NV04, the
DMA object has to be located in PCI or AGP memory. On NV05+, any DMA object is
valid. At all times, dma_get has to be <= dma_limit. Going past the limit or
getting a VM fault when attempting to read from pushbuffer results in raising
DMA_PUSHER error of type MEM_FAULT.

On pre-NV1A cards, the word read from pushbuffer is always treated as
little-endian. On NV1A:NV50 cards, the endianness is determined by the
big_endian flag. On NV50+, the PFIFO endianness is a global switch.

.. todo:: What about NVC0?

Note that pushbuffer addresses over 0xffffffff shouldn't be used in NV04-style
mode, even on NV50 - they cannot be expressed in jump commands, dma_limit, nor
subr_return. Why dma_put writing supports it is a mystery.

The usual way to use NV04-style mode is:

1. Allocate a big circular buffer
2. [NV1A+] if you intend to use subroutines, allocate space for them and write
   them out
3. Point dma_pushbuffer to the buffer, set dma_get and dma_put to its start
4. To submit commands:

   1. If there's not enough space in the pushbuffer between dma_put and end
      to fit the command + a jump command, submit a jump-to-beginning command
      first and set DMA_PUT to buffer start.
   2. Read DMA_GET/DMA_CGET until you get a value that's out of the range
      you're going to write. If on pre-NV40 and using subroutines, discard
      DMA_GET reads that are outside of the main buffer.
   3. Write out the commands at current DMA_PUT address.
   4. Set DMA_PUT to point right after the last word of commands you wrote.


IB mode
=======

NV04-style mode, while fairly flexible, can only jump between parts of
pushbuffer between commands. IB mode decouples flow control from the command
structure by using a second "master" buffer, called the IB buffer.

The IB buffer is a circular buffer of 8-byte structures called IB entries. The
IB buffer is, like the pushbuffer, accessed through dma_pushbuffer DMA object.
The address of the IB buffer, along with its size, is normally specified on
channel creation. The size has to be a power of two and can be in range ???.

.. todo:: check the ib size range

There are two indices into the IB buffer: ib_get and ib_put. They're both in
range of 0..2^ib_order-1. Whenever no pushbuffer is being processed [dma_put
=dma_get], and there are unread entries in the IB buffer [ib_put!=ib_get],
the card will read an entry from IB buffer entry #ib_get and increment ib_get
by 1. When ib_get would reach 2^ib_order, it insteads wraps around to 0.

Failure to read IB entry due to VM fault will, like pushbuffer read fault,
cause DMA_PUSHER error of type MEM_FAULT.

The IB entry is made of two 32-bit words in PFIFO endianness. Their format is:

Word 0:

- bits 0-1: unused, should be 0
- bits 2-31: ADDRESS_LOW, bits 2-31 of pushbuffer start address

Word 1:

- bits 0-7: ADDRESS_HIGH, bits 32-39 of pushbuffer start address
- bit 8: ???
- bit 9: NOT_MAIN, "not main pushbuffer" flag
- bits 10-30: SIZE, pushbuffer size in 32-bit words
- bit 31: NO_PREFETCH (probably; use for pushbuffer data generated by the GPU)

.. todo:: figure out bit 8 some day

When an IB entry is read, the pushbuffer is prepared for reading::

    dma_get[2:39] = ADDRESS
    dma_put = dma_get + SIZE * 4
    nonmain = NOT_MAIN
    if (!nonmain) dma_mget = dma_get

Subsequently, just like in NV04-style mode, words from dma_get are read until
it reaches dma_put. When that happens, processing can move on to the next IB
entry [or pause until user sends more commands]. If the nonmain flag is not
set, dma_get is copied to dma_mget whenever it's advanced, and dma_mget_val
flag is set to 1. dma_limit is ignored in IB mode.

An attempt to submit IB entry with length zero will raise DMA_PUSHER error of
type IB_EMPTY.

The nonmain flag is meant to help with a common case where pushbuffers sent
through IB can come from two sources: a "main" big circular buffer filled with
immediately generated commands, and "external" buffers containing helper data
filled and managed through other means. DMA_MGET will then contain the address
of the current position in the "main" buffer without being affected by IB
entries pulling data from other pushbuffers. It's thus similiar to DMA_CGET's
role in NV04-style mode.


The commands - pre-NVC0 format
==============================

The command stream, as assembled by NV04-style or IB mode pushbuffer read, is
then split into individual commands. The command type is determined by its
first word. The word has to match one of the following forms:

================================ ====================================
000CCCCCCCCCCC00SSSMMMMMMMMMMM00 increasing methods     [NV04+]
0000000000000001MMMMMMMMMMMMXX00 SLI conditional    [NV40+, if enabled]
00000000000000100000000000000000 return [NV1A+, NV04-style only]
0000000000000011SSSMMMMMMMMMMM00 long non-increasing methods    [IB only]
001JJJJJJJJJJJJJJJJJJJJJJJJJJJ00 old jump   [NV04+, NV04-style only]
010CCCCCCCCCCC00SSSMMMMMMMMMMM00 non-increasing methods [NV10+]
JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ01 jump       [NV1A+, NV04-style only]
JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ10 call       [NV1A+, NV04-style only]
================================ ====================================

.. todo:: do an exhaustive scan of commands

If none of the forms matches, or if the one that matches cannot be used in
current mode, the INVALID_CMD DMA_PUSHER error is raised.


The commands
============

There are two command formats the DMA pusher can use: NV04 format and NVC0
format. All cards support the NV04 format, while only NVC0+ cards support
the NVC0 format.


NV04 method submission commands
-------------------------------

================================ ====================================
000CCCCCCCCCCC00SSSMMMMMMMMMMM00 increasing methods     [NV04+]
010CCCCCCCCCCC00SSSMMMMMMMMMMM00 non-increasing methods [NV10+]
0000000000000011SSSMMMMMMMMMMM00 long non-increasing methods    [IB only]
================================ ====================================

These three commands are used to submit methods. the MM..M field selects the
first method that will be submitted. The SSS field selects the subchannel. The
CC..C field is mthd_count and says how many words will be submitted. With the
"long non-increasing methods" command, the method count is instead contained
in low 24 bits of the next word in the pushbuffer.

The subsequent mthd_count words after the first word [or second word in case
of the long command] are the method parameters to be submitted. If command
type is increasing methods, the method number increases by 4 [ie. by 1 method]
for each submitted word. If type is non-increasing, all words are submitted
to the same method.

If sli_enable is set and sli_active is not set, the methods thus assembled
will be discarded. Otherwise, they'll be appended to the CACHE.

.. todo:: didn't mthd 0 work even if sli_active=0?

The pusher watches the submitted methods: it only passes methods 0x100+ and
methods in 0..0xfc range that the puller recognises. An attempt to submit
invalid method in 0..0xfc range will cause a DMA_PUSHER error of type
INVALID_MTHD.

.. todo:: check pusher reaction on ACQUIRE submission: pause?


NV04 control flow commands
--------------------------

================================ ====================================
001JJJJJJJJJJJJJJJJJJJJJJJJJJJ00 old jump   [NV04+]
JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ01 jump       [NV1A+]
JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ10 call       [NV1A+]
00000000000000100000000000000000 return [NV1A+]
================================ ====================================

For jumps and calls, J..JJ is bits 2-28 or 2-31 of the target address. The
remaining bits of target are forced to 0.

The jump commands simply set dma_get to the target - the next command will be
read from there. There are two commands, since NV04 originally supported only
29-bit addresses, and used high bits as command type. NV1A introduced the new
jump command that instead uses low bits as type, and allows access to full 32
bits of address range.

The call command copies dma_get to subr_return, sets subr_active to 1, and
sets dma_get to the target. If subr_active is already set before the call, the
DMA_PUSHER error of type CALL_SUBR_ACTIVE is raised.

The return command copies subr_return to dma_get and clears subr_active. If
subr_active isn't set, it instead raises DMA_PUSHER error of type
RET_SUBR_INACTIVE.


NV04 SLI conditional command
----------------------------

================================ ====================================
0000000000000001MMMMMMMMMMMMXX00 SLI conditional    [NV40+]
================================ ====================================

NV40 introduced SLI functionality. One of the associated features is the SLI
conditional command. In SLI mode, sister channels are commonly created on all
cards in SLI set using a common pushbuffer. Since most of the commands set in
SLI will be identical for all cards, this saves resources. However, some of
the commands have to be sent only to a single card, or to a subgroup of cards.
The SLI conditional can be used for that purpose.

The sli_active flag determines if methods should be accepted at the moment:
when it's set, methods will be accepted. Otherwise, they'll be ignored. SLI
conditional command takes the encoded mask, MM..M, ands it with the per-card
value of sli_mask, and sets sli_active flag to 1 if result if non-0, to 0
otherwise.

The sli_enable flag determines if the command is available. If it's not set,
the command effectively doesn't exist. Note that sli_enable and sli_mask exist
on both NV40:NV50 and NV50+, but on NV40:NV50 they have to be set uniformly
for all channels on the card, while NV50+ allows independent settings for each
channel.

The XX bits in the command are ignored.


NVC0 commands
=============

NVC0 format follows the same idea, but uses all-new command encoding.

================================ ====================================
000CCCCCCCCCCC00SSSMMMMMMMMMMMXX increasing methods [old]
000XXXXXXXXXXX01MMMMMMMMMMMMXXXX SLI conditional
000XXXXXXXXXXX10MMMMMMMMMMMMXXXX SLI user mask store [new]
000XXXXXXXXXXX11XXXXXXXXXXXXXXXX SLI conditional from user mask [new]
001CCCCCCCCCCCCCSSSXMMMMMMMMMMMM increasing methods [new]
010CCCCCCCCCCC00SSSMMMMMMMMMMMXX non-increasing methods [old]
011CCCCCCCCCCCCCSSSXMMMMMMMMMMMM non-increasing methods [new]
100VVVVVVVVVVVVVSSSXMMMMMMMMMMMM inline method [new]
101CCCCCCCCCCCCCSSSXMMMMMMMMMMMM increase-once methods [new]
110XXXXXXXXXXXXXXXXXXXXXXXXXXXXX ??? [XXX] [new]
================================ ====================================

.. todo:: check bitfield bounduaries

.. todo:: check the extra SLI bits

.. todo:: look for other forms

Increasing and non-increasing methods work like on older cards. Increase-once
methods is a new command that works like the other methods commands, but sends
the first data word to method M, second and all subsequent data words to
method M+4 [ie. the next method].

Inline method command is a single-word command that submits a single method
with a short [12-bit] parameter encoded in VV..V field.

NVC0 also did away with the INVALID_MTHD error - invalid low methods are pushed
into CACHE as usual, puller will complain about them instead when it tries to
execute them.


The pusher pseudocode - pre-NVC0
================================

::

        while(1) {
                if (dma_get != dma_put) {
                        /* pushbuffer non-empty, read a word. */
                        b32 word;
                        try {
                                if (!ib_enable && dma_get >= dma_limit)
                                        throw DMA_PUSHER(MEM_FAULT);
                                if (gpu < NV1A)
                                        word = READ_DMAOBJ_32(dma_pushbuffer, dma_get, LE);
                                else if (gpu < NV50)
                                        word = READ_DMAOBJ_32(dma_pushbuffer, dma_get, big_endian?BE:LE);
                                else
                                        word = READ_DMAOBJ_32(dma_pushbuffer, dma_get, pfifo_endian);
                                dma_get += 4;
                                if (!nonmain)
                                        dma_mget = dma_get;
                        } catch (VM_FAULT) {
                                throw DMA_PUSHER(MEM_FAULT);
                        }
                        /* now, see if we're in the middle of a command */
                        if (dma_state.lenp) {
                                /* second word of long non-inc methods command - method count */
                                dma_state.lenp = 0;
                                dma_state.mcnt = word & 0xffffff;
                        } else if (dma_state.mcnt) {
                                /* data word of methods command */
                                data_shadow = word;
                                if (!PULLER_KNOWS_MTHD(dma_state.mthd))
                                        throw DMA_PUSHER(INVALID_MTHD);
                                if (!sli_enable || sli_active) {
                                        CACHE_PUSH(dma_state.subc, dma_state.mthd, word, dma_state.ni);
                                }
                                if (!dma_state.ni)
                                        dma_state.mthd++;
                                dma_state.mcnt--;
                                dcount_shadow++;
                        } else {
                                /* no command active - this is the first word of a new one */
                                rsvd_shadow = word;
                                /* match all forms */
                                if ((word & 0xe0000003) == 0x20000000 && !ib_enable) {
                                        /* old jump */
                                        dma_get_jmp_shadow = dma_get;
                                        dma_get = word & 0x1fffffff;
                                } else if ((word & 3) == 1 && !ib_enable && gpu >= NV1A) {
                                        /* jump */
                                        dma_get_jmp_shadow = dma_get;
                                        dma_get = word & 0xfffffffc;
                                } else if ((word & 3) == 2 && !ib_enable && gpu >= NV1A) {
                                        /* call */
                                        if (subr_active)
                                                throw DMA_PUSHER(CALL_SUBR_ACTIVE);
                                        subr_return = dma_get;
                                        subr_active = 1;
                                        dma_get = word & 0xfffffffc;
                                } else if (word == 0x00020000 && !ib_enable && gpu >= NV1A) {
                                        /* return */
                                        if (!subr_active)
                                                throw DMA_PUSHER(RET_SUBR_INACTIVE);
                                        dma_get = subr_return;
                                        subr_active = 0;
                                } else if ((word & 0xe0030003) == 0) {
                                        /* increasing methods */
                                        dma_state.mthd = (word >> 2) & 0x7ff;
                                        dma_state.subc = (word >> 13) & 7;
                                        dma_state.mcnt = (word >> 18) & 0x7ff;
                                        dma_state.ni = 0;
                                        dcount_shadow = 0;
                                } else if ((word & 0xe0030003) == 0x40000000 && gpu >= NV10) {
                                        /* non-increasing methods */
                                        dma_state.mthd = (word >> 2) & 0x7ff;
                                        dma_state.subc = (word >> 13) & 7;
                                        dma_state.mcnt = (word >> 18) & 0x7ff;
                                        dma_state.ni = 1;
                                        dcount_shadow = 0;
                                } else if ((word & 0xffff0003) == 0x00030000 && ib_enable) {
                                        /* long non-increasing methods */
                                        dma_state.mthd = (word >> 2) & 0x7ff;
                                        dma_state.subc = (word >> 13) & 7;
                                        dma_state.lenp = 1;
                                        dma_state.ni = 1;
                                        dcount_shadow = 0;
                                } else if ((word & 0xffff0003) == 0x00010000 && sli_enable) {
                                        if (sli_mask & ((word >> 4) & 0xfff))
                                                sli_active = 1;
                                        else
                                                sli_active = 0;
                                } else {
                                        throw DMA_PUSHER(INVALID_CMD);
                                }
                        }
                } else if (ib_enable && ib_get != ib_put) {
                        /* current pushbuffer empty, but we have more IB entries to read */
                        b64 entry;
                        try {
                                entry_low = READ_DMAOBJ_32(dma_pushbuffer, ib_address + ib_get * 8, pfifo_endian);
                                entry_high = READ_DMAOBJ_32(dma_pushbuffer, ib_address + ib_get * 8 + 4, pfifo_endian);
                                entry = entry_high << 32 | entry_low;
                                ib_get++;
                                if (ib_get == (1 << ib_order))
                                        ib_get = 0;
                        } catch (VM_FAULT) {
                                throw DMA_PUSHER(MEM_FAULT);
                        }
                        len = entry >> 42 & 0x3fffff;
                        if (!len)
                                throw DMA_PUSHER(IB_EMPTY);
                        dma_get = entry & 0xfffffffffc;
                        dma_put = dma_get + len * 4;
                        if (entry & 1 << 41)
                                nonmain = 1;
                        else
                                nonmain = 0;
                }
                /* otherwise, pushbuffer empty and IB empty or nonexistent - nothing to do. */
        }
