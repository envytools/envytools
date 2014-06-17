.. _falcon-vm:

===================
Code virtual memory
===================

.. contents::


Introduction
============

On v3+, the falcon code segment uses primitive paging/VM via simple reverse page
table. The page size is 0x100 bytes.

The physical<->virtual address mapping information is stored in hidden
TLB memory. There is one TLB cell for each physical code page, and it
specifies the virtual address corresponding to it + some flags. The flags are:

- bit 0: usable. Set if page is mapped and complete.
- bit 1: busy. Set if page is mapped, but is still being uploaded.
- bit 2: secret. Set if page contains secret code. [see :ref:`falcon-crypt`]

.. todo:: check interaction of secret / usable flags and entering/exitting auth mode

A TLB entry is considered valid if any of the three flags is set. Whenever
a virtual address is accessed, the TLBs are scanned for a valid entry
with matching virtual address. The physical page whost TLB matched is then
used to complete the access. It's an error if no page matched, or if there's
more than one match.

The number of physical pages in the code segment can be determined by looking
at :ref:`UC_CAPS register <falcon-io-uc-caps>`, bits 0-8. Number of usable
bits in virtual page index can be determined by looking at UC_CAPS2 register,
bits 16-19. Ie. valid virtual addresses of pages are
`0 .. (1 << (UC_CAPS2[16:19])) * 0x100`.

The TLBs can be modified/accessed in 6 ways:

- executing code - reads TLB corresponding to current $pc
- PTLB - looks up TLB for a given physical page
- VTLB - looks up TLB for a given virtual page
- ITLB - invalidates TLB of a given physical page
- uploading code via IO access window
- uploading code via xfer

We'll denote the flags of TLB entry of physical page i as TLB[i].flags,
and the virtual page index as TLB[i].virt.


TLB operations: PTLB, VTLB, ITLB
================================

These operations take 24-bit parameters, and except for ITLB return a 32-bit
result. They can be called from falcon microcode as instructions, or through IO
ports.

ITLB(physidx) clears the TLB entry corresponding to a specified physical
page. The page is specified as page index. ITLB, however, cannot clear pages
containing secret code - the page has to be reuploaded from scratch with
non-secret data first.

::

        void ITLB(b24 physidx) {
                if (!(TLB[physidx].flags & 4)) {
                        TLB[physidx].flags = 0;
                        TLB[physidx].virt = 0;
                }
        }

PTLB(physidx) returns the TLB of a given physical page. The format of the
result is:

- bits 0-7: 0
- bits 8-23: virtual page index
- bits 24-26: flags
- bits 27-31: 0

::

        b32 PTLB(b24 physidx) {
                return TLB[physidx].flags << 24 | TLB[physidx].virt << 8;
        }

VTLB(virtaddr) returns the TLB that covers a given virtual *address*. The
result is:

- bits 0-7: physical page index
- bits 8-23: 0
- bits 24-26: flags, ORed across all matches
- bit 30: set if >1 TLB matches [multihit error]
- bit 31: set if no TLB matches [no hit error]

::

        b32 VTLB(b24 virtaddr) {
                phys = 0;
                flags = 0;
                matches = 0;
                for (i = 0; i < UC_CAPS.CODE_PAGES; i++) {
                        if (TLB[i].flags && TLB[i].virt == (virtaddr >> 8 & ((1 << UC_CAPS2.VM_PAGES_LOG2) - 1))) {
                                flags |= TLB[i].flags;
                                phys = i;
                                matches++;
                        }
                }
                res = phys | flags << 24;
                if (matches == 0)
                        res |= 0x80000000;
                if (matches > 1)
                        res |= 0x40000000;
                return res;
        }


.. _falcon-io-tlb:

Executing TLB operations through IO
-----------------------------------

The three \*TLB operations can be executed by poking TLB_CMD register. For PTLB
and VTLB, the result will then be visible in TLB_CMD_RES register:

MMIO 0x140 / I[0x05000]: TLB_CMD
  Runs a given TLB command on write, returns last value written on read.

  - bits 0-23: Parameter to the TLB command
  - bits 24-25: TLB command to execute

    - 1: ITLB
    - 2: PTLB
    - 3: VTLB

MMIO 0x144 / I[0x05100]: TLB_CMD_RES
  Read-only, returns the result of the last PTLB or VTLB operation launched
  through TLB_CMD.


.. _falcon-isa-ptlb:
.. _falcon-isa-vtlb:

TLB readout instructions: ptlb, vtlb
------------------------------------

These instructions run the corresponding TLB readout commands and return
their results.

Instructions:
    ==== ================== ========== =========
    Name Description        Present on Subopcode
    ==== ================== ========== =========
    ptlb run PTLB operation v3+ units  2
    vtlb run VTLB operation v3+ units  3
    ==== ================== ========== =========
Instruction class:
    unsized
Operands:
    DST, SRC
Forms:
    ====== ======
    Form   Opcode
    ====== ======
    R1, R2 fe
    ====== ======
Operation:
    ::

        if (op == ptlb)
                DST = PTLB(SRC);
        else
                DST = VTLB(SRC);


.. _falcon-isa-itlb:

TLB invalidation instruction: itlb
----------------------------------

This instructions runs the ITLB command.

Instructions:
    ==== ================== ========== =========
    Name Description        Present on Subopcode
    ==== ================== ========== =========
    itlb run ITLB operation v3+ units  8
    ==== ================== ========== =========
