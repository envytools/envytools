.. _fifo-puller:

===============================================
Puller - handling of submitted commands by FIFO
===============================================

.. contents::


Introduction
============

PFIFO puller's job is taking methods out of the CACHE and delivering them to
the right place for execution, or executing them directly.

Methods 0-0xfc are special and executed by the puller. Methods 0x100 and up
are forwarded to the engine object currently bound to a given subchannel.
The methods are:

============= ========== ====================== ====================================== 
Method        Present on Name                   Description                           
============= ========== ====================== ====================================== 
0x0000        all        OBJECT                 :ref:`Binds an engine object <fifo-mthd-object>`
0x0008        GF100-     NOP                    :ref:`Does nothing <fifo-mthd-nop>`
0x0010        G84-       SEMAPHORE_ADDRESS_HIGH :ref:`New-style semaphore address high part <fifo-mthd-semaphore>`
0x0014        G84-       SEMAPHORE_ADDRESS_LOW  :ref:`New-style semaphore address low part <fifo-mthd-semaphore>`
0x0018        G84-       SEMAPHORE_SEQUENCE     :ref:`New-style semaphore payload <fifo-mthd-semaphore>`
0x001c        G84-       SEMAPHORE_TRIGGER      :ref:`New-style semaphore trigger <fifo-mthd-semaphore>`
0x0020        G84-       NOTIFY_INTR            :ref:`Triggers an interrupt <fifo-mthd-notify>`
0x0024        G84-       WRCACHE_FLUSH          :ref:`Flushes write post caches <fifo-mthd-flush>`
0x0028        MCP89-     ???                    ???                                   
0x002c        MCP89-     ???                    ???                                   
0x0050        NV10-      REF_CNT                :ref:`Writes the ref counter <fifo-mthd-ref>`
0x0060        NV1A:GF100 DMA_SEMAPHORE          :ref:`DMA object for semaphores <fifo-mthd-semaphore>`
0x0064        NV1A-      SEMAPHORE_OFFSET       :ref:`Old-style semaphore address <fifo-mthd-semaphore>`                                               
0x0068        NV1A-      SEMAPHORE_ACQUIRE      :ref:`Old-style semaphore acquire trigger and payload <fifo-mthd-semaphore>`
0x006c        NV1A-      SEMAPHORE_RELEASE      :ref:`Old-style semaphore release trigger and payload <fifo-mthd-semaphore>`
0x0070        GF100-     ???                    ???
0x0074        GF100-     ???                    ???
0x0078        GF100-     ???                    ???
0x007c        GF100-     ???                    ???
0x0080        NV40-      YIELD                  :ref:`Yield PFIFO - force channel switch <fifo-mthd-yield>`
0x0100:0x2000 NV1:NV4    ...                    Passed down to the engine
0x0100:0x0180 NV4:GF100  ...                    Passed down to the engine
0x0180:0x0200 NV4:GF100  ...                    Passed down to the engine, goes through RAMHT lookup
0x0200:0x2000 NV4:GF100  ...                    Passed down to the engine
0x0100:0x4000 GF100-     ...                    Passed down to the engine
============= ========== ====================== ====================================== 

.. todo:: missing the GF100+ methods


.. _fifo-ramht:

RAMHT and the FIFO objects
==========================

As has been already mentioned, each channel has 8 "subchannels" which can be
bound to engine objects. On pre-GF100 GPUs, these objects and DMA objects
are collectively known as "FIFO objects". FIFO objects and RAMHT don't exist
on GF100+ PFIFO.

The RAMHT is a big hash table that associates arbitrary 32-bit handles with
FIFO objects and engine ids. Whenever a method is mentioned to take an object
handle, it means the parameter is looked up in RAMHT. When such lookup fails
to find a match, a CACHE_ERROR(NO_HASH) error is raised.


NV4:GF100
---------

Internally, a FIFO object is a [usually small] block of data residing in
"instance memory". The instance memory is RAMIN for pre-G80 GPUs, and the
channel structure for G80+ GPUs. The first few bits of a FIFO object
determine its 'class'. Class is 8 bits on NV4:NV25, 12 bits on NV25:NV40,
16 bits on NV40:GF100.

The data associated with a handle in RAMHT consists of engine id, which
determines the object's behavior when bound to a subchannel, and its address
in RAMIN [pre-G80] or offset from channel structure start [G80+].

