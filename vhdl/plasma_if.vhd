---------------------------------------------------------------------
-- TITLE: Plamsa Interface (clock divider and interface to FPGA board)
-- AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
-- DATE CREATED: 6/6/02
-- FILENAME: plasma_if.vhd
-- PROJECT: Plasma CPU core
-- COPYRIGHT: Software placed into the public domain by the author.
--    Software 'as is' without warranty.  Author liable for nothing.
-- DESCRIPTION:
--    This entity divides the clock by two and interfaces to the 
--    Altera EP20K200EFC484-2X FPGA board.
---------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use work.mlite_pack.all;

entity plasma_if is
   generic(memory_type : string := "ALTERA";
           log_file    : string := "UNUSED");
   port(clk_in     : in std_logic;
        reset_n    : in std_logic;
        uart_read  : in std_logic;
        uart_write : out std_logic;

        address    : out std_logic_vector(31 downto 0);
        data       : out std_logic_vector(31 downto 0);
        we_n       : out std_logic;
        oe_n       : out std_logic;
        be_n       : out std_logic_vector(3 downto 0);
        sram0_cs_n : out std_logic;
        sram1_cs_n : out std_logic);
end; --entity plasma_if

architecture logic of plasma_if is
   signal clk_reg      : std_logic;
   signal reset_in     : std_logic;
   signal intr_in      : std_logic;
   signal mem_address  : std_logic_vector(31 downto 0);
   signal mem_pause_in : std_logic;
   signal write_enable : std_logic;
   signal mem_byte_sel : std_logic_vector(3 downto 0);
begin  --architecture
   reset_in <= not reset_n;
   intr_in <= '0';
   mem_pause_in <= '0';

   address <= mem_address;
   we_n <= not write_enable;
   oe_n <= write_enable;
   be_n <= not mem_byte_sel;
   sram0_cs_n <= not mem_address(16);
   sram1_cs_n <= not mem_address(16);

   --convert 33MHz clock to 16.5MHz clock
   clk_div: process(clk_in, reset_in, clk_reg)
   begin
      if reset_in = '1' then
         clk_reg <= '0';
      elsif rising_edge(clk_in) then
         clk_reg <= not clk_reg;
      end if;
   end process; --clk_div

   u1_plama: plasma 
      generic map (memory_type => memory_type,
                   log_file    => log_file)
      PORT MAP (
         clk_in           => clk_reg,
         reset_in         => reset_in,
         intr_in          => intr_in,

         uart_read        => uart_read,
         uart_write       => uart_write,
 
         mem_address_out  => mem_address,
         mem_data         => data,
         mem_byte_sel_out => mem_byte_sel,
         mem_write_out    => write_enable,
         mem_pause_in     => mem_pause_in);

end; --architecture logic

