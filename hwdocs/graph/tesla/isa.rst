==============
Tesla CUDA ISA
==============

.. contents::


Introduction
============

.. todo:: write me


Conventions
===========

::
    S(x): 31th bit of x for 32-bit x, 15th for 16-bit x.
    SEX(x): sign-extension of x
    ZEX(x): zero-extension of x


Instructions
============

1. Normal MOV

  [lanemask] mov b32/b16 DST SRC

  lanemask assumed 0xf for short and immediate versions.

	if (lanemask & 1 << (laneid & 3)) DST = SRC;

  Short:	0x10000000 base opcode
  		0x00008000 0: b16, 1: b32
		operands: S*DST, S*SRC1/S*SHARED

  Imm:		0x10000000 base opcode
  		0x00008000 0: b16, 1: b32
		operands: L*DST, IMM

  Long:		0x10000000 0x00000000 base opcode
  		0x00000000 0x04000000 0: b16, 1: b32
  		0x00000000 0x0003c000 lanemask
		operands: LL*DST, L*SRC1/L*SHARED

2.1. MOV from $c

  mov DST COND

  DST is 32-bit $r.

	DST = COND;

  Long:		0x00000000 0x20000000 base opcode
  		operands: LDST, COND

2.2. MOV to $c

  mov CDST SRC

  SRC is 32-bit $r. Yes, the 0x40 $c write enable flag in second word is
  actually ignored.

	CDST = SRC;

  Long:		0x00000000 0xa0000000 base opcode
  		operands: CDST, LSRC1

2.3. MOV from $a

  mov DST AREG

  DST is 32-bit $r. Setting flag normally used for autoincrement mode doesn't
  work, but still causes crash when using non-writable $a's.

	DST = AREG;

  Long:		0x00000000 0x40000000 base opcode
  		0x02000000 0x00000000 crashy flag
  		operands: LDST, AREG

2.4. SHL to $a

  shl ADST SRC SHCNT

  SRC is 32-bit $r.

	ADST = SRC << SHCNT;

  Long:		0x00000000 0xc0000000 base opcode
  		operands: ADST, LSRC1/LSHARED, HSHCNT

2.5. ADD from $a to $a

  add ADST AREG OFFS

  Like mov from $a, setting flag normally used for autoincrement mode doesn't
  work, but still causes crash when using non-writable $a's.

	ADST = AREG + OFFS;

  Long:		0xd0000000 0x20000000 base opcode
  		0x02000000 0x00000000 crashy flag
  		operands: ADST, AREG, OFFS

2.6. MOV from sreg

  mov DST physid	S=0
  mov DST clock		S=1
  mov DST sreg2		S=2
  mov DST sreg3		S=3
  mov DST pm0		S=4
  mov DST pm1		S=5
  mov DST pm2		S=6
  mov DST pm3		S=7

  DST is 32-bit $r.

	DST = SREG;

  Long:		0x00000000 0x60000000 base opcode
  		0x00000000 0x0001c000 S
  		operands: LDST

3.1. Integer ADD family

  add [sat] b32/b16 [CDST] DST SRC1 SRC2		O2=0, O1=0
  sub [sat] b32/b16 [CDST] DST SRC1 SRC2		O2=0, O1=1
  subr [sat] b32/b16 [CDST] DST SRC1 SRC2		O2=1, O1=0
  addc [sat] b32/b16 [CDST] DST SRC1 SRC2 COND		O2=1, O1=1

  All operands are 32-bit or 16-bit according to size specifier.

	b16/b32 s1, s2;
	bool c;
	switch (OP) {
		case add: s1 = SRC1, s2 = SRC2, c = 0; break;
		case sub: s1 = SRC1, s2 = ~SRC2, c = 1; break;
		case subr: s1 = ~SRC1, s2 = SRC2, c = 1; break;
		case addc: s1 = SRC1, s2 = SRC2, c = COND.C; break;
	}
	res = s1+s2+c;	// infinite precision
	CDST.C = res >> (b32 ? 32 : 16);
	res = res & (b32 ? 0xffffffff : 0xffff);
	CDST.O = (S(s1) == S(s2)) && (S(s1) != S(res));
	if (sat && CDST.O)
		if (S(res)) res = (b32 ? 0x7fffffff : 0x7fff);
		else res = (b32 ? 0x80000000 : 0x8000);
	CDST.S = S(res);
	CDST.Z = res == 0;
	DST = res;

  Short/imm:	0x20000000 base opcode
		0x10000000 O2 bit
		0x00400000 O1 bit
		0x00008000 0: b16, 1: b32
		0x00000100 sat flag
		operands: S*DST, S*SRC1/S*SHARED, S*SRC2/S*CONST/IMM, $c0

  Long:		0x20000000 0x00000000 base opcode
		0x10000000 0x00000000 O2 bit
		0x00400000 0x00000000 O1 bit
		0x00000000 0x04000000 0: b16, 1: b32
		0x00000000 0x08000000 sat flag
		operands: MCDST, LL*DST, L*SRC1/L*SHARED, L*SRC3/L*CONST3, COND

