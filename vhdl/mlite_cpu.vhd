---------------------------------------------------------------------
-- TITLE: Plasma CPU core
-- AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
-- DATE CREATED: 2/15/01
-- FILENAME: mlite_cpu.vhd
-- PROJECT: Plasma CPU core
-- COPYRIGHT: Software placed into the public domain by the author.
--    Software 'as is' without warranty.  Author liable for nothing.
-- NOTE:  MIPS(tm) and MIPS I(tm) are registered trademarks of MIPS 
--    Technologies.  MIPS Technologies does not endorse and is not 
--    associated with this project.
-- DESCRIPTION:
--    Top level VHDL document that ties the nine other entities together.
--
-- Executes all MIPS I(tm) opcodes but exceptions and non-aligned
-- memory accesses.  Based on information found in:
--    "MIPS RISC Architecture" by Gerry Kane and Joe Heinrich
--    and "The Designer's Guide to VHDL" by Peter J. Ashenden
--
-- The CPU is implemented as a two or three stage pipeline.
-- An add instruction would take the following steps (see cpu.gif):
-- Stage #1:
--    1.  The "pc_next" entity passes the program counter (PC) to the 
--        "mem_ctrl" entity which fetches the opcode from memory.
-- Stage #2:
--    2.  "Mem_ctrl" passes the opcode to the "control" entity.
--    3.  "Control" converts the 32-bit opcode to a 60-bit VLWI opcode
--        and sends control signals to the other entities.
--    4.  Based on the rs_index and rt_index control signals, "reg_bank" 
--        sends the 32-bit reg_source and reg_target to "bus_mux".
--    5.  Based on the a_source and b_source control signals, "bus_mux"
--        multiplexes reg_source onto a_bus and reg_target onto b_bus.
-- Stage #3:
--    6.  Based on the alu_func control signals, "alu" adds the values
--        from a_bus and b_bus and places the result on c_bus.
--    7.  Based on the c_source control signals, "bus_bux" multiplexes
--        c_bus onto reg_dest.
--    8.  Based on the rd_index control signal, "reg_bank" saves
--        reg_dest into the correct register.
--
-- All signals are active high.  Writing to high memory where a(31)='1' 
-- takes five cycles to meet RAM address hold times.
-- Addresses with a(31)='0' are assumed to be clocked and take three cycles.
-- Here are the signals for writing a character to address 0xffff:
--
--      intr_in                        mem_pause    
--   reset_in                        mem_write      
--      clk                     mem_byte_sel        
--     ns    mem_address m_data_r m_data_w 
-- =============================================
--   3000 1 0 0 0000002C A2820000 ZZZZZZZZ 0 0 0  (0 fetch write opcode)
--   3050 0 0 0 0000002C A2820000 ZZZZZZZZ 0 0 0
--   3100 1 0 0 00000030 340A0041 ZZZZZZZZ 0 0 0  (1 execute write opcode)
--   3150 0 0 0 00000030 340A0041 ZZZZZZZZ 0 0 0
--   3200 1 0 0 00000030 340A0041 ZZZZZZZZ 0 0 0  (2 calculating address)
--   3250 0 0 0 00000030 340A0041 ZZZZZZZZ 0 0 0
--   3300 1 0 0 0000FFFF ZZZZZZZZ 6A6A6A6A 1 1 0  (3 writing value)
--   3350 0 0 0 0000FFFF ZZZZZZZZ 6A6A6A6A 1 1 0
--   3400 1 0 0 00000034 340B0042 ZZZZZZZZ 0 0 0
--   3450 0 0 0 00000034 340B0042 ZZZZZZZZ 0 0 0
--
-- Program:
-- addr     value  opcode   args
-- ===================================
-- 002c  a2820000      sb   $v0,0($s4)
-- 0030  340a0041      li   $t2,0x41
-- 0034  340b0042      li   $t3,0x42
---------------------------------------------------------------------
library ieee;
use work.mlite_pack.all;
--library ieee, mlite_lib;
--use mlite_lib.mlite_pack.all;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity mlite_cpu is
   generic(memory_type     : string  := "DEFAULT"; --DUAL_PORT_XILINX_XC4000XLA
           adder_type      : string  := "DEFAULT"; --AREA_OPTIMIZED
           mult_type       : string  := "DEFAULT"; --AREA_OPTIMIZED
           shifter_type    : string  := "DEFAULT"; --AREA_OPTIMIZED
           alu_type        : string  := "DEFAULT"; --AREA_OPTIMIZED
           pipeline_stages : natural := 3;
           accurate_timing : boolean := true);
   port(clk         : in std_logic;
        reset_in    : in std_logic;
        intr_in     : in std_logic;

        mem_address : out std_logic_vector(31 downto 0);
        mem_data_w  : out std_logic_vector(31 downto 0);
        mem_data_r  : in std_logic_vector(31 downto 0);
        mem_byte_sel: out std_logic_vector(3 downto 0); 
        mem_write   : out std_logic;
        mem_pause   : in std_logic);
