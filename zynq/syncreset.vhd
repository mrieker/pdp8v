
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity SyncReset is
    Port (  CLOCK : in STD_LOGIC;
            RESET_N : in STD_LOGIC;
            SYNCRES_N : out STD_LOGIC);
end SyncReset;

architecture rtl of SyncReset is

    signal debounce : STD_LOGIC;

begin

    process (CLOCK, RESET_N)
    begin
        if RESET_N = '0' then
            SYNCRES_N <= '0';
            debounce <= '0';
        elsif rising_edge (CLOCK) then
            SYNCRES_N <= debounce;
            debounce <= '1';
        end if;
    end process;

end rtl;
