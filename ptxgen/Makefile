# Mass RE tool
# - generates, compiles and disassembles large amounts of .ptx
# - greps and aggregates unknowns
# - compares against cuobjdump output
PTX = $(shell find data/ptx -name '*.ptx')
CUB = $(patsubst data/ptx/%.ptx, data/cubin/%.cubin  ,$(PTX))
FUZ = $(shell find data/cubin/fuz/*)
ASM = $(patsubst data/cubin/%.cubin, data/asm/%.asm  ,$(CUB) $(FUZ))
DMP = $(patsubst data/asm/%.asm,     data/dmp/%.dmp  ,$(ASM))
UNK = $(patsubst data/asm/%.asm,     data/unk/%.unk  ,$(ASM))


CUDA     ?= /usr/local/cuda-6.0/bin/
CUDAARCH ?= sm_35
ENVY     ?= ../../build/
ENVYARCH ?= gk110
PTXFLAGS ?= -O3

ENVYDIS   = $(ENVY)/envydis/envydis -wn -m $(ENVYARCH)
PTXAS     = $(CUDA)/ptxas $(PTXFLAGS) -arch $(CUDAARCH)
OBJDUMP   = $(CUDA)/cuobjdump -sass
HEXDUMP   = ./lamedump

all:	unks.txt asm dmp cuobjfuzz
ptx:	ptxgen
		mkdir -p data/ptx
		./ptxgen -a=$(CUDAARCH)
unks.txt : 	$(UNK)
		grep -h 'unknown' $(UNK) | ./unique > '$@'
unk:		$(UNK)
asm:		$(ASM)
cubin:		$(CUB)
dmp:		$(DMP)
$(CUB):		data/cubin/%.cubin:	data/ptx/%.ptx
			mkdir -p $(shell dirname '$@')
			$(PTXAS) '$<' -o '$@'
$(ASM):		data/asm/%.asm:		data/cubin/%.cubin
			mkdir -p $(shell dirname '$@')
			$(HEXDUMP) '$<' | $(ENVYDIS) > '$@'
$(UNK):		data/unk/%.unk:		data/asm/%.asm
			mkdir -p $(shell dirname '$@')
			- grep 'unknown' '$<' | ./unique > '$@'
$(DMP):		data/dmp/%.dmp: 	data/cubin/%.cubin
		mkdir -p $(shell dirname '$@')
		$(OBJDUMP) '$<' > '$@' 2>&1
cuobjfuzz:	cuobjfuzz.c
		$(CC) '$<' -o '$@'
clean:
	- rm -rf unknowns.txt data cuobjfuzz
