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

//reset fix
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>

//custom pins
#define PIN_ETH_INT 36 //was 2, changed due to FW loading problems
#define PIN_ETH_NRST 4
#define PIN_LED 16
#define PIN_SD_CS  17  // SD chip select pin
#define PIN_SDA 21
#define PIN_SCL 22
#define PIN_ETH_CS 23
#define PIN_FAN 25
#define PIN_TP8 26
#define PIN_TP9 27
#define PIN_BTN1 34
#define PIN_TP7 36
#define PIN_TP6 39
//custom parameters
#define ANNOUNCEMENTS_PERIOD 2000 //in ms

//GPIO expander
#include <Wire.h>
#define IOEXP_ADDR 0x20 //0x75 debug board, 0x20 final

//ADC
#include "ADS1115.h"

ADS1115 adc1(ADS1115_ADDRESS_ADDR_GND);
ADS1115 adc2(ADS1115_ADDRESS_ADDR_VDD);
ADS1115 adc3(ADS1115_ADDRESS_ADDR_SCL);

// Writer to SD card
// file system object
SdFat sd;
// text file for logging
ofstream logfile;
// buffer to format data - makes it eaiser to echo to Serial / inherited
char SDbuf[300];//limited to 299 characters
// store error strings in flash to save RAM
#define error(s) sd.errorHalt(F(s))

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
const int dstPort PROGMEM = 65511;
const int srcPort PROGMEM = 65500;
uint8_t destIp[] = { 192,168,0,5 }; // UDP unicast or broadcast, this ip does not matter, will be broadcast on given IP network by DHCP
uint8_t sendUDP_Buffer[100]; //send function limited to 220
uint8_t receiveUDP_Buffer[100]; 
uint8_t error_code[50]; //error flag, 0 at index 0 means no error

//Globals
bool use_sd=1;
bool UDP_read_flag = 0;
bool BTN1_flag = 0;
//functions
uint8_t sd_create_file();
uint8_t sd_format_header();
uint8_t sd_write_sample();
uint8_t ioexp_init();
uint8_t ioexp_out_set(uint8_t port0,uint8_t port1);
uint8_t adc_init();
void udpReceiveProcess(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, const char *data, uint16_t len);
uint8_t setup_time();
void insert_time_to_send_buffer();
uint8_t ioexp_read(uint8_t *port0,uint8_t *port1);
void IRAM_ATTR ISR_BTN1(){BTN1_flag = 1;}
void log_error_code(uint8_t ec);
void hard_restart();
//debug functions
void debug_UDP_receive(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, const char *data, uint16_t len);
void debug_sd_log();
void debug_GPIOexp();
uint8_t debug_adc();
//debug macro
#define DEBUG 1 //UART logs

#if DEBUG == 1
  #define PRINTDEBUG Serial.printf
#else
  #define PRINTDEBUG
#endif 

void setup(){
  error_code[0] = 0;
  pinMode(PIN_BTN1,INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN1), ISR_BTN1, FALLING);
  pinMode(PIN_ETH_INT,INPUT);
  pinMode(PIN_ETH_NRST,OUTPUT);
  digitalWrite(PIN_ETH_NRST,0);
  pinMode(PIN_LED,OUTPUT); //LED on final HW
  Serial.begin(115200);
  Serial.println(F("\n[backSoon]"));
  delay(20);
  digitalWrite(PIN_ETH_NRST,1);
  delay(10);

  esp_efuse_read_mac(chipid);
  chipid[5]++; //use MAC address for ETH one larger than WiFi MAC (WiFi MAC is chip ID)
  // Change 'SS' to your Slave Select pin, if you arn't using the default pin
  if (ether.begin(sizeof Ethernet::buffer, chipid, PIN_ETH_CS) == 0){
    Serial.println(F("Failed to access Ethernet controller"));
    log_error_code(1);
  }
#if STATIC
  ether.staticSetup(myip, gwip);
