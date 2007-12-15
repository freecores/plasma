---------------------------------------------------------------------
-- TITLE: Plamsa Interface (clock divider and interface to FPGA board)
-- AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
-- DATE CREATED: 9/15/07
-- FILENAME: plasma_3e.vhd
-- PROJECT: Plasma CPU core
-- COPYRIGHT: Software placed into the public domain by the author.
--    Software 'as is' without warranty.  Author liable for nothing.
-- DESCRIPTION:
--    This entity divides the clock by two and interfaces to the 
--    Xilinx Spartan-3E XC3S200FT256-4 FPGA with DDR.
---------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
--use work.mlite_pack.all;

entity plasma_3e is
   port(CLK_50MHZ  : in std_logic;
        RS232_DCE_RXD : in std_logic;
        RS232_DCE_TXD : out std_logic;

        SD_CK_P    : out std_logic;     --clock_positive
        SD_CK_N    : out std_logic;     --clock_negative
        SD_CKE     : out std_logic;     --clock_enable

        SD_BA      : out std_logic_vector(1 downto 0);  --bank_address
        SD_A       : out std_logic_vector(12 downto 0); --address(row or col)
        SD_CS      : out std_logic;     --chip_select
        SD_RAS     : out std_logic;     --row_address_strobe
        SD_CAS     : out std_logic;     --column_address_strobe
        SD_WE      : out std_logic;     --write_enable

        SD_DQ      : inout std_logic_vector(15 downto 0); --data
        SD_UDM     : out std_logic;     --upper_byte_enable
        SD_UDQS    : inout std_logic;   --upper_data_strobe
        SD_LDM     : out std_logic;     --low_byte_enable
        SD_LDQS    : inout std_logic;   --low_data_strobe

        LED        : out std_logic_vector(7 downto 0);
        ROT_CENTER : in std_logic;
        ROT_A      : in std_logic;
        ROT_B      : in std_logic;
        BTN_EAST   : in std_logic;
        BTN_NORTH  : in std_logic;
        BTN_SOUTH  : in std_logic;
        BTN_WEST   : in std_logic;
        SW         : in std_logic_vector(3 downto 0));
end; --entity plasma_if


architecture logic of plasma_3e is

   component plasma
      generic(memory_type : string := "XILINX_16X"; --"DUAL_PORT_" "ALTERA_LPM";
              log_file    : string := "UNUSED");
      port(clk          : in std_logic;
           reset        : in std_logic;
           uart_write   : out std_logic;
           uart_read    : in std_logic;
   
           address      : out std_logic_vector(31 downto 2);
           byte_we      : out std_logic_vector(3 downto 0); 
           data_write   : out std_logic_vector(31 downto 0);
           data_read    : in std_logic_vector(31 downto 0);
           mem_pause_in : in std_logic;
        
           gpio0_out    : out std_logic_vector(31 downto 0);
           gpioA_in     : in std_logic_vector(31 downto 0));
   end component; --plasma

   component ddr_ctrl
      port(clk      : in std_logic;
           clk_2x   : in std_logic;
           reset_in : in std_logic;

           address  : in std_logic_vector(25 downto 2);
           byte_we  : in std_logic_vector(3 downto 0);
           data_w   : in std_logic_vector(31 downto 0);
           data_r   : out std_logic_vector(31 downto 0);
           active   : in std_logic;
           pause    : out std_logic;

           SD_CK_P  : out std_logic;     --clock_positive
           SD_CK_N  : out std_logic;     --clock_negative
           SD_CKE   : out std_logic;     --clock_enable

           SD_BA    : out std_logic_vector(1 downto 0);  --bank_address
           SD_A     : out std_logic_vector(12 downto 0); --address(row or col)
           SD_CS    : out std_logic;     --chip_select
           SD_RAS   : out std_logic;     --row_address_strobe
           SD_CAS   : out std_logic;     --column_address_strobe
           SD_WE    : out std_logic;     --write_enable

           SD_DQ    : inout std_logic_vector(15 downto 0); --data
           SD_UDM   : out std_logic;     --upper_byte_enable
           SD_UDQS  : inout std_logic;   --upper_data_strobe
           SD_LDM   : out std_logic;     --low_byte_enable
           SD_LDQS  : inout std_logic);  --low_data_strobe
   end component; --ddr

   signal clk_reg      : std_logic;
   signal address      : std_logic_vector(31 downto 2);
   signal data_write   : std_logic_vector(31 downto 0);
   signal data_read    : std_logic_vector(31 downto 0);
   signal byte_we      : std_logic_vector(3 downto 0);
   signal pause        : std_logic;
   signal active       : std_logic;
   signal reset        : std_logic;
   signal gpio0_out    : std_logic_vector(31 downto 0);
   signal gpio0_in     : std_logic_vector(31 downto 0);
   
begin  --architecture
   --Divide 50 MHz clock by two
   clk_div: process(reset, CLK_50MHZ, clk_reg)
   begin
      if reset = '1' then
         clk_reg <= '0';
      elsif rising_edge(CLK_50MHZ) then
         clk_reg <= not clk_reg;
      end if;
   end process; --clk_div

   reset <= ROT_CENTER;
   LED <= gpio0_out(7 downto 0);
   gpio0_in(31 downto 10) <= (others => '0');
   gpio0_in(9 downto 0) <= ROT_A & ROT_B & BTN_EAST & BTN_NORTH & 
                           BTN_SOUTH & BTN_WEST & SW;
   active <= '1' when address(31 downto 28) = "0001" else '0';

   u1_plama: plasma 
      generic map (memory_type => "XILINX_16X",
                   log_file    => "UNUSED")
      --generic map (memory_type => "DUAL_PORT",
      --             log_file    => "output2.txt")
      PORT MAP (
         clk          => clk_reg,
         reset        => reset,
         uart_write   => RS232_DCE_TXD,
         uart_read    => RS232_DCE_RXD,
 
         address      => address,
         byte_we      => byte_we,
         data_write   => data_write,
         data_read    => data_read,
         mem_pause_in => pause,

         gpio0_out    => gpio0_out,
         gpioA_in     => gpio0_in);
         
   u2_ddr: ddr_ctrl
      port map (
         clk      => clk_reg,
         clk_2x   => CLK_50MHZ,
         reset_in => reset,

         address  => address(25 downto 2),
         byte_we  => byte_we,
         data_w   => data_write,
         data_r   => data_read,
         active   => active,
         pause    => pause,

         SD_CK_P  => SD_CK_P,    --clock_positive
         SD_CK_N  => SD_CK_N,    --clock_negative
         SD_CKE   => SD_CKE,     --clock_enable
   
         SD_BA    => SD_BA,      --bank_address
         SD_A     => SD_A,       --address(row or col)
         SD_CS    => SD_CS,      --chip_select
         SD_RAS   => SD_RAS,     --row_address_strobe
         SD_CAS   => SD_CAS,     --column_address_strobe
         SD_WE    => SD_WE,      --write_enable

         SD_DQ    => SD_DQ,      --data
         SD_UDM   => SD_UDM,     --upper_byte_enable
         SD_UDQS  => SD_UDQS,    --upper_data_strobe
         SD_LDM   => SD_LDM,     --low_byte_enable
         SD_LDQS  => SD_LDQS);   --low_data_strobe
         
end; --architecture logic

