# Mass RE tool
# - generates, compiles and disassembles large amounts of .ptx
# - greps and aggregates unknowns
PTX = $(shell find data -name '*.ptx')
CUB = $(patsubst %.ptx,%.cubin,$(PTX))
ASM = $(patsubst %.cubin,%.asm,$(CUB))
UNK = $(patsubst %.asm,%.unk,$(ASM))

HOME  = ../../..
CUDA  = $(HOME)/blob/5.0
DIS   = $(HOME)/envytools/build/envydis/envydis -wn -mgk110
PTXAS = $(CUDA)/bin/ptxas -O3 -arch sm_35
DUMP  = ./lamedump

all:    ptx
		make unks.txt

ptx:	ptxgen
		mkdir -p data
		./ptxgen
unk:	$(UNK)
asm:	$(ASM)
cub:	$(CUB)
unks.txt : $(UNK)
		grep -h 'unknown' $(UNK) | ./unique > '$@'
$(UNK):	%.unk : %.asm
		mkdir -p $(shell dirname '$@')
		- grep 'unknown' '$<' | ./unique > '$@'
$(ASM):	%.asm : %.cubin
		mkdir -p $(shell dirname '$@')
		$(DUMP) '$<' | $(DIS) > '$@'
$(CUB):	%.cubin : %.ptx
		mkdir -p $(shell dirname '$@')
		$(PTXAS) '$<' -o '$@'
clean:
		- rm -rf unknowns.txt data
