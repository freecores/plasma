VHD = mips_pack alu bus_mux control mem_ctrl mult pc_next reg_bank shifter \
		mips_cpu

all: work/tbench/_primary.dat 

work/mips_pack/_primary.dat: mips_pack.vhd
	vcom -check_synthesis mips_pack.vhd

work/alu/_primary.dat: mips_pack.vhd alu.vhd
	vcom -check_synthesis alu.vhd

work/bus_mux/_primary.dat: mips_pack.vhd bus_mux.vhd
	vcom -check_synthesis bus_mux.vhd

work/control/_primary.dat: mips_pack.vhd control.vhd
	vcom -check_synthesis control.vhd

work/mem_ctrl/_primary.dat: mips_pack.vhd mem_ctrl.vhd
	vcom -check_synthesis mem_ctrl.vhd

work/mult/_primary.dat: mips_pack.vhd mult.vhd
	vcom -check_synthesis mult.vhd

work/pc_next/_primary.dat: mips_pack.vhd pc_next.vhd
	vcom -check_synthesis pc_next.vhd

work/reg_bank/_primary.dat: mips_pack.vhd reg_bank.vhd
	vcom -check_synthesis reg_bank.vhd

work/shifter/_primary.dat: mips_pack.vhd shifter.vhd
	vcom -check_synthesis shifter.vhd

work/mips_cpu/_primary.dat: mips_cpu.vhd \
	work/mips_pack/_primary.dat \
   work/alu/_primary.dat \
	work/bus_mux/_primary.dat \
	work/control/_primary.dat \
	work/mem_ctrl/_primary.dat \
	work/mult/_primary.dat \
	work/pc_next/_primary.dat \
	work/reg_bank/_primary.dat \
	work/shifter/_primary.dat
	vcom -check_synthesis mips_cpu.vhd

work/ram/_primary.dat: ram.vhd
	vcom -explicit ram.vhd

work/tbench/_primary.dat: tbench.vhd \
	work/mips_cpu/_primary.dat \
	work/ram/_primary.dat
	vcom tbench.vhd