Apart from method 0, the engine id is ignored. The suitability of an object
for a given method is determined by reading its class and checking if it
makes sense. Most methods other than 0 expect a DMA object, although a couple
of pre-G80 graph objects have methods that expect other graph objects.

The following are commonly accepted object classes:

- 0x0002: DMA object for reading
- 0x0003: DMA object for writing
- 0x0030: NULL object - used to effectively unbind a previously bound object
- 0x003d: DMA object for reading/writing

Other object classes are engine-specific.

For more information on DMA objects, see :ref:`nv3-dmaobj`,
:ref:`nv4-dmaobj`, or :ref:`g80-dmaobj`.


NV3
---

NV3 also has RAMHT, but it's only used for engine objects. While NV3 has DMA
objects, they have to be bound manually by the kernel. Thus, they're not
mentioned in RAMHT, and the 0x180-0x1fc methods are not implemented in
hardware - they're instead trapped and emulated in software to behave like
NV4+.

NV3 also doesn't use object classes - the object type is instead a 7-bit
number encoded in RAMHT along with engine id and object address.


NV1
---

You don't want to know how NV1 RAMHT works.


Puller state
============

======= =================== ========== =====================================
type    name                GPUs       description
======= =================== ========== =====================================
b24[8]  ctx                 NV1:NV4    objects bound to subchannels
b3      last_subc           NV1:NV4    last used subchannel
b5[8]   engines             NV4+       engines bound to subchannels
b5      last_engine         NV4+       last used engine
b32     ref                 NV10+      reference counter [shared with pusher]
bool    acquire_active      NV1A+      semaphore acquire in progress
b32     acquire_timeout     NV1A+      semaphore acquire timeout
b32     acquire_timestamp   NV1A+      semaphore acquire timestamp
b32     acquire_value       NV1A+      semaphore acquire value
dmaobj  dma_semaphore       NV11:GF100 semaphore DMA object
b12/16  semaphore_offset    NV11:GF100 old-style semaphore address
bool    semaphore_off_val   G80:GF100  semaphore_offset valid
b40     semaphore_address   G84+       new-style semaphore address
b32     semaphore_sequence  G84+       new-style semaphore value
bool    acquire_source      G84:GF100  semaphore acquire address selection
bool    acquire_mode        G84+       semaphore acquire mode
======= =================== ========== =====================================

GF100 state is likely incomplete.


.. _fifo-mthd-object:

Engine objects
==============

The main purpose of the puller is relaying methods to the engines. First,
an engine object has to be bound to a subchannel using method 0. Then, all
methods >=0x100 on the subchannel will be forwarded to the relevant engine.

On pre-NV4, the bound objects' RAMHT information is stored as part of puller
state. The last used subchannel is also remembered and each time the puller
is requested to submit commands on subchannel different from the last one,
method 0 is submitted, or channel switch occurs, the information about the
object will be forwarded to the engine through its method 0. The information
about an object is 24-bit, is known as object's "context", and has the
following fields:

- bits 0-15 [NV1]: object flags
- bits 0-15 [NV3]: object address
- bits 16-22: object type
- bit 23: engine id

The context for objects is stored directly in their RAMHT entries.

On NV4+ GPUs, the puller doesn't care about bound objects - this information
is supposed to be stored by the engine itself as part of its state. The puller
only remembers what engine each subchannel is bound to. On NV4:GF100 When
method 0 is executed, the puller looks up the object in RAMHT, getting engine
id and object address in return. The engine id is remembered in puller state,
while object address is passed down to the engine for further processing.

GF100+ did away with RAMHT. Thus, method 0 now takes the object class and
engine id directly as parameters:

- bits 0-15: object class. Not used by the puller, simply passed down to the
  engine.
- bits 16-20: engine id

The list of valid engine ids can be found on :ref:`fifo-intro`. The SOFTWARE
engine is special: all methods submitted to it, explicitely or implicitely by
binding a subchannel to it, will cause a CACHE_ERROR(EMPTY_SUBCHANNEL)
interrupt. This interrupt can then be intercepted by the driver to implement
a "software object", or can be treated as an actual error and reported.

