---------------------------------------------------------------------
-- TITLE: Register Bank
-- AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
-- DATE CREATED: 2/2/01
-- FILENAME: reg_bank.vhd
-- PROJECT: MIPS CPU core
-- COPYRIGHT: Software placed into the public domain by the author.
--    Software 'as is' without warranty.  Author liable for nothing.
-- DESCRIPTION:
--    Implements a register bank with 32 registers that are 32-bits wide.
--    There are two read-ports and one write port.
---------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;  --needed for conv_integer
use work.mips_pack.all;

entity reg_bank is
   port(clk            : in  std_logic;
        rs_index       : in  std_logic_vector(5 downto 0);
        rt_index       : in  std_logic_vector(5 downto 0);
        rd_index       : in  std_logic_vector(5 downto 0);
        reg_source_out : out std_logic_vector(31 downto 0);
        reg_target_out : out std_logic_vector(31 downto 0);
        reg_dest_new   : in  std_logic_vector(31 downto 0);
        intr_enable    : out std_logic);
end; --entity reg_bank


--------------------------------------------------------------------
-- Change mips_cpu.vhd to use the ram_block architecture.  
-- The ram_block architecture attempts to use TWO dual-port memories.
-- For a tri-port memory with one write and two read ports then
-- remove dual_port_ram2 so only one tri-port memory will be created.
-- According to the Xilinx answers database record #4075 this architecture
-- may cause Synplify to infer a synchronous dual-port RAM using RAM16x1D.  
-- For Altera use either a csdpram or lpm_ram_dq.
-- I need feedback on this section!
--------------------------------------------------------------------
architecture ram_block of reg_bank is
   signal reg_status : std_logic;
   type ram_type is array(31 downto 0) of std_logic_vector(31 downto 0);
   signal dual_port_ram1 : ram_type;
   signal dual_port_ram2 : ram_type;

   --controls access to dual-port memories
   signal addr_a1, addr_a2, addr_b : std_logic_vector(4 downto 0);
   signal data_out1, data_out2     : std_logic_vector(31 downto 0);
   signal write_enable             : std_logic;
begin

reg_proc: process(clk, rs_index, rt_index, rd_index, reg_dest_new, 
      reg_status, data_out1, data_out2)
begin
   --setup for first dual-port memory
   if rs_index = "101110" then  --reg_epc CP0 14
      addr_a1 <= "00000";
   else
      addr_a1 <= rs_index(4 downto 0);
   end if;
   case rs_index is
   when "000000" => reg_source_out <= ZERO;
   when "101100" => reg_source_out <= ZERO(31 downto 1) & reg_status;
   when "111111" => reg_source_out <= ZERO(31 downto 8) & "00110000"; --intr vector
   when others   => reg_source_out <= data_out1;
   end case;

   --setup for second dual-port memory
   addr_a2 <= rt_index(4 downto 0);
   case rt_index is
   when "000000" => reg_target_out <= ZERO;
   when others   => reg_target_out <= data_out2;
   end case;

   --setup second port (write port) for both dual-port memories
   if rd_index /= "000000" and rd_index /= "101100" then
      write_enable <= '1';
   else
      write_enable <= '0';
   end if;
   if rd_index = "101110" then  --reg_epc CP0 14
      addr_b <= "00000";
   else
      addr_b <= rd_index(4 downto 0);
   end if;

   if rising_edge(clk) then
      if rd_index = "101100" then
         reg_status <= reg_dest_new(0);
      elsif rd_index = "101110" then  --reg_epc CP0 14
         reg_status <= '0';           --disable interrupts
      end if;
   end if;

   intr_enable <= reg_status;
end process;


ram_proc: process(clk, addr_a1, addr_a2, addr_b, reg_dest_new, 
      write_enable, dual_port_ram1, dual_port_ram2)
