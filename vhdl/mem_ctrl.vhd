---------------------------------------------------------------------
-- TITLE: Memory Controller
-- AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
-- DATE CREATED: 1/31/01
-- FILENAME: mem_ctrl.vhd
-- PROJECT: MIPS CPU core
-- COPYRIGHT: Software placed into the public domain by the author.
--    Software 'as is' without warranty.  Author liable for nothing.
-- DESCRIPTION:
--    Memory controller for the MIPS CPU.
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
use work.mips_pack.all;

entity mem_ctrl is
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
        mem_write    : out std_logic;
        mem_pause    : in std_logic);
end; --entity mem_ctrl

architecture logic of mem_ctrl is
   --"00" = big_endian; "11" = little_endian
   constant little_endian : std_logic_vector(1 downto 0) := "00";
   signal opcode_reg : std_logic_vector(31 downto 0);
   signal next_opcode_reg : std_logic_vector(31 downto 0);

   subtype setup_state_type is std_logic_vector(1 downto 0);
   signal setup_state : setup_state_type;
   constant STATE_FETCH  : setup_state_type := "00";
   constant STATE_ADDR   : setup_state_type := "01";
   constant STATE_WRITE  : setup_state_type := "10";
   constant STATE_PAUSE  : setup_state_type := "11";
begin

mem_proc: process(clk, reset_in, pause_in, nullify_op, 
                  address_pc, address_data, mem_source, data_write, 
                  mem_data_r, mem_pause,
                  opcode_reg, next_opcode_reg, setup_state)
   variable data, datab   : std_logic_vector(31 downto 0);
   variable opcode_next   : std_logic_vector(31 downto 0);
   variable byte_sel_next : std_logic_vector(3 downto 0);
   variable write_next    : std_logic;
   variable setup_state_next : setup_state_type;
   variable pause         : std_logic;
   variable address_next  : std_logic_vector(31 downto 0);
   variable bits          : std_logic_vector(1 downto 0);
   variable mem_data_w_v  : std_logic_vector(31 downto 0);
begin
   byte_sel_next := "0000";
   write_next := '0';
   pause := '0';
   setup_state_next := setup_state;

   address_next := address_pc;
   data := mem_data_r;
   datab := ZERO;
   mem_data_w_v := ZERO; 

   case mem_source is
   when mem_read32 =>
      datab := data;
   when mem_read16 | mem_read16s =>
      if address_data(1) = little_endian(1) then
         datab(15 downto 0) := data(31 downto 16);
      else
         datab(15 downto 0) := data(15 downto 0);
      end if;
      if mem_source = mem_read16 or datab(15) = '0' then
         datab(31 downto 16) := ZERO(31 downto 16);
      else
         datab(31 downto 16) := ONES(31 downto 16);
      end if;
   when mem_read8 | mem_read8s =>
      bits := address_data(1 downto 0) xor little_endian;
      case bits is
      when "00" => datab(7 downto 0) := data(31 downto 24);
      when "01" => datab(7 downto 0) := data(23 downto 16);
      when "10" => datab(7 downto 0) := data(15 downto 8);
      when others => datab(7 downto 0) := data(7 downto 0);
      end case;
      if mem_source = mem_read8 or datab(7) = '0' then
         datab(31 downto 8) := ZERO(31 downto 8);
      else
         datab(31 downto 8) := ONES(31 downto 8);
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

   opcode_next := opcode_reg;
   if mem_source = mem_none then 
      setup_state_next := STATE_FETCH; 
      if pause_in = '0' and mem_pause = '0' then
         opcode_next := data;
      end if;
   else
      if setup_state = STATE_FETCH then
         pause := '1';
         byte_sel_next := "0000";
         setup_state_next := STATE_ADDR;
      elsif setup_state = STATE_ADDR then
         address_next := address_data;
         if write_next ='1' and address_data(31) = '0' then
            pause := '1';
            byte_sel_next := "0000";
            setup_state_next := STATE_WRITE;       --4 cycle access
         else
            if mem_pause = '0' then
               opcode_next := next_opcode_reg;
               setup_state_next := STATE_FETCH;    --2 cycle access
            end if;
         end if;
      elsif setup_state = STATE_WRITE then
         pause := '1';
         address_next := address_data;
         if mem_pause = '0' then
            setup_state_next := STATE_PAUSE; 
         end if;
      elsif setup_state = STATE_PAUSE then
         address_next := address_data;
         byte_sel_next := "0000";
         opcode_next := next_opcode_reg;
         setup_state_next := STATE_FETCH;
      end if;
   end if;

   if nullify_op = '1' then
      opcode_next := ZERO;  --NOP
   end if;
   if reset_in = '1' then
      setup_state_next := STATE_FETCH;
      opcode_next := ZERO;
   end if;

   if rising_edge(clk) then
      opcode_reg <= opcode_next;
      if setup_state = STATE_FETCH then
         next_opcode_reg <= data;
      end if;
      setup_state <= setup_state_next;
   end if;

   opcode_out <= opcode_reg;
   data_read <= datab;
   pause_out <= mem_pause or pause;
   mem_byte_sel <= byte_sel_next;
   mem_address <= address_next;
   if write_next = '1' and setup_state /= STATE_FETCH then
      mem_write <= '1';
      mem_data_w <= mem_data_w_v;
   else
      mem_write <= '0';
      mem_data_w <= HIGH_Z; --ZERO;
   end if;

end process; --data_proc

end; --architecture logic

