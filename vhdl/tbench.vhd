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
   port(write_pin : out std_logic;
        read_pin  : in std_logic);
end; --entity tbench

architecture logic of tbench is
   constant memory_type : string := "GENERIC";
--   constant memory_type : string := "ALTERA";
--   constant memory_type : string := "XILINX";
   signal clk         : std_logic := '1';
   signal reset       : std_logic := '1';
   signal interrupt   : std_logic := '0';
   signal mem_write   : std_logic;
   signal mem_read    : std_logic;
   signal mem_address : std_logic_vector(31 downto 0);
   signal mem_data    : std_logic_vector(31 downto 0);
   signal mem_pause   : std_logic := '0';
   signal mem_byte_sel: std_logic_vector(3 downto 0);
   signal uart_sel    : std_logic;
begin  --architecture
   clk <= not clk after 50 ns;
   reset <= '0' after 320 ns;
--   mem_pause <= '0';
   mem_read <= not mem_write;
   uart_sel <= '1' when mem_address(12 downto 0) = ONES(12 downto 0) and mem_byte_sel /= "0000" else
               '0';

   --Uncomment the line below to test interrupts
--   interrupt <= '1' after 20 us when interrupt = '0' else '0' after 400 ns;

   u1: mlite_cpu 
      generic map (memory_type => memory_type)
      PORT MAP (
         clk          => clk,
         reset_in     => reset,
         intr_in      => interrupt,
 
         mem_address  => mem_address,
         mem_data_w   => mem_data,
         mem_data_r   => mem_data,
         mem_byte_sel => mem_byte_sel,
         mem_write    => mem_write,
         mem_pause    => mem_pause);

   generic_ram:
   if memory_type /= "ALTERA" generate
      u2: ram 
         generic map ("code.txt")
         PORT MAP (
            clk          => clk,
            mem_byte_sel => mem_byte_sel,
            mem_write    => mem_write,
            mem_address  => mem_address(15 downto 0),
            mem_data_w   => mem_data,
            mem_data_r   => mem_data);
   end generate; --generic_ram

   altera_ram:
   if memory_type = "ALTERA" generate
      uart_component: uart
         generic map ("output.txt")
         port map(
            clk       => clk,
            reset     => reset,
            uart_sel  => uart_sel,
            data      => mem_data(7 downto 0),
            write_pin => write_pin,
            read_pin  => read_pin,
            pause     => mem_pause);

      lpm_ram_io_component0 : lpm_ram_io
         GENERIC MAP (
            intended_device_family => "UNUSED",
            lpm_width => 8,
            lpm_widthad => 11,
            lpm_indata => "REGISTERED",
            lpm_address_control => "UNREGISTERED",
            lpm_outdata => "UNREGISTERED",
            lpm_file => "code0.hex",
            use_eab => "ON",
            lpm_type => "LPM_RAM_DQ")
         PORT MAP (
            outenab => mem_read,
            address => mem_address(12 downto 2),
            inclock => clk,
            we      => mem_byte_sel(3),
            dio     => mem_data(31 downto 24));

      lpm_ram_io_component1 : lpm_ram_io
         GENERIC MAP (
            intended_device_family => "UNUSED",
            lpm_width => 8,
            lpm_widthad => 11,
            lpm_indata => "REGISTERED",
            lpm_address_control => "UNREGISTERED",
            lpm_outdata => "UNREGISTERED",
            lpm_file => "code1.hex",
            use_eab => "ON",
            lpm_type => "LPM_RAM_DQ")
         PORT MAP (
            outenab => mem_read,
            address => mem_address(12 downto 2),
            inclock => clk,
            we      => mem_byte_sel(2),
            dio     => mem_data(23 downto 16));

      lpm_ram_io_component2 : lpm_ram_io
         GENERIC MAP (
            intended_device_family => "UNUSED",
            lpm_width => 8,
            lpm_widthad => 11,
            lpm_indata => "REGISTERED",
            lpm_address_control => "UNREGISTERED",
            lpm_outdata => "UNREGISTERED",
            lpm_file => "code2.hex",
            use_eab => "ON",
            lpm_type => "LPM_RAM_DQ")
         PORT MAP (
            outenab => mem_read,
            address => mem_address(12 downto 2),
            inclock => clk,
            we      => mem_byte_sel(1),
            dio     => mem_data(15 downto 8));

      lpm_ram_io_component3 : lpm_ram_io
         GENERIC MAP (
            intended_device_family => "UNUSED",
            lpm_width => 8,
            lpm_widthad => 11,
            lpm_indata => "REGISTERED",
            lpm_address_control => "UNREGISTERED",
            lpm_outdata => "UNREGISTERED",
            lpm_file => "code3.hex",
            use_eab => "ON",
            lpm_type => "LPM_RAM_DQ")
         PORT MAP (
            outenab => mem_read,
            address => mem_address(12 downto 2),
            inclock => clk,
            we      => mem_byte_sel(0),
            dio     => mem_data(7 downto 0));
   end generate; --altera_ram

end; --architecture logic