end; --entity mlite_cpu

architecture logic of mlite_cpu is
   --When using a two stage pipeline "sigD <= sig".
   --When using a three stage pipeline "sigD <= sig when rising_edge(clk)",
   --  so sigD is delayed by one clock cycle.
   signal opcode         : std_logic_vector(31 downto 0);
   signal rs_index       : std_logic_vector(5 downto 0);
   signal rt_index       : std_logic_vector(5 downto 0);
   signal rd_index       : std_logic_vector(5 downto 0);
   signal rd_indexD      : std_logic_vector(5 downto 0);
   signal reg_source     : std_logic_vector(31 downto 0);
   signal reg_target     : std_logic_vector(31 downto 0);
   signal reg_dest       : std_logic_vector(31 downto 0);
   signal reg_destD      : std_logic_vector(31 downto 0);
   signal a_bus          : std_logic_vector(31 downto 0);
   signal a_busD         : std_logic_vector(31 downto 0);
   signal b_bus          : std_logic_vector(31 downto 0);
   signal b_busD         : std_logic_vector(31 downto 0);
   signal c_bus          : std_logic_vector(31 downto 0);
   signal c_alu          : std_logic_vector(31 downto 0);
   signal c_shift        : std_logic_vector(31 downto 0);
   signal c_mult         : std_logic_vector(31 downto 0);
   signal c_memory       : std_logic_vector(31 downto 0);
   signal imm            : std_logic_vector(15 downto 0);
   signal pc             : std_logic_vector(31 downto 0);
   signal pc_plus4       : std_logic_vector(31 downto 0);
   signal alu_func       : alu_function_type;
   signal alu_funcD      : alu_function_type;
   signal shift_func     : shift_function_type;
   signal shift_funcD    : shift_function_type;
   signal mult_func      : mult_function_type;
   signal mult_funcD     : mult_function_type;
   signal branch_func    : branch_function_type;
   signal take_branch    : std_logic;
   signal a_source       : a_source_type;
   signal b_source       : b_source_type;
   signal c_source       : c_source_type;
   signal pc_source      : pc_source_type;
   signal mem_source     : mem_source_type;
   signal pause_mult     : std_logic;
   signal pause_ctrl     : std_logic;
   signal pause_pipeline : std_logic;
   signal pause_any      : std_logic;
   signal pause_non_ctrl : std_logic;
   signal pause_bank     : std_logic;
   signal nullify_op     : std_logic;
   signal intr_enable    : std_logic;
   signal intr_signal    : std_logic;
   signal reset_reg      : std_logic_vector(3 downto 0);
   signal reset          : std_logic;