begin
   -- Simulate two dual-port RAMs
   data_out1 <= dual_port_ram1(conv_integer(addr_a1));
   data_out2 <= dual_port_ram2(conv_integer(addr_a2));
   if rising_edge(clk) then
      if write_enable = '1' then
         dual_port_ram1(conv_integer(addr_b)) <= reg_dest_new;
         dual_port_ram2(conv_integer(addr_b)) <= reg_dest_new;
      end if;
   end if;


   -- Simulate one tri-port RAM
   -- Remember to comment out dual_port_ram2
--   data_out1 <= dual_port_ram1(conv_integer(addr_a1));
--   data_out2 <= dual_port_ram1(conv_integer(addr_a2));
--   if rising_edge(clk) then
--      if write_enable = '1' then
--         dual_port_ram1(conv_integer(addr_b)) <= reg_dest_new;
--      end if;
--   end if;


   -- Generic Two-Port Synchronous RAM
   -- generic_tpram can be obtained from:
   -- http://www.opencores.org/cvsweb.shtml/generic_memories/
   -- Supports ASICs (Artisan, Avant, and Virage) and Xilinx FPGA
   -- Remember to comment out dual_port_ram1 and dual_port_ram2
--   bank1 : generic_tpram port map (
--      clk_a  => clk,
--      rst_a  => '0',
--      ce_a   => '1',
--      we_a   => '0',
--      oe_a   => '1',
--      addr_a => addr_a1,
--      di_a   => ZERO,
--      do_a   => data_out1,
--
--      clk_b  => clk,
--      rst_b  => '0',
--      ce_b   => '1',
--      we_b   => write_enable,
--      oe_b   => '0',
--      addr_b => addr_b,
--      di_a   => reg_dest_new);
--
--   bank2 : generic_tpram port map (
--      clk_a  => clk,
--      rst_a  => '0',
--      ce_a   => '1',
--      we_a   => '0',
--      oe_a   => '1',
--      addr_a => addr_a2,
--      di_a   => ZERO,
--      do_a   => data_out2,
--
--      clk_b  => clk,
--      rst_b  => '0',
--      ce_b   => '1',
--      we_b   => write_enable,
--      oe_b   => '0',
--      addr_b => addr_b,
--      di_a   => reg_dest_new);


   -- Xilinx mode using four 16x16 banks
   -- Remember to comment out dual_port_ram1 and dual_port_ram2
--   bank1_high: ramb4_s16_s16 port map (
--      clka  => clk,
--      rsta  => sig_false,
--      addra => addr_a1,
--      dia   => ZERO(31 downto 16),
--      ena   => sig_true,
--      wea   => sig_false,
--      doa   => data_out1(31 downto 16),
--
--      clkb  => clk,
--      rstb  => sig_false,
--      addrb => addr_b,
--      dib   => reg_dest_new(31 downto 16),
--      enb   => sig_true,
--      web   => write_enable);
--
--   bank1_low: ramb4_s16_s16 port map (
--      clka  => clk,
--      rsta  => sig_false,
--      addra => addr_a1,
--      dia   => ZERO(15 downto 0),
--      ena   => sig_true,
--      wea   => sig_false,
--      doa   => data_out1(15 downto 0),
--
--      clkb  => clk,
--      rstb  => sig_false,
--      addrb => addr_b,
--      dib   => reg_dest_new(15 downto 0),
--      enb   => sig_true,
--      web   => write_enable);
--
--   bank2_high: ramb4_s16_s16 port map (
--      clka  => clk,
--      rsta  => sig_false,
--      addra => addr_a2,
--      dia   => ZERO(31 downto 16),
--      ena   => sig_true,
--      wea   => sig_false,
--      doa   => data_out2(31 downto 16),
--
--      clkb  => clk,
--      rstb  => sig_false,
--      addrb => addr_b,
--      dib   => reg_dest_new(31 downto 16),
--      enb   => sig_true,
--      web   => write_enable);
--
--   bank2_low: ramb4_s16_s16 port map (
--      clka  => clk,
--      rsta  => sig_false,
--      addra => addr_a2,
--      dia   => ZERO(15 downto 0),
--      ena   => sig_true,
--      wea   => sig_false,
--      doa   => data_out2(15 downto 0),
--
--      clkb  => clk,
--      rstb  => sig_false,
--      addrb => addr_b,
--      dib   => reg_dest_new(15 downto 0),
--      enb   => sig_true,
--      web   => write_enable);

