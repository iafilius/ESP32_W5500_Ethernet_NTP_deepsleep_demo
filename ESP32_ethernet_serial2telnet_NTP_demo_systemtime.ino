// Demo of a W5500 Ethernet module on ESP32 and a few concepts demo's:
// Hardware
// -ESP32 (Doitdev devkit v1)
// -W5500 based 10/100Mbps Wired/UTP Ethernet (not Wifi/LWIP)  (Ethernet(2) Libary)  (Not needed when using WiFI)
// Concepts :
// -(S)NTP (Time over tcp/ip) (UTP/ethernet)   (Custom build packets) (Note: standard/building libary can be used when using WiFi instead Ethernet)
// -setting/getting systemtime   (standard/buildin C libary call's)
// -timezone using exact timezone specification string (standard/buildin C libary)
// -timezone using using filespec (:CET) (standard/buildin C libary) (FAILED)
// -daylightsavings (standard/buildin C libary)
// -using RTC time, actual time surviving deep sleep (standard/buildin C libary)
// -Deep sleep (esp32/buildin libary)
// -Using RTC memory to store deep sleep persistent variabele  (esp32/buildin libary)

// Warning
// Avoid using the non working lwip based SNTP/DNS call's in combination of the W5500/Ethernet driver, a it is not integrated in LWIP.
// So buildin SNTP (esp32) function can't be used currently.


// Tested and working Pinout/connections:
// W5500  | ESP32
// --------------------
// 5V     | NC
// GND    | GND
// RST    | GPIO4
// INT    | NC
// NC     | NC
// 3.3V   | 3.3V
// MISO   | GPIO19
// MOSI   | GPIO23
// SCS    | GPIO5
// SCLK   | GPIO18



// https://platformio.org/lib/show/134/Ethernet/examples?file=LinkStatus.ino
// https://www.pjrc.com/teensy/td_libs_Ethernet.html

// modified SPI.h with adding:
// vi ./AppData/Local/Arduino15/packages/esp32/hardware/esp32/1.0.0/libraries/SPI/src/SPI.h
//    void transfer(uint8_t * data, uint32_t size) { transferBytes(data, data, size); }
// See example SPI_Multgiple busses for pintout:
//initialise hspi with default pins
//SCLK = 14, MISO = 12, MOSI = 13, SS = 15

#include <SPI.h>

#include <Ethernet.h>
#include <EthernetUDP.h>
#include <time.h>
#include <sys/time.h>


IPAddress ip(192, 168, 1, 250);
//IPAddress server(192, 168, 1, 200);
IPAddress server(194, 9, 85, 141); // CONNECT abc.nl:80 HTTP/1.1
// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 23 is default for telnet;
// if you're using Processing's ChatServer, use port 10002):

EthernetClient client;
// Using GPIO port 4 for resetting W5500 at boot time
#define W5500_RST_PORT   4
byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };


// NTP
unsigned int localPort = 8888;         // local port to listen for UDP packets
char timeServer[] = "nl.pool.ntp.org"; // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48;        // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE];   //buffer to hold incoming and outgoing packets
// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;


// http://www.lucadentella.it/en/2017/05/11/esp32-17-sntp/
//


// Deep sleep
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60        /* Time ESP32 will go to sleep (in seconds) */

// RTC stored variabele
RTC_DATA_ATTR int bootCount = 0;


void setup() {
  bootCount = bootCount + 1;

  // Always set timezone, after boot or wake-up
  setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
  // man7.org/linux/man-pages/man3/tzset.3.html
  //setenv("TZ",":CET",1);
  tzset();

  Serial.begin(112500);


  Serial.println("ESP32 serial W5500, NTP over ethernet, deep sleep...  demo");
  Serial.print("bootCount: ");
  Serial.println(bootCount);

  Serial.println("Printing time after boot/wake-up (which fails when not set):");
  printLocalTime();


  Serial.println("Used/default ESP32 pinout:");
  Serial.print("MOSI:");
  Serial.println(MOSI);
  Serial.print("MISO:");
  Serial.println(MISO);
  Serial.print("SCK:");
  Serial.println(SCK);
  Serial.print("SS:");
  Serial.println(SS);
  //  Serial.println(RST);
  Serial.print("Reset:");
  Serial.println(W5500_RST_PORT);

  SPI.begin();


  Serial.println("Resetting WizNet for consistent results");
  pinMode(W5500_RST_PORT, OUTPUT);
  digitalWrite(W5500_RST_PORT, LOW);
  delayMicroseconds(500);
  digitalWrite(W5500_RST_PORT, HIGH);
  delayMicroseconds(1000);

  // https://www.arduino.cc/en/Reference/EthernetInit
  // You can use Ethernet.init(pin) to configure the CS pin
  //Ethernet.init(10);  // Most Arduino shields
  //Ethernet.init(5);   // MKR ETH shield
  //Ethernet.init(0);   // Teensy 2.0
  //Ethernet.init(20);  // Teensy++ 2.0
  //Ethernet.init(15);  // ESP8266 with Adafruit Featherwing Ethernet
  //Ethernet.init(33);  // ESP32 with Adafruit Featherwing Ethernet
  Ethernet.init(SS);    // NOT optional!!!

  Serial.println("Starting ethernet");
  // https://www.arduino.cc/en/Reference/EthernetBegin
  Ethernet.begin(mac);
  //Ethernet.begin(mac,ip);

  delay(1000);

  switch (Ethernet.hardwareStatus()) {
    case EthernetNoHardware:
      Serial.println("Ethernet Hardware was not found, can't continue...");
      while (true) {
        delay(1); // do nothing
      }
      break;
    case EthernetW5100:
      Serial.println("W5100 hardware found");
      break;
    case EthernetW5200:
      Serial.println("W5200 hardware found");
      break;
    case EthernetW5500:
      Serial.println("W5500 hardware found");
      break;
    default:
      Serial.print("Undefined hardware found:");
      Serial.println(Ethernet.hardwareStatus());

      Serial.print("No Hardware value::");
      Serial.println(EthernetNoHardware);

      break;
  }


  do {
    delay(500);
    switch (Ethernet.linkStatus()) {
      case Unknown:
        Serial.println("Unknown link status");
        break;
      case LinkOFF:
        Serial.println("LinkOFF");
        break;
      case LinkON:
        Serial.println("LinkON");
        break;
      default:
        Serial.println("Undefined link status");
    }
  } while (Ethernet.linkStatus() != LinkON);


  // https://www.pjrc.com/arduino-ethernet-library-2-0-0/
  Serial.print("LocalIP:");
  Serial.println(Ethernet.localIP());

  Serial.print("GW:");
  Serial.println(Ethernet.gatewayIP());

  Serial.print("Subnet Netmask:");
  Serial.println(Ethernet.subnetMask());

  Serial.print("DNS Server IP:");
  Serial.println(Ethernet.dnsServerIP());

  //Serial.print("MAC:");
  //Serial.println(Ethernet.MACAddress());

  Udp.begin(localPort);

  // give the Ethernet shield a second to initialize:
  //delay(1000);

  // Serial 2 network port bridge demo
  Serial.print("connecting to ");
  Serial.print(server);
  Serial.println("...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    Serial.println("connected");
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }

}