The engines run asynchronously. The puller will send them commands whenever
they have space in their input queues and won't wait for completion of a
command before sending more. However, when engines are switched [ie. puller
has to submit a command to a different engine than last used by the channel],
the puller will wait until the last used engine is done with this channel's
commands. Several special puller methods will also wait for engines to go
idle.

.. todo:: verify this on all card families.

On NV4:GF100 GPUs, methods 0x180-0x1fc are treated specially: while other
methods are forwarded directly to engine without modification, these methods
are expected to take object handles as parameters and will be looked up in
RAMHT by the puller before forwarding. Ie. the engine will get the object's
address found in RAMHT.

mthd 0x0000 / 0x000: OBJECT
 On NV1:GF100, takes the handle of the object that should be bound to the
 subchannel it was submitted on. On GF100+, it instead takes engine+class
 directly.

::

	if (gpu < NV4) {
		b24 newctx = RAMHT_LOOKUP(param);
		if (newctx & 0x800000) {
			/* engine == PGRAPH */
			if (ENGINE_CUR_CHANNEL(PGRAPH) != chan)
				ENGINE_CHANNEL_SWITCH(PGRAPH, chan);
			ENGINE_SUBMIT_MTHD(PGRAPH, subc, 0, newctx);
			ctx[subc] = newctx;
			last_subc = subc;
		} else {
			/* engine == SOFTWARE */
			while (!ENGINE_IDLE(PGRAPH))
				;
			throw CACHE_ERROR(EMPTY_SUBCHANNEL);
		}
	} else {
		/* NV4+ GPU */
		b5 engine; b16 eparam;
		if (gpu >= GF100) {
			eparam = param & 0xffff;
			engine = param >> 16 & 0x1f;
			/* XXX: behavior with more bitfields? does it forward the whole thing? */
		} else {
			engine = RAMHT_LOOKUP(param).engine;
			eparam = RAMHT_LOOKUP(param).addr;
		}
		if (engine != last_engine) {
			while (ENGINE_CUR_CHANNEL(last_engine) == chan && !ENGINE_IDLE(last_engine))
				;
		}
		if (engine == SOFTWARE) {
			throw CACHE_ERROR(EMPTY_SUBCHANNEL);
		} else {
			if (ENGINE_CUR_CHANNEL(engine) != chan)
				ENGINE_CHANNEL_SWITCH(engine, chan);
			ENGINE_SUBMIT_MTHD(engine, subc, 0, eparam);
			last_engine = engines[subc] = engine;
		}
	}

mthd 0x0100-0x3ffc / 0x040-0xfff: [forwarded to engine]

::

	if (gpu < NV4) {
		if (subc != last_subc) {
			if (ctx[subc] & 0x800000) {
				/* engine == PGRAPH */
				if (ENGINE_CUR_CHANNEL(PGRAPH) != chan)
					ENGINE_CHANNEL_SWITCH(PGRAPH, chan);
				ENGINE_SUBMIT_MTHD(PGRAPH, subc, 0, ctx[subc]);
				last_subc = subc;
			} else {
				/* engine == SOFTWARE */
				while (!ENGINE_IDLE(PGRAPH))
					;
				throw CACHE_ERROR(EMPTY_SUBCHANNEL);
			}
		}
		if (ctx[subc] & 0x800000) {
			/* engine == PGRAPH */
			if (ENGINE_CUR_CHANNEL(PGRAPH) != chan)
				ENGINE_CHANNEL_SWITCH(PGRAPH, chan);
			ENGINE_SUBMIT_MTHD(PGRAPH, subc, mthd, param);
		} else {
			/* engine == SOFTWARE */
			while (!ENGINE_IDLE(PGRAPH))
				;
			throw CACHE_ERROR(EMPTY_SUBCHANNEL);
		}
	} else {
		/* NV4+ */
		if (gpu < GF100 && mthd >= 0x180/4 && mthd < 0x200/4) {
			param = RAMHT_LOOKUP(param).addr;
		}
		if (engines[subc] != last_engine) {
			while (ENGINE_CUR_CHANNEL(last_engine) == chan && !ENGINE_IDLE(last_engine))
				;
		}
		if (engines[subc] == SOFTWARE) {
			throw CACHE_ERROR(EMPTY_SUBCHANNEL);
		} else {
			if (ENGINE_CUR_CHANNEL(engine) != chan)
				ENGINE_CHANNEL_SWITCH(engine, chan);
			ENGINE_SUBMIT_MTHD(engine, subc, mthd, param);
			last_engine = engines[subc];
		}
	}


