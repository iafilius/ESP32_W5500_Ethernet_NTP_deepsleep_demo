# ESP32_W5500_Ethernet_NTP_deepsleep_demo

This is a demo using the hardware ESP32 and W5500 ethernet modules 
using NTP over ethernet, setting timezone, serial 2 network session bridge, keep time and variabele over deep sleep


## Hardware

* ESP32 (Doitdev devkit v1)
* W5500 based 10/100Mbps Wired/UTP Ethernet (not Wifi/LWIP)  (Ethernet(2) Libary)  (Not needed when using WiFI)

No hardware modifictions where needed, just wire up, and start using it.

## Concepts demo'd:

* (S)NTP (Time over tcp/ip) (UTP/ethernet)   (Custom build packets) (Note: standard/building libary can be used when using WiFi instead Ethernet)
* setting/getting systemtime   (standard/buildin C libary call's)
* timezone using exact timezone specification string (standard/buildin C libary)
* timezone using using filespec (:CET) (standard/buildin C libary) (FAILED)
* daylightsavings (standard/buildin C libary)
* using RTC time, actual time surviving deep sleep (standard/buildin C libary)
* Deep sleep (esp32/buildin libary)
* Using RTC memory to store deep sleep persistent variabele  (esp32/buildin libary)
* Creating a low priority for NTP updates in the background


## Costs

The ESP32 dev bord can be optained for about $4 and the W5500 module for about $2.50 (incl postage)
A more compact version of the W5500 module costs $1 more at this time. (11/2018)

## remarks

The Ethernet module/libary is NOT integrated into the LWIP stack on the ESP32 currently, so you need to use the Ethernet libary functions. ESP32 native sntp is not usable for example, and the list is likely much longer.

For LWIP support you may have a close look at:

[ESP8266 W5500lwIP](https://github.com/d-a-v/W5500lwIP)
and
[ESP32 W5500lwIP](https://github.com/johnnytolengo/w5500lwip)

At the time of writing it seemed not ready for use yet, but progres is made.




