
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity Zynq is
    Port (  CLOCK : in STD_LOGIC;
            RESET_N : in STD_LOGIC;
            LEDoutR : out STD_LOGIC;     -- IO_B34_LN6 R14
            LEDoutG : out STD_LOGIC;     -- IO_B34_LP7 Y16
            LEDoutB : out STD_LOGIC;     -- IO_B34_LN7 Y17

            GP0OUT : out std_logic_vector (33 downto 0);
            LEDS : out std_logic_vector (7 downto 0);
            TRIGGR : out std_logic;
            DEBUGS : out std_logic_vector (13 downto 0);

            -- arm processor memory bus interface (AXI)
            -- we are a slave for accessing the control registers (read & write)
            saxi_ARADDR : in std_logic_vector (11 downto 0);
            saxi_ARREADY : out std_logic;
            saxi_ARVALID : in std_logic;
            saxi_AWADDR : in std_logic_vector (11 downto 0);
            saxi_AWREADY : out std_logic;
            saxi_AWVALID : in std_logic;
            saxi_BREADY : in std_logic;
            saxi_BRESP : out std_logic_vector (1 downto 0);
            saxi_BVALID : out std_logic;
            saxi_RDATA : out std_logic_vector (31 downto 0);
            saxi_RREADY : in std_logic;
            saxi_RRESP : out std_logic_vector (1 downto 0);
            saxi_RVALID : out std_logic;
            saxi_WDATA : in std_logic_vector (31 downto 0);
            saxi_WREADY : out std_logic;
            saxi_WVALID : in std_logic;

        -- - we are a master for accessing the ring buffer (read only)
        maxi_ARADDR : out std_logic_vector (31 downto 0);
        maxi_ARBURST : out std_logic_vector (1 downto 0);
        maxi_ARCACHE : out std_logic_vector (3 downto 0);
        maxi_ARID : out std_logic_vector (0 downto 0);
        maxi_ARLEN : out std_logic_vector (7 downto 0);
        maxi_ARLOCK : out std_logic_vector (1 downto 0);
        maxi_ARPROT : out std_logic_vector (2 downto 0);
        maxi_ARQOS : out std_logic_vector (3 downto 0);
        maxi_ARREADY : in std_logic;
        maxi_ARREGION : out std_logic_vector (3 downto 0);
        maxi_ARSIZE : out std_logic_vector (2 downto 0);
        maxi_ARUSER : out std_logic_vector (0 downto 0);
        maxi_ARVALID : out std_logic;

        maxi_AWADDR : out std_logic_vector (31 downto 0);
        maxi_AWBURST : out std_logic_vector (1 downto 0);
        maxi_AWCACHE : out std_logic_vector (3 downto 0);
        maxi_AWID : out std_logic_vector (0 downto 0);
        maxi_AWLEN : out std_logic_vector (7 downto 0);
        maxi_AWLOCK : out std_logic_vector (1 downto 0);
        maxi_AWPROT : out std_logic_vector (2 downto 0);
        maxi_AWQOS : out std_logic_vector (3 downto 0);
        maxi_AWREADY : in std_logic;
        maxi_AWREGION : out std_logic_vector (3 downto 0);
        maxi_AWSIZE : out std_logic_vector (2 downto 0);
        maxi_AWUSER : out std_logic_vector (0 downto 0);
        maxi_AWVALID : out std_logic;

        maxi_BREADY : out std_logic;
        maxi_BVALID : in std_logic;

        maxi_RDATA : in std_logic_vector (31 downto 0);
        maxi_RLAST : in std_logic;
        maxi_RREADY : out std_logic;
        maxi_RVALID : in std_logic;

        maxi_WDATA : out std_logic_vector (31 downto 0);
        maxi_WLAST : out std_logic;
        maxi_WREADY : in std_logic;
        maxi_WSTRB : out std_logic_vector (3 downto 0);
        maxi_WUSER : out std_logic_vector (0 downto 0);
        maxi_WVALID : out std_logic);
end Zynq;

