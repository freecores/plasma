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

component mlite_cpu
   port(clk         : in std_logic;
        reset_in    : in std_logic;
        intr_in     : in std_logic;

        mem_address : out std_logic_vector(31 downto 0);
        mem_data_w  : out std_logic_vector(31 downto 0);
        mem_data_r  : in std_logic_vector(31 downto 0);
        mem_byte_sel: out std_logic_vector(3 downto 0); 
        mem_write   : out std_logic;
        mem_pause   : in std_logic);
end component;

component ram 
   generic(load_file_name : string);
   port(clk          : in std_logic;
        mem_byte_sel : in std_logic_vector(3 downto 0);
        mem_write    : in std_logic;
        mem_address  : in std_logic_vector;
        mem_data_w   : in std_logic_vector(31 downto 0);
        mem_data_r   : out std_logic_vector(31 downto 0));
end component;

   signal clk         : std_logic := '1';
   signal reset       : std_logic := '1';
   signal interrupt   : std_logic := '0';
   signal mem_write   : std_logic;
   signal mem_address : std_logic_vector(31 downto 0);
   signal mem_data_w  : std_logic_vector(31 downto 0);
   signal mem_data_r  : std_logic_vector(31 downto 0);
   signal mem_pause   : std_logic;
   signal mem_byte_sel: std_logic_vector(3 downto 0);
begin  --architecture
   clk <= not clk after 50 ns;
   reset <= '0' after 320 ns;
   mem_pause <= '0';

   --Uncomment the line below to test interrupts
--   interrupt <= '1' after 20 us when interrupt = '0' else '0' after 400 ns;

   u1: mlite_cpu PORT MAP (
        clk          => clk,
        reset_in     => reset,
        intr_in      => interrupt,

        mem_address  => mem_address,
        mem_data_w   => mem_data_w,
        mem_data_r   => mem_data_r,
        mem_byte_sel => mem_byte_sel,
        mem_write    => mem_write,
        mem_pause    => mem_pause);

   u2: ram generic map ("code.txt")
       PORT MAP (
        clk          => clk,
        mem_byte_sel => mem_byte_sel,
        mem_write    => mem_write,
        mem_address  => mem_address(15 downto 0),
        mem_data_w   => mem_data_w,
        mem_data_r   => mem_data_r);

end; --architecture logic