Instruction class:
    unsized
Operands:
    SRC
Forms:
    ==== ======
    Form Opcode
    ==== ======
    R2   f9
    ==== ======
Operation:
    ::

        ITLB(SRC);


.. _falcon-trap-vm:

VM usage on code execution
==========================

Whenever instruction fetch is attempted, the VTLB operation is done on fetch
address. If it returns no-hit or multihit error, a trap is generated and the
$tstatus reason field is set to 0xa [for no-hit] or 0xb [for multihit]. Note
that, if the faulting instruction happens to cross a page bounduary and the
second page triggered a fault, the $pc register saved in $tstatus wiill not
point to the page that faulted.

If no error was triggered, flag 0 [usable] is checked. If it's set, the access
is finished using the physical page found by VTLB. If usable isn't set, but
flag 1 [busy] is set, the fetch is paused and will be retried when TLBs are
modified in any way. Otherwise, flag 2 [secret] must be the only flag set.
In this case, a switch to authenticated mode is attempted - see :ref:`falcon-crypt`
for details.


.. _falcon-io-code:

Code upload and peeking
=======================

Code can be uploaded in two ways: direct upload via a window in IO space, or
by an xfer [see :ref:`falcon-xfer`]. The IO registers relevant are:

MMIO 0x180 / I[0x06000]: CODE_INDEX
  Selects the place in code segment accessed by CODE reg.

  - bits 2-15: bits 2-15 of the physical code address to poke
  - bit 24: write autoincrement flag: if set, every write to corresponding CODE
    register increments the address by 4
  - bit 25: read autoincrement flag: like 24, but for reads
  - bit 28: secret: if set, will attempt a switch to secret lockdown on next
    CODE write attempt and will mark uploaded code as secret.
  - bit 29: secret lockdown [RO]: if set, currently in secret lockdown mode
    - CODE_INDEX cannot be modified manually until a complete page is
    uploaded and will auto-increment on CODE writes irrespective of
    write autoincrement flag. Reads will fail and won't auto-increment.
  - bit 30: secret fail [RO]: if set, entering secret lockdown failed due to
    attempt to start upload from not page aligned address.
  - bit 31: secret reset scrubber active [RO]: if set, the window isn't
    currently usable because the reset scrubber is busy.

  See :ref:`falcon-crypt` for the secret stuff.

MMIO 0x184 / I[0x06100]: CODE
  Writes execute CST(CODE_INDEX & 0xfffc, value); and increment the address
  if write autoincrement is enabled or secret lockdown is in effect. Reads
  return the contents of code segment at physical address CODE_INDEX & 0xfffc
  and increment if read autoincrement is enabled and secret lockdown is not
  in effect. Attempts to read from physical code pages with the secret flag
  will return 0xdead5ec1 instead of the real contents. The values read/written
  are 32-bit LE numbers corresponding to 4 bytes in the code segment.

MMIO 0x188 / I[0x06200]: CODE_VIRT
  Selects the virtual page index for uploaded code. The index is sampled when
  writing word 0 of each page.

CST is defined thus::

	void CST(addr, value) {
		physidx = addr >> 8;
		// if secret lockdown needed for the page, but starting from non-0 address, fail.
		if ((addr & 0xfc) != 0 && (CODE_INDEX.secret || TLB[physidx] & 4) && !CODE_INDEX.secret_lockdown)
			CODE_INDEX.secret_fail = 1;
		if (CODE_INDEX.secret_fail || CODE_INDEX.secret_scrubber_active) {
			// nothing.
		} else {
			enter_lockdown = 0;
			exit_lockdown = 0;
			if ((addr & 0xfc) == 0) {
				// if first word uploaded...
				if (CODE_INDEX.secret || TLB[physidx].flags & 4) {
					// if uploading secret code, or uploading code to replace secret code, enter lockdown
					enter_lockdown = 1;
				}
				// store virt addr
				TLB[physid].virt = CODE_VIRT;
				// clear usable flag, set busy flag
				TLB[physid].flags = 2;
				if (CODE_INDEX.secret)
					TLB[physid].flags |= 4;
			}
			code[addr] = value; // write 4 bytes to code segment
			if ((addr & 0xfc) == 0xfc) {
				// last word uploaded, page now complete.
				exit_lockdown = 1;
				// clear busy, set usable or secret
				if (CODE_INDEX.secret)
					TLB[physid].flags = 4;
				else
					TLB[physid].flags = 1;
			}
			if (CODE_INDEX.write_autoincrement || CODE_INDEX.secret_lockdown)
				addr += 4;
			if (enter_lockdown)
				CODE_INDEX.secret_lockdown = 1;
			if (exit_lockdown)
				CODE_INDEX.secret_lockdown = 0;
	}

In summary, to upload a single page of code:

1. Set CODE_INDEX to physical_addr | 0x1000000 [and | 0x10000000 if uploading secret code]
2. Set CODE_VIRT to virtual page index it should be mapped at
3. Write 0x40 words to CODE

Uploading code via xfers will set TLB[physid].virt = ext_offset >> 8 and
TLB[physid].flags = (secret ? 6 : 2) right after the xfer is started, then
set TLB[physid].flags = (secret ? 4 : 1) when it's complete. See :ref:`falcon-xfer`
for more information.
