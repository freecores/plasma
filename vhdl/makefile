#Makefile for Plasma

#for ModelSim
#WORK_DIR = work
#DEP_FILE = _primary.dat
#COMPILE = vcom -check_synthesis

#for FREE VHDL simulator http://www.symphonyeda.com 
WORK_DIR = work.sym
DEP_FILE = prim.dep
COMPILE = vhdlp -s

all: $(WORK_DIR)/tbench/$(DEP_FILE) 

run: all
	-@echo off > output.txt
	-@echo on
	-@start tail -f output.txt
	vhdle -t 30ms tbench
	-@diff output.txt ../tools/output.txt

opcodes: all
	gmake -C ..\tools opcodes
	vhdle -t 200us tbench
	@type output.txt|more

simulate: all
	vhdle -s -t 40us tbench -do simili.cmd -list trace.txt
	-@..\tools\tracehex.exe
	-@start ed trace2.txt

$(WORK_DIR)/lpm_pack/$(DEP_FILE): lpm_pack.vhd
	$(COMPILE) lpm_pack.vhd

$(WORK_DIR)/lpm_model/$(DEP_FILE): lpm_model.vhd
	$(COMPILE) -87 lpm_model.vhd

$(WORK_DIR)/mlite_pack/$(DEP_FILE): mlite_pack.vhd
	$(COMPILE) mlite_pack.vhd

$(WORK_DIR)/alu/$(DEP_FILE): mlite_pack.vhd alu.vhd
	$(COMPILE) alu.vhd

$(WORK_DIR)/bus_mux/$(DEP_FILE): mlite_pack.vhd bus_mux.vhd
	$(COMPILE) bus_mux.vhd

$(WORK_DIR)/control/$(DEP_FILE): mlite_pack.vhd control.vhd
	$(COMPILE) control.vhd

$(WORK_DIR)/mem_ctrl/$(DEP_FILE): mlite_pack.vhd mem_ctrl.vhd
	$(COMPILE) mem_ctrl.vhd

$(WORK_DIR)/mult/$(DEP_FILE): mlite_pack.vhd mult.vhd
	$(COMPILE) mult.vhd

$(WORK_DIR)/pc_next/$(DEP_FILE): mlite_pack.vhd pc_next.vhd
	$(COMPILE) pc_next.vhd

$(WORK_DIR)/reg_bank/$(DEP_FILE): mlite_pack.vhd reg_bank.vhd 
	$(COMPILE) reg_bank.vhd

$(WORK_DIR)/shifter/$(DEP_FILE): mlite_pack.vhd shifter.vhd
	$(COMPILE) shifter.vhd

$(WORK_DIR)/mlite_cpu/$(DEP_FILE): mlite_cpu.vhd \
	$(WORK_DIR)/mlite_pack/$(DEP_FILE) \
	$(WORK_DIR)/alu/$(DEP_FILE) \
	$(WORK_DIR)/bus_mux/$(DEP_FILE) \
	$(WORK_DIR)/control/$(DEP_FILE) \
	$(WORK_DIR)/mem_ctrl/$(DEP_FILE) \
	$(WORK_DIR)/mult/$(DEP_FILE) \
	$(WORK_DIR)/pc_next/$(DEP_FILE) \
	$(WORK_DIR)/reg_bank/$(DEP_FILE) \
	$(WORK_DIR)/shifter/$(DEP_FILE)
	$(COMPILE) mlite_cpu.vhd

$(WORK_DIR)/ram/$(DEP_FILE): mlite_pack.vhd ram.vhd
	$(COMPILE) -87 ram.vhd

$(WORK_DIR)/uart/$(DEP_FILE): mlite_pack.vhd uart.vhd
	$(COMPILE) -87 uart.vhd

$(WORK_DIR)/plasma/$(DEP_FILE): mlite_pack.vhd plasma.vhd \
	$(WORK_DIR)/mlite_cpu/$(DEP_FILE) \
	$(WORK_DIR)/ram/$(DEP_FILE) \
	$(WORK_DIR)/uart/$(DEP_FILE) 
	$(COMPILE) plasma.vhd

$(WORK_DIR)/tbench/$(DEP_FILE): mlite_pack.vhd tbench.vhd \
	$(WORK_DIR)/plasma/$(DEP_FILE) 
	$(COMPILE) tbench.vhd

altera: $(WORK_DIR)/lpm_pack/$(DEP_FILE) \
	$(WORK_DIR)/lpm_model/$(DEP_FILE) 

