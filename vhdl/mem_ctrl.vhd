---------------------------------------------------------------------
-- TITLE: Memory Controller
-- AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
-- DATE CREATED: 1/31/01
-- FILENAME: mem_ctrl.vhd
-- PROJECT: Plasma CPU core
-- COPYRIGHT: Software placed into the public domain by the author.
--    Software 'as is' without warranty.  Author liable for nothing.
-- DESCRIPTION:
--    Memory controller for the Plasma CPU.
--    Supports Big or Little Endian mode.
--    Four cycles for a write unless a(31)='1' then two cycles.
--    This entity could implement interfaces to:
--       Data cache
--       Address cache
--       Memory management unit (MMU)
--       DRAM controller
---------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use work.mlite_pack.all;

entity mem_ctrl is
   generic(ACCURATE_TIMING : boolean := false);
   port(clk          : in std_logic;
        reset_in     : in std_logic;
        pause_in     : in std_logic;
        nullify_op   : in std_logic;
        address_pc   : in std_logic_vector(31 downto 0);
        opcode_out   : out std_logic_vector(31 downto 0);

        address_data : in std_logic_vector(31 downto 0);
        mem_source   : in mem_source_type;
        data_write   : in std_logic_vector(31 downto 0);
        data_read    : out std_logic_vector(31 downto 0);
        pause_out    : out std_logic;
        
        mem_address  : out std_logic_vector(31 downto 0);
        mem_data_w   : out std_logic_vector(31 downto 0);
        mem_data_r   : in std_logic_vector(31 downto 0);
        mem_byte_sel : out std_logic_vector(3 downto 0);
        mem_write    : out std_logic);
end; --entity mem_ctrl

architecture logic of mem_ctrl is
   --"00" = big_endian; "11" = little_endian
   constant little_endian  : std_logic_vector(1 downto 0) := "00";
   signal opcode_reg       : std_logic_vector(31 downto 0);
   signal next_opcode_reg  : std_logic_vector(31 downto 0);

   subtype mem_state_type is std_logic_vector(1 downto 0);
   signal mem_state_reg  : mem_state_type;
   constant STATE_FETCH  : mem_state_type := "00";
   constant STATE_ADDR   : mem_state_type := "01";
   constant STATE_WRITE  : mem_state_type := "10";
   constant STATE_PAUSE  : mem_state_type := "11";

   --ACCURATE_TIMING notes:
   --The VHDL compiler's timing calculation isn't able to realize that
   --memory reads take two clock cycles.  It notices that reg_bank:reg_dest
   --is dependent on mem_ctrl:mem_data_r which is dependent on 
   --mem_ctrl:mem_address which is dependent on alu:c_alu.  However,
   --this dependency is only true for memory read or write cycles
   --which are multiple clock cycles.  Enabling ACCURATE_TIMING
   --creates an additional 32-bit register that does nothing other
   --than letting the VHDL compiler accurately predict the maximum
   --clock speed.
   signal address_reg        : std_logic_vector(31 downto 0);
   signal write_reg          : std_logic;
   signal byte_sel_reg       : std_logic_vector(3 downto 0);
   signal mem_state_next_sig : mem_state_type;
   signal opcode_next_sig    : std_logic_vector(31 downto 0);
   signal write_next_sig     : std_logic;
   signal byte_sel_next_sig  : std_logic_vector(3 downto 0);
   
begin

GEN_REGS: process(clk, reset_in)
begin
   if reset_in = '1' then
      mem_state_reg <= STATE_FETCH;
      opcode_reg <= ZERO;
      next_opcode_reg <= ZERO;
   elsif rising_edge(clk) then
      mem_state_reg <= mem_state_next_sig;
      opcode_reg <= opcode_next_sig;
      if mem_state_reg = STATE_FETCH then
         next_opcode_reg <= mem_data_r;
      end if;
   end if;
end process;

GEN_REGS2: process(clk, address_data, write_next_sig, byte_sel_next_sig)
begin
   if rising_edge(clk) then
      if ACCURATE_TIMING then
         address_reg <= address_data;
         write_reg <= write_next_sig;
         byte_sel_reg <= byte_sel_next_sig;
      end if;
   end if;
   if not ACCURATE_TIMING then
      address_reg <= address_data;
      write_reg <= write_next_sig;
      byte_sel_reg <= byte_sel_next_sig;
   end if;
end process;
  
mem_proc: process(clk, reset_in, pause_in, nullify_op, 
                  address_pc, address_data, mem_source, data_write, 
                  mem_data_r,
                  opcode_reg, next_opcode_reg, mem_state_reg,
                  address_reg, write_reg, byte_sel_reg)
   variable data           : std_logic_vector(31 downto 0);
   variable opcode_next    : std_logic_vector(31 downto 0);
   variable byte_sel_next  : std_logic_vector(3 downto 0);
   variable byte_sel       : std_logic_vector(3 downto 0);
   variable write_next     : std_logic;
   variable write_line     : std_logic;
   variable mem_state_next : mem_state_type;
   variable pause          : std_logic;
   variable address        : std_logic_vector(31 downto 0);
   variable bits           : std_logic_vector(1 downto 0);
   variable mem_data_w_v   : std_logic_vector(31 downto 0);