3.2. Integer short MUL

  mul [CDST] DST u16/s16 SRC1 u16/s16 SRC2

  DST is 32-bit, SRC1 and SRC2 are 16-bit.

	b32 s1, s2;
	if (src1_signed)
		s1 = SEX(SRC1);
	else
		s1 = ZEX(SRC1);
	if (src2_signed)
		s2 = SEX(SRC2);
	else
		s2 = ZEX(SRC2);
	b32 res = s1*s2;	// modulo 2^32
	CDST.O = 0;
	CDST.C = 0;
	CDST.S = S(res);
	CDST.Z = res == 0;
	DST = res;

  Short/imm:	0x40000000 base opcode
  		0x00008000 src1 is signed
  		0x00000100 src2 is signed
  		operands: SDST, SHSRC/SHSHARED, SHSRC2/SHCONST/IMM

  Long:		0x40000000 0x00000000 base opcode
  		0x00000000 0x00008000 src1 is signed
  		0x00000000 0x00004000 src2 is signed
  		operands: MCDST, LLDST, LHSRC1/LHSHARED, LHSRC2/LHCONST2

3.3. Integer 24-bit MUL

  mul [CDST] DST [high] u24/s24 SRC1 SRC2

  All operands are 32-bit.

	b48 s1, s2;
	if (signed) {
		s1 = SEX((b24)SRC1);
		s2 = SEX((b24)SRC2);
	} else {
		s1 = ZEX((b24)SRC1);
		s2 = ZEX((b24)SRC2);
	}
	b48 m = s1*s2;	// modulo 2^48
	b32 res = (high ? m >> 16 : m & 0xffffffff);
	CDST.O = 0;
	CDST.C = 0;
	CDST.S = S(res);
	CDST.Z = res == 0;
	DST = res;

  Short/imm:	0x40000000 base opcode
  		0x00008000 src are signed
  		0x00000100 high
  		operands: SDST, SSRC/SSHARED, SSRC2/SCONST/IMM

  Long:		0x40000000 0x00000000 base opcode
  		0x00000000 0x00008000 src are signed
  		0x00000000 0x00004000 high
  		operands: MCDST, LLDST, LSRC1/LSHARED, LSRC2/LCONST2

