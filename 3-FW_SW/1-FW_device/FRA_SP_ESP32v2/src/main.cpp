// Demonstrates usage of the new udpServer feature.
// You can register the same function to multiple ports,
// and multiple functions to the same port.
//
// 2013-4-7 Brian Lee <cybexsoft@hotmail.com>
//
// License: GPLv2

#include <EtherCard.h>
#include <IPAddress.h>

#define STATIC 0  // set to 1 to disable DHCP (adjust myip/gwip values below)

#if STATIC
// ethernet interface ip address
static byte myip[] = { 192,168,0,10 };
// gateway ip address
static byte gwip[] = { 192,168,0,2 };
#endif

// ethernet mac address - must be unique on your network
static byte mymac[] = { 0x70,0x69,0x69,0x2D,0x30,0x31 };

byte Ethernet::buffer[500]; // tcp/ip send and receive buffer

//UDP sender
const int dstPort PROGMEM = 1337;
const int srcPort PROGMEM = 4321;
static byte destIp[] = { 192,168,0,210 }; // UDP unicast on network
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
}

void setup(){
  pinMode(33,OUTPUT);
  pinMode(27,OUTPUT);
  digitalWrite(27,0);
  Serial.begin(921600);
  Serial.println(F("\n[backSoon]"));
  delay(300);
  digitalWrite(33,1);
  delay(300);
  digitalWrite(33,0);
  // Change 'SS' to your Slave Select pin, if you arn't using the default pin
  if (ether.begin(sizeof Ethernet::buffer, mymac, 23) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
#if STATIC
  ether.staticSetup(myip, gwip);
#else
  if (!ether.dhcpSetup())
    Serial.println(F("DHCP failed"));
#endif

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);
  ether.printIp("DNS: ", ether.dnsip);
  ether.printIp("Sending to: ", destIp);//Destination IP for sender

  //register udpSerialPrint() to port 1337
  ether.udpServerListenOnPort(&udpSerialPrint, 1337);

  //register udpSerialPrint() to port 42.
  ether.udpServerListenOnPort(&udpSerialPrint, 42);

  //uint8_t ipDestinationAddress[IP_LEN];
  //ether.parseIp(ipDestinationAddress, "192.168.0.210");
}


void loop(){

  static uint32_t bl = 0;
  static uint8_t count = 0;
  

  if (millis()>(bl+100)){
    bl = millis();
    digitalWrite(33, !digitalRead(33));

    textToSend[0] = count++; //debug scroll though ASCII table to see it moving
  
    ether.sendUdp(textToSend, sizeof(textToSend), srcPort, destIp, dstPort );
  }
  //this must be called for ethercard functions to work.
  ether.packetLoop(ether.packetReceive());
  
  
}

/*
//Processing sketch to send test UDP packets.

import hypermedia.net.*;

 UDP udp;  // define the UDP object


 void setup() {
 udp = new UDP( this, 6000 );  // create a new datagram connection on port 6000
 //udp.log( true );     // <-- printout the connection activity
 udp.listen( true );           // and wait for incoming message
 }

 void draw()
 {
 }

 void keyPressed() {
 String ip       = "192.168.0.200";  // the remote IP address
 int port        = 1337;    // the destination port

 udp.send("Greetings via UDP!", ip, port );   // the message to send

 }

 void receive( byte[] data ) {       // <-- default handler
 //void receive( byte[] data, String ip, int port ) {  // <-- extended handler

 for(int i=0; i < data.length; i++)
 print(char(data[i]));
 println();
 }
*/