begin  --architecture

   pause_any <= (mem_pause or pause_ctrl) or (pause_mult or pause_pipeline);
   pause_non_ctrl <= (mem_pause or pause_mult) or pause_pipeline;
   pause_bank <= (mem_pause or pause_ctrl or pause_mult) and not pause_pipeline;
   nullify_op <= '1' when (pc_source = FROM_LBRANCH and take_branch = '0')
                          or intr_signal = '1'
                          else '0';
   c_bus <= c_alu or c_shift or c_mult;
   reset <= '1' when reset_in = '1' or reset_reg /= "1111" else '0';

   --synchronize reset and interrupt pins
   intr_proc: process(clk, reset_in, reset_reg, intr_in, intr_enable, 
      pc_source, pc, pause_any)
   begin
      if reset_in = '1' then
         reset_reg <= "0000";
         intr_signal <= '0';
      elsif rising_edge(clk) then
         if reset_reg /= "1111" then
            reset_reg <= reset_reg + 1;
         end if;

         --don't try to interrupt a multi-cycle instruction
         if pause_any = '0' then
            if intr_in = '1' and intr_enable = '1' and
                  pc_source = from_inc4 and pc(2) = '0' then
               --the epc will be backed up one opcode (pc-4)
               intr_signal <= '1';
            else
               intr_signal <= '0';
            end if;
         end if;

      end if;
   end process;

   u1_pc_next: pc_next PORT MAP (
        clk          => clk,
        reset_in     => reset,
        take_branch  => take_branch,
        pause_in     => pause_any,
        pc_new       => c_bus(31 downto 2),
        opcode25_0   => opcode(25 downto 0),
        pc_source    => pc_source,
        pc_out       => pc,
        pc_out_plus4 => pc_plus4);

   u2_mem_ctrl: mem_ctrl 
      generic map (ACCURATE_TIMING => accurate_timing)
      PORT MAP (
        clk          => clk,
        reset_in     => reset,
        pause_in     => pause_non_ctrl,
        nullify_op   => nullify_op,
        address_pc   => pc,
        opcode_out   => opcode,

        address_data => c_bus,
        mem_source   => mem_source,
        data_write   => reg_target,
        data_read    => c_memory,
        pause_out    => pause_ctrl,
        
        mem_address  => mem_address,
        mem_data_w   => mem_data_w,
        mem_data_r   => mem_data_r,
        mem_byte_sel => mem_byte_sel,
        mem_write    => mem_write);

   u3_control: control PORT MAP (
        opcode       => opcode,
        intr_signal  => intr_signal,
        rs_index     => rs_index,
        rt_index     => rt_index,
        rd_index     => rd_index,
        imm_out      => imm,
        alu_func     => alu_func,
        shift_func   => shift_func,
        mult_func    => mult_func,
        branch_func  => branch_func,
        a_source_out => a_source,
        b_source_out => b_source,
        c_source_out => c_source,
        pc_source_out=> pc_source,
        mem_source_out=> mem_source);

   u4_reg_bank: reg_bank 
      generic map(memory_type => memory_type)
      port map (
        clk            => clk,
        reset_in       => reset,
        pause          => pause_bank,
        rs_index       => rs_index,
        rt_index       => rt_index,
        rd_index       => rd_indexD,
        reg_source_out => reg_source,
        reg_target_out => reg_target,
        reg_dest_new   => reg_destD,
        intr_enable    => intr_enable);

   u5_bus_mux: bus_mux port map (
        imm_in       => imm,
        reg_source   => reg_source,
        a_mux        => a_source,
        a_out        => a_bus,

        reg_target   => reg_target,
        b_mux        => b_source,
        b_out        => b_bus,

        c_bus        => c_bus,
        c_memory     => c_memory,
        c_pc         => pc,
        c_pc_plus4   => pc_plus4,
        c_mux        => c_source,
        reg_dest_out => reg_dest,

        branch_func  => branch_func,
        take_branch  => take_branch);

   u6_alu: alu 
      generic map (adder_type => adder_type,
                   alu_type   => alu_type)
      port map (
        a_in         => a_busD,
        b_in         => b_busD,
        alu_function => alu_funcD,
        c_alu        => c_alu);

   u7_shifter: shifter
      generic map (shifter_type => shifter_type)
      port map (
        value        => b_busD,
        shift_amount => a_busD(4 downto 0),
        shift_func   => shift_funcD,
        c_shift      => c_shift);

   u8_mult: mult 
      generic map (adder_type => adder_type,
                   mult_type  => mult_type)
      port map (
        clk       => clk,
        reset_in  => reset,
        a         => a_busD,
        b         => b_busD,
        mult_func => mult_funcD,
        c_mult    => c_mult,
        pause_out => pause_mult);

   pipeline2: if pipeline_stages <= 2 generate
      a_busD <= a_bus;
      b_busD <= b_bus;
      alu_funcD <= alu_func;
      shift_funcD <= shift_func;
      mult_funcD <= mult_func;
      rd_indexD <= rd_index;

      reg_destD <= reg_dest;
      pause_pipeline <= '0';
   end generate; --pipeline2

   pipeline3: if pipeline_stages >= 3 generate
      --When operating in three stage pipeline mode, the following signals
      --are delayed by one clock cycle:  a_bus, b_bus, alu/shift/mult_func,
      --c_source, and rd_index.
   u9_pipeline: pipeline port map (
        clk            => clk,
        reset          => reset,
        a_bus          => a_bus,
        a_busD         => a_busD,
        b_bus          => b_bus,
        b_busD         => b_busD,
        alu_func       => alu_func,
        alu_funcD      => alu_funcD,
        shift_func     => shift_func,
        shift_funcD    => shift_funcD,
        mult_func      => mult_func,
        mult_funcD     => mult_funcD,
        reg_dest       => reg_dest,
        reg_destD      => reg_destD,
        rd_index       => rd_index,
        rd_indexD      => rd_indexD,

        rs_index       => rs_index,
        rt_index       => rt_index,
        pc_source      => pc_source,
        mem_source     => mem_source,
        a_source       => a_source,
        b_source       => b_source,
        c_source       => c_source,
        c_bus          => c_bus,
        pause_any      => pause_any,
        pause_pipeline => pause_pipeline);
   end generate; --pipeline3

end; --architecture logic

