# EtherCard
I have ported this ENC28J60 arduino library for ESP32 micro controller. original efforts of writing this library 
is someone else. You can find original library here https://github.com/njh/EtherCard

**EtherCard** is a driver for the Microchip ENC28J60 chip, compatible with Arduino IDE.
It is adapted and extended from code written by Guido Socher and Pascal Stang.

High-level routines are provided to allow a variety of purposes 
including simple data transfer through to HTTP handling.

License: GPL2


## Requirements

* Hardware: This library **only** supports the ENC28J60 chip.
* Hardware: Only **ESP32** microcontroller is supported
* Hardware: This library uses the SPI interface of the microcontroller,
  and will require at least one dedicated pin for CS, plus the SO, SI, and
  SCK pins of the SPI interface.
* Software: Any Arduino IDE >= 1.0.0 should be fine


## Library Installation

It can be downloaded directly from GitHub:

1. Download the ZIP file from https://github.com/njh/EtherCard/archive/master.zip
2. Rename the downloaded file to `ethercard.zip`
3. From the Arduino IDE: Sketch -> Include Library... -> Add .ZIP Library...
4. Restart the Arduino IDE to see the new "EtherCard" library with examples

See the comments in the example sketches for details about how to try them out.


## Physical Installation

### PIN Connections (Using ESP32 wroom):

If using HSPI (Default)

| ENC28J60 | ESP32       | Notes                                       |
|----------|-------------|---------------------------------------------|
| VCC      | Vin(5v)     |                                             |
| GND      | GND         |                                             |
| SCLK     | GPIO14      |                                             |
| MISO     | GPIO12      |                                             |
| MOSI     | GPIO13      |                                             |
| CS       | GPIO15      |                                             |

