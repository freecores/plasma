---------------------------------------------------------------------
-- TITLE: UART
-- AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
-- DATE CREATED: 5/29/02
-- FILENAME: uart.vhd
-- PROJECT: Plasma CPU core
-- COPYRIGHT: Software placed into the public domain by the author.
--    Software 'as is' without warranty.  Author liable for nothing.
-- DESCRIPTION:
--    Implements the UART.
--    Stalls the CPU until the charater has been transmitted.
---------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_misc.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_textio.all;
use ieee.std_logic_unsigned.all;
use std.textio.all;
use work.mlite_pack.all;

entity uart is
   generic(log_file : string := "UNUSED");
   port(clk        : in std_logic;
        reset      : in std_logic;
        uart_sel   : in std_logic;
        data       : in std_logic_vector(7 downto 0);
        uart_read  : in std_logic;
        uart_write : out std_logic;
        pause      : out std_logic);
end; --entity ram

architecture logic of uart is
   signal data_reg : std_logic_vector(8 downto 0);
   signal bits_reg : std_logic_vector(3 downto 0);
   signal div_reg  : std_logic_vector(9 downto 0);
begin

uart_proc: process(clk, reset, data_reg, bits_reg, div_reg, uart_sel, data)
   constant DIV_VALUE : std_logic_vector(9 downto 0) :=
      "0100011110";  --33MHz/2/57600Hz = 0x11e
--      "0000000010";  --for debug
   variable data_next : std_logic_vector(8 downto 0);
   variable bits_next : std_logic_vector(3 downto 0);
   variable div_next  : std_logic_vector(9 downto 0);
begin
   data_next := data_reg;
   bits_next := bits_reg;
   div_next  := div_reg;

   if uart_sel = '1' then
      data_next := data & '0';
      bits_next := "1010";
      div_next  := ZERO(9 downto 0);
   elsif div_reg = DIV_VALUE then
      data_next := '1' & data_reg(8 downto 1);
      if bits_reg /= "0000" then
         bits_next := bits_reg - 1;
      end if;
      div_next  := ZERO(9 downto 0);
   else
      div_next := div_reg + 1;
   end if;

   if reset = '1' then
      data_reg <= ZERO(8 downto 0);
      bits_reg <= "0000";
      div_reg <= ZERO(9 downto 0);
   elsif rising_edge(clk) then
      data_reg <= data_next;
      bits_reg <= bits_next;
      div_reg  <= div_next;
   end if;

   uart_write <= data_reg(0);
   if uart_sel = '0' and bits_reg /= "0000" 
         and log_file = "UNUSED" 
         then
      pause <= '1';
   else
      pause <= '0';
   end if;
end process;

   uart_logger:
   if log_file /= "UNUSED" generate
      uart_proc: process(clk, uart_sel, data)
         file store_file : text is out log_file;
         variable hex_file_line : line;
         variable c : character;
         variable index : natural;
         variable line_length : natural := 0;
      begin
         if rising_edge(clk) then
            if uart_sel = '1' then
               index := conv_integer(data(6 downto 0));
               if index /= 10 then
                  c := character'val(index);
                  write(hex_file_line, c);
                  line_length := line_length + 1;
               end if;
               if index = 10 or line_length >= 72 then
--The following line had to be commented out for synthesis
                  writeline(store_file, hex_file_line);
                  line_length := 0;
               end if;
            end if; --uart_sel
         end if; --rising_edge(clk)
      end process; --uart_proc
   end generate; --uart_logger

end; --architecture logic