#else
  if (error_code[0]==0){
    Serial.println(F("Waiting for DHCP...")); //60s timeout
    if (!ether.dhcpSetup()){ //blocking
      Serial.println(F("DHCP failed"));
      log_error_code(2);
    }
  }
#endif
  Serial.printf("ETH  MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",chipid[0], chipid[1], chipid[2], chipid[3], chipid[4], chipid[5]);
  chipid[5]--;
  Serial.printf("WiFi MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",chipid[0], chipid[1], chipid[2], chipid[3], chipid[4], chipid[5]);
  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);
  ether.printIp("DNS: ", ether.dnsip);
  for (int r=0;r<3;r++)
    destIp[r] = ether.gwip[r];
  destIp[3]=255; //broadcast address
  ether.printIp("Destination IP default: ", destIp);//Destination IP for sender
  //register udpReceiveProcess() to port 65500
  ether.udpServerListenOnPort(&udpReceiveProcess, srcPort);

  // Initialize at the highest speed supported by the board that is
  // not over 50 MHz. Try a lower speed if SPI errors occur.
  use_sd = 1;
  if (!sd.begin(PIN_SD_CS, SD_SCK_MHZ(20))) {  //SPI clock , 50MHz or 40M does not wrok, 20M ok, 
    sd.initErrorHalt(); //halt turned off, but will give logs to serial
    use_sd=0;
    log_error_code(3);
  }

  //debug_sd_log();

  if(!Wire.begin(PIN_SDA,PIN_SCL,400000UL))
    log_error_code(4);
  
  if (!ioexp_init()){
    error("failed to init GPIO expander");
    log_error_code(5);
  }
  //debug_GPIOexp();

  if(!adc_init()){
   error("failed to init ADC");
   log_error_code(6);
  }
}
/*
 ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄       ▄▄ 
▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░▌     ▐░░▌
▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌░▌   ▐░▐░▌
▐░▌          ▐░▌          ▐░▌▐░▌ ▐░▌▐░▌
▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌ ▐░▐░▌ ▐░▌
▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌
▐░█▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀█░▌▐░▌   ▀   ▐░▌
▐░▌                    ▐░▌▐░▌       ▐░▌
▐░▌           ▄▄▄▄▄▄▄▄▄█░▌▐░▌       ▐░▌
▐░▌          ▐░░░░░░░░░░░▌▐░▌       ▐░▌
 ▀            ▀▀▀▀▀▀▀▀▀▀▀  ▀         ▀ 
*/
void loop(){
  
  uint32_t bl = 0;
  uint32_t timer1 = 0;
  uint8_t adc_active_channel = 0; //looping thorugh channels of ADC
  uint32_t adc_last_measurement_ms = 0; //millis of last measurement
  uint32_t adc_period_ms = 1000; //every X ms 
  uint8_t adc_PGA_setting[3][4]; //ADC is first index, ch second
  uint16_t adc_result_raw[3][4]; //ADC is first index
  
  enum states {
      s0_START,
      s1_send_announcement,
      s2_wait_for_ACK,
      s3_IDLE,
      s4_report_error,
      s5_set_PGA,
      s6_set_active_channel,
      s7_trigger_and_wait,
      s8_send_meas_to_UDP,
      s9_write_meas_to_SD,
      s200_debug_OK = 200,
    } state;
  state = s0_START;

  while(1){
    switch(state) {
      case s0_START:
        PRINTDEBUG("FSM start\n");
        state = s1_send_announcement;     
        break;
      case s1_send_announcement:
        insert_time_to_send_buffer();
        sendUDP_Buffer[4] = 0x04; //Announcement
        ether.sendUdp((char *)sendUDP_Buffer, 5, srcPort, destIp, dstPort ); //broadcast
        ether.packetLoop(ether.packetReceive()); //reduntant but just to be safe
        timer1 = millis();
        state = s2_wait_for_ACK;
        break;
      case s2_wait_for_ACK:
        if (UDP_read_flag){
          if (receiveUDP_Buffer[4] == 0x01){
            //set IP for next sending to be IP of master (one who sent ACK) - done in udpReceiveProcess
            PRINTDEBUG("Destination IP changed: %d.%d.%d.%d\n",destIp[0],destIp[1],destIp[2],destIp[3]);
            if (!setup_time()){
              error("error in setting time, time is from the past, ignoring");
              log_error_code(7);
            }
            UDP_read_flag = 0; //done with data - processed
            state = s3_IDLE;
            break;
          } else {
            UDP_read_flag = 0; //not good ACK - discard data.
            state = s1_send_announcement; //keep sending Announcements till you get real ACK
          }
        }
        if (millis()>(timer1+ANNOUNCEMENTS_PERIOD)){
          state = s1_send_announcement; //nothing received for 2s - keep sending Announcements
          timer1 = millis();
        }
        break;
      case s3_IDLE:
        if (error_code[0] != 0)
          state = s4_report_error;
        if (UDP_read_flag){
          switch (receiveUDP_Buffer[4]){//master is commanding!, commands start at 5
            case 1://acknowledgement
              //now useless
              UDP_read_flag = 0;
              break;
            case 2://reset unit
              hard_restart();
              //ESP.restart(); - does not restart completely
              break;
            case 5://Sensor power control
              PRINTDEBUG("Received power setting [5-6] %02x:%02x\n",receiveUDP_Buffer[5],receiveUDP_Buffer[6]);
              ioexp_out_set(~receiveUDP_Buffer[5] & 0xFF,~receiveUDP_Buffer[6] & 0xFF);
              PRINTDEBUG("sent to ioexp [5-6] %02x:%02x\n",~receiveUDP_Buffer[5] & 0xFF,~receiveUDP_Buffer[6] & 0xFF);
              uint8_t port0_check;
              uint8_t port1_check;
              if(!ioexp_read(&port0_check,&port1_check)){
                PRINTDEBUG("failed to read IO exp status\n");
                log_error_code(9);
              }
              PRINTDEBUG("IO exp state: %02x:%02x\n",port0_check,port1_check);
              if (!(((~receiveUDP_Buffer[5] & 0xFF) == port0_check)&((~receiveUDP_Buffer[6] & 0xFF) == port1_check))){
                log_error_code(8);//not matching
                PRINTDEBUG("setting is not matching\n");
              }
              insert_time_to_send_buffer();
              sendUDP_Buffer[4] = 0x05; //Power report send to master
              sendUDP_Buffer[5] = ether.myip[3];
              sendUDP_Buffer[6] = ~port0_check;
              sendUDP_Buffer[7] = ~port1_check;
              ether.sendUdp((char *)sendUDP_Buffer, 8, srcPort, destIp, dstPort );
              ether.packetLoop(ether.packetReceive()); //reduntant but just to be safe
              UDP_read_flag = 0;
              break;
            }//if  command end
          UDP_read_flag = 0;
        }
        if ((adc_last_measurement_ms + adc_period_ms) < millis()){
          //time ot measure a sample
          adc_last_measurement_ms = millis();
          adc_active_channel = 0;
          state = s5_set_PGA;
        }
        break;
      case s4_report_error:
        for (uint8_t q=0;(q<254)&(error_code[q]!=0);q++){
          PRINTDEBUG("Sending error report code %d\n",error_code[q]);
          insert_time_to_send_buffer();
          sendUDP_Buffer[4] = 0x03; //error message type
          sendUDP_Buffer[5] = ether.myip[3]; //unit number as well as last byte of IP
          //must be sent because some routers strip source IP address when coming from ethernet to wifi on same network
          sendUDP_Buffer[6] = error_code[q]; //error message type
          ether.sendUdp((char *)sendUDP_Buffer, 7, srcPort, destIp, dstPort ); //unicast to master
          ether.packetLoop(ether.packetReceive());
          delay(1);
        }
        error_code[0] = 0; //reset error code array 0 on index 0 is no error
        state = s3_IDLE;
        break;
      case s5_set_PGA:
        adc_PGA_setting[0][adc_active_channel] = ADS1115_PGA_6P144;
        adc_PGA_setting[1][adc_active_channel] = ADS1115_PGA_6P144;
        adc_PGA_setting[2][adc_active_channel] = ADS1115_PGA_6P144;
        //todo autoranging, now all ADCs are on max scale
        adc1.setGain(adc_PGA_setting[0][adc_active_channel]);
        adc2.setGain(adc_PGA_setting[1][adc_active_channel]); 
        adc3.setGain(adc_PGA_setting[2][adc_active_channel]); 
        state = s6_set_active_channel;
        break;
      case s6_set_active_channel:
        adc1.setMultiplexer(ADS1115_MUX_P0_NG + adc_active_channel); //channel 0 single ended
        adc2.setMultiplexer(ADS1115_MUX_P0_NG + adc_active_channel); //channel 0 single ended
        adc3.setMultiplexer(ADS1115_MUX_P0_NG + adc_active_channel); //channel 0 single ended
        state = s7_trigger_and_wait;
        break;
      case s7_trigger_and_wait:
        adc1.triggerConversion(); //16ms wait after this, then poll, valid for 64SPS
        adc2.triggerConversion(); 
        adc3.triggerConversion(); 
        delay(14); //todo use time more wisely? - log range array
        //decrease delay if you change samples per second
        //for 64SPS - 14ms ideal, same time as without delay
        //quiet time for conversion. I2C is close to sensitive signals.
        //todo log to range array and value array
        //PRINTDEBUG(" Conv.trg'd ch %d\n",adc_active_channel);
        if(!adc1.pollConversion(I2CDEV_DEFAULT_READ_TIMEOUT))
          log_error_code(11);
        adc_result_raw[0][adc_active_channel] = adc1.getConversion(false);
        if(!adc2.pollConversion(I2CDEV_DEFAULT_READ_TIMEOUT))
          log_error_code(12);
        adc_result_raw[1][adc_active_channel] = adc2.getConversion(false);
        if(!adc3.pollConversion(I2CDEV_DEFAULT_READ_TIMEOUT))
          log_error_code(13);
        adc_result_raw[2][adc_active_channel] = adc3.getConversion(false);
        adc_active_channel++;
        if (adc_active_channel == 4){
          adc_active_channel == 0;
          state = s8_send_meas_to_UDP; //measurement done on all 4 channels
          break;
        }
        state = s5_set_PGA;
        break;
      case s8_send_meas_to_UDP:
        insert_time_to_send_buffer();
        sendUDP_Buffer[4] = 0x02; //Measurement sample type message
        sendUDP_Buffer[5] = ether.myip[3];
        for (uint8_t adcn=0;adcn<3;adcn++){
          for (uint8_t chn=0;chn<4;chn++){
            //range goes to 6, 9, 12, 15, adc2 18, 21, 24, 27 adc3 30 
            uint8_t index_sample_datagram = 6+(adcn*12)+chn*3; //defines range field
            sendUDP_Buffer[index_sample_datagram]=adc_PGA_setting[adcn][chn];
            //ADC result goes to 7..8, 10..11, 
            sendUDP_Buffer[index_sample_datagram+1]=adc_result_raw[adcn][chn]>>8;
            sendUDP_Buffer[index_sample_datagram+2]=adc_result_raw[adcn][chn]&0xFF;
          }
        }
        ether.sendUdp((char *)sendUDP_Buffer, 41, srcPort, destIp, dstPort ); //unicast to master
        ether.packetLoop(ether.packetReceive());
        if (use_sd){
          state = s9_write_meas_to_SD;
        }
        state = s3_IDLE;
        break;
      case s200_debug_OK:
        
        break;
    }//FSM end

    //this must be called for ethercard functions to work. 
    //Returns Length of received data. If non-zero, something is coming.
    if (UDP_read_flag == 0) //if receive buffer ready for data, check if thre is incoming DTG
      if (digitalRead(PIN_ETH_INT)==0){ //if interrupt of incoming data or want to send data
        ether.packetLoop(ether.packetReceive()); //Returns Length of received data. If non-zero, something is coming.
      }
      //data were already processed and put into receiveUDP_buffer.
      //flag is set to 1 by the same function

    //DEBUG CODE
    #ifdef DEBUG
      //toogle PIN LED on every iteration - scope check
      digitalWrite(PIN_LED, !digitalRead(PIN_LED));
      if (BTN1_flag){PRINTDEBUG("Button pressed!\n");BTN1_flag = 0;}
      if (millis()>(bl+10000)){ //every 1s
        bl=millis();
        Serial.printf("t: %d\t st:%d\n",now(),(int)state);
      }
    #endif
  }
}
/* 
 ▄▄▄▄▄▄▄▄▄▄▄  ▄         ▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄ 
▐░░░░░░░░░░░▌▐░▌       ▐░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌
▐░█▀▀▀▀▀▀▀▀▀ ▐░▌       ▐░▌▐░▌░▌     ▐░▌▐░█▀▀▀▀▀▀▀▀▀  ▀▀▀▀█░█▀▀▀▀  ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░▌░▌     ▐░▌▐░█▀▀▀▀▀▀▀▀▀ 
▐░▌          ▐░▌       ▐░▌▐░▌▐░▌    ▐░▌▐░▌               ▐░▌          ▐░▌     ▐░▌       ▐░▌▐░▌▐░▌    ▐░▌▐░▌          
▐░█▄▄▄▄▄▄▄▄▄ ▐░▌       ▐░▌▐░▌ ▐░▌   ▐░▌▐░▌               ▐░▌          ▐░▌     ▐░▌       ▐░▌▐░▌ ▐░▌   ▐░▌▐░█▄▄▄▄▄▄▄▄▄ 
▐░░░░░░░░░░░▌▐░▌       ▐░▌▐░▌  ▐░▌  ▐░▌▐░▌               ▐░▌          ▐░▌     ▐░▌       ▐░▌▐░▌  ▐░▌  ▐░▌▐░░░░░░░░░░░▌
▐░█▀▀▀▀▀▀▀▀▀ ▐░▌       ▐░▌▐░▌   ▐░▌ ▐░▌▐░▌               ▐░▌          ▐░▌     ▐░▌       ▐░▌▐░▌   ▐░▌ ▐░▌ ▀▀▀▀▀▀▀▀▀█░▌
▐░▌          ▐░▌       ▐░▌▐░▌    ▐░▌▐░▌▐░▌               ▐░▌          ▐░▌     ▐░▌       ▐░▌▐░▌    ▐░▌▐░▌          ▐░▌
▐░▌          ▐░█▄▄▄▄▄▄▄█░▌▐░▌     ▐░▐░▌▐░█▄▄▄▄▄▄▄▄▄      ▐░▌      ▄▄▄▄█░█▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌▐░▌     ▐░▐░▌ ▄▄▄▄▄▄▄▄▄█░▌
▐░▌          ▐░░░░░░░░░░░▌▐░▌      ▐░░▌▐░░░░░░░░░░░▌     ▐░▌     ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░▌      ▐░░▌▐░░░░░░░░░░░▌
 ▀            ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀  ▀▀▀▀▀▀▀▀▀▀▀       ▀       ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀  ▀▀▀▀▀▀▀▀▀▀▀
*/
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
    bout << ',' << ia; //TODO result array writing
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

