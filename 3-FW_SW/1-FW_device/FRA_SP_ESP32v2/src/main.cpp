// FRA main board source code
// Main pheripherals: ADCs, SD card, IO expander, Ethernet module
// License: GPLv2

//For Ethernet
#include <EtherCard.h>
#include <IPAddress.h>

//For time
#include <Time.h>
#include <TimeLib.h>

// Writer to SD card
#include <SPI.h>
#include "SdFat.h" //JR all SysCall::halt(); commented so the system will not go off
#include "sdios.h"
#include "FreeStack.h"

//GPIO expander
#include <Wire.h>
#define IOEXP_ADDR 0x75

// Writer to SD card
#define SD_CHIP_SELECT  17  // SD chip select pin
// file system object
SdFat sd;
// text file for logging
ofstream logfile;
// buffer to format data - makes it eaiser to echo to Serial / inherited
char SDbuf[300];//limited to 299 characters
// store error strings in flash to save RAM
#define error(s) sd.errorHalt(F(s))
bool use_sd=1;

//For Ethernet
#define STATIC 0  // set to 1 to disable DHCP (adjust myip/gwip values below)
//without DHCP there was a problem with UDP
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
uint8_t sendUDP_Buffer[100]; 
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

//functions
uint8_t sd_create_file();
uint8_t sd_format_header();
uint8_t sd_write_sample();
uint8_t ioexp_init();
uint8_t ioexp_out_all_low();
//debug functions
void debug_sd_log();
void debug_GPIOexp();