architecture rtl of Zynq is

    -- declare axi slave port signals (used by sim ps code to access our control registers)
    ATTRIBUTE X_INTERFACE_INFO : STRING;
    ATTRIBUTE X_INTERFACE_INFO OF saxi_ARADDR: SIGNAL IS "xilinx.com:interface:aximm:1.0 S00_AXI ARADDR";
    ATTRIBUTE X_INTERFACE_INFO OF saxi_ARREADY: SIGNAL IS "xilinx.com:interface:aximm:1.0 S00_AXI ARREADY";
    ATTRIBUTE X_INTERFACE_INFO OF saxi_ARVALID: SIGNAL IS "xilinx.com:interface:aximm:1.0 S00_AXI ARVALID";
    ATTRIBUTE X_INTERFACE_INFO OF saxi_AWADDR: SIGNAL IS "xilinx.com:interface:aximm:1.0 S00_AXI AWADDR";
    ATTRIBUTE X_INTERFACE_INFO OF saxi_AWREADY: SIGNAL IS "xilinx.com:interface:aximm:1.0 S00_AXI AWREADY";
    ATTRIBUTE X_INTERFACE_INFO OF saxi_AWVALID: SIGNAL IS "xilinx.com:interface:aximm:1.0 S00_AXI AWVALID";
    ATTRIBUTE X_INTERFACE_INFO OF saxi_BREADY: SIGNAL IS "xilinx.com:interface:aximm:1.0 S00_AXI BREADY";
    ATTRIBUTE X_INTERFACE_INFO OF saxi_BRESP: SIGNAL IS "xilinx.com:interface:aximm:1.0 S00_AXI BRESP";
    ATTRIBUTE X_INTERFACE_INFO OF saxi_BVALID: SIGNAL IS "xilinx.com:interface:aximm:1.0 S00_AXI BVALID";
    ATTRIBUTE X_INTERFACE_INFO OF saxi_RDATA: SIGNAL IS "xilinx.com:interface:aximm:1.0 S00_AXI RDATA";
    ATTRIBUTE X_INTERFACE_INFO OF saxi_RREADY: SIGNAL IS "xilinx.com:interface:aximm:1.0 S00_AXI RREADY";
    ATTRIBUTE X_INTERFACE_INFO OF saxi_RRESP: SIGNAL IS "xilinx.com:interface:aximm:1.0 S00_AXI RRESP";
    ATTRIBUTE X_INTERFACE_INFO OF saxi_RVALID: SIGNAL IS "xilinx.com:interface:aximm:1.0 S00_AXI RVALID";
    ATTRIBUTE X_INTERFACE_INFO OF saxi_WDATA: SIGNAL IS "xilinx.com:interface:aximm:1.0 S00_AXI WDATA";
    ATTRIBUTE X_INTERFACE_INFO OF saxi_WREADY: SIGNAL IS "xilinx.com:interface:aximm:1.0 S00_AXI WREADY";
    ATTRIBUTE X_INTERFACE_INFO OF saxi_WVALID: SIGNAL IS "xilinx.com:interface:aximm:1.0 S00_AXI WVALID";

    -- declare axi master port signals (used by this code to access ring contents via dma)
    ATTRIBUTE X_INTERFACE_INFO OF maxi_ARADDR: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI ARADDR";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_ARBURST: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI ARBURST";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_ARCACHE: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI ARCACHE";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_ARID: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI ARID";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_ARLEN: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI ARLEN";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_ARLOCK: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI ARLOCK";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_ARPROT: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI ARPROT";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_ARQOS: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI ARQOS";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_ARREADY: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI ARREADY";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_ARREGION: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI ARREGION";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_ARSIZE: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI ARSIZE";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_ARUSER: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI ARUSER";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_ARVALID: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI ARVALID";

    ATTRIBUTE X_INTERFACE_INFO OF maxi_AWADDR: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI AWADDR";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_AWBURST: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI AWBURST";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_AWCACHE: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI AWCACHE";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_AWID: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI AWID";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_AWLEN: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI AWLEN";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_AWLOCK: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI AWLOCK";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_AWPROT: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI AWPROT";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_AWQOS: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI AWQOS";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_AWREADY: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI AWREADY";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_AWREGION: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI AWREGION";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_AWSIZE: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI AWSIZE";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_AWUSER: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI AWUSER";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_AWVALID: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI AWVALID";

    ATTRIBUTE X_INTERFACE_INFO OF maxi_BREADY: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI BREADY";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_BVALID: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI BVALID";

    ATTRIBUTE X_INTERFACE_INFO OF maxi_RDATA: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI RDATA";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_RLAST: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI RLAST";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_RREADY: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI RREADY";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_RVALID: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI RVALID";

    ATTRIBUTE X_INTERFACE_INFO OF maxi_WDATA: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI WDATA";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_WLAST: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI WLAST";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_WREADY: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI WREADY";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_WSTRB: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI WSTRB";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_WUSER: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI WUSER";
    ATTRIBUTE X_INTERFACE_INFO OF maxi_WVALID: SIGNAL IS "xilinx.com:interface:aximm:1.0 M00_AXI WVALID";

    constant VERSION : std_logic_vector (31 downto 0) := x"00000150";

    constant BURSTLEN : natural := 10;

    signal saxiARREADY, saxiAWREADY, saxiBVALID, saxiRVALID, saxiWREADY : std_logic;
    signal ledr, ledg, ledb : std_logic;

    constant PERIOD : natural := 1024*1024*256;     -- power of 2

    signal blubright, divider, fader, grnbright, ratio, redbright : natural range 0 to PERIOD-1;
    signal countup : boolean;

    signal rpi_qena, rpi_dena : std_logic;

    signal paddlrda, paddlrdb, paddlrdc, paddlrdd : std_logic_vector (31 downto 0);
    signal paddlwra, paddlwrb, paddlwrc, paddlwrd : std_logic_vector (31 downto 0);
    signal boardena : std_logic_vector (5 downto 0);

    signal readaddr, writeaddr : std_logic_vector (11 downto 2);
    signal gpinput, gpoutput, gpcompos : std_logic_vector (31 downto 0);
    signal denadata, qenadata : std_logic_vector (12 downto 0);

    signal dmardaddr, dmawtaddr : std_logic_vector (31 downto 0);
    signal maxiARVALID, maxiRREADY, maxiAWVALID, maxiWVALID, maxiBREADY : std_logic;

    signal dmareadsel, dmawritesel : natural range 0 to 9;
    signal temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7, temp8, temp9 : std_logic_vector (31 downto 0);

    -- board enables
    signal aclena, aluena, maena, pcena, rpiena, seqena : boolean;

    -- signals output by the acl board
    signal bus_acq       , acl_acq       , pad_acq       : std_logic_vector (11 downto 0);
    signal bus_acqzero   , acl_acqzero   , pad_acqzero   : std_logic;
    signal bus_grpb_skip , acl_grpb_skip , pad_grpb_skip : std_logic;
    signal bus_N_lnq     , acl_N_lnq     , pad_N_lnq     : std_logic;
    signal bus_lnq       , acl_lnq       , pad_lnq       : std_logic;

    -- signals output by the alu board
    signal bus_N_alucout , alu_N_alucout , pad_N_alucout : std_logic;
    signal bus_N_aluq    , alu_N_aluq    , pad_N_aluq    : std_logic_vector (11 downto 0);
    signal bus_N_newlink , alu_N_newlink , pad_N_newlink : std_logic;

    -- signals output by the ma board
    signal bus_N_maq , ma_N_maq , pad_N_maq : std_logic_vector (11 downto 0);
    signal bus_maq   , ma_maq   , pad_maq   : std_logic_vector (11 downto 0);

    -- signals output by the pc board
    signal bus_pcq , pc_pcq , pad_pcq : std_logic_vector (11 downto 0);

    -- signals output by the rpi board -- we use the gpio registers
    signal bus_clok2 , rpi_clok2 , pad_clok2 : std_logic;
    signal bus_intrq , rpi_intrq , pad_intrq : std_logic;
    signal bus_ioskp , rpi_ioskp , pad_ioskp : std_logic;
    signal bus_mql   , rpi_mql   , pad_mql   : std_logic;
    signal bus_mq    , rpi_mq    , pad_mq    : std_logic_vector (11 downto 0);
    signal bus_reset , rpi_reset , pad_reset : std_logic;

    -- signals output by the seq board
    signal bus_N_ac_aluq   , seq_N_ac_aluq   , pad_N_ac_aluq   : std_logic;
    signal bus_N_ac_sc     , seq_N_ac_sc     , pad_N_ac_sc     : std_logic;
    signal bus_N_alu_add   , seq_N_alu_add   , pad_N_alu_add   : std_logic;
    signal bus_N_alu_and   , seq_N_alu_and   , pad_N_alu_and   : std_logic;
    signal bus_N_alua_m1   , seq_N_alua_m1   , pad_N_alua_m1   : std_logic;
    signal bus_N_alua_ma   , seq_N_alua_ma   , pad_N_alua_ma   : std_logic;
    signal bus_alua_mq0600 , seq_alua_mq0600 , pad_alua_mq0600 : std_logic;
    signal bus_alua_mq1107 , seq_alua_mq1107 , pad_alua_mq1107 : std_logic;
    signal bus_alua_pc0600 , seq_alua_pc0600 , pad_alua_pc0600 : std_logic;
    signal bus_alua_pc1107 , seq_alua_pc1107 , pad_alua_pc1107 : std_logic;
    signal bus_alub_1      , seq_alub_1      , pad_alub_1      : std_logic;
    signal bus_N_alub_ac   , seq_N_alub_ac   , pad_N_alub_ac   : std_logic;
    signal bus_N_alub_m1   , seq_N_alub_m1   , pad_N_alub_m1   : std_logic;
    signal bus_N_grpa1q    , seq_N_grpa1q    , pad_N_grpa1q    : std_logic;
    signal bus_N_dfrm      , seq_N_dfrm      , pad_N_dfrm      : std_logic;
    signal bus_N_jump      , seq_N_jump      , pad_N_jump      : std_logic;
    signal bus_inc_axb     , seq_inc_axb     , pad_inc_axb     : std_logic;
    signal bus_N_intak     , seq_N_intak     , pad_N_intak     : std_logic;
    signal bus_intak1q     , seq_intak1q     , pad_intak1q     : std_logic;
    signal bus_ioinst      , seq_ioinst      , pad_ioinst      : std_logic;
    signal bus_iot2q       , seq_iot2q       , pad_iot2q       : std_logic;
    signal bus_N_ln_wrt    , seq_N_ln_wrt    , pad_N_ln_wrt    : std_logic;
    signal bus_N_ma_aluq   , seq_N_ma_aluq   , pad_N_ma_aluq   : std_logic;
    signal bus_N_mread     , seq_N_mread     , pad_N_mread     : std_logic;
    signal bus_N_mwrite    , seq_N_mwrite    , pad_N_mwrite    : std_logic;
    signal bus_N_pc_aluq   , seq_N_pc_aluq   , pad_N_pc_aluq   : std_logic;
    signal bus_N_pc_inc    , seq_N_pc_inc    , pad_N_pc_inc    : std_logic;
    signal bus_tad3q       , seq_tad3q       , pad_tad3q       : std_logic;
    signal bus_fetch1q     , seq_fetch1q     , pad_fetch1q     : std_logic;
    signal bus_fetch2q     , seq_fetch2q     , pad_fetch2q     : std_logic;
    signal bus_defer1q     , seq_defer1q     , pad_defer1q     : std_logic;
    signal bus_defer2q     , seq_defer2q     , pad_defer2q     : std_logic;
    signal bus_defer3q     , seq_defer3q     , pad_defer3q     : std_logic;
    signal bus_exec1q      , seq_exec1q      , pad_exec1q      : std_logic;
    signal bus_exec2q      , seq_exec2q      , pad_exec2q      : std_logic;
    signal bus_exec3q      , seq_exec3q      , pad_exec3q      : std_logic;
    signal bus_irq         , seq_irq         , pad_irq         : std_logic_vector (11 downto 9);

