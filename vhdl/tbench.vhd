---------------------------------------------------------------------
-- TITLE: Test Bench
-- AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
-- DATE CREATED: 4/21/01
-- FILENAME: tbench.vhd
-- PROJECT: Plasma CPU core
-- COPYRIGHT: Software placed into the public domain by the author.
--    Software 'as is' without warranty.  Author liable for nothing.
-- DESCRIPTION:
--    This entity provides a test bench for testing the Plasma CPU core.
---------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use work.mlite_pack.all;

entity tbench is
end; --entity tbench

architecture logic of tbench is
   constant memory_type : string := 
   "GENERIC";
--   "ALTERA";
--   "XILINX";

   constant log_file  : string := 
--   "UNUSED"
   "output.txt";

   signal clk         : std_logic := '1';
   signal reset       : std_logic := '1';
   signal interrupt   : std_logic := '0';
   signal mem_write   : std_logic;
   signal mem_address : std_logic_vector(31 downto 0);
   signal mem_data    : std_logic_vector(31 downto 0);
   signal mem_pause   : std_logic := '0';
   signal mem_byte_sel: std_logic_vector(3 downto 0);
   signal uart_read   : std_logic;
   signal uart_write  : std_logic;
begin  --architecture
   clk <= not clk after 50 ns;
   reset <= '0' after 500 ns;

   --Uncomment the line below to test interrupts
--   interrupt <= '1' after 20 us when interrupt = '0' else '0' after 400 ns;
   --Uncomment the line below to test mem_pause
--   mem_pause <= '1' after 100 ns when mem_pause = '0' else '0' after 100 ns;

   u1: plasma
      generic map (memory_type => memory_type,
                   log_file    => log_file)
      PORT MAP (
         clk_in       => clk,
         reset_in     => reset,
         intr_in      => interrupt,

         uart_read    => uart_read,
         uart_write   => uart_write,
 
         mem_address_out  => mem_address,
         mem_data         => mem_data,
         mem_byte_sel_out => mem_byte_sel,
         mem_write_out    => mem_write,
         mem_pause_in     => mem_pause);

end; --architecture logic