end process;

end; --architecture ram_block

--------------------------------------------------------------------

architecture logic of reg_bank is
   signal reg31, reg01, reg02, reg03 : std_logic_vector(31 downto 0);
   --For Altera simulations, comment out reg04 through reg30
   signal reg04, reg05, reg06, reg07 : std_logic_vector(31 downto 0);
   signal reg08, reg09, reg10, reg11 : std_logic_vector(31 downto 0);
   signal reg12, reg13, reg14, reg15 : std_logic_vector(31 downto 0);
   signal reg16, reg17, reg18, reg19 : std_logic_vector(31 downto 0);
   signal reg20, reg21, reg22, reg23 : std_logic_vector(31 downto 0);
   signal reg24, reg25, reg26, reg27 : std_logic_vector(31 downto 0);
   signal reg28, reg29, reg30        : std_logic_vector(31 downto 0);
   signal reg_epc                    : std_logic_vector(31 downto 0);
   signal reg_status                 : std_logic;
begin

reg_proc: process(clk, rs_index, rt_index, rd_index, reg_dest_new,
   reg31, reg01, reg02, reg03, reg04, reg05, reg06, reg07,
   reg08, reg09, reg10, reg11, reg12, reg13, reg14, reg15,
   reg16, reg17, reg18, reg19, reg20, reg21, reg22, reg23,
   reg24, reg25, reg26, reg27, reg28, reg29, reg30,
   reg_epc, reg_status)
begin
   case rs_index is
   when "000000" => reg_source_out <= ZERO;
   when "000001" => reg_source_out <= reg01;
   when "000010" => reg_source_out <= reg02;
   when "000011" => reg_source_out <= reg03;
   when "000100" => reg_source_out <= reg04;
   when "000101" => reg_source_out <= reg05;
   when "000110" => reg_source_out <= reg06;
   when "000111" => reg_source_out <= reg07;
   when "001000" => reg_source_out <= reg08;
   when "001001" => reg_source_out <= reg09;
   when "001010" => reg_source_out <= reg10;
   when "001011" => reg_source_out <= reg11;
   when "001100" => reg_source_out <= reg12;
   when "001101" => reg_source_out <= reg13;
   when "001110" => reg_source_out <= reg14;
   when "001111" => reg_source_out <= reg15;
   when "010000" => reg_source_out <= reg16;
   when "010001" => reg_source_out <= reg17;
   when "010010" => reg_source_out <= reg18;
   when "010011" => reg_source_out <= reg19;
   when "010100" => reg_source_out <= reg20;
   when "010101" => reg_source_out <= reg21;
   when "010110" => reg_source_out <= reg22;
   when "010111" => reg_source_out <= reg23;
   when "011000" => reg_source_out <= reg24;
   when "011001" => reg_source_out <= reg25;
   when "011010" => reg_source_out <= reg26;
   when "011011" => reg_source_out <= reg27;
   when "011100" => reg_source_out <= reg28;
   when "011101" => reg_source_out <= reg29;
   when "011110" => reg_source_out <= reg30;
   when "011111" => reg_source_out <= reg31;
   when "101100" => reg_source_out <= ZERO(31 downto 1) & reg_status;
   when "101110" => reg_source_out <= reg_epc;     --CP0 14
   when "111111" => reg_source_out <= ZERO(31 downto 8) & "00110000"; --intr vector
   when others =>   reg_source_out <= ZERO;
   end case;

   case rt_index is
   when "000000" => reg_target_out <= ZERO;
   when "000001" => reg_target_out <= reg01;
   when "000010" => reg_target_out <= reg02;
   when "000011" => reg_target_out <= reg03;
   when "000100" => reg_target_out <= reg04;
   when "000101" => reg_target_out <= reg05;
   when "000110" => reg_target_out <= reg06;
   when "000111" => reg_target_out <= reg07;
   when "001000" => reg_target_out <= reg08;
   when "001001" => reg_target_out <= reg09;
   when "001010" => reg_target_out <= reg10;
   when "001011" => reg_target_out <= reg11;
   when "001100" => reg_target_out <= reg12;
   when "001101" => reg_target_out <= reg13;
   when "001110" => reg_target_out <= reg14;
   when "001111" => reg_target_out <= reg15;
   when "010000" => reg_target_out <= reg16;
   when "010001" => reg_target_out <= reg17;
   when "010010" => reg_target_out <= reg18;
   when "010011" => reg_target_out <= reg19;
   when "010100" => reg_target_out <= reg20;
   when "010101" => reg_target_out <= reg21;
   when "010110" => reg_target_out <= reg22;
   when "010111" => reg_target_out <= reg23;
   when "011000" => reg_target_out <= reg24;
   when "011001" => reg_target_out <= reg25;
   when "011010" => reg_target_out <= reg26;
   when "011011" => reg_target_out <= reg27;
   when "011100" => reg_target_out <= reg28;
   when "011101" => reg_target_out <= reg29;
   when "011110" => reg_target_out <= reg30;
   when "011111" => reg_target_out <= reg31;
   when others =>   reg_target_out <= ZERO;
   end case;

   if rising_edge(clk) then
