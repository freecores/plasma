---------------------------------------------------------------------
-- TITLE: Random Access Memory
-- AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
-- DATE CREATED: 4/21/01
-- FILENAME: ram.vhd
-- PROJECT: Plasma CPU core
-- COPYRIGHT: Software placed into the public domain by the author.
--    Software 'as is' without warranty.  Author liable for nothing.
-- DESCRIPTION:
--    Implements the RAM, reads the executable from either "code.txt",
--    or for Altera "code[0-3].hex".
--    Modified from "The Designer's Guide to VHDL" by Peter J. Ashenden
---------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_misc.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;
use ieee.std_logic_textio.all;
use std.textio.all;
use work.mlite_pack.all;

entity ram is
   generic(memory_type : string := "GENERIC");
   port(clk          : in std_logic;
        mem_byte_sel : in std_logic_vector(3 downto 0);
        mem_write    : in std_logic;
        mem_address  : in std_logic_vector(31 downto 0);
        mem_data_w   : in std_logic_vector(31 downto 0);
        mem_data_r   : out std_logic_vector(31 downto 0));
end; --entity ram

architecture logic of ram is
   constant ADDRESS_WIDTH   : natural := 13;
   signal clk_inv           : std_logic;
   signal mem_sel           : std_logic;
   signal read_enable       : std_logic;
   signal write_byte_enable : std_logic_vector(3 downto 0);
begin
   clk_inv <= not clk;
   mem_sel <= '1' when mem_address(30 downto ADDRESS_WIDTH) = ZERO(30 downto ADDRESS_WIDTH) else
              '0';
   read_enable <= mem_sel and not mem_write;
   write_byte_enable <= mem_byte_sel when mem_sel = '1' else
                        "0000";

   generic_ram:
   if memory_type = "GENERIC" generate
   ram_proc: process(clk, mem_byte_sel, mem_write, 
         mem_address, mem_data_w, mem_sel)
      variable mem_size : natural := 2 ** ADDRESS_WIDTH;
      variable data : std_logic_vector(31 downto 0); 
      subtype word is std_logic_vector(mem_data_w'length-1 downto 0);
      type storage_array is
         array(natural range 0 to mem_size/4 - 1) of word;
      variable storage : storage_array;
      variable index : natural := 0;
      file load_file : text is in "code.txt";
      variable hex_file_line : line;
   begin
      --load in the ram executable image
      if index = 0 then
         while not endfile(load_file) loop
--The following two lines had to be commented out for synthesis
            readline(load_file, hex_file_line);
            hread(hex_file_line, data);
            storage(index) := data;
            index := index + 1;
         end loop;
      end if;

      index := conv_integer(mem_address(ADDRESS_WIDTH-1 downto 2));
      data := storage(index);

      if mem_sel = '1' then
         if mem_write = '0' then
            mem_data_r <= data;
         end if;
         if mem_byte_sel(0) = '1' then
            data(7 downto 0) := mem_data_w(7 downto 0);
         end if;
         if mem_byte_sel(1) = '1' then
            data(15 downto 8) := mem_data_w(15 downto 8);
         end if;
         if mem_byte_sel(2) = '1' then
            data(23 downto 16) := mem_data_w(23 downto 16);
         end if;
         if mem_byte_sel(3) = '1' then
            data(31 downto 24) := mem_data_w(31 downto 24);
         end if;
      end if;
      
      if rising_edge(clk) then
         if mem_write = '1' then
            storage(index) := data;
         end if;
      end if;
   end process;
   end generate; --generic_ram


   altera_ram:
   if memory_type = "ALTERA" generate
      --Quartus II does not allow asynchronous RAM to be initialized
      --since the RAM may see glitches on the write enable during powerup.
      --Making lpm_address_control="REGISTERED" makes the RAM synchronous
      --but then the reads are delayed by a clock cycle.
      --Inverting the RAM clock appears to solve the clock cycle delay problem.
      lpm_ram_io_component0 : lpm_ram_dq
         GENERIC MAP (
            intended_device_family => "UNUSED",
            lpm_width => 8,
            lpm_widthad => ADDRESS_WIDTH-2,
            lpm_indata => "REGISTERED",
            lpm_address_control => "REGISTERED",
            lpm_outdata => "UNREGISTERED",
            lpm_file => "code0.hex",
            use_eab => "ON",
            lpm_type => "LPM_RAM_DQ")
         PORT MAP (
            data    => mem_data_w(31 downto 24),
            address => mem_address(ADDRESS_WIDTH-1 downto 2),
            inclock => clk_inv,
            we      => write_byte_enable(3),
            q       => mem_data_r(31 downto 24));

      lpm_ram_io_component1 : lpm_ram_dq
         GENERIC MAP (
            intended_device_family => "UNUSED",
            lpm_width => 8,
            lpm_widthad => ADDRESS_WIDTH-2,
            lpm_indata => "REGISTERED",
            lpm_address_control => "REGISTERED",
            lpm_outdata => "UNREGISTERED",
            lpm_file => "code1.hex",
            use_eab => "ON",
            lpm_type => "LPM_RAM_DQ")
         PORT MAP (
            data    => mem_data_w(23 downto 16),
            address => mem_address(ADDRESS_WIDTH-1 downto 2),
            inclock => clk_inv,
            we      => write_byte_enable(2),
            q       => mem_data_r(23 downto 16));

      lpm_ram_io_component2 : lpm_ram_dq
         GENERIC MAP (
            intended_device_family => "UNUSED",
            lpm_width => 8,
            lpm_widthad => ADDRESS_WIDTH-2,
            lpm_indata => "REGISTERED",
            lpm_address_control => "REGISTERED",
            lpm_outdata => "UNREGISTERED",
            lpm_file => "code2.hex",
            use_eab => "ON",
            lpm_type => "LPM_RAM_DQ")
         PORT MAP (
            data    => mem_data_w(15 downto 8),
            address => mem_address(ADDRESS_WIDTH-1 downto 2),
            inclock => clk_inv,
            we      => write_byte_enable(1),
            q       => mem_data_r(15 downto 8));

      lpm_ram_io_component3 : lpm_ram_dq
         GENERIC MAP (
            intended_device_family => "UNUSED",
            lpm_width => 8,
            lpm_widthad => ADDRESS_WIDTH-2,
            lpm_indata => "REGISTERED",
            lpm_address_control => "REGISTERED",
            lpm_outdata => "UNREGISTERED",
            lpm_file => "code3.hex",
            use_eab => "ON",
            lpm_type => "LPM_RAM_DQ")
         PORT MAP (
            data    => mem_data_w(7 downto 0),
            address => mem_address(ADDRESS_WIDTH-1 downto 2),
            inclock => clk_inv,
            we      => write_byte_enable(0),
            q       => mem_data_r(7 downto 0));

   end generate; --altera_ram

end; --architecture logic