3.4. Integer MUL-ADD

  addop [CDST] DST mul u16 SRC1 SRC2 SRC3		O1=0 O2=000	S2=0 S1=0
  addop [CDST] DST mul s16 SRC1 SRC2 SRC3		O1=0 O2=001	S2=0 S1=1
  addop sat [CDST] DST mul s16 SRC1 SRC2 SRC3		O1=0 O2=010	S2=1 S1=0
  addop [CDST] DST mul u24 SRC1 SRC2 SRC3		O1=0 O2=011	S2=1 S1=1
  addop [CDST] DST mul s24 SRC1 SRC2 SRC3		O1=0 O2=100
  addop sat [CDST] DST mul s24 SRC1 SRC2 SRC3		O1=0 O2=101
  addop [CDST] DST mul high u24 SRC1 SRC2 SRC3	O1=0 O2=110
  addop [CDST] DST mul high s24 SRC1 SRC2 SRC3	O1=0 O2=111
  addop sat [CDST] DST mul high s24 SRC1 SRC2 SRC3	O1=1 O2=000

  addop is one of:

  add	O3=00	S4=0 S3=0
  sub	O3=01	S4=0 S3=1
  subr	O3=10	S4=1 S3=0
  addc	O3=11	S4=1 S3=1

  If addop is addc, insn also takes an additional COND parameter. DST and
  SRC3 are always 32-bit, SRC1 and SRC2 are 16-bit for u16/s16 variants,
  32-bit for u24/s24 variants. Only a few of the variants are encodable as
  short/immediate, and they're restricted to DST=SRC3.

	if (u24 || s24) {
		b48 s1, s2;
		if (s24) {
			s1 = SEX((b24)SRC1);
			s2 = SEX((b24)SRC2);
		} else {
			s1 = ZEX((b24)SRC1);
			s2 = ZEX((b24)SRC2);
		}
		b48 m = s1*s2;	// modulo 2^48
		b32 mres = (high ? m >> 16 : m & 0xffffffff);
	} else {
		b32 s1, s2;
		if (s16) {
			s1 = SEX(SRC1);
			s2 = SEX(SRC2);
		} else {
			s1 = ZEX(SRC1);
			s2 = ZEX(SRC2);
		}
		b32 mres = s1*s2;	// modulo 2^32
	}
	b32 s1, s2;
	bool c;
	switch (OP) {
		case add: s1 = mres, s2 = SRC3, c = 0; break;
		case sub: s1 = mres, s2 = ~SRC3, c = 1; break;
		case subr: s1 = ~mres, s2 = SRC3, c = 1; break;
		case addc: s1 = mres, s2 = SRC3, c = COND.C; break;
	}
	res = s1+s2+c;	// infinite precision
	CDST.C = res >> 32;
	res = res & 0xffffffff;
	CDST.O = (S(s1) == S(s2)) && (S(s1) != S(res));
	if (sat && CDST.O)
		if (S(res)) res = 0x7fffffff;
		else res = 0x80000000;
	CDST.S = S(res);
	CDST.Z = res == 0;
	DST = res;

  Short/imm:	0x60000000 base opcode
  		0x00000100 S1
		0x00008000 S2
		0x00400000 S3
		0x10000000 S4
		operands: SDST, S*SRC/S*SHARED, S*SRC2/S*CONST/IMM, SDST, $c0

  Long:		0x60000000 0x00000000 base opcode
  		0x10000000 0x00000000 O1
		0x00000000 0xe0000000 O2
		0x00000000 0x0c000000 O3
		operands: MCDST, LLDST, L*SRC1/L*SHARED, L*SRC2/L*CONST2, L*SRC3/L*CONST3, COND

3.5. Integer SAD

  sad [CDST] DST u16/s16/u32/s32 SRC1 SRC2 SRC3

  Short variant is restricted to DST same as SRC3. All operands are 32-bit or
  16-bit according to size specifier.

	int s1, s2; // infinite precision
	if (signed) {
		s1 = SEX(SRC1);
		s2 = SEX(SRC2);
	} else {
		s1 = ZEX(SRC1);
		s2 = ZEX(SRC2);
	}
	b32 mres = abs(s1-s2);	// modulo 2^32
	res = mres+s3;		// infinite precision
	CDST.C = res >> (b32 ? 32 : 16);
	res = res & (b32 ? 0xffffffff : 0xffff);
	CDST.O = (S(mres) == S(s3)) && (S(mres) != S(res));
	CDST.S = S(res);
	CDST.Z = res == 0;
	DST = res;

  Short:	0x50000000 base opcode
  		0x00008000 0: b16 1: b32
		0x00000100 src are signed
		operands: DST, SDST, S*SRC/S*SHARED, S*SRC2/S*CONST, SDST

  Long:		0x50000000 0x00000000 base opcode
		0x00000000 0x04000000 0: b16, 1: b32
  		0x00000000 0x08000000 src sre signed
		operands: MCDST, LLDST, L*SRC1/L*SHARED, L*SRC2/L*CONST2, L*SRC3/L*CONST3

