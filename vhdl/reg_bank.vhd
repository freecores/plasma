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
use ieee.std_logic_unsigned.all;
use work.mips_pack.all;

entity reg_bank is
   port(clk            : in  std_logic;
        reset_in       : in  std_logic;
        rs_index       : in  std_logic_vector(5 downto 0);
        rt_index       : in  std_logic_vector(5 downto 0);
        rd_index       : in  std_logic_vector(5 downto 0);
        reg_source_out : out std_logic_vector(31 downto 0);
        reg_target_out : out std_logic_vector(31 downto 0);
        reg_dest_new   : in  std_logic_vector(31 downto 0);
        intr_enable    : out std_logic);
end; --entity reg_bank


--------------------------------------------------------------------
-- The ram_block architecture attempts to use TWO dual-port memories.
-- Different FPGAs and ASICs need different implementations.
-- Choose one of the RAM implementations below.
-- I need feedback on this section!
--------------------------------------------------------------------
architecture ram_block of reg_bank is
   signal reg_status : std_logic;
   type ram_type is array(31 downto 0) of std_logic_vector(31 downto 0);

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
      if reset_in = '1' or rd_index = "101110" then  --reg_epc CP0 14
         reg_status <= '0';           --disable interrupts
      elsif rd_index = "101100" then
         reg_status <= reg_dest_new(0);
      end if;
   end if;

   intr_enable <= reg_status;
end process;


------------------------------------------------------------
-- Pick only ONE of the dual-port RAM implementations below!
------------------------------------------------------------


   -- Option #1
   -- One tri-port RAM, two read-ports, one write-port
   -- 32 registers 32-bits wide
   ram_proc: process(clk, addr_a1, addr_a2, addr_b, reg_dest_new, 
         write_enable)
   variable tri_port_ram : ram_type;
   begin
      data_out1 <= tri_port_ram(conv_integer(addr_a1));
      data_out2 <= tri_port_ram(conv_integer(addr_a2));
      if rising_edge(clk) then
         if write_enable = '1' then
            tri_port_ram(conv_integer(addr_b)) := reg_dest_new;
         end if;
      end if;
   end process;


   -- Option #2
   -- Two dual-port RAMs, each with one read-port and one write-port
   -- According to the Xilinx answers database record #4075 this 
   -- architecture may cause Synplify to infer synchronous dual-port 
   -- RAM using RAM16x1D.  
--   ram_proc: process(clk, addr_a1, addr_a2, addr_b, reg_dest_new, 
--         write_enable)
--   variable dual_port_ram1 : ram_type;
--   variable dual_port_ram2 : ram_type;
--   begin
--      data_out1 <= dual_port_ram1(conv_integer(addr_a1));
--      data_out2 <= dual_port_ram2(conv_integer(addr_a2));
--      if rising_edge(clk) then
--         if write_enable = '1' then
--            dual_port_ram1(conv_integer(addr_b)) := reg_dest_new;
--            dual_port_ram2(conv_integer(addr_b)) := reg_dest_new;
--         end if;
--      end if;
--   end process;


   -- Option #3
   -- Generic Two-Port Synchronous RAM
   -- generic_tpram can be obtained from:
   -- http://www.opencores.org/cvsweb.shtml/generic_memories/
   -- Supports ASICs (Artisan, Avant, and Virage) and Xilinx FPGA
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


   -- Option #4
   -- Xilinx mode using four 16x16 banks
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


   -- Option #5
   -- Altera LPM_RAM_DP
--   bank1: LPM_RAM_DP 
--	generic map (
--      LPM_WIDTH    => 32,
--      LPM_WIDTHAD  => 5,
--      LPM_NUMWORDS => 32,
--??      LPM_INDATA   => "UNREGISTERED",
--??      LPM_OUTDATA  => "UNREGISTERED",
--??      LPM_RDADDRESS_CONTROL => "UNREGISTERED",
--??      LPM_WRADDRESS_CONTROL => "UNREGISTERED"
--   )
--   port map (RDCLOCK => clk,
--      RDADDRESS => addr_a1,
--      DATA      => reg_dest_new,
--      WRADDRESS => addr_b,
--      WREN      => write_enable,
--      WRCLOCK   => clk,
--      Q         => data_out1);
--
--   bank2: LPM_RAM_DP 
--	generic map (
--      LPM_WIDTH    => 32,
--      LPM_WIDTHAD  => 5,
--      LPM_NUMWORDS => 32,
--??      LPM_INDATA   => "UNREGISTERED",
--??      LPM_OUTDATA  => "UNREGISTERED",
--??      LPM_RDADDRESS_CONTROL => "UNREGISTERED",
--??      LPM_WRADDRESS_CONTROL => "UNREGISTERED"
--   )
--   port map (RDCLOCK => clk,
--      RDADDRESS => addr_a2,
--      DATA      => reg_dest_new,
--      WRADDRESS => addr_b,
--      WREN      => write_enable,
--      WRCLOCK   => clk,
--      Q         => data_out2);


end; --architecture ram_block