--      assert reg_dest_new'last_event >= 100 ps
--         report "Reg_dest timing error";
      case rd_index is
      when "000001" => reg01 <= reg_dest_new;
      when "000010" => reg02 <= reg_dest_new;
      when "000011" => reg03 <= reg_dest_new;
      when "000100" => reg04 <= reg_dest_new;
      when "000101" => reg05 <= reg_dest_new;
      when "000110" => reg06 <= reg_dest_new;
      when "000111" => reg07 <= reg_dest_new;
      when "001000" => reg08 <= reg_dest_new;
      when "001001" => reg09 <= reg_dest_new;
      when "001010" => reg10 <= reg_dest_new;
      when "001011" => reg11 <= reg_dest_new;
      when "001100" => reg12 <= reg_dest_new;
      when "001101" => reg13 <= reg_dest_new;
      when "001110" => reg14 <= reg_dest_new;
      when "001111" => reg15 <= reg_dest_new;
      when "010000" => reg16 <= reg_dest_new;
      when "010001" => reg17 <= reg_dest_new;
      when "010010" => reg18 <= reg_dest_new;
      when "010011" => reg19 <= reg_dest_new;
      when "010100" => reg20 <= reg_dest_new;
      when "010101" => reg21 <= reg_dest_new;
      when "010110" => reg22 <= reg_dest_new;
      when "010111" => reg23 <= reg_dest_new;
      when "011000" => reg24 <= reg_dest_new;
      when "011001" => reg25 <= reg_dest_new;
      when "011010" => reg26 <= reg_dest_new;
      when "011011" => reg27 <= reg_dest_new;
      when "011100" => reg28 <= reg_dest_new;
      when "011101" => reg29 <= reg_dest_new;
      when "011110" => reg30 <= reg_dest_new;
      when "011111" => reg31 <= reg_dest_new;
      when "101100" => reg_status <= reg_dest_new(0);
      when "101110" => reg_epc <= reg_dest_new;  --CP0 14
                       reg_status <= '0';        --disable interrupts
      when others =>
      end case;
   end if;
   intr_enable <= reg_status;
end process;

end; --architecture logic


