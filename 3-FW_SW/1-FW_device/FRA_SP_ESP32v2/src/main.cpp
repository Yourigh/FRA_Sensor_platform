// Demonstrates usage of the new udpServer feature.
// You can register the same function to multiple ports,
// and multiple functions to the same port.
//
// 2013-4-7 Brian Lee <cybexsoft@hotmail.com>
//
// License: GPLv2

//For Ethernet
#include <EtherCard.h>
#include <IPAddress.h>

//For time
#include <Time.h>
#include <TimeLib.h>

#define STATIC 0  // set to 1 to disable DHCP (adjust myip/gwip values below)

#if STATIC
// ethernet interface ip address
static byte myip[] = { 192,168,0,10 };
// gateway ip address
static byte gwip[] = { 192,168,0,2 };
#endif

// ethernet mac address - must be unique on your network
uint8_t chipid[6];

byte Ethernet::buffer[500]; // tcp/ip send and receive buffer

//UDP sender
const int dstPort PROGMEM = 1111;
const int srcPort PROGMEM = 1100;
static byte destIp[] = { 192,168,0,5 }; // UDP unicast on network
char textToSend[] = "test 123"; //debug

//callback that prints received packets to the serial port
void udpSerialPrint(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, const char *data, uint16_t len){
  IPAddress src(src_ip[0],src_ip[1],src_ip[2],src_ip[3]);

  Serial.print("dest_port: ");
  Serial.println(dest_port);
  Serial.print("src_port: ");
  Serial.println(src_port);


  Serial.print("src_IP: ");
  ether.printIp(src_ip);
  Serial.println("\ndata: ");
  Serial.println(data);

  Serial.printf("DataHEX: %02x:%02x:%02x:%02x:%02x\n",data[0], data[1], data[2], data[3], data[4]);
  uint32_t epoch_time = 0;
  epoch_time = ((uint32_t)data[0]<<24) | ((uint32_t)data[1]<<16) | ((uint32_t)data[2]<<8) | ((uint32_t)data[3]);
  Serial.printf("Epoch time received: %d\n",epoch_time);
  if (epoch_time > 1558883838){ //sanity check, if pc is sending time from past May 26 2019, ignore
    setTime(epoch_time);
    Serial.printf("Epoch time accepted: %d\n",epoch_time);
  }
}

void setup(){
  #define ETH_NRST_PIN 4
  pinMode(ETH_NRST_PIN,OUTPUT);
  digitalWrite(ETH_NRST_PIN,0);
  pinMode(33,OUTPUT);
  pinMode(2,OUTPUT); //LED devkit V1
  pinMode(27,OUTPUT);
  digitalWrite(27,0);
  Serial.begin(115200);
  Serial.println(F("\n[backSoon]"));
  delay(300);
  digitalWrite(33,1);
  digitalWrite(2,1);
  delay(300);
  digitalWrite(33,0);
  digitalWrite(2,0);
  digitalWrite(ETH_NRST_PIN,1);
  delay(10);

  esp_efuse_read_mac(chipid);
  chipid[5]++; //use MAC address for ETH one larger than WiFi MAC (WiFi MAC is chip ID)
  // Change 'SS' to your Slave Select pin, if you arn't using the default pin
  if (ether.begin(sizeof Ethernet::buffer, chipid, 23) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
#if STATIC
  ether.staticSetup(myip, gwip);
#else
  if (!ether.dhcpSetup())
    Serial.println(F("DHCP failed"));
#endif

  Serial.printf("ETH  MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",chipid[0], chipid[1], chipid[2], chipid[3], chipid[4], chipid[5]);
  chipid[5]--;
  Serial.printf("WiFi MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",chipid[0], chipid[1], chipid[2], chipid[3], chipid[4], chipid[5]);
  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);
  ether.printIp("DNS: ", ether.dnsip);
  ether.printIp("Sending to: ", destIp);//Destination IP for sender

  //register udpSerialPrint() to port 1337
  ether.udpServerListenOnPort(&udpSerialPrint, 1100);
/*
  //register udpSerialPrint() to port 42.
  ether.udpServerListenOnPort(&udpSerialPrint, 42);*/

  
  textToSend[0] = 0xAA;
  textToSend[1] = 0xBB;
  textToSend[2] = 1;
  textToSend[3] = 1;
  textToSend[4] = 4;
  textToSend[5] = 0;
}


void loop(){

  static uint32_t bl = 0;
  
  

static uint8_t count = 1;
  if (millis()>(bl+10000)){ //every 100ms
    bl = millis();
    digitalWrite(33, !digitalRead(33));
    digitalWrite(2, !digitalRead(2));
    textToSend[0] = count++; //debug scroll though ASCII table to see it moving
  
    ether.sendUdp(textToSend, sizeof(textToSend), srcPort, destIp, dstPort );

    bl = millis();
    Serial.printf("time: %d\n",now());
  }
  //this must be called for ethercard functions to work.
  ether.packetLoop(ether.packetReceive());
  
  //setTime(t);
  //now();
}