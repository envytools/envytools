/*
 * Copyright (C) 2014 Roy Spliet <rspliet@eclipso.eu>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

/**
 * SEQ script decoder
 *
 * XXX: Hook this up to envydis. Current envydis API does not allow SEQ
 * decoding for two reasons
 * 1) envydis does not seem to allow operations beyond 8 bytes.
 * 2) envydis does not support opcodes of arbitrary length, as required for 0x21
 * A possibly useful intermediate solution now is adding a main() to this file
 * that decodes an isolated script from stdin or file. I don't think we need
 * assembly support, since we implement our own PDAEMON scripting language
 * anyway.
 */

struct seq_op {
	const char *txt;
	const char *binop;
	unsigned int min_size;
};

#define OP_MOV(l) {"MOV", ":= ", l}
#define OP_OR(l)  {"OR" , "|= ", l}
#define OP_AND(l) {"AND", "&= ", l}
#define OP_ADD(l) {"ADD", "+= ", l}
#define OP_SHL(l) {"SHL", "<<=", l}
#define OP(str,l) {str , "", l}

const struct seq_op seq_ops[] = {
	/* 0x00 */
	OP_MOV(2),
	OP_MOV(2),
	OP_OR(2),
	OP_OR(2),
	OP_AND(2),
	OP_AND(2),
	OP_ADD(2),
	OP_ADD(2),
	OP_SHL(2),
	OP_SHL(2),
	OP_MOV(1),
	OP_MOV(2),
	OP_MOV(2),
	OP_MOV(1),
	OP_MOV(2),
	OP_MOV(2),
	/* 0x10 */
	OP("EXIT",1),
	OP("EXIT",1),
	OP("EXIT",1),
	OP("WAIT",2),
	OP("WAIT STATUS",3),
	OP("WAIT BITMASK",3),
	OP("EXIT",2),
	OP("CMP",2),
	OP("BRANCH EQ",2),
	OP("BRANCH NEQ",2),
	OP("BRANCH LT",2),
	OP("BRANCH GT",2),
	OP("BRANCH",2),
	OP("IRQ DISABLE",1),
	OP("IRQ ENABLE",1),
	OP_AND(2),
	/* 0x20 */
	OP("",2),
	OP_MOV(3),
	OP_MOV(2),
	OP_MOV(2),
	OP_MOV(3),
	OP_MOV(3),
	OP_MOV(2),
	OP_MOV(2),
	OP_MOV(2),
	OP_MOV(2),
	OP_ADD(3),
	OP("CMP",2),
	OP_OR(2),
	OP("DISPLAY UNK",3),
	OP("WAIT",2),
	OP("EXIT",1),
	/* 0x30 */
	OP_OR(2),
	OP_OR(2),
	OP_AND(2),
	OP_AND(2),
	OP("MOV TS",2),
	OP("MOV TS",2),
	OP("EXIT",1),
	OP("EXIT",1),
	OP("NOP",0),
	OP("EXIT",1),
	OP("EXIT",1),
	OP_ADD(2),
	OP_ADD(2),
};

const char * const seq_wait_status[] = {
	"UNK01",
	"!UNK01",
	"FB_PAUSED",
	"!FB_PAUSED",
	"CRTC0_VBLANK",
	"!CRTC0_VBLANK",
	"CRTC1_VBLANK",
	"!CRTC1_VBLANK",
	"CRTC0_HBLANK",
	"!CRTC0_HBLANK",
	"CRTC1_HBLANK",
	"!CRTC1_HBLANK",
};

#define seq_out(p,s,...) printf("%06x: "s,((p) << 2), ##__VA_ARGS__)
#define seq_out_op(p,op,s,...) seq_out(p,"%-14s"s, seq_ops[op].txt, ##__VA_ARGS__)
#define seq_outlast(p,op,val) \
	seq_out_op(p,op,"%s      %s 0x%08x\n", \
			((op) & 0x1) ? "reg_last":"val_last", \
			seq_ops[((op) >> 1)].binop, \
			(val))

/**
 * Print a SEQ script to stdout in human-readable format.
 * @param script Script to print, native endianness, in 32-bit words.
 * @param len Length of the script in 32-bit words.
 */
