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
---------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_misc.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_textio.all;
use std.textio.all;

use ieee.std_logic_unsigned.all;
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
   signal uart_data_reg : std_logic_vector(8 downto 0);
   signal uart_bits_reg : std_logic_vector(3 downto 0);
   signal uart_div_reg  : std_logic_vector(7 downto 0);
begin

uart_proc: process(clk, reset, uart_sel, data,
      uart_data_reg, uart_bits_reg, uart_div_reg)
   variable uart_data_next : std_logic_vector(8 downto 0);
   variable uart_bits_next : std_logic_vector(3 downto 0);
   variable uart_div_next  : std_logic_vector(7 downto 0);
begin
   uart_data_next := uart_data_reg;
   uart_bits_next := uart_bits_reg;
   uart_div_next  := uart_div_reg;

   if uart_bits_reg = "0000" and uart_sel = '1' then
      uart_data_next := data & '0';
      uart_bits_next := "1010";
      uart_div_next := ZERO(7 downto 0);
   elsif uart_bits_reg /= "0000" and 
         ((log_file /= "UNUSED" and uart_div_reg = "00000010") or
         (log_file = "UNUSED" and uart_div_reg = "10001100")) then
      uart_data_next := '1' & uart_data_reg(8 downto 1);
      uart_bits_next := uart_bits_reg - 1;
      uart_div_next := ZERO(7 downto 0);
   else
      uart_div_next := uart_div_reg + 1;
   end if;

   if reset = '1' then
      uart_data_next := ONES(8 downto 0);
      uart_bits_next := "0000";
      uart_div_next := ZERO(7 downto 0);
   end if;

   if rising_edge(clk) then
      uart_data_reg <= uart_data_next;
      uart_bits_reg <= uart_bits_next;
      uart_div_reg <= uart_div_next;
   end if;

   uart_write <= uart_data_reg(0);
   if uart_bits_reg = ZERO(7 downto 0) or uart_sel = '1' then
      pause <= '0';
   elsif log_file = "UNUSED" then
      pause <= '1';
   else
--      pause <= '1';
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
                  writeline(store_file, hex_file_line);
                  line_length := 0;
               end if;
            end if; --uart_sel
         end if; --rising_edge(clk)
      end process; --uart_proc
   end generate; --uart_logger

end; --architecture logic