If using VSPI (Need to uncomment //#define SPI_VSPI 1 from enc28j60.cpp file)

| ENC28J60 | ESP32       | Notes                                       |
|----------|-------------|---------------------------------------------|
| VCC      | Vin(5v)     |                                             |
| GND      | GND         |                                             |
| SCLK     | GPIO18      |                                             |
| MISO     | GPIO19      |                                             |
| MOSI     | GPIO23      |                                             |
| CS       | GPIO5       |                                             |


## Using the library

Several [example sketches] are provided with the library which demonstrate various features.
Below are descriptions on how to use the library.

Note: `ether` is a globally defined instance of the `EtherCard` class and may be used to access the library.

### Getting Start
You can use below code to test ethernet connection. Here you have to connect enc28j60 to wifi router lan port.
Check that you ge IP address from router.
```cpp
#include <EtherCard.h>
#define STATIC 0 // set to 1 to disable DHCP (adjust myip/gwip values below)

// mac address
static byte mymac[] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };
// ethernet interface ip address
static byte myip[] = { 192, 168, 0, 200 };
// gateway ip address
static byte gwip[] = { 192, 168, 0, 1 };

// LED to control output
int ledPin10 = 9;

byte Ethernet::buffer[700];

char const page[] PROGMEM =
  "HTTP/1.0 503 Service Unavailable\r\n"
  "Content-Type: text/html\r\n"
  "Retry-After: 600\r\n"
  "\r\n"
  "<html>"
  "<head><title>"
  "Service Temporarily Unavailable"
  "</title></head>"
  "<body>"
  "<h3>This page is used behind the scene</h3>"
  "<p><em>"
  "Commands to control LED are transferred to Arduino.<br />"
  "The syntax: http://192.168.0.XX/?LED10=OFF or ON"
  "</em></p>"
  "</body>"
  "</html>"
  ;

void setup () {
  pinMode(ledPin10, OUTPUT);

  Serial.begin(115200);
  Serial.println("Trying to get an IP...");

  Serial.print("MAC: ");
  for (byte i = 0; i < 6; ++i) {
    Serial.print(mymac[i], HEX);
    if (i < 5)
      Serial.print(':');
  }
  Serial.println();

  if (ether.begin(sizeof Ethernet::buffer, mymac,15) == 0)
  {
    Serial.println( "Failed to access Ethernet controller");
  }
  else
  {
    Serial.println("Ethernet controller access: OK");
  }
  ;

#if STATIC
  Serial.println( "Getting static IP.");
  if (!ether.staticSetup(myip, gwip)) {
    Serial.println( "could not get a static IP");
    blinkLed(); // blink forever to indicate a problem
  }
#else

  Serial.println("Setting up DHCP");
  if (!ether.dhcpSetup()) {
    Serial.println( "DHCP failed");
    blinkLed(); // blink forever to indicate a problem
  }
#endif

  ether.printIp("My IP: ", ether.myip);
  ether.printIp("Netmask: ", ether.netmask);
  ether.printIp("GW IP: ", ether.gwip);
  ether.printIp("DNS IP: ", ether.dnsip);
}

void loop () {

  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);

  // IF LED10=ON turn it ON
  if (strstr((char *)Ethernet::buffer + pos, "GET /?LED10=ON") != 0) {
    Serial.println("Received ON command");
    digitalWrite(ledPin10, HIGH);
  }

  // IF LED10=OFF turn it OFF
  if (strstr((char *)Ethernet::buffer + pos, "GET /?LED10=OFF") != 0) {
    Serial.println("Received OFF command");
    digitalWrite(ledPin10, LOW);
  }

  // show some data to the user
  memcpy_P(ether.tcpOffset(), page, sizeof page);
  ether.httpServerReply(sizeof page - 1);
}

void blinkLed() {
  while (true) {
    digitalWrite(ledPin10, HIGH);
    delay(500);
    digitalWrite(ledPin10, LOW);
    delay(500);
  }
}
```

### Initialising the library

Initiate To initiate the library call `ether.begin()`.

```cpp
uint8_t Ethernet::buffer[700]; // configure buffer size to 700 octets
static uint8_t mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 }; // define (unique on LAN) hardware (MAC) address

uint8_type vers = ether.begin(sizeof Ethernet::buffer, mymac);
if(vers == 0)
{
    // handle failure to initiate network interface
}
```

### Configure using DHCP

To configure IP address via DHCP use `ether.dhcpSetup()`.

```cpp
if(!ether.dhcpSetup())
{
    // handle failure to obtain IP address via DHCP
}
ether.printIp("IP:   ", ether.myip); // output IP address to Serial
ether.printIp("GW:   ", ether.gwip); // output gateway address to Serial
ether.printIp("Mask: ", ether.netmask); // output netmask to Serial
ether.printIp("DHCP server: ", ether.dhcpip); // output IP address of the DHCP server
```

### Static IP Address

To configure a static IP address use `ether.staticSetup()`.

```cpp
const static uint8_t ip[] = {192,168,0,100};
const static uint8_t gw[] = {192,168,0,254};
const static uint8_t dns[] = {192,168,0,1};

if(!ether.staticSetup(ip, gw, dns);
{
    // handle failure to configure static IP address (current implementation always returns true!)
}
```

### Send UDP packet

To send a UDP packet use `ether.sendUdp()`.

```C
char payload[] = "My UDP message";
uint8_t nSourcePort = 1234;
uint8_t nDestinationPort = 5678;
uint8_t ipDestinationAddress[IP_LEN];
ether.parseIp(ipDestinationAddress, "192.168.0.200");

ether.sendUdp(payload, sizeof(payload), nSourcePort, ipDestinationAddress, nDestinationPort);
```

### DNS Lookup

To perform a DNS lookup use `ether.dnsLookup()`.

```
if(!ether.dnsLookup("google.com"))
{
    // handle failure of DNS lookup
}
ether.printIp("Server: ", ether.hisip); // Result of DNS lookup is placed in the hisip member of EtherCard.
```