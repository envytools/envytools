.. _falcon-isa:

===
ISA
===

This file deals with description of the ISA used by the falcon microprocessor,
which is described in :ref:`falcon-intro`.

.. contents::


Registers
=========

There are 16 32-bit GPRs, $r0-$r15. There are also a dozen or so special
registers:

.. _falcon-sr:

===== ========= ============ =========================
Index Name      Present on   Description              
===== ========= ============ =========================
$sr0  $iv0      all units    :ref:`Interrupt 0 vector <falcon-sr-iv>`
$sr1  $iv1      all units    :ref:`Interrupt 1 vector <falcon-sr-iv>`
$sr3  $tv       all units    :ref:`Trap vector <falcon-sr-tv>`
$sr4  $sp       all units    :ref:`Stack pointer <falcon-sr-sp>`
$sr5  $pc       all units    :ref:`Program counter <falcon-sr-pc>`
$sr6  $xcbase   all units    :ref:`Code xfer external base <falcon-sr-xcbase>`
$sr7  $xdbase   all units    :ref:`Data xfer external base <falcon-sr-xdbase>`
$sr8  $flags    all units    :ref:`Misc flags <falcon-sr-flags>`
$sr9  $cx       crypto units :ref:`Crypt xfer mode <falcon-sr-cx>`
$sr10 $cauth    crypto units :ref:`Crypt auth code selection <falcon-sr-cauth>`
$sr11 $xtargets all units    :ref:`Xfer port selection <falcon-sr-xtargets>`
$sr12 $tstatus  v3+ units    :ref:`Trap status <falcon-sr-tstatus>`
===== ========= ============ =========================


.. _falcon-sr-flags:

$flags register
---------------

$flags [:ref:`$sr8 <falcon-sr>`] register contains various flags controlling
the operation of the falcon microprocessor. It is split into the following
bitfields:

===== ======= ========== ===========
Bits  Name    Present on Description
===== ======= ========== ===========
0-7   $p0-$p7 all units  :ref:`General-purpose predicates <falcon-flags-p>`
8     c       all units  :ref:`Carry flag <falcon-flags-arith>`
9     o       all units  :ref:`Signed overflow flag <falcon-flags-arith>`
10    s       all units  :ref:`Sign/negative flag <falcon-flags-arith>`
11    z       all units  :ref:`Zero flag <falcon-flags-arith>`
16    ie0     all units  :ref:`Interrupt 0 enable <falcon-flags-ie>`
17    ie1     all units  :ref:`Interrupt 1 enable <falcon-flags-ie>`
18    ???     v4+ units  ???
20    is0     all units  :ref:`Interrupt 0 saved enable <falcon-flags-is>`
21    is1     all units  :ref:`Interrupt 1 saved enable <falcon-flags-is>`
22    ???     v4+ units  ???
24    ta      all units  :ref:`Trap handler active <falcon-flags-ta>`
26-28 ???     v4+ units  ???
29-31 ???     v4+ units  ???
===== ======= ========== ===========

.. todo:: figure out v4+ stuff


.. _falcon-flags-p:

$p predicates
~~~~~~~~~~~~~

$flags.p0-p7 are general-purpose single-bit flags. They can be used to store
single-bit variables. They can be set via :ref:`bset, bclr, btgl <falcon-isa-bit>`,
and :ref:`setp <falcon-isa-setp>` instructions. They can be read by
:ref:`xbit <falcon-isa-xbit>` instruction, or checked by
:ref:`sleep <falcon-isa-sleep>` and :ref:`bra <falcon-isa-bra>` instructions.



Instructions
============

Instructions have 2, 3, or 4 bytes. First byte of instruction determines its
length and format. High 2 bits of the first byte determine the instruction's
operand size; 00 means 8-bit, 01 means 16-bit, 10 means 32-bit, and 11 means
an instruction that doesn't use operand sizing. The set of available opcodes
varies greatly with the instruction format.

The subopcode can be stored in one of the following places:

- O1: subopcode goes to low 4 bits of byte 0
- O2: subopcode goes to low 4 bits of byte 1
- OL: subopcode goes to low 6 bits of byte 1
- O3: subopcode goes to low 4 bits of byte 2

The operands are denoted as follows:

- R1x: register encoded in low 4 bits of byte 1
- R2x: register encoded in high 4 bits of byte 1
- R3x: register encoded in high 4 bits of byte 2
- RxS: register used as source
- RxD: register used as destination
- RxSD: register used as both source and destination
- I8: 8-bit immediate encoded in byte 2
- I16: 16-bit immediate encoded in bytes 2 [low part] and 3 [high part]


Sized
-----

Sized opcodes are [low 6 bits of opcode]:

- 0x: O1 R2S R1S I8
- 1x: O1 R1D R2S I8
- 2x: O1 R1D R2S I16
- 30: O2 R2S I8
- 31: O2 R2S I16
- 34: O2 R2D I8
- 36: O2 R2SD I8
- 37: O2 R2SD I16
- 38: O3 R2S R1S
- 39: O3 R1D R2S
- 3a: O3 R2D R1S
- 3b: O3 R2SD R1S
- 3c: O3 R3D R2S R1S
- 3d: O2 R2SD

.. todo:: long call/branch

The subopcodes are as follows:

=========== == == == == == == == == == == == == == == === ==== ===== ====== ========== =============
Instruction 0x 1x 2x 30 31 34 36 37 38 39 3a 3b 3c 3d imm flg0 flg3+ Cycles Present on Description
=========== == == == == == == == == == == == == == == === ==== ===== ====== ========== =============
st           0                       0                 U  \-   \-         1 all units  :ref:`store <falcon-isa-st>`
st [sp]               1              1                 U  \-   \-           all units  :ref:`store <falcon-isa-st>`
cmpu                  4  4           4                 U  CZ   CZ         1 all units  :ref:`unsigned compare <falcon-isa-cmp>`
cmps                  5  5           5                 S  CZ   CZ         1 all units  :ref:`signed compare <falcon-isa-cmp>`
cmp                   6  6           6                 S  N/A  COSZ       1 v3+ units  :ref:`compare <falcon-isa-cmp>`
add             0  0           0  0           0  0     U  COSZ COSZ       1 all units  :ref:`add <falcon-isa-add>`
adc             1  1           1  1           1  1     U  COSZ COSZ       1 all units  :ref:`add with carry <falcon-isa-add>`
sub             2  2           2  2           2  2     U  COSZ COSZ       1 all units  :ref:`substract <falcon-isa-add>`
sbb             3  3           3  3           3  3     U  COSZ COSZ       1 all units  :ref:`substract with borrow <falcon-isa-add>`
shl             4              4              4  4     U  C    COSZ       1 all units  :ref:`shift left <falcon-isa-shift>`
shr             5              5              5  5     U  C    COSZ       1 all units  :ref:`shift right <falcon-isa-shift>`
sar             7              7              7  7     U  C    COSZ       1 all units  :ref:`shift right with sign <falcon-isa-shift>`
ld              8                                8     U  \-   \-         1 all units  :ref:`load <falcon-isa-ld>`
shlc            c              c              c  c     U  C    COSZ       1 all units  :ref:`shift left with carry <falcon-isa-shift>`
shrc            d              d              d  d     U  C    COSZ       1 all units  :ref:`shift right with carry <falcon-isa-shift>`
ld [sp]                     0              0           U  \-   \-           all units  :ref:`load <falcon-isa-ld>`
not                                     0           0     OSZ  OSZ        1 all units  :ref:`bitwise not <falcon-isa-unary>`
neg                                     1           1     OSZ  OSZ        1 all units  :ref:`sign negation <falcon-isa-unary>`
movf                                    2           2     OSZ  N/A        1 v0 units   :ref:`move <falcon-isa-unary>`
mov                                     2           2     N/A  \-         1 v3+ units  :ref:`move <falcon-isa-unary>`
hswap                                   3           3     OSZ  OSZ        1 all units  :ref:`swap halves <falcon-isa-unary>`
clear                                               4     \-   \-         1 all units  :ref:`set to 0 <falcon-isa-clear>`
setf                                                5     N/A  OSZ        1 v3+ units  :ref:`set flags from value <falcon-isa-setf>`
=========== == == == == == == == == == == == == == == === ==== ===== ====== ========== =============


Unsized
-------

Unsized opcodes are:

- cx: O1 R1D R2S I8
- dx: O1 R2S R1S I8
- ex: O1 R1D R2S I16
- f0: O2 R2SD I8
- f1: O2 R2SD I16
- f2: O2 R2S I8
- f4: OL I8
- f5: OL I16
- f8: O2
- f9: O2 R2S
- fa: O3 R2S R1S
- fc: O2 R2D
- fd: O3 R2SD R1S
- fe: O3 R1D R2S
- ff: O3 R3D R2S R1S

The subopcodes are as follows:

=========== == == == == == == == == == == == == == == == === ==== ===== ====== ============= ============
Instruction cx dx ex f0 f1 f2 f4 f5 f8 f9 fa fc fd fe ff imm flg0 flg3+ cycles Present on    Description
=========== == == == == == == == == == == == == == == == === ==== ===== ====== ============= ============
mulu         0     0  0  0                       0     0  U  \-   \-         1 all units     :ref:`unsigned 16x16 -> 32 multiply <falcon-isa-mul>`
muls         1     1  1  1                       1     1  S  \-   \-         1 all units     :ref:`signed 16x16 -> 32 multiply <falcon-isa-mul>`
sext         2        2                          2     2  U  SZ   SZ         1 all units     :ref:`sign extend <falcon-isa-sext>`
extrs        3     3                                   3  U  N/A  SZ         1 v3+ units     :ref:`extract signed bitfield <falcon-isa-extr>`
sethi                 3  3                                H  \-   \-         1 all units     :ref:`set high 16 bits <falcon-isa-loadimm>`
and          4     4  4  4                       4     4  U  \-   COSZ       1 all units     :ref:`bitwise and <falcon-isa-logop>`
or           5     5  5  5                       5     5  U  \-   COSZ       1 all units     :ref:`bitwise or <falcon-isa-logop>`
xor          6     6  6  6                       6     6  U  \-   COSZ       1 all units     :ref:`bitwise xor <falcon-isa-logop>`
extr         7     7                                   7  U  N/A  SZ         1 v3+ units     :ref:`extract bitfield <falcon-isa-extr>`
mov                   7  7                                S  \-   \-         1 all units     :ref:`move <falcon-isa-loadimm>`
xbit         8                                         8  U  \-   SZ         1 all units     :ref:`extract single bit <falcon-isa-xbit>`
bset                  9                          9        U  \-   \-         1 all units     :ref:`set single bit <falcon-isa-bit>`
bclr                  a                          a        U  \-   \-         1 all units     :ref:`clear single bit <falcon-isa-bit>`
btgl                  b                          b        U  \-   \-         1 all units     :ref:`toggle single bit <falcon-isa-bit>`
ins          b     b                                      U  N/A  \-         1 v3+ units     :ref:`insert bitfield <falcon-isa-ins>`
xbit[fl]              c                             c     U  \-   SZ           all units     :ref:`extract single bit <falcon-isa-xbit>`
div          c     c                                   c  U  N/A  \-     30-33 v3+ units     :ref:`divide <falcon-isa-div>`
mod          d     d                                   d  U  N/A  \-     30-33 v3+ units     :ref:`modulus <falcon-isa-div>`
???          e                                         e  U  \-   \-           all units     :ref:`??? IO port <falcon-isa-iord>`
iord         f                                         f  U  \-   \-      ~1-x all units     :ref:`read IO port <falcon-isa-iord>`
iowr            0                          0              U  \-   \-       1-x all units     :ref:`write IO port asynchronous <falcon-isa-iowr>`
iowrs           1                          1              U  N/A  \-       9-x v3+ units     :ref:`write IO port synchronous <falcon-isa-iowr>`
xcld                                       4                 \-   \-           all units     :ref:`code xfer to falcon <falcon-isa-xfer>`
xdld                                       5                 \-   \-           all units     :ref:`data xfer to falcon <falcon-isa-xfer>`
xdst                                       6                 \-   \-           all units     :ref:`data xfer from falcon <falcon-isa-xfer>`
setp                        8              8                 \-   \-           all units     :ref:`set predicate <falcon-isa-setp>`
ccmd                        c 3c 3c                          \-   \-           crypto units  :ref:`crypto coprocessor command <falcon-isa-ccmd>`
bra                           0x 0x                       S  \-   \-         5 all units     :ref:`branch relative conditional <falcon-isa-bra>`
bra                           1x 1x                       S  \-   \-         5 all units     :ref:`branch relative conditional <falcon-isa-bra>`
jmp                           20 20     4                 U  \-   \-       4-5 all units     :ref:`branch absolute <falcon-isa-jmp>`
call                          21 21     5                 U  \-   \-       4-5 all units     :ref:`call subroutine <falcon-isa-call>`
sleep                         28                          U  \-   \-        NA all units     :ref:`sleep until interrupt <falcon-isa-sleep>`
add [sp]                      30 30     1                 S  \-   \-         1 all units     :ref:`add to stack pointer <falcon-isa-addsp>`
bset[fl]                      31        9                 U  \-   \-           all units     :ref:`set single bit <falcon-isa-bit>`
bclr[fl]                      32        a                 U  \-   \-           all units     :ref:`clear single bit <falcon-isa-bit>`
btgl[fl]                      33        b                 U  \-   \-           all units     :ref:`toggle single bit <falcon-isa-bit>`
ret                                  0                       \-   \-       5-6 all units     :ref:`return from subroutine <falcon-isa-ret>`
iret                                 1                       \-   \-           all units     :ref:`return from interrupt handler <falcon-isa-iret>`
exit                                 2                       \-   \-           all units     :ref:`halt microcontroller <falcon-isa-exit>`
xdwait                               3                       \-   \-           all units     :ref:`wait for data xfer <falcon-isa-xwait>`
???                                  6                       \-   \-           all units     ???
xcwait                               7                       \-   \-           all units     :ref:`wait for code xfer <falcon-isa-xwait>`
trap 0                               8                       N/A  \-           v3+ units     :ref:`trigger software trap <falcon-isa-trap>`
trap 1                               9                       N/A  \-           v3+ units     :ref:`trigger software trap <falcon-isa-trap>`
trap 2                               a                       N/A  \-           v3+ units     :ref:`trigger software trap <falcon-isa-trap>`
trap 3                               b                       N/A  \-           v3+ units     :ref:`trigger software trap <falcon-isa-trap>`
push                                    0                    \-   \-         1 all units     :ref:`push onto stack <falcon-isa-push>`
itlb                                    8                    N/A  \-           v3+ units     :ref:`drop TLB entry <falcon-isa-itlb>`
pop                                           0              \-   \-         1 all units     :ref:`pop from stack <falcon-isa-pop>`
mov[>sr]                                            0        \-   \-           all units     :ref:`move to special register <falcon-isa-movsr>`
mov[<sr]                                            1        \-   \-           all units     :ref:`move from special register <falcon-isa-movsr>`
ptlb                                                2        N/A  \-           v3+ units     :ref:`lookup TLB by phys address <falcon-isa-ptlb>`
vtlb                                                3        N/A  \-           v3+ units     :ref:`lookup TLB by virt address <falcon-isa-vtlb>`
=========== == == == == == == == == == == == == == == == === ==== ===== ====== ============= ============


Code segment
============

falcon has separate code and data spaces. Code segment, like data segment, is
located in small piece of SRAM in the microcontroller. Its size can be
determined by looking at MMIO address falcon+0x108, bits 0-8 shifted left by 8.

Code is byte-oriented, but can only be accessed by 32-bit words from outside,
and can only be modified in 0x100-byte [page] units.

On pre-NVA3, code segment is just a flat piece of RAM, except for the per-page
secret flag. See :ref:`falcon-io-upload` for information on uploading code
and data.

On NVA3+, code segment is paged with virtual -> physical translation and needs
special handling. See :ref:`falcon-io` for details.

Code execution is started by host via MMIO from arbitrary entry point, and is
stopped either by host or by the microcode itself, see :ref:`falcon-isa-exit`,
:ref:`falcon-io-uc-ctrl`.


.. _falcon-trap-invalid-opcode:

Invalid opcode handling
=======================

When an invalid opcode is hit, $pc is unmodified and a trap is generated. On
NVA3+, $tstatus reason field is set to 8. Pre-NVA3 cards don't have $tstatus
register, but this is the only trap type they support anyway.