3.6. Integer MIN/MAX

  min u16/u32/s16/s32 [CDST] DST SRC1 SRC2
  max u16/u32/s16/s32 [CDST] DST SRC1 SRC2

  All operands are 32-bit or 16-bit according to size specifier.

	if (SRC1 < SRC2) { // signed comparison for s16/s32, unsigned for u16/u32.
		res = (min ? SRC1 : SRC2);
	} else {
		res = (min ? SRC2 : SRC1);
	}
	CDST.O = 0;
	CDST.C = 0;
	CDST.S = S(res);
	CDST.Z = res == 0;
	DST = res;

  Long:		0x30000000 0x80000000 base opcode
  		0x00000000 0x20000000 0: max, 1: min
		0x00000000 0x08000000 0: u16/u32, 1: s16/s32
		0x00000000 0x04000000 0: b16, 1: b32
		operands: MCDST, LL*DST, L*SRC1/L*SHARED, L*SRC2/L*CONST2

3.7 Integer SET

  set [CDST] DST cond u16/s16/u32/s32 SRC1 SRC2

  cond can be any subset of {l, g, e}.

  All operands are 32-bit or 16-bit according to size specifier.
 
	int s1, s2; // infinite precision
	if (signed) {
		s1 = SEX(SRC1);
		s2 = SEX(SRC2);
	} else {
		s1 = ZEX(SRC1);
		s2 = ZEX(SRC2);
	}
	bool c;
	if (s1 < s2)
		c = cond.l;
	else if (s1 == s2)
		c = cond.e;
	else /* s1 > s2 */
		c = cond.g;
	if (c) {
		res = (b32?0xffffffff:0xffff);
	} else {
		res = 0;
	}
	CDST.O = 0;
	CDST.C = 0;
	CDST.S = S(res);
	CDST.Z = res == 0;
	DST = res;

  Long:		0x30000000 0x60000000 base opcode
		0x00000000 0x08000000 0: u16/u32, 1: s16/s32
		0x00000000 0x04000000 0: b16, 1: b32
		0x00000000 0x00010000 cond.g
		0x00000000 0x00008000 cond.e
		0x00000000 0x00004000 cond.l
		operands: MCDST, LL*DST, L*SRC1/L*SHARED, L*SRC2/L*CONST2

4.1. Bit operations

  and b32/b16 [CDST] DST [not] SRC1 [not] SRC2		O2=0, O1=0
  or b32/b16 [CDST] DST [not] SRC1 [not] SRC2		O2=0, O1=1
  xor b32/b16 [CDST] DST [not] SRC1 [not] SRC2		O2=1, O1=0
  mov2 b32/b16 [CDST] DST [not] SRC1 [not] SRC2		O2=1, O1=1

  Immediate forms only allows 32-bit operands, and cannot negate second op.

	s1 = (not1 ? ~SRC1 : SRC1);
	s2 = (not2 ? ~SRC2 : SRC2);
	switch (OP) {
		case and: res = s1 & s2; break;
		case or: res = s1 | s2; break;
		case xor: res = s1 ^ s2; break;
		case mov2: res = s2; break;
	}
	CDST.O = 0;
	CDST.C = 0;
	CDST.S = S(res);
	CDST.Z = res == 0;
	DST = res;

  Imm:		0xd0000000 base opcode
		0x00400000 not1
		0x00008000 O2 bit
		0x00000100 O1 bit
		operands: SDST, SSRC/SSHARED, IMM
		assumed: not2=0 and b32.

  Long:		0xd0000000 0x00000000 base opcode
		0x00000000 0x04000000 0: b16, 1: b32
		0x00000000 0x00020000 not2
		0x00000000 0x00010000 not1
		0x00000000 0x00008000 O2 bit
		0x00000000 0x00004000 O1 bit
		operands: MCDST, LL*DST, L*SRC1/L*SHARED, L*SRC2/L*CONST2