//puts all outputs to state as in arguments
uint8_t ioexp_out_set(uint8_t port0,uint8_t port1){
  uint8_t error_ioexp;
  Wire.beginTransmission(IOEXP_ADDR);
  Wire.write(0x02); //output reg
  Wire.write(port0); //all outputs high
  Wire.write(port1); 
  error_ioexp = Wire.endTransmission(); 
  if (error_ioexp){
    return 0;
  }
  return 1;
}
uint8_t ioexp_read(uint8_t *port0,uint8_t *port1){
  uint8_t error_ioexp;
  Wire.beginTransmission(IOEXP_ADDR);
  Wire.requestFrom(IOEXP_ADDR, 2);
  uint32_t timeout_ioexp;
  timeout_ioexp = millis();
  if (Wire.available()){
    *port0 = (uint8_t)Wire.read();
    *port1 = (uint8_t)Wire.read();
  } else {
    if (millis()>(timeout_ioexp+1000)) //1 s timeout
      return 0;
  }
  error_ioexp = Wire.endTransmission(); 
  if (error_ioexp){
    return 0;
  }
  return 1;
}

uint8_t adc_init(){
  if (!adc1.testConnection())
    return 0;
  adc1.initialize(); // initialize ADS1115 16 bit A/D chip
  adc1.setMode(ADS1115_MODE_SINGLESHOT);
  adc1.setRate(ADS1115_RATE_64);
  adc1.setGain(ADS1115_PGA_6P144); //6.144V range
  adc1.setMultiplexer(ADS1115_MUX_P1_NG); //channel 0 single ended
  if (!adc2.testConnection())
    return 0;
  adc2.initialize(); // initialize ADS1115 16 bit A/D chip
  adc2.setMode(ADS1115_MODE_SINGLESHOT);
  adc2.setRate(ADS1115_RATE_64); 
  adc2.setGain(ADS1115_PGA_6P144); //6.144V range
  adc2.setMultiplexer(ADS1115_MUX_P1_NG); //channel 0 single ended
  if (!adc2.testConnection())
    return 0;
  adc3.initialize(); // initialize ADS1115 16 bit A/D chip
  adc3.setMode(ADS1115_MODE_SINGLESHOT);
  adc3.setRate(ADS1115_RATE_64); 
  adc3.setGain(ADS1115_PGA_6P144); //6.144V range
  adc3.setMultiplexer(ADS1115_MUX_P1_NG); //channel 0 single ended
  return 1;
}
//callback that prints received packets to the serial port
void udpReceiveProcess(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, const char *data, uint16_t len){
  //debug_UDP_receive(dest_port, &src_ip[IP_LEN], src_port, data, len);
  //callback is executed only if data received from 65500
  memcpy(receiveUDP_Buffer, data, len);
  for (uint8_t o=0;o<len;o++)
    PRINTDEBUG("%02x:",receiveUDP_Buffer[o]);
  PRINTDEBUG(" Data read buffer\n");
  if (receiveUDP_Buffer[4] == 0x01){
    destIp[3] = src_ip[3];
    //memcpy(destIp,src_ip,IP_LEN);  //if it is acknowledgment, change destination IP to unicast.
    memcpy(ether.gwip,src_ip,IP_LEN);  //change gateway to master
  }
  UDP_read_flag = 1;
}
uint8_t setup_time(){
  uint32_t epoch_time = 0;
  epoch_time = ((uint32_t)receiveUDP_Buffer[0]<<24) | ((uint32_t)receiveUDP_Buffer[1]<<16) | ((uint32_t)receiveUDP_Buffer[2]<<8) | ((uint32_t)receiveUDP_Buffer[3]);
  PRINTDEBUG("Epoch time received: %d\n",epoch_time);

  if (epoch_time > 1558883838){ //sanity check, if pc is sending time from past May 26 2019, ignore
    setTime(epoch_time);
    PRINTDEBUG("Epoch time accepted: %d\n",epoch_time);
  } else {
    return 0;
  }
  return 1;
}
void insert_time_to_send_buffer(){
  uint32_t timenow = 0;
  timenow = now();
  sendUDP_Buffer[0] = timenow >> 24;
  sendUDP_Buffer[1] = timenow >> 16;
  sendUDP_Buffer[2] = timenow >> 8;
  sendUDP_Buffer[3] = timenow;
  return;
}
void log_error_code(uint8_t ec){
  for (uint8_t i = 0;i<49;i++){
    if (error_code[i]==0){
      error_code[i] = ec;
      error_code[i+1] = 0;
      return;
    }
  }
}
void hard_restart() {
  esp_task_wdt_init(1,true);
  esp_task_wdt_add(NULL);
  while(true);
}
////////////////////////////DEBUG FUNCTIONS
void debug_UDP_receive(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, const char *data, uint16_t len){
  IPAddress src(src_ip[0],src_ip[1],src_ip[2],src_ip[3]);
  Serial.print("UDP RECEIVE DEBUG \n");
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
  /* this should be in FSM
  if (epoch_time > 1558883838){ //sanity check, if pc is sending time from past May 26 2019, ignore
    setTime(epoch_time);
    Serial.printf("Epoch time accepted: %d\n",epoch_time);
  }*/
}