begin
    DEBUGS <= b"00000000000000";

    -- bus values that are constants
    saxi_BRESP <= b"00";        -- A3.4.4/A10.3 transfer OK
    saxi_RRESP <= b"00";        -- A3.4.4/A10.3 transfer OK

    maxi_ARBURST <= b"01";      -- A3.4.1/A10.3 burst type = INCR
    maxi_ARCACHE <= b"0000"; ----TODO---- b"0110";    -- A4.2 use read cache
    maxi_ARID <= b"0";          -- A10.3 transaction id 0
    maxi_ARLEN <= std_logic_vector (to_unsigned (BURSTLEN - 1, 8));  -- A3.4.1/A10.3 burst length
    maxi_ARLOCK <= b"00";       -- A7.4/A10.3 normal access
    maxi_ARPROT <= b"001";      -- A4.7 access permissions (privileged, secure, data)
    maxi_ARQOS <= b"0000";      -- A8.1.1/A10.3 no QoS requirement
    maxi_ARREGION <= b"0000";
    maxi_ARSIZE <= b"010";      -- A3.4.1 transfer size = 4 bytes each
    maxi_ARUSER <= b"0";

    maxi_AWBURST <= b"01";      -- A3.4.1/A10.3 burst type = INCR
    maxi_AWCACHE <= b"0000";
    maxi_AWID <= b"0";
    maxi_AWLEN <= std_logic_vector (to_unsigned (BURSTLEN - 1, 8));  -- A3.4.1/A10.3 burst length
    maxi_AWLOCK <= b"00";
    maxi_AWPROT <= b"001";      -- A4.7 access permissions (privileged, secure, data)
    maxi_AWQOS <= b"0000";
    maxi_AWREGION <= b"0000";
    maxi_AWSIZE <= b"010";      -- A3.4.1 transfer size = 4 bytes each
    maxi_AWUSER <= b"0";
    maxi_WSTRB <= b"1111";
    maxi_WUSER <= b"0";

    -- buffered outputs (outputs we read internally)
    saxi_ARREADY <= saxiARREADY;
    saxi_AWREADY <= saxiAWREADY;
    saxi_BVALID  <= saxiBVALID;
    saxi_RVALID  <= saxiRVALID;
    saxi_WREADY  <= saxiWREADY;

    maxi_ARVALID <= maxiARVALID;
    maxi_RREADY  <= maxiRREADY;

    maxi_AWVALID <= maxiAWVALID;
    maxi_WVALID  <= maxiWVALID;
    maxi_WLAST   <= '1' when dmawritesel = BURSTLEN - 1 else '0';
    maxi_BREADY  <= maxiBREADY;

    maxi_WDATA <=
        temp0 when dmawritesel = 0 else
        temp1 when dmawritesel = 1 else
        temp2 when dmawritesel = 2 else
        temp3 when dmawritesel = 3 else
        temp4 when dmawritesel = 4 else
        temp5 when dmawritesel = 5 else
        temp6 when dmawritesel = 6 else
        temp7 when dmawritesel = 7 else
        temp8 when dmawritesel = 8 else
        temp9 when dmawritesel = 9 else
        x"DEADBEEF";

    LEDoutR <= ledr;
    LEDoutG <= ledg;
    LEDoutB <= ledb;

    ratio <= (divider mod 65536) * (PERIOD / 65536);    -- 0..PERIOD-1 for pulse-width modulation
    ledr <= '0' when ratio < redbright else '1';        -- turn on red for time proportional to redbright/ratio
    ledg <= '0' when ratio < grnbright else '1';        -- turn on green for time proportional to grnbright/ratio
    ledb <= '0' when ratio < blubright else '1';        -- turn on blue for time proportional to blubright/ratio

    -- zero for first half of PERIOD
    -- then ramps up from zero to PERIOD in second half
    fader <= (divider - PERIOD/2) * 2 when (divider >= PERIOD/2) else 0;

    -- linear when counting up
    -- ramp down quickly when counting down
    grnbright <= divider / 2 when countup else fader / 2;

    -- ramp up quickly in second half when counting up
    -- linear when counting down
    redbright <= fader when countup else divider;

    -- full on bright at bottom then fade out either side going up
    blubright <= (PERIOD/2 - divider) * 2 when divider < PERIOD/2 else 0;

    process (CLOCK, RESET_N)
    begin
        if RESET_N = '0' then
            divider <= 0;
            countup <= true;
        elsif rising_edge (CLOCK) then
            if countup then
                if divider = PERIOD-1 then
                    countup <= false;
                else
                    divider <= divider + 1;
                end if;
            else
                if divider = 0 then
                    countup <= true;
                else
                    divider <= divider - 1;
                end if;
            end if;
        end if;
    end process;

    saxi_RDATA <=        gpinput  when readaddr = b"0000000000" else
                         gpoutput when readaddr = b"0000000001" else
                         gpcompos when readaddr = b"0000000010" else
                          VERSION when readaddr = b"0000000011" else
     x"000000" & b"00" & boardena when readaddr = b"0000000100" else
                         paddlrda when readaddr = b"0000001000" else
                         paddlrdb when readaddr = b"0000001001" else
                         paddlrdc when readaddr = b"0000001010" else
                         paddlrdd when readaddr = b"0000001011" else
                         paddlwra when readaddr = b"0000001100" else
                         paddlwrb when readaddr = b"0000001101" else
                         paddlwrc when readaddr = b"0000001110" else
                         paddlwrd when readaddr = b"0000001111" else
                        dmardaddr when readaddr = b"0100000000" else
                        dmawtaddr when readaddr = b"0100000001" else
                            temp0 when readaddr = b"1000000000" else
                            temp1 when readaddr = b"1000000001" else
                            temp2 when readaddr = b"1000000010" else
                            temp3 when readaddr = b"1000000011" else
                            temp4 when readaddr = b"1000000100" else
                            temp5 when readaddr = b"1000000101" else
                            temp6 when readaddr = b"1000000110" else
                            temp7 when readaddr = b"1000000111" else
                            temp8 when readaddr = b"1000001000" else
                            temp9 when readaddr = b"1000001001" else
                    x"BAADD" & b"00" & readaddr;

    -- A3.3.1 Read transaction dependencies
    -- A3.3.1 Write transaction dependencies
    --        AXI4 write response dependency
    process (CLOCK, RESET_N)
    begin
        if RESET_N = '0' then
            saxiARREADY <= '1';                             -- we are ready to accept read address
            saxiRVALID <= '0';                              -- we are not sending out read data

            saxiAWREADY <= '1';                             -- we are ready to accept write address
            saxiWREADY <= '0';                              -- we are not ready to accept write data
            saxiBVALID <= '0';                              -- we are not acknowledging any write

            gpoutput <= x"00000000";                        -- reset the PDP8
            boardena <= b"111111";                          -- by default all boards are enabled

            -- reset dma read registers
            dmardaddr <= (others => '0');
            maxiARVALID <= '0';
            maxiRREADY <= '0';

            -- reset dma write registers
            dmawtaddr <= (others => '0');
            maxiAWVALID <= '0';
            maxiWVALID <= '0';
            maxiBREADY <= '0';
        elsif rising_edge (CLOCK) then

            ---------------------
            --  register read  --
            ---------------------

            -- check for PS sending us a read address
            if saxiARREADY = '1' and saxi_ARVALID = '1' then
                readaddr <= saxi_ARADDR(11 downto 2);       -- save address bits we care about
                saxiARREADY <= '0';                         -- we are no longer accepting a read address
                saxiRVALID <= '1';                          -- we are sending out the corresponding data

            -- check for PS acknowledging receipt of data
            elsif saxiRVALID = '1' and saxi_RREADY = '1' then
                saxiARREADY <= '1';                         -- we are ready to accept an address again
                saxiRVALID <= '0';                          -- we are no longer sending out data
            end if;

            ----------------------
            --  register write  --
            ----------------------

            -- check for PS sending us write data
            if saxiWREADY = '1' and saxi_WVALID = '1' then
                case writeaddr is                           -- write data to register
                    when b"0000000001" => gpoutput  <= saxi_WDATA;
                    when b"0000000100" => boardena  <= saxi_WDATA(5 downto 0);
                    when b"0000001100" => paddlwra  <= saxi_WDATA;
                    when b"0000001101" => paddlwrb  <= saxi_WDATA;
                    when b"0000001110" => paddlwrc  <= saxi_WDATA;
                    when b"0000001111" => paddlwrd  <= saxi_WDATA;
                    when b"0100000000" => dmardaddr <= saxi_WDATA;
                    when b"0100000001" => dmawtaddr <= saxi_WDATA;
                    when b"1000000000" => temp0 <= saxi_WDATA;
                    when b"1000000001" => temp1 <= saxi_WDATA;
                    when b"1000000010" => temp2 <= saxi_WDATA;
                    when b"1000000011" => temp3 <= saxi_WDATA;
                    when b"1000000100" => temp4 <= saxi_WDATA;
                    when b"1000000101" => temp5 <= saxi_WDATA;
                    when b"1000000110" => temp6 <= saxi_WDATA;
                    when b"1000000111" => temp7 <= saxi_WDATA;
                    when b"1000001000" => temp8 <= saxi_WDATA;
                    when b"1000001001" => temp9 <= saxi_WDATA;
                    when others => null;
                end case;
                saxiAWREADY <= '1';                         -- we are ready to accept an address again
                saxiWREADY <= '0';                          -- we are no longer accepting write data
                saxiBVALID <= '1';                          -- we have accepted the data

            else
                -- check for PS sending us a write address
                if saxiAWREADY = '1' and saxi_AWVALID = '1' then
                    writeaddr <= saxi_AWADDR(11 downto 2);  -- save address bits we care about
                    saxiAWREADY <= '0';                     -- we are no longer accepting a write address
                    saxiWREADY <= '1';                      -- we are ready to accept write data
                end if;

                -- check for PS acknowledging write acceptance
                if saxiBVALID = '1' and saxi_BREADY = '1' then
                    saxiBVALID <= '0';
                end if;
            end if;

            -----------------------------------------
            --  dma read                           --
            --  read into temp0..9 from dmardaddr  --
            -----------------------------------------

            if maxiARVALID = '0' and maxiRREADY = '0' and dmardaddr(0) = '1' then
                maxi_ARADDR <= dmardaddr and x"FFFFF7FC";
                dmareadsel  <= 0;
                maxiARVALID <= '1';
                maxiRREADY  <= '1';
            end if;

            if maxiARVALID = '1' and maxi_ARREADY = '1' then
                maxiARVALID <= '0';
            end if;

            if maxiRREADY = '1' and maxi_RVALID = '1' then
                case dmareadsel is
                    when 0 => temp0 <= maxi_RDATA;
                    when 1 => temp1 <= maxi_RDATA;
                    when 2 => temp2 <= maxi_RDATA;
                    when 3 => temp3 <= maxi_RDATA;
                    when 4 => temp4 <= maxi_RDATA;
                    when 5 => temp5 <= maxi_RDATA;
                    when 6 => temp6 <= maxi_RDATA;
                    when 7 => temp7 <= maxi_RDATA;
                    when 8 => temp8 <= maxi_RDATA;
                    when 9 => temp9 <= maxi_RDATA;
                    when others => null;
                end case;
                dmardaddr(11 downto 2) <= std_logic_vector (unsigned (dmardaddr(11 downto 2)) + 1);
                if dmareadsel = BURSTLEN - 1 then
                    maxiRREADY   <= '0';
                    dmardaddr(0) <= '0';
                else
                    dmareadsel   <= dmareadsel + 1;
                end if;
            end if;

            ------------------------------------------
            --  dma write                           --
            --  write from temp0..9 into dmawtaddr  --
            ------------------------------------------

            -- if not doing anything and enabled to start, start writing
            if maxiAWVALID = '0' and maxiWVALID = '0' and maxiBREADY = '0' and dmawtaddr(0) = '1' then
                maxi_AWADDR <= dmawtaddr and x"FFFFF7FC";
                dmawritesel <= 0;
                maxiAWVALID <= '1';                     -- start sending address
                maxiWVALID  <= '1';                     -- start sending temp0
            end if;

            -- if mem controller accepted the address, stop sending it and accept completion status
            if maxiAWVALID = '1' and maxi_AWREADY = '1' then
                maxiAWVALID <= '0';                     -- stop sending address
                maxiBREADY  <= '1';                     -- able to accept completion status
            end if;

            -- if mem controller accepted data, stop sending last word or start sending next word
            if maxiWVALID = '1' and maxi_WREADY = '1' then
                dmawtaddr(11 downto 2) <= std_logic_vector (unsigned (dmawtaddr(11 downto 2)) + 1);
                if dmawritesel = BURSTLEN - 1 then
                    maxiWVALID  <= '0';                 -- stop sending last word
                else
                    dmawritesel <= dmawritesel + 1;     -- start sending next word
                end if;
            end if;

            -- if mem controller completed write, start writing next burst or stop writing
            if maxiBREADY = '1' and maxi_BVALID = '1' then
                maxiBREADY   <= '0';                    -- no longer accepting completion status
                dmawtaddr(1) <= '0';                    -- shift down start bit
                dmawtaddr(0) <= dmawtaddr(1);
            end if;
        end if;
    end process;

    -- 43C00000 : read gpio pins : they read signals from the bus
    gpinput(3 downto 0) <= b"0000";
    gpinput(15 downto 4) <= not bus_N_aluq; -- pin isn't in G_REVIS
    gpinput(16) <= not bus_N_lnq;           -- pin isn't in G_REVIS
    gpinput(21 downto 17) <= b"00000";
    gpinput(22) <= not bus_N_jump;          -- pin isn't in G_REVIS
    gpinput(23) <= not bus_ioinst;          -- pin is in G_REVIS
    gpinput(24) <= not bus_N_dfrm;          -- pin isn't in G_REVIS
    gpinput(25) <= not bus_N_mread;         -- pin isn't in G_REVIS
    gpinput(26) <= not bus_N_mwrite;        -- pin isn't in G_REVIS
    gpinput(27) <= not bus_N_intak;         -- pin isn't in G_REVIS
    gpinput(31 downto 28) <= b"0000";

    -- 43C00004 : write gpio pins : they write signals to the bus only when rpi board is enabled
    rpi_clok2 <= not gpoutput(2);           -- pin is in G_REVOS
    rpi_reset <= not gpoutput(3);           -- pin is in G_REVOS
    rpi_mq    <= not gpoutput(15 downto 4); -- pins are in G_REVOS
    rpi_mql   <= not gpoutput(16);          -- pin is in G_REVOS
    rpi_ioskp <= not gpoutput(17);          -- pin is in G_REVOS
    TRIGGR    <=     gpoutput(18);          -- trigger scope
    rpi_qena  <=     gpoutput(19);          -- pin isn't in G_REVOS
    rpi_intrq <= not gpoutput(20);          -- pin is in G_REVOS
    rpi_dena  <=     gpoutput(21);          -- pin isn't in G_REVOS

    -- 43C00008 : composite readback
    denadata <= gpinput (16 downto 4) when rpi_dena = '1' else b"0000000000000";
    qenadata <= gpoutput(16 downto 4) when rpi_qena = '1' else b"0000000000000";

    gpcompos(0)            <= gpoutput(0);
    gpcompos(1)            <= '0';
    gpcompos(3 downto 2)   <= gpoutput(3 downto 2);
    gpcompos(16 downto 4)  <= denadata or qenadata;
    gpcompos(21 downto 17) <= gpoutput(21 downto 17);
    gpcompos(27 downto 22) <= gpinput(27 downto 22);
    gpcompos(31 downto 28) <= b"0000";

    -- paddles reading the bus bits

    paddlrda( 1-1) <= '0'             ;
    paddlrda( 2-1) <= bus_acq(11)     ;
    paddlrda( 3-1) <= bus_N_aluq(11)  ;
    paddlrda( 4-1) <= bus_N_maq(11)   ;
    paddlrda( 5-1) <= bus_maq(11)     ;
    paddlrda( 6-1) <= bus_mq(11)      ;
    paddlrda( 7-1) <= bus_pcq(11)     ;
    paddlrda( 8-1) <= bus_irq(11)     ;
    paddlrda( 9-1) <= bus_acq(10)     ;
    paddlrda(10-1) <= bus_N_aluq(10)  ;
    paddlrda(11-1) <= bus_N_maq(10)   ;
    paddlrda(12-1) <= bus_maq(10)     ;
    paddlrda(13-1) <= bus_mq(10)      ;
    paddlrda(14-1) <= bus_pcq(10)     ;
    paddlrda(15-1) <= bus_irq(10)     ;
    paddlrda(16-1) <= bus_acq( 9)     ;
    paddlrda(17-1) <= bus_N_aluq( 9)  ;
    paddlrda(18-1) <= bus_N_maq( 9)   ;
    paddlrda(19-1) <= bus_maq( 9)     ;
    paddlrda(20-1) <= bus_mq( 9)      ;
    paddlrda(21-1) <= bus_pcq( 9)     ;
    paddlrda(22-1) <= bus_irq( 9)     ;
    paddlrda(23-1) <= bus_acq( 8)     ;
    paddlrda(24-1) <= bus_N_aluq( 8)  ;
    paddlrda(25-1) <= bus_N_maq( 8)   ;
    paddlrda(26-1) <= bus_maq( 8)     ;
    paddlrda(27-1) <= bus_mq( 8)      ;
    paddlrda(28-1) <= bus_pcq( 8)     ;
    paddlrda(30-1) <= bus_acq( 7)     ;
    paddlrda(31-1) <= bus_N_aluq( 7)  ;
    paddlrda(32-1) <= bus_N_maq( 7)   ;

    paddlrdb( 1-1) <= '0'             ;
    paddlrdb( 2-1) <= bus_maq( 7)     ;
    paddlrdb( 3-1) <= bus_mq( 7)      ;
    paddlrdb( 4-1) <= bus_pcq( 7)     ;
    paddlrdb( 6-1) <= bus_acq( 6)     ;
    paddlrdb( 7-1) <= bus_N_aluq( 6)  ;
    paddlrdb( 8-1) <= bus_N_maq( 6)   ;
    paddlrdb( 9-1) <= bus_maq( 6)     ;
    paddlrdb(10-1) <= bus_mq( 6)      ;
    paddlrdb(11-1) <= bus_pcq( 6)     ;
    paddlrdb(12-1) <= bus_N_jump      ;
    paddlrdb(13-1) <= bus_acq( 5)     ;
    paddlrdb(14-1) <= bus_N_aluq( 5)  ;
    paddlrdb(15-1) <= bus_N_maq( 5)   ;
    paddlrdb(16-1) <= bus_maq( 5)     ;
    paddlrdb(17-1) <= bus_mq( 5)      ;
    paddlrdb(18-1) <= bus_pcq( 5)     ;
    paddlrdb(19-1) <= bus_N_alub_m1   ;
    paddlrdb(20-1) <= bus_acq( 4)     ;
    paddlrdb(21-1) <= bus_N_aluq( 4)  ;
    paddlrdb(22-1) <= bus_N_maq( 4)   ;
    paddlrdb(23-1) <= bus_maq( 4)     ;
    paddlrdb(24-1) <= bus_mq( 4)      ;
    paddlrdb(25-1) <= bus_pcq( 4)     ;
    paddlrdb(26-1) <= bus_acqzero     ;
    paddlrdb(27-1) <= bus_acq( 3)     ;
    paddlrdb(28-1) <= bus_N_aluq( 3)  ;
    paddlrdb(29-1) <= bus_N_maq( 3)   ;
    paddlrdb(30-1) <= bus_maq( 3)     ;
    paddlrdb(31-1) <= bus_mq( 3)      ;
    paddlrdb(32-1) <= bus_pcq( 3)     ;

    paddlrdc( 1-1) <= '0'             ;
    paddlrdc( 2-1) <= bus_acq( 2)     ;
    paddlrdc( 3-1) <= bus_N_aluq( 2)  ;
    paddlrdc( 4-1) <= bus_N_maq( 2)   ;
    paddlrdc( 5-1) <= bus_maq( 2)     ;
    paddlrdc( 6-1) <= bus_mq( 2)      ;
    paddlrdc( 7-1) <= bus_pcq( 2)     ;
    paddlrdc( 8-1) <= bus_N_ac_sc     ;
    paddlrdc( 9-1) <= bus_acq( 1)     ;
    paddlrdc(10-1) <= bus_N_aluq( 1)  ;
    paddlrdc(11-1) <= bus_N_maq( 1)   ;
    paddlrdc(12-1) <= bus_maq( 1)     ;
    paddlrdc(13-1) <= bus_mq( 1)      ;
    paddlrdc(14-1) <= bus_pcq( 1)     ;
    paddlrdc(15-1) <= bus_intak1q     ;
    paddlrdc(16-1) <= bus_acq( 0)     ;
    paddlrdc(17-1) <= bus_N_aluq( 0)  ;
    paddlrdc(18-1) <= bus_N_maq( 0)   ;
    paddlrdc(19-1) <= bus_maq( 0)     ;
    paddlrdc(20-1) <= bus_mq( 0)      ;
    paddlrdc(21-1) <= bus_pcq( 0)     ;
    paddlrdc(22-1) <= bus_fetch1q     ;
    paddlrdc(23-1) <= bus_N_ac_aluq   ;
    paddlrdc(24-1) <= bus_N_alu_add   ;
    paddlrdc(25-1) <= bus_N_alu_and   ;
    paddlrdc(26-1) <= bus_N_alua_m1   ;
    paddlrdc(27-1) <= bus_N_alucout   ;
    paddlrdc(28-1) <= bus_N_alua_ma   ;
    paddlrdc(29-1) <= bus_alua_mq0600 ;
    paddlrdc(30-1) <= bus_alua_mq1107 ;
    paddlrdc(31-1) <= bus_alua_pc0600 ;
    paddlrdc(32-1) <= bus_alua_pc1107 ;

    paddlrdd( 1-1) <= '0'             ;
    paddlrdd( 2-1) <= bus_alub_1      ;
    paddlrdd( 3-1) <= bus_N_alub_ac   ;
    paddlrdd( 4-1) <= bus_clok2       ;
    paddlrdd( 5-1) <= bus_fetch2q     ;
    paddlrdd( 6-1) <= bus_N_grpa1q    ;
    paddlrdd( 7-1) <= bus_defer1q     ;
    paddlrdd( 8-1) <= bus_defer2q     ;
    paddlrdd( 9-1) <= bus_defer3q     ;
    paddlrdd(10-1) <= bus_exec1q      ;
    paddlrdd(11-1) <= bus_grpb_skip   ;
    paddlrdd(12-1) <= bus_exec2q      ;
    paddlrdd(13-1) <= bus_N_dfrm      ;
    paddlrdd(14-1) <= bus_inc_axb     ;
    paddlrdd(15-1) <= bus_N_intak     ;
    paddlrdd(16-1) <= bus_intrq       ;
    paddlrdd(17-1) <= bus_exec3q      ;
    paddlrdd(18-1) <= bus_ioinst      ;
    paddlrdd(19-1) <= bus_ioskp       ;
    paddlrdd(20-1) <= bus_iot2q       ;
    paddlrdd(21-1) <= bus_N_ln_wrt    ;
    paddlrdd(22-1) <= bus_N_lnq       ;
    paddlrdd(23-1) <= bus_lnq         ;
    paddlrdd(24-1) <= bus_N_ma_aluq   ;
    paddlrdd(25-1) <= bus_mql         ;
    paddlrdd(26-1) <= bus_N_mread     ;
    paddlrdd(27-1) <= bus_N_mwrite    ;
    paddlrdd(28-1) <= bus_N_pc_aluq   ;
    paddlrdd(29-1) <= bus_N_pc_inc    ;
    paddlrdd(30-1) <= bus_reset       ;
    paddlrdd(31-1) <= bus_N_newlink   ;
    paddlrdd(32-1) <= bus_tad3q       ;

    -- paddle bits written by the zynq ps (raspictl zynqlib)
    -- to make up for missing boards when board is disabled via boardena bit

    pad_acq(11)     <= paddlwra( 2-1);
    pad_N_aluq(11)  <= paddlwra( 3-1);
    pad_N_maq(11)   <= paddlwra( 4-1);
    pad_maq(11)     <= paddlwra( 5-1);
    pad_mq(11)      <= paddlwra( 6-1);
    pad_pcq(11)     <= paddlwra( 7-1);
    pad_irq(11)     <= paddlwra( 8-1);
    pad_acq(10)     <= paddlwra( 9-1);
    pad_N_aluq(10)  <= paddlwra(10-1);
    pad_N_maq(10)   <= paddlwra(11-1);
    pad_maq(10)     <= paddlwra(12-1);
    pad_mq(10)      <= paddlwra(13-1);
    pad_pcq(10)     <= paddlwra(14-1);
    pad_irq(10)     <= paddlwra(15-1);
    pad_acq( 9)     <= paddlwra(16-1);
    pad_N_aluq( 9)  <= paddlwra(17-1);
    pad_N_maq( 9)   <= paddlwra(18-1);
    pad_maq( 9)     <= paddlwra(19-1);
    pad_mq( 9)      <= paddlwra(20-1);
    pad_pcq( 9)     <= paddlwra(21-1);
    pad_irq( 9)     <= paddlwra(22-1);
    pad_acq( 8)     <= paddlwra(23-1);
    pad_N_aluq( 8)  <= paddlwra(24-1);
    pad_N_maq( 8)   <= paddlwra(25-1);
    pad_maq( 8)     <= paddlwra(26-1);
    pad_mq( 8)      <= paddlwra(27-1);
    pad_pcq( 8)     <= paddlwra(28-1);
    pad_acq( 7)     <= paddlwra(30-1);
    pad_N_aluq( 7)  <= paddlwra(31-1);
    pad_N_maq( 7)   <= paddlwra(32-1);

    pad_maq( 7)     <= paddlwrb( 2-1);
    pad_mq( 7)      <= paddlwrb( 3-1);
    pad_pcq( 7)     <= paddlwrb( 4-1);
    pad_acq( 6)     <= paddlwrb( 6-1);
    pad_N_aluq( 6)  <= paddlwrb( 7-1);
    pad_N_maq( 6)   <= paddlwrb( 8-1);
    pad_maq( 6)     <= paddlwrb( 9-1);
    pad_mq( 6)      <= paddlwrb(10-1);
    pad_pcq( 6)     <= paddlwrb(11-1);
    pad_N_jump      <= paddlwrb(12-1);
    pad_acq( 5)     <= paddlwrb(13-1);
    pad_N_aluq( 5)  <= paddlwrb(14-1);
    pad_N_maq( 5)   <= paddlwrb(15-1);
    pad_maq( 5)     <= paddlwrb(16-1);
    pad_mq( 5)      <= paddlwrb(17-1);
    pad_pcq( 5)     <= paddlwrb(18-1);
    pad_N_alub_m1   <= paddlwrb(19-1);
    pad_acq( 4)     <= paddlwrb(20-1);
    pad_N_aluq( 4)  <= paddlwrb(21-1);
    pad_N_maq( 4)   <= paddlwrb(22-1);
    pad_maq( 4)     <= paddlwrb(23-1);
    pad_mq( 4)      <= paddlwrb(24-1);
    pad_pcq( 4)     <= paddlwrb(25-1);
    pad_acqzero     <= paddlwrb(26-1);
    pad_acq( 3)     <= paddlwrb(27-1);
    pad_N_aluq( 3)  <= paddlwrb(28-1);
    pad_N_maq( 3)   <= paddlwrb(29-1);
    pad_maq( 3)     <= paddlwrb(30-1);
    pad_mq( 3)      <= paddlwrb(31-1);
    pad_pcq( 3)     <= paddlwrb(32-1);

    pad_acq( 2)     <= paddlwrc( 2-1);
    pad_N_aluq( 2)  <= paddlwrc( 3-1);
    pad_N_maq( 2)   <= paddlwrc( 4-1);
    pad_maq( 2)     <= paddlwrc( 5-1);
    pad_mq( 2)      <= paddlwrc( 6-1);
    pad_pcq( 2)     <= paddlwrc( 7-1);
    pad_N_ac_sc     <= paddlwrc( 8-1);
    pad_acq( 1)     <= paddlwrc( 9-1);
    pad_N_aluq( 1)  <= paddlwrc(10-1);
    pad_N_maq( 1)   <= paddlwrc(11-1);
    pad_maq( 1)     <= paddlwrc(12-1);
    pad_mq( 1)      <= paddlwrc(13-1);
    pad_pcq( 1)     <= paddlwrc(14-1);
    pad_intak1q     <= paddlwrc(15-1);
    pad_acq( 0)     <= paddlwrc(16-1);
    pad_N_aluq( 0)  <= paddlwrc(17-1);
    pad_N_maq( 0)   <= paddlwrc(18-1);
    pad_maq( 0)     <= paddlwrc(19-1);
    pad_mq( 0)      <= paddlwrc(20-1);
    pad_pcq( 0)     <= paddlwrc(21-1);
    pad_fetch1q     <= paddlwrc(22-1);
    pad_N_ac_aluq   <= paddlwrc(23-1);
    pad_N_alu_add   <= paddlwrc(24-1);
    pad_N_alu_and   <= paddlwrc(25-1);
    pad_N_alua_m1   <= paddlwrc(26-1);
    pad_N_alucout   <= paddlwrc(27-1);
    pad_N_alua_ma   <= paddlwrc(28-1);
    pad_alua_mq0600 <= paddlwrc(29-1);
    pad_alua_mq1107 <= paddlwrc(30-1);
    pad_alua_pc0600 <= paddlwrc(31-1);
    pad_alua_pc1107 <= paddlwrc(32-1);

    pad_alub_1      <= paddlwrd( 2-1);
    pad_N_alub_ac   <= paddlwrd( 3-1);
    pad_clok2       <= paddlwrd( 4-1);
    pad_fetch2q     <= paddlwrd( 5-1);
    pad_N_grpa1q    <= paddlwrd( 6-1);
    pad_defer1q     <= paddlwrd( 7-1);
    pad_defer2q     <= paddlwrd( 8-1);
    pad_defer3q     <= paddlwrd( 9-1);
    pad_exec1q      <= paddlwrd(10-1);
    pad_grpb_skip   <= paddlwrd(11-1);
    pad_exec2q      <= paddlwrd(12-1);
    pad_N_dfrm      <= paddlwrd(13-1);
    pad_inc_axb     <= paddlwrd(14-1);
    pad_N_intak     <= paddlwrd(15-1);
    pad_intrq       <= paddlwrd(16-1);
    pad_exec3q      <= paddlwrd(17-1);
    pad_ioinst      <= paddlwrd(18-1);
    pad_ioskp       <= paddlwrd(19-1);
    pad_iot2q       <= paddlwrd(20-1);
    pad_N_ln_wrt    <= paddlwrd(21-1);
    pad_N_lnq       <= paddlwrd(22-1);
    pad_lnq         <= paddlwrd(23-1);
    pad_N_ma_aluq   <= paddlwrd(24-1);
    pad_mql         <= paddlwrd(25-1);
    pad_N_mread     <= paddlwrd(26-1);
    pad_N_mwrite    <= paddlwrd(27-1);
    pad_N_pc_aluq   <= paddlwrd(28-1);
    pad_N_pc_inc    <= paddlwrd(29-1);
    pad_reset       <= paddlwrd(30-1);
    pad_N_newlink   <= paddlwrd(31-1);
    pad_tad3q       <= paddlwrd(32-1);

    -- split board enable register into its various bits
    -- when bit it set, use the corresponding module outputs
    -- when bit is clear, use the corresponding paddle outputs
    aclena <= boardena(0) = '1';
    aluena <= boardena(1) = '1';
    maena  <= boardena(2) = '1';
    pcena  <= boardena(3) = '1';
    rpiena <= boardena(4) = '1';
    seqena <= boardena(5) = '1';

    -- for bus signals output by the acl board, select either the module or the paddles
    bus_acq       <= acl_acq       when aclena else pad_acq;
    bus_acqzero   <= acl_acqzero   when aclena else pad_acqzero;
    bus_grpb_skip <= acl_grpb_skip when aclena else pad_grpb_skip;
    bus_N_lnq     <= acl_N_lnq     when aclena else pad_N_lnq;
    bus_lnq       <= acl_lnq       when aclena else pad_lnq;

    -- for bus signals output by the alu board, select either the module or the paddles
    bus_N_alucout <= alu_N_alucout when aluena else pad_N_alucout;
    bus_N_aluq    <= alu_N_aluq    when aluena else pad_N_aluq;
    bus_N_newlink <= alu_N_newlink when aluena else pad_N_newlink;

    -- for bus signals output by the ma board, select either the module or the paddles
    bus_N_maq <= ma_N_maq when maena else pad_N_maq;
    bus_maq   <= ma_maq   when maena else pad_maq;

    -- for bus signals output by the pc board, select either the module or the paddles
    bus_pcq <= pc_pcq when pcena else pad_pcq;

    -- for bus signals output by the rpi board, select either the gpio signals or the paddles
    bus_clok2 <= rpi_clok2 when rpiena else pad_clok2;
    bus_intrq <= rpi_intrq when rpiena else pad_intrq;
    bus_ioskp <= rpi_ioskp when rpiena else pad_ioskp;
    bus_mql   <= rpi_mql   when rpiena else pad_mql;
    bus_mq    <= rpi_mq    when rpiena else pad_mq;
    bus_reset <= rpi_reset when rpiena else pad_reset;

    -- for bus signals output by the seq board, select either the module or the paddles
    bus_N_ac_aluq   <= seq_N_ac_aluq   when seqena else pad_N_ac_aluq;
    bus_N_ac_sc     <= seq_N_ac_sc     when seqena else pad_N_ac_sc;
    bus_N_alu_add   <= seq_N_alu_add   when seqena else pad_N_alu_add;
    bus_N_alu_and   <= seq_N_alu_and   when seqena else pad_N_alu_and;
    bus_N_alua_m1   <= seq_N_alua_m1   when seqena else pad_N_alua_m1;
    bus_N_alua_ma   <= seq_N_alua_ma   when seqena else pad_N_alua_ma;
    bus_alua_mq0600 <= seq_alua_mq0600 when seqena else pad_alua_mq0600;
    bus_alua_mq1107 <= seq_alua_mq1107 when seqena else pad_alua_mq1107;
    bus_alua_pc0600 <= seq_alua_pc0600 when seqena else pad_alua_pc0600;
    bus_alua_pc1107 <= seq_alua_pc1107 when seqena else pad_alua_pc1107;
    bus_alub_1      <= seq_alub_1      when seqena else pad_alub_1;
    bus_N_alub_ac   <= seq_N_alub_ac   when seqena else pad_N_alub_ac;
    bus_N_alub_m1   <= seq_N_alub_m1   when seqena else pad_N_alub_m1;
    bus_N_grpa1q    <= seq_N_grpa1q    when seqena else pad_N_grpa1q;
    bus_N_dfrm      <= seq_N_dfrm      when seqena else pad_N_dfrm;
    bus_N_jump      <= seq_N_jump      when seqena else pad_N_jump;
    bus_inc_axb     <= seq_inc_axb     when seqena else pad_inc_axb;
    bus_N_intak     <= seq_N_intak     when seqena else pad_N_intak;
    bus_intak1q     <= seq_intak1q     when seqena else pad_intak1q;
    bus_ioinst      <= seq_ioinst      when seqena else pad_ioinst;
    bus_iot2q       <= seq_iot2q       when seqena else pad_iot2q;
    bus_N_ln_wrt    <= seq_N_ln_wrt    when seqena else pad_N_ln_wrt;
    bus_N_ma_aluq   <= seq_N_ma_aluq   when seqena else pad_N_ma_aluq;
    bus_N_mread     <= seq_N_mread     when seqena else pad_N_mread;
    bus_N_mwrite    <= seq_N_mwrite    when seqena else pad_N_mwrite;
    bus_N_pc_aluq   <= seq_N_pc_aluq   when seqena else pad_N_pc_aluq;
    bus_N_pc_inc    <= seq_N_pc_inc    when seqena else pad_N_pc_inc;
    bus_tad3q       <= seq_tad3q       when seqena else pad_tad3q;
    bus_fetch1q     <= seq_fetch1q     when seqena else pad_fetch1q;
    bus_fetch2q     <= seq_fetch2q     when seqena else pad_fetch2q;
    bus_defer1q     <= seq_defer1q     when seqena else pad_defer1q;
    bus_defer2q     <= seq_defer2q     when seqena else pad_defer2q;
    bus_defer3q     <= seq_defer3q     when seqena else pad_defer3q;
    bus_exec1q      <= seq_exec1q      when seqena else pad_exec1q;
    bus_exec2q      <= seq_exec2q      when seqena else pad_exec2q;
    bus_exec3q      <= seq_exec3q      when seqena else pad_exec3q;
    bus_irq         <= seq_irq         when seqena else pad_irq;

    -- instantiate the modules
    -- for input pins, use the bus signals
    -- for output pins, use the module-specific signal

    aclinst: entity aclcirc port map (
        uclk => CLOCK,
        N_ac_aluq   => bus_N_ac_aluq,
        N_ac_sc     => bus_N_ac_sc,
        acq         => acl_acq,
        acqzero     => acl_acqzero,
        N_aluq      => bus_N_aluq,
        clok2       => bus_clok2,
        N_grpa1q    => bus_N_grpa1q,
        grpb_skip   => acl_grpb_skip,
        iot2q       => bus_iot2q,
        N_ln_wrt    => bus_N_ln_wrt,
        N_lnq       => acl_N_lnq,
        lnq         => acl_lnq,
        N_maq       => bus_N_maq,
        maq         => bus_maq,
        mql         => bus_mql,
        N_newlink   => bus_N_newlink,
        reset       => bus_reset,
        tad3q       => bus_tad3q
    );

    aluinst: entity alucirc port map (
        uclk => CLOCK,
        acq         => bus_acq,
        N_alu_add   => bus_N_alu_add,
        N_alu_and   => bus_N_alu_and,
        N_alua_m1   => bus_N_alua_m1,
        N_alua_ma   => bus_N_alua_ma,
        alua_mq0600 => bus_alua_mq0600,
        alua_mq1107 => bus_alua_mq1107,
        alua_pc0600 => bus_alua_pc0600,
        alua_pc1107 => bus_alua_pc1107,
        alub_1      => bus_alub_1,
        N_alub_ac   => bus_N_alub_ac,
        N_alub_m1   => bus_N_alub_m1,
        N_alucout   => alu_N_alucout,
        N_aluq      => alu_N_aluq,
        N_grpa1q    => bus_N_grpa1q,
        inc_axb     => bus_inc_axb,
        N_lnq       => bus_N_lnq,
        lnq         => bus_lnq,
        N_maq       => bus_N_maq,
        maq         => bus_maq,
        mq          => bus_mq,
        N_newlink   => alu_N_newlink,
        pcq         => bus_pcq
    );

    mainst: entity macirc port map (
        uclk => CLOCK,
        N_aluq      => bus_N_aluq,
        clok2       => bus_clok2,
        N_ma_aluq   => bus_N_ma_aluq,
        N_maq       => ma_N_maq,
        maq         => ma_maq,
        reset       => bus_reset
    );

    pcinst: entity pccirc port map (
        uclk => CLOCK,
        N_aluq      => bus_N_aluq,
        clok2       => bus_clok2,
        N_pc_aluq   => bus_N_pc_aluq,
        N_pc_inc    => bus_N_pc_inc,
        pcq         => pc_pcq,
        reset       => bus_reset
    );

    seqinst: entity seqcirc port map (
        uclk => CLOCK,
        N_ac_aluq   => seq_N_ac_aluq,
        N_ac_sc     => seq_N_ac_sc,
        acqzero     => bus_acqzero,
        N_alu_add   => seq_N_alu_add,
        N_alu_and   => seq_N_alu_and,
        N_alua_m1   => seq_N_alua_m1,
        N_alua_ma   => seq_N_alua_ma,
        alua_mq0600 => seq_alua_mq0600,
        alua_mq1107 => seq_alua_mq1107,
        alua_pc0600 => seq_alua_pc0600,
        alua_pc1107 => seq_alua_pc1107,
        alub_1      => seq_alub_1,
        N_alub_ac   => seq_N_alub_ac,
        N_alub_m1   => seq_N_alub_m1,
        N_alucout   => bus_N_alucout,
        clok2       => bus_clok2,
        N_grpa1q    => seq_N_grpa1q,
        grpb_skip   => bus_grpb_skip,
        N_dfrm      => seq_N_dfrm,
        N_jump      => seq_N_jump,
        inc_axb     => seq_inc_axb,
        N_intak     => seq_N_intak,
        intrq       => bus_intrq,
        intak1q     => seq_intak1q,
        ioinst      => seq_ioinst,
        ioskp       => bus_ioskp,
        iot2q       => seq_iot2q,
        N_ln_wrt    => seq_N_ln_wrt,
        N_ma_aluq   => seq_N_ma_aluq,
        N_maq       => bus_N_maq,
        maq         => bus_maq,
        mq          => bus_mq,
        N_mread     => seq_N_mread,
        N_mwrite    => seq_N_mwrite,
        N_pc_aluq   => seq_N_pc_aluq,
        N_pc_inc    => seq_N_pc_inc,
        reset       => bus_reset,
        tad3q       => seq_tad3q,
        fetch1q     => seq_fetch1q,
        fetch2q     => seq_fetch2q,
        defer1q     => seq_defer1q,
        defer2q     => seq_defer2q,
        defer3q     => seq_defer3q,
        exec1q      => seq_exec1q,
        exec2q      => seq_exec2q,
        exec3q      => seq_exec3q,
        irq         => seq_irq
    );
end rtl;
