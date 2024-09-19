--    Copyright (C) Mike Rieker, Beverly, MA USA
--    www.outerworldapps.com
--
--    This program is free software; you can redistribute it and/or modify
--    it under the terms of the GNU General Public License as published by
--    the Free Software Foundation; version 2 of the License.
--
--    This program is distributed in the hope that it will be useful,
--    but WITHOUT ANY WARRANTY; without even the implied warranty of
--    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--    GNU General Public License for more details.
--
--    EXPECT it to FAIL when someone's HeALTh or PROpeRTy is at RISk.
--
--    You should have received a copy of the GNU General Public License
--    along with this program; if not, write to the Free Software
--    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
--
--    http://www.gnu.org/licenses/gpl-2.0.html

-- main program for the zynq implementation
-- contains gpio-like and paddle registers accessed via the axi bus
-- also contains a dma circuit just for testing dma code (not used for pdp-8)
--  and contains a led pwm circuit just for testing led (not used for pdp-8)

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity Zynq is
    Port (  CLOCK : in STD_LOGIC;
            RESET_N : in STD_LOGIC;
            LEDoutR : out STD_LOGIC;     -- IO_B34_LN6 R14
            LEDoutG : out STD_LOGIC;     -- IO_B34_LP7 Y16
            LEDoutB : out STD_LOGIC;     -- IO_B34_LN7 Y17

            fpscli : out STD_LOGIC;     -- composite static of SCL line
            fpsclo : in STD_LOGIC;      -- SCL coming from master
            fpsclt : in STD_LOGIC;      -- master is sending on SCL
            fpsdai : out STD_LOGIC;     -- composite state of SDA line
            fpsdao : in STD_LOGIC;      -- SDA coming from master
            fpsdat : in STD_LOGIC;      -- master is sending on SDA

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

    constant VERSION : std_logic_vector (31 downto 0) := x"00000171";

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
    signal numtrisoff, numtottris : std_logic_vector (9 downto 0);
    signal fpscl, fpsda, fpsdaj : std_logic;

    signal readaddr, writeaddr : std_logic_vector (11 downto 2);
    signal gpinput, gpoutput, gpcompos : std_logic_vector (31 downto 0);
    signal denadata, qenadata : std_logic_vector (12 downto 0);
    signal fpinput, fpoutput : std_logic_vector (31 downto 0);

    signal dmardaddr, dmawtaddr : std_logic_vector (31 downto 0);
    signal maxiARVALID, maxiRREADY, maxiAWVALID, maxiWVALID, maxiBREADY : std_logic;

    signal dmareadsel, dmawritesel : natural range 0 to 9;
    signal temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7, temp8, temp9 : std_logic_vector (31 downto 0);

begin

    -- bus values that are constants
    saxi_BRESP <= b"00";        -- A3.4.4/A10.3 transfer OK
    saxi_RRESP <= b"00";        -- A3.4.4/A10.3 transfer OK

    -- buffered outputs (outputs we read internally)
    saxi_ARREADY <= saxiARREADY;
    saxi_AWREADY <= saxiAWREADY;
    saxi_BVALID  <= saxiBVALID;
    saxi_RVALID  <= saxiRVALID;
    saxi_WREADY  <= saxiWREADY;

    ---------------------------------------------
    --  DMA test code -- not needed for pdp8v  --
    ---------------------------------------------

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

    -------------------------------------------------------
    --  silly LED PWM test code -- not needed for pdp8v  --
    -------------------------------------------------------

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

    -- send register being read to ARM

    saxi_RDATA <=        gpinput  when readaddr = b"0000000000" else
                         gpoutput when readaddr = b"0000000001" else
                         gpcompos when readaddr = b"0000000010" else
                          VERSION when readaddr = b"0000000011" else
     x"000000" & b"00" & boardena when readaddr = b"0000000100" else
     b"000000" & numtottris & b"000000" & numtrisoff when readaddr = b"0000000101" else
                         paddlrda when readaddr = b"0000001000" else
                         paddlrdb when readaddr = b"0000001001" else
                         paddlrdc when readaddr = b"0000001010" else
                         paddlrdd when readaddr = b"0000001011" else
                         paddlwra when readaddr = b"0000001100" else
                         paddlwrb when readaddr = b"0000001101" else
                         paddlwrc when readaddr = b"0000001110" else
                         paddlwrd when readaddr = b"0000001111" else
                         fpinput  when readaddr = b"0000010000" else
                         fpoutput when readaddr = b"0000010001" else
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
            fpoutput <= x"00000000";                        -- by default, front panel is disabled

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
                    when b"0000010001" => fpoutput  <= saxi_WDATA;
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

    -- pdp8v processor code in here

    bpinst: entity backplane port map (
        CLOCK => CLOCK,
        -- TRIGGR => TRIGGR,
        -- DEBUGS => DEBUGS,

        paddlrda => paddlrda,
        paddlrdb => paddlrdb,
        paddlrdc => paddlrdc,
        paddlrdd => paddlrdd,
        paddlwra => paddlwra,
        paddlwrb => paddlwrb,
        paddlwrc => paddlwrc,
        paddlwrd => paddlwrd,

        boardena => boardena,

        gpinput => gpinput,
        gpoutput => gpoutput,
        gpcompos => gpcompos,
        nto => numtrisoff,
        ntt => numtottris
    );

    -- front panel code in here

    fpscli <= fpscl;
    fpscl  <= fpsclo when fpsclt = '1' else '1';
    fpsda  <= fpsdao when fpsdat = '1' else '1';

    TRIGGR <= not fpsclo or not fpsdao or fpsclt or fpsdat;
    DEBUGS(0) <= fpsclo;
    DEBUGS(1) <= fpsdao;
    DEBUGS(2) <= fpsclt;
    DEBUGS(3) <= fpsdat;
    DEBUGS(4) <= fpsdaj;
    fpsdai <= fpsdaj;
    DEBUGS(13 downto 5) <= (others => '0');

    fpinst: entity frontpanel port map (
        scl    => fpscl,
        sdax   => fpsda,
        sday   => fpsdaj,
        CLOCK  => CLOCK,
        --TRIGGR => TRIGGR,
        --DEBUGS => DEBUGS,

        paddlrda => paddlrda,
        paddlrdb => paddlrdb,
        paddlrdc => paddlrdc,
        paddlrdd => paddlrdd,

        fpinput  => fpinput,
        fpoutput => fpoutput,
        gpcompos => gpcompos
    );
end rtl;
