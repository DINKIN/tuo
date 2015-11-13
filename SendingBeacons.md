# MAC80211 #

set the fallow flag in the driver initialization.

local->hw.flags & IEEE80211\_HW\_HOST\_GEN\_BEACON\_TEMPLATE

# Atheros Chipset #

setting AP mode

AR5K\_STA\_ID1 |= AR5K\_STA\_ID1\_AP

initialize beacon register

AR5K\_TIMER0
AR5K\_TIMER2
AR5K\_TIMER3
AR5K\_TIMER4

AR5K\_BEACON |= AR5K\_BEACON\_ENABLE [| AR5K\_BEACON\_RESET\_TSF ](.md)

enable SWBA

AR5K\_PIMR |= AR5K\_INT\_SWBA

configure the Receive filter register

AR5K\_RX\_FILTER |= AR5K\_RX\_FILTER\_BEACON