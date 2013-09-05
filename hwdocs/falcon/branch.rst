.. _falcon-branch:

===================
Branch instructions
===================

.. contents::

.. todo:: document ljmp/lcall


Introduction
============

The flow control instructions on falcon include conditional relative branches,
unconditional absolute branches, absolute calls, and returns. Calls use the
stack in data segment for storage for return addresses [see :ref:`falcon-stack`].
The conditions available for branching are based on the low 12 bits of $flags
register:

- bits 0-7: p0-p7, general-purpose predicates
- bit 8: c, carry flag
- bit 9: o, signed overflow flag
- bit a: s, sign flag
- bit b: z, zero flag

c, o, s, z flags are automatically set by many ALU instructions, p0-p7 have
to be explicitely manipulated. See :ref:`falcon-flags-arith` for more details.

When a branching instruction is taken, the execution time is either 4 or
5 cycles. The execution time depends on the address of the next instruction to
be executed. If this instruction can be loaded in one cycle (the instruction
is contained in a single aligned 32-bit memory block in the code section),
4 cycles will be necessary. If the instruction is split in two blocks,
5 cycles will then be necessary.


.. _falcon-sr-pc:

$pc register
============

Address of the current instruction is always available through the read-only
$pc special register.


.. _falcon-branch-conventions:

Pseudocode conventions
======================

$pc is usually automatically incremented by opcode length after each
instruction - documentation for other kinds of instructions doesn't mention
it explicitely for each insn. However, due to the nature of this category
of instructions, all effects on $pc are mentioned explicitely in this file.

oplen is the length of the currently executed instruction in bytes.

See also conventions for :ref:`arithmetic <falcon-arith-conventions>` and
:ref:`<data <falcon-data-conventions>` instructions.


.. _falcon-isa-bra:

Conditional branch: bra
=======================

Branches to a given location if the condition evaluates to true. Target is
$pc-relative.

Instructions:
    ======= ========================== ========== =========
    Name    Description                Present on Subopcode
    ======= ========================== ========== =========
    bra pX  if predicate true          all units  00+X
    bra c   if carry                   all units  08
    bra b   if unsigned below          all units  08
    bra o   if overflow                all units  09
    bra s   if sign set / negative     all units  0a
    bra z   if zero                    all units  0b
    bra e   if equal                   all units  0b
    bra a   if unsigned above          all units  0c
    bra na  if not unsigned above      all units  0d
    bra be  if unsigned below or equal all units  0d
    bra     always                     all units  0e
    bra npX if predicate false         all units  10+X
    bra nc  if not carry               all units  18
    bra nb  if not unsigned below      all units  18
    bra ae  if unsigned above or equal all units  18
    bra no  if not overflow            all units  19
    bra ns  if sign unset / positive   all units  1a
    bra nz  if not zero                all units  1b
    bra ne  if not equal               all units  1b
    bra g   if signed greater          v3+ units  1c
    bra le  if signed less or equal    v3+ units  1d
    bra l   if signed less             v3+ units  1e
    bra ge  if signed greater or equal v3+ units  1f
    ======= ========================== ========== =========
Instruction class:
    unsized
Execution time:
    1 cycle if not taken, 4-5 cycles if taken
Operands:
    DIFF
Forms:
    ==== ======
    Form Opcode
    ==== ======
    I8   f4
    I16  f5
    ==== ======
Immediates:
    sign-extended
Operation:
    ::

        switch (cc) {
                case $pX: // $p0..$p7
                        cond = $flags.$pX;
                        break;
                case c:
                        cond = $flags.c;
                        break;
                case o:
                        cond = $flags.o;
                        break;
                case s:
                        cond = $flags.s;
                        break;
                case z:
                        cond = $flags.z;
                        break;
                case a:
                        cond = !$flags.c && !$flags.z;
                        break;
                case na:
                        cond = $flags.c || $flags.z;
                        break;
                case (none):
                        cond = 1;
                        break;
                case not $pX: // $p0..$p7
                        cond = !$flags.$pX;
                        break;
                case nc:
                        cond = !$flags.c;
                        break;
                case no:
                        cond = !$flags.o;
                        break;
                case ns:
                        cond = !$flags.s;
                        break;
                case nz:
                        cond = !$flags.z;
                        break;
                case g:
                        cond = !($flags.o ^ $flags.s) && !$flags.z;
                        break;
                case le:
                        cond = ($flags.o ^ $flags.s) || $flags.z;
                        break;
                case l:
                        cond = $flags.o ^ $flags.s;
                        break;
                case ge:
                        cond = !($flags.o ^ $flags.s);
                        break;
        }
        if (cond)
                $pc = $pc + DIFF;
        else
                $pc = $pc + oplen;


.. _falcon-isa-jmp:

Unconditional branch: jmp
=========================

Branches to the target. Target is specified as absolute address. Yes, the
immediate forms are pretty much redundant with the relative branch form.

Instructions:
    ==== ================================= ========================== =====================
    Name Description                       Subopcode - opcodes f4, f5 Subopcode - opcode f9
    ==== ================================= ========================== =====================
    jmp  Unconditional jump                20                         4
    ==== ================================= ========================== =====================
Instruction class:
    unsized
Execution time:
    4-5 cycles
Operands:
    TRG
Forms:
    ==== ======
    Form Opcode
    ==== ======
    I8   f4
    I16  f5
    R2   f9
    ==== ======
Immediates:
    zero-extended
Operation:
    ::

        $pc = TRG;


.. _falcon-isa-call:

Subroutine call: call
=====================

Pushes return address onto stack and branches to the target. Target is
specified as absolute address.

Instructions:
    ==== ================================= ========================== =====================
    Name Description                       Subopcode - opcodes f4, f5 Subopcode - opcode f9
    ==== ================================= ========================== =====================
    call Call a subroutine                 21                         5
    ==== ================================= ========================== =====================
Instruction class:
    unsized
Execution time:
    4-5 cycles
Operands:
    TRG
Forms:
    ==== ======
    Form Opcode
    ==== ======
    I8   f4
    I16  f5
    R2   f9
    ==== ======
Immediates:
    zero-extended
Operation:
    ::

        $sp -= 4;
        ST(32, $sp, $pc + oplen);
        $pc = TRG;


.. _falcon-isa-ret:

Subroutine return: ret
======================

Returns from a previous call.

Instructions:
    ==== ======================== =========
    Name Description              Subopcode
    ==== ======================== =========
    ret  Return from a subroutine 0
    ==== ======================== =========
Instruction class:
    unsized
Execution time:
    5-6 cycles
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

        $pc = LD(32, $sp);
        $sp += 4;