void loop() {
  // https://esp32.com/viewtopic.php?t=5398
  //ESP_LOGD(TAG, "%s()", __FUNCTION__);  // function to demo time

  // Get time using NTP
  syncTime_Ethernet ();



  // if there are incoming bytes available
  // from the server, read them and print them:
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

  // as long as there are bytes in the serial queue,
  // read them and send them out the socket if it's open:
  while (Serial.available() > 0) {
    char inChar = Serial.read();
    if (client.connected()) {
      client.print(inChar);
    }
  }

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    // do nothing:
    /*    while (true) {
          delay(1);
        }
    */
  }



  delay(5000);
  printLocalTime();

  if (millis() > 60000) {
    //Serial.print("millis:");
    //Serial.println(millis());
    Serial.println("Time to go asleep, until later again..");
    Serial.flush();
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
  }
}


void syncTime_Ethernet() {
  static bool hasrun = false;

  if (hasrun == false)
  {
    //hasrun=true;
    sendNTPpacket(timeServer); // send an NTP packet to a time server
    delay(1000);
    if ( Udp.parsePacket() ) {
      // We've received a packet, read the data from it
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

      //the timestamp starts at byte 40 of the received packet and is four bytes,
      // or two words, long. First, esxtract the two words:

      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      Serial.print("Seconds since Jan 1 1900 = " );
      Serial.println(secsSince1900);

      // now convert NTP time into everyday time:
      Serial.print("Unix time = ");
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      //unsigned long epoch = secsSince1900 - seventyYears;
      time_t epoch = secsSince1900 - seventyYears;
      // print Unix time:
      Serial.println(epoch);


      // print the hour, minute and second:
      Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
      Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
      Serial.print(':');
      if ( ((epoch % 3600) / 60) < 10 ) {
        // In the first 10 minutes of each hour, we'll want a leading '0'
        Serial.print('0');
      }
      Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
      Serial.print(':');
      if ( (epoch % 60) < 10 ) {
        // In the first 10 seconds of each minute, we'll want a leading '0'
        Serial.print('0');
      }
      Serial.println(epoch % 60); // print the second


      {
        struct tm tm;
        char char_buf[1024];

        // use _r restartable libary call's
        localtime_r(&epoch, &tm);
        time_t t = mktime(&tm);

        // use _r restartable libary call's
        printf("Setting time: %s", asctime_r(&tm, char_buf));
        //struct timeval now = { .tv_sec = t };
        struct timeval now = { .tv_sec = epoch };

        settimeofday(&now, NULL);
        hasrun = true;

      }


    }
  }


}





// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(char* address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}




// https://esp32.com/viewtopic.php?t=5398
// http://zetcode.com/articles/cdatetime/
// The strftime() function formats date and time.
// better use: size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);
// https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.3.0/com.ibm.zos.v2r3.bpxbd00/strfti.htm

void printLocalTime()
{
  struct tm timeinfo;
  char str_buf[1024];

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  strftime(str_buf, sizeof(str_buf), "%A, %B %d %Y %H:%M:%S %F %T (%Z) weekday(sun): %u weeknumber(sun): %U weekday(mon): %w weeknumber(mon): %W TimeZoneOffset: %z ", &timeinfo);
  Serial.println(str_buf);

  // Code below seems limited by a 63 buffer, and prints garbled char when exceeding, do not use (code above just works fine)!!!
  /*
    // Serial has a default buffer of 64 chars, do not exceed...
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    Serial.print(&timeinfo, "%F %T (%Z) ");
    Serial.print(&timeinfo, "weekday(sun): %u weeknumber(sun): %U " );
    Serial.print(&timeinfo, "weekday(mon): %w weeknumber(mon): %W ");
    Serial.println(&timeinfo, "TimeZoneOffset: %z Zone: (%Z)");

    Serial.print("Epoch:");
    Serial.println(mktime(&timeinfo));
  */
}