.. todo:: verify all of the pseudocode...


Puller builtin methods
======================

.. _fifo-mthd-ref:

Syncing with host: reference counter
------------------------------------

NV10 introduced a "reference counter". It's a per-channel 32-bit register that
is writable by the puller and readable through the channel control area [see
:ref:`fifo-dma-pusher`]. It can be used to tell host which commands have already
completed: after every interesting batch of commands, add a method that will
set the ref counter to monotonically increasing values. The host code can then
read the counter from channel control area and deduce which batches are
already complete.

The method to set the reference counter is REF_CNT, and it simply sets the
ref counter to its parameter. When it's executed, it'll also wait for all
previously submitted commands to complete execution.

mthd 0x0050 / 0x014: REF_CNT [NV10:]
::

	while (ENGINE_CUR_CHANNEL(last_engine) == chan && !ENGINE_IDLE(last_engine))
		;
	ref = param;


.. _fifo-mthd-semaphore:

Semaphores
----------

NV1A PFIFO introduced a concept of "semaphores". A semaphore is a 32-bit word
located in memory. G84 also introduced "long" semaphores, which are 4-word
memory structures that include a normal semaphore word and a timestamp.

The PFIFO semaphores can be "acquired" and "released". Note that these
operations are NOT the familiar P/V semaphore operations, they're just fancy
names for "wait until value == X" and "write X".

There are two "versions" of the semaphore functionality. The "old-style"
semaphores are implemented by NV1A:GF100 GPUs. The "new-style" semaphores
are supported by G84+ GPUs. The differences are:

Old-style semaphores

- limitted addressing range: 12-bit [NV1A:G80] or 16-bit [G80:GF100] offset
  in a DMA object. Thus a special DMA object is required.
- release writes a single word
- acquire supports only "wait for value equal to X" mode

New-style semaphores

- full 40-bit addressing range
- release writes word + timestamp, ie. long semaphore
- acquire supports "wait for value equal to X" and "wait for value greater
  or equal X" modes

Semaphores have to be 4-byte aligned. All values are stored with endianness
selected by big_endian flag [NV1A:G80] or by PFIFO endianness [G80+]

On pre-GF100, both old-style semaphores and new-style semaphores use the DMA
object stored in dma_semaphore, which can be set through DMA_SEMAPHORE method.
Note that this method is buggy on pre-G80 GPUs and accepts only *write-only*
DMA objects of class 0x0002. You have to work around the bug by preparing such
DMA objects [or using a kernel that intercepts the error and does the binding
manually].

Old-style semaphores read/write the location specified in semaphore_offset,
which can be set by SEMAPHORE_OFFSET method. The offset has to be divisible
by 4 and fit in 12 bits [NV1A:G80] or 16 bits [G80:GF100]. An acquire is
triggered by using the SEMAPHORE_ACQUIRE mthd with the expected value as the
parameter - further command processing will halt until the memory location
contains the selected value. A release is triggered by using the
SEMAPHORE_RELEASE method with the value as parameter - the value will be
written into the semaphore location.

New-style semaphores use the location specified in semaphore_address, whose
low/high parts can be set through SEMAPHORE_ADDRESS_HIGH and _LOW methods.
The value for acquire/release is stored in semaphore_sequence and specified
by SEMAPHORE_SEQUENCE method. Acquire and release are triggered by using the
SEMAPHORE_TRIGGER method with the requested operation as parameter.

The new-style release operation writes the following 16-byte structure to
memory at semaphore_address:

- 0x00: [32-bit] semaphore_sequence
- 0x04: [32-bit] 0
- 0x08: [64-bit] PTIMER timestamp [see :ref:`ptimer`]

The new-style "acquire equal" operation behaves exactly like old-style
acquire, but uses semaphore_address instead of semaphore_offset and
semaphore_sequence instead of SEMAPHORE_RELEASE param. The "acquire greater
or equal" operation, instead of waiting for the semaphore value to be equal to
semaphore_sequence, it waits for value that satisfies (int32_t)(val -
semaphore_sequence) >= 0, ie. for a value that's greater or equal to
semaphore_sequence in 32-bit wrapping arithmetic. The "acquire mask" operation
waits for a value that, ANDed with semaphore_sequence, gives a non-0 result
[GF100+ only].