uint8_t debug_adc(){
  uint32_t m=micros();
  uint32_t n=micros();
  /* 
  Serial.println(adc1.testConnection() ? "ADC1 connection successful" : "ADC1 connection failed");
  adc1.initialize(); // initialize ADS1115 16 bit A/D chip

  adc1.setMode(ADS1115_MODE_SINGLESHOT);
  adc1.setRate(ADS1115_RATE_64); //16 SPS is enough even for 100ms sampling
  adc1.setGain(ADS1115_PGA_6P144); //6.144V range
  //ALERT/RDY pin already disabled in INIT
  */
  adc1.setMultiplexer(ADS1115_MUX_P1_NG); //channel 0 single ended
  adc2.setMultiplexer(ADS1115_MUX_P1_NG); //channel 0 single ended
  adc3.setMultiplexer(ADS1115_MUX_P1_NG); //channel 0 single ended
  //while(1){
    adc1.triggerConversion(); //16ms wait after this, then poll
    m=micros();
    if(!adc1.pollConversion(I2CDEV_DEFAULT_READ_TIMEOUT))
      return 0;
    n=micros();
    Serial.printf("Conversion took %d us ",n-m); //typically for 64SPS : under 16400 us
    Serial.print("1A1: "); Serial.print(adc1.getMilliVolts(false)); Serial.print("mV\t\n");
    //delay(200);
    adc2.triggerConversion(); //16ms wait after this, then poll
    m=micros();
    if(!adc2.pollConversion(I2CDEV_DEFAULT_READ_TIMEOUT))
      return 0;
    n=micros();
    Serial.printf("Conversion took %d us ",n-m); //typically for 64SPS : 16246 us
    Serial.print("2A1: "); Serial.print(adc2.getMilliVolts(false)); Serial.print("mV\t\n");
    //delay(200);
    adc3.triggerConversion(); //16ms wait after this, then poll
    m=micros();
    if(!adc3.pollConversion(I2CDEV_DEFAULT_READ_TIMEOUT))
      return 0;
    n=micros();
    Serial.printf("Conversion took %d us ",n-m); //typically for 64SPS : 16246 us
    Serial.print("3A1: "); Serial.print(adc3.getMilliVolts(false)); Serial.print("mV\t\n");
    //delay(200);
  //}
  return 1;
}
void debug_GPIOexp(){
  while(1){

  if (!ioexp_init()) //puts all high
    error("failed to init GPIO expander");
  delay(500);
  if (!ioexp_out_set(0x00,0x00))
    error("failed to communicate with GPIO exp.");
  delay(500);
  
  }
  //old version
  //init old
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
  Serial.println("SD debugging:\n");
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