4.2. Bit shifts

  shl b16/b32 [CDST] DST SRC1 SRC2
  shl b16/b32 [CDST] DST SRC1 SHCNT
  shr u16/u32 [CDST] DST SRC1 SRC2
  shr u16/u32 [CDST] DST SRC1 SHCNT
  shr s16/s32 [CDST] DST SRC1 SRC2
  shr s16/s32 [CDST] DST SRC1 SHCNT

    All operands 16/32-bit according to size specifier, except SHCNT. Shift
    counts are always treated as unsigned, passing negative value to shl
    doesn't get you a shr.

    	int size = (b32 ? 32 : 16);
	if (shl) {
		res = SRC1 << SRC2; // infinite precision, shift count doesn't wrap.
		if (SRC2 < size) { // yes, <. So if you shift 1 left by 32 bits, you DON'T get CDST.C set. but shift 2 left by 31 bits, and it gets set just fine.
			CDST.C = (res >> size) & 1; // basically, the bit that got shifted out.
		} else {
			CDST.C = 0;
		}
		res = res & (b32 ? 0xffffffff : 0xffff);
	} else {
		res = SRC1 >> SRC2; // infinite precision, shift count doesn't wrap.
		if (signed && S(SRC1)) {
			if (SRC2 < size)
				res |= (1<<size)-(1<<(size-SRC2)); // fill out the upper bits with 1's.
			else
				res |= (1<<size)-1;
		}
		if (SRC2 < size && SRC2 > 0) {
			CDST.C = (SRC1 >> (SRC2-1)) & 1;
		} else {
			CDST.C = 0;
		}
	}
	if (SRC2 == 1) {
		CDST.O = (S(SRC1) != S(res));
	} else {
		CDST.O = 0;
	}
	CDST.S = S(res);
	CDST.Z = res == 0;
	DST = res;

  Long:		0x30000000 0xc0000000 base opcode
  		0x00000000 0x20000000 0: shl, 1: shr
		0x00000000 0x08000000 0: u16/u32, 1: s16/s32 [shr only]
		0x00000000 0x04000000 0: b16, 1: b32
		0x00000000 0x00010000 0: use SRC2, 1: use SHCNT
		operands: MCDST, LL*DST, L*SRC1/L*SHARED, L*SRC2/L*CONST2/SHCNT