Failures of semaphore-related methods will trigger the SEMAPHORE error. The
SEMAPHORE error has several subtypes, depending on card generation.

NV1A:G80 SEMAPHORE error subtypes:

- 1: INVALID_OPERAND: wrong parameter to a method
- 2: INVALID_STATE: attempt to acquire/release without proper setup

G80:GF100 SEMAPHORE error subtypes:

- 1: ADDRESS_UNALIGNED: address not divisible by 4
- 2: INVALID_STATE: attempt to acquire/release without proper setup
- 3: ADDRESS_TOO_LARGE: attempt to set >40-bit address or >16-bit offset
- 4: MEM_FAULT: got VM fault when reading/writing semaphore

GF100 SEMAPHORE error subtypes:

.. todo:: figure this out

If the acquire doesn't immediately succeed, the acquire parameters are written
to puller state, and the read will be periodically retried. Further puller
processing will be blocked on current channel until acquire succeeds. Note
that, on G84+ GPUs, the retry reads are issued from SEMAPHORE_BG VM engine
instead of the PFIFO VM engine. There's also apparently a timeout, but it's
not REd yet.

.. todo:: RE timeouts

mthd 0x0060 / 0x018: DMA_SEMAPHORE [O] [NV1A:GF100]
  ::

	obj = RAMHT_LOOKUP(param).addr;
	if (gpu < G80) {
		if (OBJECT_CLASS(obj) != 2)
			throw SEMAPHORE(INVALID_OPERAND);
		if (DMAOBJ_RIGHTS(obj) != WO)
			throw SEMAPHORE(INVALID_OPERAND);
		if (!DMAOBJ_PT_PRESENT(obj))
			throw SEMAPHORE(INVALID_OPERAND);
	}
	/* G80 doesn't bother with verification */
	dma_semaphore = obj;

.. todo:: is there ANY way to make G80 reject non-DMA object classes?

mthd 0x0064 / 0x019: SEMAPHORE_OFFSET [NV1A-]
  ::

	if (gpu < G80) {
		if (param & ~0xffc)
			throw SEMAPHORE(INVALID_OPERAND);
		semaphore_offset = param;
	} else if (gpu < GF100) {
		if (param & 3)
			throw SEMAPHORE(ADDRESS_UNALIGNED);
		if (param & 0xffff0000)
			throw SEMAPHORE(ADDRESS_TOO_LARGE);
		semaphore_offset = param;
		semaphore_off_val = 1;
	} else {
		semaphore_address[0:31] = param;
	}

mthd 0x0068 / 0x01a: SEMAPHORE_ACQUIRE [NV1A-]
  ::

	if (gpu < G80 && !dma_semaphore)
		/* unbound DMA object */
		throw SEMAPHORE(INVALID_STATE);
	if (gpu >= G80 && !semaphore_off_val)
		throw SEMAPHORE(INVALID_STATE);
	b32 word;
	if (gpu < G80) {
		word = READ_DMAOBJ_32(dma_semaphore, semaphore_offset, big_endian?BE:LE);
	} else {
		try {
			word = READ_DMAOBJ_32(dma_semaphore, semaphore_offset, pfifo_endian);
		} catch (VM_FAULT) {
			throw SEMAPHORE(MEM_FAULT);
		}
	}
	if (word == param) {
		/* already done */
	} else {
		/* acquire_active will block further processing and schedule retries */
		acquire_active = 1;
		acquire_value = param;
		acquire_timestamp = ???;
		/* XXX: figure out timestamp/timeout business */
		if (gpu >= G80) {
			acquire_mode = 0;
			acquire_source = 0;
		}
	}

mthd 0x006c / 0x01b: SEMAPHORE_RELEASE [NV1A-]
  ::

	if (gpu < G80 && !dma_semaphore)
		/* unbound DMA object */
		throw SEMAPHORE(INVALID_STATE);
	if (gpu >= G80 && !semaphore_off_val)
		throw SEMAPHORE(INVALID_STATE);
	if (gpu < G80) {
		WRITE_DMAOBJ_32(dma_semaphore, semaphore_offset, param, big_endian?BE:LE);
	} else {
		try {
			WRITE_DMAOBJ_32(dma_semaphore, semaphore_offset, param, pfifo_endian);
		} catch (VM_FAULT) {
			throw SEMAPHORE(MEM_FAULT);
		}
	}