void
seq_print(uint32_t *script, uint32_t len)
{
	unsigned int pc, op, size;
	char *reg0;
	unsigned int reg1, i;
	const char *wait_op;

	/* Validate script, bail if invalid */
	for(pc = 0; pc < len; pc += size) {
		op = script[pc] & 0xffff;
		size = (script[pc] & 0xffff0000) >> 16;

		if(op > 0x35 || size > 0x3ff || size == 0)
			return;

		if(pc + size > len)
			return;
	}

	printf("SEQ script, size: %uB\n", len << 2);

	for(pc = 0; pc < len; pc += size) {
		op = script[pc] & 0xffff;
		size = (script[pc] & 0xffff0000) >> 16;

		if(op > 0x3c) {
			seq_out(pc,"Invalid op\n");
			return;
		}

		if(seq_ops[op].min_size > size){
			seq_out(pc,"Opcode size too small\n");
			return;
		}

		switch(op) {
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
		case 0x8:
		case 0x9:
			seq_outlast(pc,op,script[pc+1]);
			break;
		case 0xa:
		case 0xb:
		case 0xc:
			reg0 = "";
			reg1 = 0;

			if(op == 0xa || op == 0x0c) {
			        reg0 = "reg_last+";
			}

			if (op == 0xb || op == 0xc) {
				reg1 = script[pc+1];
			}
			seq_out_op(pc,op,"val_last   :=  R[%s0x%06x]\n",reg0,reg1);
			break;
		case 0xd:
		case 0xe:
		case 0xf:
			reg0 = "";
			reg1 = 0;

			if(op == 0xd || op == 0xf) {
			        reg0 = "reg_last+";
			}

			if (op == 0xe || op == 0xf) {
				reg1 = script[pc+1];
			}
			seq_out_op(pc,op,"R[%s0x%06x]   :=  val_last\n",reg0,reg1);
			break;
		case 0x13:
		case 0x2e:
			seq_out_op(pc,op,"%u ns\n",script[pc+1]);
			break;
		case 0x14:
			//seq_out_op(pc,op,"%s, %u ns\n",seq_wait_status[script[pc+1] & 0xff],script[pc+2]);
			switch(script[pc+1] & 0xffff) {
			case 0x0:
				wait_op = "CRTC0_VBLANK";
				break;
			case 0x1:
				wait_op = "CRTC1_VBLANK";
				break;
			case 0x100:
				wait_op = "CRTC0_HBLANK";
				break;
			case 0x101:
				wait_op = "CRTC1_HBLANK";
				break;
			case 0x300:
				wait_op = "FB_PAUSED   ";
				break;
			case 0x400:
				wait_op = "PGRAPH_IDLE ";
				break;
			default:
				wait_op = "(unknown)   ";
				seq_out(pc,"Invalid param %08x for op 0x14\n", script[pc+1]);
			}

			if(script[pc+1] & 0x10000) {
				seq_out_op(pc,op,"!%s , %u ns\n",wait_op,script[pc+2]);
			} else {
				seq_out_op(pc,op,"%s  , %u ns\n",wait_op,script[pc+2]);
			}
			break;
		case 0x15:
			seq_out_op(pc,op,"R[last_reg] & 0x%08x == val_last, %u ns\n",script[pc+1],script[pc+2]);
			break;
		case 0x16:
			seq_out_op(pc,op,"\n");
			break;
		case 0x17:
			seq_out_op(pc,op,"val_last, %08x\n", script[pc+1]);
			break;
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
			seq_out_op(pc,op,"0x%06x\n", script[pc+1] << 2);
			break;
		case 0x1d:
		case 0x1e:
			seq_out_op(pc,op,"\n");
			break;
		case 0x1f:
		case 0x2c:
			seq_out_op(pc,op,"val_last      %s R[0x%06x]\n", seq_ops[op].binop, script[pc+1]);
			break;
		case 0x20:
			if(script[pc+1] == 1)
				seq_out(pc,"FB PAUSE\n");
			else
				seq_out(pc,"FB RESUME\n");
			break;
		case 0x21:
			seq_out(pc,"SET REGISTERS:\n");
			for (i = 1; i < size; i += 2) {
				seq_out(pc+i,"              R[0x%06x]   :=  0x%08x\n", script[pc+i], script[pc+i+1]);
			}
			seq_out(pc+i-2,"              reg_last      :=  0x%08x\n", script[pc+i-2]);
			seq_out(pc+i-2,"              val_last      :=  0x%08x\n", script[pc+i-1]);
			break;
		case 0x22:
		case 0x30:
		case 0x32:
			seq_out_op(pc,op,"OUT[0x%x]      %s val_last\n", script[pc+1], seq_ops[op].binop);
			break;
		case 0x23:
		case 0x31:
		case 0x33:
			seq_out_op(pc,op,"OUT[OUT[0x%x]] %s val_last\n", script[pc+1], seq_ops[op].binop);
			break;
		case 0x24:
			seq_out_op(pc,op,"OUT[0x%x]      :=  %08x\n", script[pc+1], script[pc+2]);
			break;
		case 0x25:
			seq_out_op(pc,op,"OUT[OUT[0x%x]] :=  %08x\n", script[pc+1], script[pc+2]);
			break;
		case 0x26:
		case 0x3b:
			seq_out_op(pc,op,"val_last      %s OUT[0x%x]\n", seq_ops[op].binop, script[pc+1]);
			break;
		case 0x27:
		case 0x3c:
			seq_out_op(pc,op,"val_last      %s OUT[OUT[0x%x]]\n", seq_ops[op].binop, script[pc+1]);
			break;
		case 0x28:
			seq_out_op(pc,op,"reg_last      :=  OUT[0x%x]\n", script[pc+1]);
			break;
		case 0x29:
			seq_out_op(pc,op,"reg_last      :=  OUT[OUT[0x%x]]\n", script[pc+1]);
			break;
		case 0x2a:
			seq_out_op(pc,op,"OUT[0x%x]      +=  0x%08x (%d)\n", script[pc+1], script[pc+2], script[pc+2]);
			break;
		case 0x2b:
			seq_out_op(pc,op,"OUT[0x%x], 0x%08x\n", script[pc+1], script[pc+2]);
			break;
		case 0x2d:
			seq_out_op(pc,op,"CRTC%d CRTC%d\n", script[pc+1], script[pc+2]);
			break;
		case 0x34:
			seq_out_op(pc,op,"OUT[0x%x]\n", script[pc+1]);
			break;
		case 0x35:
			seq_out_op(pc,op,"OUT[OUT[0x%x]]\n", script[pc+1]);
			break;
		default:
			seq_out_op(pc,op,"\n");
			break;
		}
	}
}