void setup(){
  #define ETH_NRST_PIN 4
  #define ETH_CS_PIN 23
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
  if (ether.begin(sizeof Ethernet::buffer, chipid, ETH_CS_PIN) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
#if STATIC
  ether.staticSetup(myip, gwip);
#else
  Serial.println(F("Waiting for DHCP..."));
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
  
  //Default values for buffer
  sendUDP_Buffer[0] = 0xFF;
  sendUDP_Buffer[1] = 0xFF;
  sendUDP_Buffer[2] = 0xFF;
  sendUDP_Buffer[3] = 0xFF;
  sendUDP_Buffer[4] = 4;

  // Initialize at the highest speed supported by the board that is
  // not over 50 MHz. Try a lower speed if SPI errors occur.
  use_sd = 1;
  if (!sd.begin(SD_CHIP_SELECT, SD_SCK_MHZ(20))) {  //TO INCREASE ON FINAL PCB
    sd.initErrorHalt(); //halt turned off, but will give logs to serial
    use_sd=0;
  }

  //debug_sd_log();

  Wire.begin(21,22,400000UL);
  
  while(1){

  if (!ioexp_init())
    error("failed to init GPIO expander");
  delay(500);
  if (!ioexp_out_all_low())
    error("failed to communicate with GPIO exp.");
  delay(500);
  
  }
  debug_GPIOexp();
  
}


void loop(){
uint32_t bl = 0;
uint32_t timenow = 0;
uint8_t count = 1;
  while(1){
    if (millis()>(bl+2000)){ //every 2s
      bl = millis();
      digitalWrite(33, !digitalRead(33));
      digitalWrite(2, !digitalRead(2));

      timenow = now();
      sendUDP_Buffer[0] = timenow >> 24;
      sendUDP_Buffer[1] = timenow >> 16;
      sendUDP_Buffer[2] = timenow >> 8;
      sendUDP_Buffer[3] = timenow;
      ether.sendUdp((char *)sendUDP_Buffer, 5, srcPort, destIp, dstPort );

      bl = millis();
      Serial.printf("time: %d\n",now());
    }

    //this must be called for ethercard functions to work.
    ether.packetLoop(ether.packetReceive());
  }
}

uint8_t sd_create_file(){
    // create a new file in root, the current working directory
  char name[] = "logger0000.csv";

  for (uint16_t i = 0; i < 10000; i++) {
    name[6] = i/1000 + '0';
    name[7] = (i/100)%10 + '0';
    name[8] = (i/10)%10 + '0';
    name[9] = i%10 + '0';
    if (sd.exists(name)) {
      continue;
    }
    logfile.open(name);
    break;
  }
  if (!logfile.is_open()) {
    return 0;
  }
  Serial.printf("Log file: %s\n",name);
  return 1; //success
}
uint8_t sd_format_header(){
    // format header in buffer
  obufstream bout(SDbuf, sizeof(SDbuf));
  bout << F("Time ,");
  bout << F("Sample ");

  //TODO real header
  for (uint8_t i = 0; i < 12; i++) {
    bout << F(",sens") << int(i);
  }
  logfile << SDbuf << endl;
  // check for error
  if (!logfile) {
    return 0; //write data error
  }
  return 1;
}
uint8_t sd_write_sample(){
  obufstream bout(SDbuf, sizeof(SDbuf));
  //check if logfile is open
  if (!logfile.is_open()) {
    return 0;
  }

  //last row write time was:
  bout << now() << ","; //log current time
  // log sample number - TODO sample number variable
  bout << 1;

  for (uint8_t ia = 0; ia < 3; ia++) {
    bout << ',' << analogRead(ia+A2); //TODO result array writing
  }
  //flush = write to SD
  logfile << SDbuf << flush;
  // check for error
  if (!logfile) {
    return 0; //write data error
  }
  return 1;
}

//io expander initialization, Wire must be already on, puts all outputs to HIGH
uint8_t ioexp_init(){
  uint8_t error_ioexp;
  Wire.beginTransmission(IOEXP_ADDR);
  Wire.write(0x06); //conf reg
  Wire.write(0x00); //all as output
  Wire.write(0x00); //all as output
  error_ioexp = Wire.endTransmission();
  if (error_ioexp){
    return 0;
  }
  delay(1);
  Wire.beginTransmission(IOEXP_ADDR);
  Wire.write(0x04); //polarity inversion
  Wire.write(0x00); // no inversion
  Wire.write(0x00); // no inversion
  error_ioexp = Wire.endTransmission(); 
  if (error_ioexp){
    return 0;
  }
  delay(1);
  Wire.beginTransmission(IOEXP_ADDR);
  Wire.write(0x02); //output reg
  Wire.write(0xFF); //all outputs high
  Wire.write(0xFF); 
  error_ioexp = Wire.endTransmission(); 
  if (error_ioexp){
    return 0;
  }
  return 1;
}

//puts all outputs to low
uint8_t ioexp_out_all_low(){
  uint8_t error_ioexp;
  Wire.beginTransmission(IOEXP_ADDR);
  Wire.write(0x02); //output reg
  Wire.write(0x00); //all outputs high
  Wire.write(0x00); 
  error_ioexp = Wire.endTransmission(); 
  if (error_ioexp){
    return 0;
  }
  return 1;
}

////////////////////////////DEBUG FUNCTIONS
void debug_GPIOexp(){
  /*Wire.beginTransmission(IOEXP_ADDR);
  Wire.write(0x06); //conf reg
  Wire.write(0x00); //all as output
  Wire.write(0x00); //all as output
  Wire.endTransmission();*/
  uint16_t word_toio;
  while(1){
    delay(500);
    Wire.beginTransmission(IOEXP_ADDR);
    Wire.write(0x02); //output reg
    Wire.write(0x00); 
    Wire.write(0x00); 
    Wire.endTransmission();
    delay(500);
    Wire.beginTransmission(IOEXP_ADDR);
    Wire.write(0x02); //conf reg
    Wire.write(0xFF); 
    Wire.write(0xFF); 
    Wire.endTransmission();
    for (word i = 0; i <= 16; i++) {// This loop will set one output pin LOW at a time while all other pins are set LOW., all way to 16 so there are all zeros too
      word_toio = ~(0x0000 | (1 << i));
      Wire.beginTransmission(IOEXP_ADDR);
      Wire.write(0x02); //conf reg
      Wire.write(word_toio & 0x00FF); 
      Wire.write(word_toio >> 8); 
      Wire.endTransmission();
      delay(200);
    }
  }
}
void debug_sd_log(){
  //test function, will be used like this in FSM
  if (use_sd) {
    if (!sd_create_file()){
      error("file.open");
      use_sd = 0;
    }
  }
  if (use_sd) {
    if(!sd_format_header())
      error("write data failed");
  }
  if (use_sd) {
    if(!sd_write_sample())
      error("write data failed");
  }
  if (use_sd) {
    logfile.close();
  }
}