5. TBD

  interp [cent] [flat] DST v[] [SRC]

    Gets interpolated FP input, optionally multiplying by a given value

  rcp f32 DST SRC
  rsqrt f32 DST SRC
  lg2 f32 DST SRC
  sin f32 DST SRC
  cos f32 DST SRC
  ex2 f32 DST SRC

    Computes a transcendential function of the argument. rcp is 1/x, rsqrt is
    1/sqrt(x). sin, cos, ex2 need arguments preprocessed by appropriate pre
    insn. rcp, rsqrt, lg2 take a float argument directly.

  presin f32 DST SRC
  preex2 f32 DST SRC

    Preprocesses a float argument for use in subsequent sin/cos or ex2
    operation, respectively.

  mov lock CDST DST s[]

    Tries to lock a word of s[] memory and load a word from it. CDST tells
    you if it was successfully locked+loaded, or no. A successfully locked
    word can't be locked by any other thread until it is unlocked.

  mov unlock s[] SRC

    Stores a word to previously-locked s[] word and unlocks it.

  PREDICATE vote any/all CDST

    This instruction doesn't use the predicate field for conditional execution,
    abusing it instead as an input argument. vote any sets CDST to true iff the
    input predicate evaluated to true in any of the warp's active threads.
    vote all sets it to true iff the predicate evaluated to true in all acive
    threads of the current warp.

  set [CDST] DST <cmpop> f32/f64 SRC1 SRC2

    Does given comparison operation on SRC1 and SRC2. DST is set to 0xffffffff
    if comparison evaluats true, 0 if it evaluates false. if used, CDST.SZ are
    set according to DST.

  min f32/f64 DST SRC1 SRC2
  max f32/f64 DST SRC1 SRC2

    Sets DST to the smaller/larger of two SRC1 operands. If one operand is NaN,
    DST is set to the non-NaN operand. If both are NaN, DST is set to NaN.

  cvt <integer dst> <integer src>
  cvt <integer rounding modifier> <integer dst> <float src>
  cvt <rounding modifier> <float dst> <integer src>
  cvt <rounding modifier> <float dst> <float src>
  cvt <integer rounding modifier> <float dst> <float src>

    Converts between formats. For integer destinations, always clamps result
    to target type range.

  add [sat] rn/rz f32 DST SRC1 SRC2

    Adds two floating point numbers together.

  mul [sat] rn/rz f32 DST SRC1 SRC2

    Multiplies two floating point numbers together

  slct b32 DST SRC1 SRC2 f32 SRC3

    Sets DST to SRC1 if SRC3 is positive or 0, to SRC2 if SRC3 negative or NaN.

  quadop f32 <op1> <op2> <op3> <op4> DST <srclane> SRC1 SRC2

    Intra-quad information exchange instruction. Mad as a hatter.
    First, SRC1 is taken from the given lane in current quad. Then
    op<currentlanenumber> is executed on it and SRC2, results get
    written to DST. ops can be add [SRC1+SRC2], sub [SRC1-SRC2],
    subr [SRC2-SRC1], mov2 [SRC2]. srclane can be at least l0, l1,
    l2, l3, and these work everywhere. If you're running in FP, looks
    like you can also use dox [use current lane number ^ 1] and doy
    [use current lane number ^ 2], but using these elsewhere results
    in always getting 0 as the result...

  add f32 DST mul SRC1 SRC2 SRC3

    A multiply-add instruction. With intermediate rounding. Nothing
    interesting. DST = SRC1 * SRC2 + SRC3;

  fma f64 DST SRC1 SRC2 SRC3

    Fused multiply-add, with no intermediate rounding.

  texauto [deriv] live/all <texargs>

    Does a texture fetch. Inputs are: x, y, z, array index, dref [skip all
    that your current sampler setup doesn't use]. x, y, z, dref are floats,
    array index is integer. If running in FP or the deriv flag is on,
    derivatives are computed based on coordinates in all threads of current
    quad. Otherwise, derivatives are assumed 0. For FP, if the live flag
    is on, the tex instruction is only run for fragments that are going to
    be actually written to the render target, ie. for ones that are inside
    the rendered primitive and haven't been discarded yet. all executes
    the tex even for non-visible fragments, which is needed if they're going
    to be used for further derivatives, explicit or implicit.

  texbias [deriv] live/all <texargs>

    Same as texauto, except takes an additional [last] float input specifying
    the LOD bias to add. Note that bias needs to be the same for all threads
    in the current quad executing the texbias insn.

  texlod live/all <texargs>

    Does a texture fetch with given coordinates and LOD. Inputs are like
    texbias, except you have explicit LOD instead of the bias. Just like
    in texbias, the LOD should be the same for all threads involved.

  texsize live/all <texargs>

    Gives you (width, height, depth, mipmap level count) in output, takes
    integer LOD parameter as its only input.

  texfetch live/all <texargs>

    A single-texel fetch. The inputs are x, y, z, index, lod, and are all
    integer.

  emit

    GP-only instruction that emits current contents of $o registers as the
    next vertex in the output primitive and clears $o for some reason.

  restart

    GP-only instruction that finishes current output primitive and starts
    a new one.

  bra <code target>

    Branches to the given place in the code. If only some subset of threads
    in the current warp executes it, one of the paths is chosen as the active
    one, and the other is suspended until the active path exits or rejoins.

  call <code target>

    Pushes address of the next insn onto the stack and branches to given place.
    Cannot be predicated.

  ret

    Returns from a called function. If there's some not-yet-returned divergent
    path on the current stack level, switches to it. Otherwise pops off the
    entry from stack, rejoins all the paths to the pre-call state, and
    continues execution from the return address on stack. Accepts predicates.

  breakaddr <code target>

    Like call, except doesn't branch anywhere, uses given operand as the
    return address, and pushes a different type of entry onto the stack.

  break
  
    Like ret, except accepts breakaddr's stack entry type, not call's.

  quadon

    Temporarily enables all threads in the current quad, even if they were
    disabled before [by diverging, exitting, or not getting started at all].
    Nesting this is probably a bad idea, and so is using any non-quadpop
    control insns while this is active. For diverged threads, the saved PC
    is unaffected by this temporal enabling.

  quadpop

    Undoes a previous quadon command.

  bar sync <barrier number>

    Waits until all threads in the block arrive at the barrier, then continues
    execution... probably... somehow...

  trap

    Causes an error, killing the program instantly.

  joinat <code target>

    The arugment is address of a future join instruction and gets pushed
    onto the stack, together with a mask of currently active threads, for
    future rejoining.

  brkpt
  
    Doesn't seem to do anything, probably generates a breakpoint when enabled
    somewhere in PGRAPH, somehow.
  
  exit

    Actually, not a separate instruction, just a modifier available on all
    long insns. Finishes thread's execution after the current insn ends.

  join

    Also a modifier. Switches to other diverged execution paths on the same
    stack level, until they've all reached the join point, then pops off the
    entry and continues execution with a rejoined path.

-------

Control instructions:

They don't seem to have short or immediate forms, nor use op2. Needs to be
checked some day.

op	tested	pred	insn
0	F	+	discard
1	*	+	bra
2	*	-	call
3	*	+	ret		
4	*	-	breakaddr
5	*	+	break
6	*	-	quadon
7	*	-	quadpop
8	C	-	bar sync
9	*	-	trap
a	*	-	joinat
b	C	?	brkpt	sm11
c	!C
d	!C
e	!C
f	!C

Short instructions:

Usual format:

0x00000003: set to 0
0x000000fc: S*DST
0x00000100: flag1
0x00007e00: S*SRC or S*SHARED
0x00008000: flag2
0x003f0000: S*SRC2 or S*CONST
0x00400000: flag3
0x00800000: use S*CONST
0x01000000: use S*SHARED
0x0e000000: addressing
0xf0000000: opcode

op	tested	insn
0	!CF
1	*	mov
2	*	add
3	*	add
4	*	mul
5	*	sad
6	*	madd
7	*	madd
8	F !C	interp
9	*	rcp
a
b	*	add f32
c	*	mul f32
d	!CF
e	*	madd f32
f	*	tex

Immediate instructions:

Looks like there are no immediate instructions with non-0 op2, but let's check
anyway.

Usual format:

0x0000000000000003: set to 1
0x00000000000000fc: S*DST
0x0000000000000100: flag1
0x0000000000007e00: S*SRC or S*SHARED
0x0000000000008000: flag2
0x00000000003f0000: IMMD, low part
0x0000000000400000: flag3
0x0000000000800000: -
0x0000000001000000: use S*SHARED
0x000000000e000000: addressing
0x00000000f0000000: opcode
0x0000000300000000: set to 3
0x0ffffffc00000000: IMMD, high part
0x1000000000000000: -
0xe000000000000000: op2

op	op2	tested	insn
0	0	!C
1	0	*	mov
2	0	*	add
3	0	*	add
4	0	*	mul
5	0	!C
6	0	*	madd
7	0	*	madd
8	0	!C
9	0	!C
a	0	!C
b	0	*	add f32
c	0	*	mul f32
d	0	*	bitop
e	0	*	madd f32
f	0	!C

All non-0 op2's were tried on CP and didn't work.


Long instructions

Mostly, destination is in L*DST or LLDST, sources are in:
 - 1: L*SRC or L*SHARED
 - 2: L*SRC2 or L*CONST2, or SHCNT for shift insns
 - 3: L*SRC3 or L*CONST3

Some insns can also set $c registers with MCDST, or read them eith COND.

Usual format:

0x0000000000000003: set to 1
0x00000000000001fc: L*DST
0x000000000000fe00: L*SRC or L*SHARED
0x00000000007f0000: L*SRC2 or L*CONST2
0x0000000000800000: use L*CONST2
0x0000000001000000: use L*CONST3
0x000000000e000000: addressing
0x00000000f0000000: opcode
0x0000000300000000: 0 normal, 1 with exit, 2 with join
0x0000000400000000: addressing
0x0000000800000000: $o DST instead of $r
0x0000003000000000: $c reg to set
0x0000004000000000: enable setting that $c.
0x00000f8000000000: predicate condition
0x0000300000000000: $c to use for predicate and/or carry input
0x001fc00000000000: L*SRC3 or L*CONST3
0x0020000000000000: use L*SHARED
0x03c0000000000000: c[] space to use
0x0c00000000000000: misc flags
0x1000000000000000: -
0xe000000000000000: op2

[op2 shifted left to be in alignment with the hexit you see]

op	op2	tested	insn
0	0	!FC	mov from a[]
0	2	*	mov from $c
0	4	*	mov from $a
0	6	*	mov from special reg
0	8	!FC
0	a	*	mov to $c
0	c	*	shl to $a
0	e	C	mov to s[]	XXX	WTF? does something on FP

1	0	*	mov
1	2	*	mov from c[]
1	4	C !F	mov from s[]	sm11
1	6	C	vote		sm12
1	8	!FC
1	a	!FC
1	c	!FC
1	e	!FC

2	0	*	add
2	2	!FC
2	4	!FC
2	6	!FC
2	8	!FC
2	a	!FC
2	c	!FC
2	e	!FC

3	0	*	add
3	2	!FC
3	4	!FC
3	6	*	set
3	8	*	max
3	a	*	min
3	c	*	shl
3	e	*	shr

4	0	*	mul
4	2	!C
4	4	!C
4	6	!C
4	8	!C
4	a	!C
4	c	!C
4	e	!C

5	0	*	sad
5	2	!FC
5	4	!FC
5	6	!FC
5	8	!FC
5	a	!FC
5	c	!FC
5	e	!FC

6	0	*	madd
6	2	*	madd
6	4	*	madd
6	6	*	madd
6	8	*	madd
6	a	*	madd
6	c	*	madd
6	e	*	madd

7	0	*	madd
7	2		XXX ??? \
7	4		XXX ??? |
7	6		XXX ??? |
7	8		XXX ??? | seem to be copies of 6/6 for some reason.
7	a		XXX ??? |
7	c		XXX ??? |
7	e		XXX ??? /

8	0	F !C	interp
8	2	!FC
8	4	!FC
8	6	!FC
8	8	!FC
8	a	!FC
8	c	!FC
8	e	!FC

9	0	*	rcp f32
9	2	!FC
9	4	*	rsqrt f32
9	6	*	lg2 f32
9	8	*	sin f32
9	a	*	cos f32
9	c	*	ex2 f32
9	e	!FC

a	*	*	cvt

b	0	*	add f32
b	2	*	add f32
b	4	!FC
b	6	*	set f32
b	8	*	max f32
b	a	*	min f32
b	c	*	pre f32
b	e	!FC

c	0	*	mul f32
c	2	!FC
c	4	*	slct f32
c	6	*	slct f32 neg
c	8	*	quadop
c	a	!FC
c	c	!FC
c	e	!FC

d	0	*	bitop
d	2	*	add from $a to $a
d	4	*	mov from l[]
d	6	*	mov to l[]
d	8	C	mov from g[]		weird format
d	a	C	mov to g[]
d	c	C	atom g[]
d	e	C	ld atom g[]

e	0	*	madd f32
e	2	*	madd f32
e	4	C	madd f64
e	6	C	add f64
e	8	C	mul f64
e	a	C	min f64
e	c	C	max f64
e	e	C	set f64

f	0	*	tex
f	2	*	tex
f	4	*	tex
f	6	*	tex
f	8	*	tex
f	a	C	XXX ??? looks like it does exactly nothing.
f	c	G !C	emit,restart
f	e	*/C	nop/pmevent