mthd 0x0010 / 0x004: SEMAPHORE_ADDRESS_HIGH [G84:]
  ::

	if (param & 0xffffff00)
		throw SEMAPHORE(ADDRESS_TOO_LARGE);
	semaphore_address[32:39] = param;

mthd 0x0014 / 0x005: SEMAPHORE_ADDRESS_LOW [G84:]
  ::

	if (param & 3)
		throw SEMAPHORE(ADDRESS_UNALIGNED);
	semaphore_address[0:31] = param;

mthd 0x0018 / 0x006: SEMAPHORE_SEQUENCE [G84:]
  ::

	semaphore_sequence = param;

mthd 0x001c / 0x007: SEMAPHORE_TRIGGER [G84:]
  bits 0-2: operation
    - 1: ACQUIRE_EQUAL
    - 2: WRITE_LONG
    - 4: ACQUIRE_GEQUAL
    - 8: ACQUIRE_MASK [GF100-]

  .. todo:: bit 12 does something on GF100?

  ::

	op = param & 7;
	b64 timestamp = PTIMER_GETTIME();
	if (param == 2) {
		if (gpu < GF100) {
			try {
				WRITE_DMAOBJ_32(dma_semaphore, semaphore_address+0x0, param, pfifo_endian);
				WRITE_DMAOBJ_32(dma_semaphore, semaphore_address+0x4, 0, pfifo_endian);
				WRITE_DMAOBJ_64(dma_semaphore, semaphore_address+0x8, timestamp, pfifo_endian);
			} catch (VM_FAULT) {
				throw SEMAPHORE(MEM_FAULT);
			}
		} else {
			WRITE_VM_32(semaphore_address+0x0, param, pfifo_endian);
			WRITE_VM_32(semaphore_address+0x4, 0, pfifo_endian);
			WRITE_VM_64(semaphore_address+0x8, timestamp, pfifo_endian);
		}
	} else {
		b32 word;
		if (gpu < GF100) {
			try {
				word = READ_DMAOBJ_32(dma_semaphore, semaphore_address, pfifo_endian);
			} catch (VM_FAULT) {
				throw SEMAPHORE(MEM_FAULT);
			}
		} else {
			word = READ_VM_32(semaphore_address, pfifo_endian);
		}
		if ((op == 1 && word == semaphore_sequence) || (op == 4 && (int32_t)(word - semaphore_sequence) >= 0) || (op == 8 && word & semaphore_sequence)) {
			/* already done */
		} else {
			/* XXX GF100 */
			acquire_source = 1;
			acquire_value = semaphore_sequence;
			acquire_timestamp = ???;
			if (op == 1) {
				acquire_active = 1;
				acquire_mode = 0;
			} else if (op == 4) {
				acquire_active = 1;
				acquire_mode = 1;
			} else {
				/* invalid combination - results in hang */
			}
		}
	}


.. _fifo-mthd-yield:
.. _fifo-mthd-notify:
.. _fifo-mthd-flush:
.. _fifo-mthd-nop:

Misc puller methods
-------------------

NV40 introduced the YIELD method which, if there are any other busy channels
at the moment, will cause PFIFO to switch to another channel immediately,
without waiting for the timeslice to expire.

mthd 0x0080 / 0x020: YIELD [NV40:]
    ::
	PFIFO_YIELD();

G84 introduced the NOTIFY_INTR method, which simply raises an interrupt that
notifies the host of its execution. It can be used for sync primitives.

mthd 0x0020 / 0x008: NOTIFY_INTR [G84:]
    ::
	PFIFO_NOTIFY_INTR();

.. todo:: check how this is reported on GF100

The G84+ WRCACHE_FLUSH method can be used to flush PFIFO's write post caches.
[see :ref:`g80-vm`]

mthd 0x0024 / 0x009: WRCACHE_FLUSH [G84:]
    ::
	VM_WRCACHE_FLUSH(PFIFO);

The GF100+ NOP method does nothing:

mthd 0x0008 / 0x002: NOP [GF100:]

    ::

	/* do nothing */