begin
   byte_sel_next := "0000";
   write_next := '0';
   pause := '0';
   mem_state_next := mem_state_reg;

   data := ZERO;
   mem_data_w_v := ZERO; 

   case mem_source is
   when mem_read32 =>
      data := mem_data_r;
   when mem_read16 | mem_read16s =>
      if address_reg(1) = little_endian(1) then
         data(15 downto 0) := mem_data_r(31 downto 16);
      else
         data(15 downto 0) := mem_data_r(15 downto 0);
      end if;
      if mem_source = mem_read16 or data(15) = '0' then
         data(31 downto 16) := ZERO(31 downto 16);
      else
         data(31 downto 16) := ONES(31 downto 16);
      end if;
   when mem_read8 | mem_read8s =>
      bits := address_reg(1 downto 0) xor little_endian;
      case bits is
      when "00" => data(7 downto 0) := mem_data_r(31 downto 24);
      when "01" => data(7 downto 0) := mem_data_r(23 downto 16);
      when "10" => data(7 downto 0) := mem_data_r(15 downto 8);
      when others => data(7 downto 0) := mem_data_r(7 downto 0);
      end case;
      if mem_source = mem_read8 or data(7) = '0' then
         data(31 downto 8) := ZERO(31 downto 8);
      else
         data(31 downto 8) := ONES(31 downto 8);
      end if;
   when mem_write32 =>
      write_next := '1';
      mem_data_w_v := data_write;
      byte_sel_next := "1111";
   when mem_write16 =>
      write_next := '1';
      mem_data_w_v := data_write(15 downto 0) & data_write(15 downto 0);
      if address_data(1) = little_endian(1) then
         byte_sel_next := "1100";
      else
         byte_sel_next := "0011";
      end if;
   when mem_write8 =>
      write_next := '1';
      mem_data_w_v := data_write(7 downto 0) & data_write(7 downto 0) &
                  data_write(7 downto 0) & data_write(7 downto 0);
      bits := address_data(1 downto 0) xor little_endian;
      case bits is
      when "00" =>
         byte_sel_next := "1000"; 
      when "01" => 
         byte_sel_next := "0100"; 
      when "10" =>
         byte_sel_next := "0010"; 
      when others =>
         byte_sel_next := "0001"; 
      end case;
   when others =>
   end case;
   byte_sel_next_sig <= byte_sel_next;
   write_next_sig <= write_next;
   
   opcode_next := opcode_reg;
   case mem_state_reg is             --State Machine
   when STATE_FETCH =>
      address := address_pc;
      write_line := '0';
      byte_sel := "0000";
      if mem_source = mem_fetch then --opcode fetch
         mem_state_next := STATE_FETCH;
         if pause_in = '0' then
            opcode_next := mem_data_r;
         end if;
      else                           --memory read or write
         pause := '1';
         if pause_in = '0' then
            mem_state_next := STATE_ADDR;
         end if;
      end if;
   when STATE_ADDR =>  --address lines pre-hold
      address := address_reg;
      write_line := write_reg;
      if write_reg = '1' and address_reg(31) = '1' then
         pause := '1';
         byte_sel := "0000";
         if pause_in = '0' then
            mem_state_next := STATE_WRITE;    --4 cycle access
         end if;
      else
         byte_sel := byte_sel_reg;
         if pause_in = '0' then
            opcode_next := next_opcode_reg;
            mem_state_next := STATE_FETCH;    --2 cycle access
         end if;
      end if;
   when STATE_WRITE =>
      pause := '1';
      address := address_reg;
      write_line := write_reg;
      byte_sel := byte_sel_reg;
      if pause_in = '0' then
         mem_state_next := STATE_PAUSE; 
      end if;
   when OTHERS =>  --STATE_PAUSE address lines post-hold
      address := address_reg;
      write_line := write_reg;
      byte_sel := "0000";
      if pause_in = '0' then
         opcode_next := next_opcode_reg;
         mem_state_next := STATE_FETCH;
      end if;
   end case;
   
   if nullify_op = '1' and pause_in = '0' then
      opcode_next := ZERO;  --NOP after beql
   end if;

   mem_state_next_sig <= mem_state_next;
   opcode_next_sig <= opcode_next;

   if reset_in = '1' then
      write_line := '0';     
   end if;

   opcode_out <= opcode_reg;
   data_read <= data;
   pause_out <= pause;
   mem_byte_sel <= byte_sel;
   mem_address <= address;
   mem_write <= write_line;

   if write_line = '1' then
      mem_data_w <= mem_data_w_v;
   else
      mem_data_w <= HIGH_Z; --ZERO;
   end if;

end process; --data_proc

end; --architecture logic

