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
#define PIN_TP8 26 //used as fake SDA for real SDA line manipulation
#define PIN_TP9 27
#define PIN_BTN1 34
#define PIN_TP7 36
#define PIN_TP6 39
//custom parameters
#define ANNOUNCEMENTS_PERIOD 2000 //in ms
#define LMP91000_ADR 0x48 //checked with scanner

//GPIO expander
#include <Wire.h>
#define IOEXP_ADDR 0x20 //0x75 debug board, 0x20 final

//debug macro
#define DEBUG 1 //UART logs
#if DEBUG == 1
  #define PRINTDEBUG Serial.printf
#else
  #define PRINTDEBUG
#endif 

#include "Adafruit_VEML7700.h"
Adafruit_VEML7700 veml = Adafruit_VEML7700();
#include "Adafruit_Si7021.h"
Adafruit_Si7021 Si7021 = Adafruit_Si7021();

//ADC
#include "ADS1115.h"

ADS1115 adc1(ADS1115_ADDRESS_ADDR_SDA);//change on V4, was GND
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
uint8_t sendUDP_len; //length of data is going always along the array
uint8_t receiveUDP_Buffer[100]; 
uint8_t error_code[50]; //error flag, 0 at index 0 means no error

//Globals
bool use_sd=1;
bool use_veml=1;//light sensor VEML7700
bool use_Si7021=1;//Temperature and RH sensor
bool use_tgs24444=0; //amonia sensor
bool UDP_read_flag = 0;
bool BTN1_flag = 0;
  uint8_t LMPreg_TIACN = 0xFF;
  uint8_t LMPreg_REFCN = 0xFF;
  uint8_t LMPreg_MODECN = 0xFF;
//functions
uint8_t sd_create_file();
uint8_t sd_format_header(uint32_t adc_period_ms);
uint8_t sd_write_sample(uint32_t sample_num);
uint8_t ioexp_init(uint8_t port1,uint8_t port2);
uint8_t ioexp_out_set(uint8_t port0,uint8_t port1);
uint8_t adc_init();
void udpReceiveProcess(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, const char *data, uint16_t len);
uint8_t setup_time();
void insert_to_send_buffer_header(uint32_t val);
uint8_t ioexp_read(uint8_t *port0,uint8_t *port1);
void IRAM_ATTR ISR_BTN1(){BTN1_flag = 1;}
void log_error_code(uint8_t ec);
void hard_restart();
uint8_t adc_PGA_autorange(uint8_t old_PGA,int16_t last_reading);
uint8_t ioexp_out_set_AIport(uint8_t AIport, bool statepin);
uint8_t is_sensor_out_signed(uint8_t sensor_type);
void LMP91000_setup(uint8_t configuration_set);
//debug functions
void debug_UDP_receive(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, const char *data, uint16_t len);
void debug_sd_log();
void debug_GPIOexp();
uint8_t debug_adc();
void debug_Si7021();
void debug_ioexp_AIport();
void debug_lmp91000();



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
  
  if (!ioexp_init(0x01,0x3F)){ //unrouted expander outputs low, used low (5V on) except A3.2 is high (5V off).
    error("failed to init GPIO expander");
    log_error_code(5);
  } else {
    uint8_t port0_check;
    uint8_t port1_check;
    ioexp_read(&port0_check,&port1_check);
    if (!((port0_check == 0x01)&&(port1_check == 0x3F))){
      log_error_code(8); //setting not matching
    }
  }
  //debug_GPIOexp();

  if(!adc_init()){
   error("failed to init ADC");
   log_error_code(6);
  }

  if (!veml.begin()) {
    log_error_code(18);
    error("VEML init error\n");
    use_veml = 0;
  }
  //if(use_veml) PRINTDEBUG("Lux,White,ALS: %f\t%f\t%d\n",veml.readLux(),veml.readWhite(),veml.readALS());
  if (!Si7021.begin()) {
    log_error_code(20);
    error("Si7021 init error\n");
    use_Si7021 = 0;
  }
  //debug_Si7021();
  //debug_ioexp_AIport();
  //debug_lmp91000();
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
  int16_t adc_result_raw[3][4]; //result [ADC][channel]
  uint32_t meas_sample_number = 0; //0 is measurement not active, from 1 on - sample number
  uint8_t port0_check;
  uint8_t port1_check;
  uint8_t conf_set = 0;

  //FSM states
  enum states { //FSM states
      s0_START,
      s1_send_announcement,
      s2_wait_for_ACK,
      s3_IDLE,
      s4_report_error,
      s5_set_PGA,
      s6_set_active_channel,
      s7_trigger_and_wait,
      s8_send_meas_to_UDP,
      s9_create_log_file,
      s10_write_meas_to_SD,
      s11_start_measurement,
      s12_save_data,
      s13_start_sample,
      s14_get_amonia,
      s15_setup_lmp91000,
    } state;
  state = s0_START;

  while(1){
    yield(); //CPU housekeeping,also resets WDT
    switch(state) {
      case s0_START:
        PRINTDEBUG("FSM start\n");
        state = s1_send_announcement;     
        break;
      case s1_send_announcement:
        insert_to_send_buffer_header(now());
        sendUDP_Buffer[4] = 0x04; //Announcement
        sendUDP_len = 5;
        ether.sendUdp((char *)sendUDP_Buffer, sendUDP_len, srcPort, destIp, dstPort ); //broadcast
        ether.packetLoop(ether.packetReceive()); //reduntant but just to be safe
        timer1 = millis();
        state = s2_wait_for_ACK;
        break;
      case s2_wait_for_ACK:
        if (UDP_read_flag){
          if (receiveUDP_Buffer[4] == 0x01){
            //set IP for next sending to be IP of master (one who sent ACK) - done in udpReceiveProcess
            PRINTDEBUG("Destination IP changed: %d.%d.%d.%d\n",destIp[0],destIp[1],destIp[2],destIp[3]);
            setup_time();
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
        if (error_code[0] != 0){
          state = s4_report_error;
          break;
        }
        if (UDP_read_flag){
          switch (receiveUDP_Buffer[4]){//master is commanding!, commands start at 5
            case 1://acknowledgement
              //now useless
              UDP_read_flag = 0;
              break;
            case 2://reset unit
              hard_restart();
              //ESP.restart(); - does not restart completely
              UDP_read_flag = 0;
              break;
            case 3: //Measurement trigger start
              setup_time(); //update time from master
              if (receiveUDP_Buffer[5])//not equal 0
                adc_period_ms = receiveUDP_Buffer[5]*100; //sample period setting in ms, received in 0.1s units
              if (receiveUDP_Buffer[6] & 0b01) {use_tgs24444 = 1; adc_period_ms = 250;PRINTDEBUG("Amonia will be used\n");}
              if (receiveUDP_Buffer[6] & 0b10) {
                state = s15_setup_lmp91000; //will continue to s11 after this state
                conf_set = receiveUDP_Buffer[6];
                if (conf_set & 0b100){ //debug sending registers 
                  LMPreg_TIACN = receiveUDP_Buffer[7]; 
                  LMPreg_REFCN = receiveUDP_Buffer[8]; 
                  LMPreg_MODECN = receiveUDP_Buffer[9]; 
                }
                UDP_read_flag = 0;
                break;
              }
              state = s11_start_measurement;
              UDP_read_flag = 0;
              break;
            case 4: //measurement trigger stop
              meas_sample_number = 0; //stop sampling
              if (use_sd) 
                logfile.close();
              //stay in idle
              UDP_read_flag = 0;
              break;
            case 5://Sensor power control
              PRINTDEBUG("Received power setting [5-6] %02x:%02x\n",receiveUDP_Buffer[5],receiveUDP_Buffer[6]);
              ioexp_out_set(~receiveUDP_Buffer[5] & 0xFF,~receiveUDP_Buffer[6] & 0xFF);
              PRINTDEBUG("sent to ioexp [5-6] %02x:%02x\n",~receiveUDP_Buffer[5] & 0xFF,~receiveUDP_Buffer[6] & 0xFF);
              
              if(!ioexp_read(&port0_check,&port1_check)){
                PRINTDEBUG("failed to read IO exp status\n");
                log_error_code(9);
              }
              PRINTDEBUG("IO exp state: %02x:%02x\n",port0_check,port1_check);
              if (!(((~receiveUDP_Buffer[5] & 0xFF) == port0_check)&((~receiveUDP_Buffer[6] & 0xFF) == port1_check))){
                log_error_code(8);//not matching
                PRINTDEBUG("setting is not matching\n");
              }
              insert_to_send_buffer_header(now());
              sendUDP_Buffer[4] = 0x05; //Power report send to master
              sendUDP_Buffer[5] = ether.myip[3];
              sendUDP_Buffer[6] = ~port0_check;
              sendUDP_Buffer[7] = ~port1_check;
              sendUDP_len = 8;
              ether.sendUdp((char *)sendUDP_Buffer, sendUDP_len, srcPort, destIp, dstPort );
              ether.packetLoop(ether.packetReceive()); //reduntant but just to be safe
              UDP_read_flag = 0;
              break;
            default:
              PRINTDEBUG("Unknown command from master\n");
            }//if  command end
          UDP_read_flag = 0;
          break;
        }
        if (meas_sample_number){ //non zero value indicates active measurement
          if ((adc_last_measurement_ms + adc_period_ms) < millis()){
          //time ot measure a sample
            adc_last_measurement_ms = millis();
            adc_active_channel = 0;
            state = s13_start_sample;
            meas_sample_number++;  //first sample sent will be #2
            break;
          }  
        }
        break;
      case s4_report_error:
        for (uint8_t q=0;(q<254)&(error_code[q]!=0);q++){
          PRINTDEBUG("Sending error report code %d\n",error_code[q]);
          insert_to_send_buffer_header(now());
          sendUDP_Buffer[4] = 0x03; //error message type
          sendUDP_Buffer[5] = ether.myip[3]; //unit number as well as last byte of IP
          //must be sent because some routers strip source IP address when coming from ethernet to wifi on same network
          sendUDP_Buffer[6] = error_code[q]; //error message type
          sendUDP_len = 7;
          ether.sendUdp((char *)sendUDP_Buffer, sendUDP_len, srcPort, destIp, dstPort ); //unicast to master
          ether.packetLoop(ether.packetReceive());
          delay(1);
        }
        error_code[0] = 0; //reset error code array 0 on index 0 is no error
        state = s3_IDLE;
        break;
      case s5_set_PGA:
        for (uint8_t adcn=0;adcn<3;adcn++){ 
          if (meas_sample_number < 10){ //begin at max range, till buffer fills
            adc_PGA_setting[adcn][adc_active_channel] = ADS1115_PGA_6P144;
          } else {
            adc_PGA_setting[adcn][adc_active_channel] = adc_PGA_autorange(adc_PGA_setting[adcn][adc_active_channel],adc_result_raw[adcn][adc_active_channel]);
            //if (adcn == 0 & adc_active_channel == 0)
            //  PRINTDEBUG("%d\n",adc_PGA_setting[adcn][adc_active_channel]);
          }
        }
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
          adc_active_channel = 0;
          if (use_tgs24444)
            state = s14_get_amonia;
          else
            state = s12_save_data;
          break;
        }
        state = s5_set_PGA;
        break;
      case s8_send_meas_to_UDP:
        ether.sendUdp((char *)sendUDP_Buffer, sendUDP_len, srcPort, destIp, dstPort ); //unicast to master
        ether.packetLoop(ether.packetReceive());
        state = s3_IDLE;
        break;
      case s9_create_log_file:
        if (!sd_create_file()){
          PRINTDEBUG("SD card - failed to create file\n");
          log_error_code(14);
          use_sd = 0;
          state = s3_IDLE;
          break;
        } 
        if(!sd_format_header(adc_period_ms)){
          PRINTDEBUG("SD write data failed\n");
          log_error_code(15);
          use_sd = 0;
        }
        state = s3_IDLE;
        break;
      case s10_write_meas_to_SD:
        if(!sd_write_sample(meas_sample_number)){
          PRINTDEBUG("SD write data failed\n");
          log_error_code(15);
          use_sd = 0;
        }
        state = s8_send_meas_to_UDP;
        break;
      case s11_start_measurement:
        if (meas_sample_number > 0){
          //already running, do not create SD file again
          if (!use_sd) 
            log_error_code(17); //SD not used in this measurement
          state = s3_IDLE;
        } else { //was stopped or unit powered and is starting
          meas_sample_number = 1; //start sampling
          if (use_sd)
            state = s9_create_log_file;
          else{
            state = s13_start_sample;
            log_error_code(17);
          }
        }
        break;
      case s12_save_data:
        insert_to_send_buffer_header(meas_sample_number); //instead of time
        sendUDP_Buffer[4] = 0x02; //Measurement sample type message
        sendUDP_Buffer[5] = ether.myip[3];
        for (uint8_t adcn=0;adcn<3;adcn++){
          for (uint8_t chn=0;chn<4;chn++){
            //range goes to 6, 9, 12, 15, adc2 18, 21, 24, 27 adc3 30 
            sendUDP_len = 6+(adcn*12)+chn*3; //defines range field
            sendUDP_Buffer[sendUDP_len]=adc_PGA_setting[adcn][chn] | (0xF0 & (((adcn*4)+chn)<<4)); //first 4 bits is ADC input number, second 4 bits is PGA setting
            //ADC result goes to 7..8, 10..11, 
            sendUDP_Buffer[++sendUDP_len]=adc_result_raw[adcn][chn]>>8;
            sendUDP_Buffer[++sendUDP_len]=adc_result_raw[adcn][chn]&0xFF;
          }
        }
        if(use_veml){
          sendUDP_Buffer[++sendUDP_len]=(veml.getGain() | 0xF0);
          static uint16_t ALSreading;
          ALSreading = veml.readALS(); //todo error checking
          sendUDP_Buffer[++sendUDP_len]=(ALSreading >> 8);
          sendUDP_Buffer[++sendUDP_len]=(ALSreading & 0xFF);
        }
        if(use_Si7021){
          static uint16_t Sit = 0b11;
          static uint16_t SiRH = 0b11;
          if(!Si7021.read_requested(&Sit,&SiRH)){
            use_Si7021 = 0;
            log_error_code(21);
            PRINTDEBUG("Si7021 not reponsing to read requested - did you leave enouch time from request?\n");
          }
          sendUDP_Buffer[++sendUDP_len]=0xFD; //temperature
          sendUDP_Buffer[++sendUDP_len]=(Sit >> 8);
          sendUDP_Buffer[++sendUDP_len]=(Sit & 0xFF);
          sendUDP_Buffer[++sendUDP_len]=0xFE; //RH
          sendUDP_Buffer[++sendUDP_len]=(SiRH >> 8);
          sendUDP_Buffer[++sendUDP_len]=(SiRH & 0xFF);
        }
        sendUDP_Buffer[++sendUDP_len] = 0xFF; //end byte
        sendUDP_len++;
        if (use_sd)
          state = s10_write_meas_to_SD; //measurement done on all 4 channels SD, then UDP
        else
          state = s8_send_meas_to_UDP;
        break;
      case s13_start_sample:
        if(use_Si7021)
          if(!Si7021.RHrequest_measurement()){ //will be available for reading in 23ms
            log_error_code(21);
            PRINTDEBUG("Si7021 not responding to request\n");
            use_Si7021 = 0;
          }
        state = s5_set_PGA;
        break;
      case s14_get_amonia:
        //prepare ADC
        #define FAKE_ADDR 0x01 //I2C address only for purpose to keep up communication
        adc3.setGain(ADS1115_PGA_6P144);//to adjust for reasonable range
        adc3.setMultiplexer(ADS1115_MUX_P2_NG); //A3.2
        adc3.setRate(ADS1115_RATE_860);
        delayMicroseconds(500);
        //14ms pulse on power
        //5ms pulse low on SDA, 2ms after power pulse. - low pass is on sensor board.
        if(!ioexp_read(&port0_check,&port1_check)){
            PRINTDEBUG("failed to read IO exp status\n");
            log_error_code(9);
          }
        delay(1);//wait for SDA to rise after lowpass
        ioexp_out_set(port0_check & 0b11111110,port1_check); //turn on A3.2 5V
        delayMicroseconds(1850);//tuned value kinda
        //does mor matter much if I begin sooner, because comparator
        //does react on changes only after 2.2ms after powerup
        //keep some dummy communication on I2C so SDA is not high
        //low pass and comparator will keep the high pulse on FET
        for (uint16_t quee=0;quee<34;quee++){//110us one iteration
          Wire.beginTransmission(FAKE_ADDR);
          Wire.endTransmission();
          delayMicroseconds(1);
        }
        adc3.triggerConversion();
        if(!adc3.pollConversion(I2CDEV_DEFAULT_READ_TIMEOUT)){delayMicroseconds(1);};
        //puse to FET ends automatically after about 200us
        ets_delay_us(6800); //tuned value - 
        //need to wait to turn off power to sensor. only after ADC can be read via I2C
        ioexp_out_set(port0_check | 0b00000001,port1_check); //turn off A3.2 5V
        delayMicroseconds(500);
        //if(!adc1.pollConversion(I2CDEV_DEFAULT_READ_TIMEOUT)){delayMicroseconds(1);};
        adc_result_raw[2][2] = adc3.getConversion(0);//do not poll
        adc_PGA_setting[2][2] = 0xA0; //A3.2  (6), range 6.44v (0)
        //recover ADC settings as before
        
        adc3.setRate(ADS1115_RATE_64); //back to default rate for this setup
        state = s12_save_data;
        break;
      case s15_setup_lmp91000:{
        LMP91000_setup(conf_set);
        state = s11_start_measurement;
        }
        break;
      default:
        PRINTDEBUG("Uknown FSM state %d",state);
        log_error_code(19);
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
      if (BTN1_flag){
        BTN1_flag = 0;
        static uint32_t startms;
        startms = millis();
        PRINTDEBUG("Button pressed\n");
        delay(20); //debounce
        uint32_t LED_timing = 0;
        static uint16_t ton = 80;
        while (1){
          if (millis() > (LED_timing + ton)){
              LED_timing = millis();
              digitalWrite(PIN_LED,!digitalRead(PIN_LED)); //toogle LED
          } 
          if (uint16_t(millis()) > (startms + 30000)){break;}  //30s timeout
          
          if (digitalRead(PIN_BTN1)){break;} //released button
          
          if (millis()-startms < 1000){
            //PRINTDEBUG("Pressed under 1s, nothing happens\n");
          } else if (millis()-startms < 4000) {
            //1-10 s start measurement
            //PRINTDEBUG("Pressed under 1-3.99s, starting measuremnt\n");
            ton = 750;
          } else if (millis()-startms < 15000){
            //PRINTDEBUG("Pressed under 10-15s, reset\n");
            ton = 100;
          } else if (millis()-startms > 15000){
            break;
          }
          delay(10);
        }
        if (ton==750) {
          state = s11_start_measurement;
          PRINTDEBUG("Starting measurement on button request\n");
        }
        if (ton==100) hard_restart();
      }
      /*
      if (millis()>(bl+1000)){ //every 1s
        bl=millis();
        PRINTDEBUG("t:%u\t st:%d\n",(uint32_t)now(),(int)state);
      }*/
    #endif
  }//while 1
}//loop

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
  char name[] = "FRAlog0000.csv";

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
  PRINTDEBUG("Log file: %s\n",name);
  return 1; //success
}

uint8_t sd_format_header(uint32_t adc_period_ms){
    // format header in buffer
  obufstream bout(SDbuf, sizeof(SDbuf));
  char chbuf[64]; //max characters
  bout << F("File created :");
  snprintf(chbuf,64,"%u",(uint32_t)now());
  bout << chbuf;
  bout << F("s (epoch time)\n");
  snprintf(chbuf,64,"%u",(uint32_t)adc_period_ms);
  bout << F("Sample Period :");
  bout << chbuf;
  bout << F("ms\n");
  bout << F("Unit #");
  char name[] = "0000";
  name[0] = ether.myip[3]/1000 + '0';
  name[1] = (ether.myip[3]/100)%10 + '0';
  name[2] = (ether.myip[3]/10)%10 + '0';
  name[3] = ether.myip[3]%10 + '0';
  bout << name;//log unit number
  bout << F("\nTime,Sample#");

  for (uint8_t i = 0; i < 2; i++) { //just example how it goes, exact number of columns is not known at this time
    bout << F(",sensor_type_") << int(i);
    bout << F(",value_") << int(i);
  }
  bout << F(",..."); //and so on

  logfile << SDbuf << endl;
  // check for error
  if (!logfile) {
    return 0; //write data error
  }
  return 1;
}
uint8_t sd_write_sample(uint32_t sample_num){
  obufstream bout(SDbuf, sizeof(SDbuf));
  char chbuf[64]; //max characters
  //check if logfile is open
  if (!logfile.is_open()) {
    return 0;
  }
  bout << "\n";
  snprintf(chbuf,64,"%u,",(uint32_t)now());
  bout << chbuf; //log current time
  snprintf(chbuf,64,"%u",(uint32_t)sample_num);
  bout << chbuf; //log current sample #
  // log sample number - sample number variable

  //for (uint8_t o=0;o<sendUDP_len;o++)
  //  {PRINTDEBUG("%02x:",sendUDP_Buffer[o]);}
  //PRINTDEBUG(" Data send buffer\n");
  uint8_t ia = 6;
  uint8_t sensor_type;
  while ((ia<(sendUDP_len-1)) & (ia < 150)) { //150 to be safe, prevent infinite loop in case of some bug
    sensor_type = (uint8_t)sendUDP_Buffer[ia++];
    snprintf(chbuf,64,"%u",sensor_type);
    bout << ',' << chbuf; //range or sensor type
    if (is_sensor_out_signed(sensor_type))
      snprintf(chbuf,64,"%d",(int16_t)((((uint16_t)sendUDP_Buffer[ia]) << 8) | ((uint16_t)sendUDP_Buffer[ia+1])));
    else
      snprintf(chbuf,64,"%u",(((uint16_t)sendUDP_Buffer[ia]) << 8) | ((uint16_t)sendUDP_Buffer[ia+1]));
    ia +=2;
    bout << ',' << chbuf; //value MSB and LSB
  }
  //flush = write to SD
  logfile << SDbuf << flush;
  // check for error
  if (!logfile) {
    return 0; //write data error
  }
  return 1;
}
//io expander initialization, Wire must be already on, puts all outputs to LOW
uint8_t ioexp_init(uint8_t port1,uint8_t port2){
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
  Wire.write(port1); //inverted to sensor ports
  Wire.write(port2); 
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
  timeout_ioexp = millis();//no loop?
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

uint8_t ioexp_out_set_AIport(uint8_t AIport, bool statepin){
  static uint8_t p0,p1,p0ch,p1ch,mask0,mask1; 
  //must be static because ioexp read does not work when not static (some funny thing with addresses)
  if(!ioexp_read(&p0,&p1)){
      PRINTDEBUG("failed to read IO exp status\n");
      log_error_code(9);
      return 0;
    }
  //PRINTDEBUG("//p0r: %02x\t",p0);
  //PRINTDEBUG("//p1r: %02x\t",p1);
  switch (AIport){
    case 0: mask0 = 0b00001000; mask1 = 0b00000000; break;
    case 1: mask0 = 0b00000100; mask1 = 0b00000000; break;
    case 2: mask0 = 0b01000000; mask1 = 0b00000000; break;
    case 3: mask0 = 0b10000000; mask1 = 0b00000000; break;
    case 4: mask0 = 0b00000000; mask1 = 0b01000000; break;
    case 5: mask0 = 0b00000000; mask1 = 0b10000000; break;
    case 6: mask0 = 0b00010000; mask1 = 0b00000000; break;
    case 7: mask0 = 0b00100000; mask1 = 0b00000000; break;
    case 8: mask0 = 0b00000010; mask1 = 0b00000000; break;
    case 9: mask0 = 0b00000001; mask1 = 0b00000000; break;
    default : mask0 = 0b00000000; mask1 = 0b00000000;
  }
  if (statepin){
    //putting AI port high (5V), IO exp putput is low
    p0 &= (~mask0);
    p1 &= (~mask1);
  }else{
    //putting AI port low (0V), IO exp output is high
    p0 |= mask0;
    p1 |= mask1;
  }
  //PRINTDEBUG("p0s: %02x//\t",p0);
  //PRINTDEBUG("p1s: %02x//\t",p1);
  ioexp_out_set(p0,p1);

  if(!ioexp_read(&p0ch,&p1ch)){
      PRINTDEBUG("failed to read IO exp status\n");
      log_error_code(9);
      return 0;
    }

  if ((p0!=p0ch) | (p1!=p1ch)){
    return 0;
    log_error_code(8);//not matching
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
  adc1.setMultiplexer(ADS1115_MUX_P0_NG); //channel 0 single ended
  if (!adc2.testConnection())
    return 0;
  adc2.initialize(); // initialize ADS1115 16 bit A/D chip
  adc2.setMode(ADS1115_MODE_SINGLESHOT);
  adc2.setRate(ADS1115_RATE_64); 
  adc2.setGain(ADS1115_PGA_6P144); //6.144V range
  adc2.setMultiplexer(ADS1115_MUX_P0_NG); //channel 0 single ended
  if (!adc2.testConnection())
    return 0;
  adc3.initialize(); // initialize ADS1115 16 bit A/D chip
  adc3.setMode(ADS1115_MODE_SINGLESHOT);
  adc3.setRate(ADS1115_RATE_64); 
  adc3.setGain(ADS1115_PGA_6P144); //6.144V range
  adc3.setMultiplexer(ADS1115_MUX_P0_NG); //channel 0 single ended
  return 1;
}
//callback that prints received packets to the serial port
void udpReceiveProcess(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, const char *data, uint16_t len){
  //debug_UDP_receive(dest_port, &src_ip[IP_LEN], src_port, data, len);
  //callback is executed only if data received from 65500
  memcpy(receiveUDP_Buffer, data, len);
  for (uint8_t o=0;o<len;o++)
    {PRINTDEBUG("%02x:",receiveUDP_Buffer[o]);}
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

  if (epoch_time > 1558883838){ //sanity check, if pc is sending time from past May 26 2019, ignore
    setTime(epoch_time);
    PRINTDEBUG("Time updated: %d\n",epoch_time);
  } else {
    log_error_code(7);
    return 0;
  }
  return 1;
}
void insert_to_send_buffer_header(uint32_t val){
  sendUDP_Buffer[0] = val >> 24;
  sendUDP_Buffer[1] = val >> 16;
  sendUDP_Buffer[2] = val >> 8;
  sendUDP_Buffer[3] = val;
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

uint8_t adc_PGA_autorange(uint8_t old_PGA,int16_t last_reading){
  #define SWITCH_LOW 12000
  #define SWITCH_HIGH 24000

  if (last_reading < 0)
    last_reading *= -1;

  uint8_t new_PGA;

  if (old_PGA > 0){ //not max range
    if (last_reading > SWITCH_HIGH){
      new_PGA = ADS1115_PGA_6P144;//0x00
      return new_PGA;
    }
  }

  if (old_PGA < 5){ //not the lowest range
    if (last_reading < SWITCH_LOW){
      new_PGA = old_PGA + 1;
      return new_PGA;
    }
  } 

  return old_PGA;
}
//new sensor integration - follow this
uint8_t is_sensor_out_signed(uint8_t sensor_type){
  if (((sensor_type & 0x0F) < 0x8) & ((sensor_type >> 4) < 0xC))
    return 1; //yes is signed, all ADCs are
  else
    return 0;
}

void LMP91000_setup(uint8_t configuration_set){
  #define LMP_STATUS_REG 0x00
  #define LMP_LOCK_REG 0x01
  #define LMP_TIACN_REG 0x10
  #define LMP_REFCN_REG 0x11
  #define LMP_MODECN_REG 0x12
  uint8_t LMPenable[10]=  {1,1,1,1,1,1,1,1,1,1};
  LMPenable[10] = use_tgs24444 ? 0 : 1; //if anomia used, do not use port A3.2
  uint8_t tiacn[10] ={0x18,0x18,0x18,0x19,0x15,0x14,0x12,0x0C,0x09,0x09};//4:2 tia gain, 1:0 rload
  uint8_t refcn[10] ={0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x27,0x00};
  uint8_t modecn[10]={0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03};

  uint8_t LMPsett;
  LMPsett = ((configuration_set & 0b00011110)>>1);
  if (LMPsett == 0b0001){
    //index is AI input, value is register value
    //use initialized values
  }
  if (LMPsett == 0b0011){ //test all configured the same as from labview
    for (uint8_t boo;boo<10;boo++){
      LMPenable[boo] = 1;
      tiacn[boo] = LMPreg_TIACN;
      refcn[boo] = LMPreg_REFCN;
      modecn[boo] = LMPreg_MODECN;
    }
  }

  //read ioexp so it can be put to original state should be done here
  
  //this is only copy from debug function.. TODO better..
  uint8_t statusLMP;
  if(!ioexp_out_set(0x00,0x00)) PRINTDEBUG("err\n");
  PRINTDEBUG("set all AI to high\n");
  //to avoid 2 active LMPs on I2C bus
  for (int qui=0;qui<10;qui++){ //scan channels
    if(LMPenable[qui]){
      if(!ioexp_out_set_AIport(qui,0)) PRINTDEBUG("err2");
      PRINTDEBUG("AI %d low ...",qui);
      
      //check if LMP is there
      Wire.beginTransmission(LMP91000_ADR);
      Wire.write(LMP_STATUS_REG);//status register
      if (Wire.endTransmission()) {
        PRINTDEBUG("no LMP");
        if(!ioexp_out_set_AIport(qui,1)) PRINTDEBUG("err3");
        PRINTDEBUG(".. back to high\n");
        continue;
      }
      Wire.beginTransmission(LMP91000_ADR);
      Wire.requestFrom(LMP91000_ADR,1);
      if (Wire.available()){
        statusLMP = Wire.read();
        if (statusLMP) {
          PRINTDEBUG("LMP Ready\n");
          Wire.write(LMP_LOCK_REG);//0x11
          Wire.write(0x00);//disable write protection
          if (Wire.endTransmission()) PRINTDEBUG("Err r");
          //lock disabled
          //Write TIA
          Wire.beginTransmission(LMP91000_ADR);
          Wire.write(LMP_TIACN_REG);//0x10
          Wire.write(tiacn[qui]);//0x18 for A1.1
          if (Wire.endTransmission()) PRINTDEBUG("Err r");
          //Write REF
          Wire.beginTransmission(LMP91000_ADR);
          Wire.write(LMP_REFCN_REG);//0x11
          Wire.write(refcn[qui]);
          if (Wire.endTransmission()) PRINTDEBUG("Err r");
          //Write MODECN
          Wire.beginTransmission(LMP91000_ADR);
          Wire.write(LMP_MODECN_REG);//0x12
          Wire.write(modecn[qui]);
          //Wire.write(0b111);//temperature
          if (Wire.endTransmission()) PRINTDEBUG("Err r");
          //readback TIA
          Wire.beginTransmission(LMP91000_ADR);
          Wire.write(LMP_TIACN_REG);//0x10
          if (Wire.endTransmission()) PRINTDEBUG("Err r");
          Wire.beginTransmission(LMP91000_ADR);
          Wire.requestFrom(LMP91000_ADR,1);
          PRINTDEBUG("TIAREG was set: 0x%02x\n",Wire.read());
          if (Wire.endTransmission()) PRINTDEBUG("Err r");
          //readback REFCN
          Wire.beginTransmission(LMP91000_ADR);
          Wire.write(LMP_REFCN_REG);//0x11
          if (Wire.endTransmission()) PRINTDEBUG("Err r");
          Wire.beginTransmission(LMP91000_ADR);
          Wire.requestFrom(LMP91000_ADR,1);
          PRINTDEBUG("REFCN was set: 0x%02x\n",Wire.read());
          if (Wire.endTransmission()) PRINTDEBUG("Err r");
          //readback MODECN
          Wire.beginTransmission(LMP91000_ADR);
          Wire.write(LMP_MODECN_REG);//0x12
          if (Wire.endTransmission()) PRINTDEBUG("Err r");
          Wire.beginTransmission(LMP91000_ADR);
          Wire.requestFrom(LMP91000_ADR,1);
          PRINTDEBUG("MODECN was set: 0x%02x\n",Wire.read());
        }
        else
          PRINTDEBUG("LMP not Ready");
      }
      if (Wire.endTransmission()) PRINTDEBUG("Err r");
      
      if(!ioexp_out_set_AIport(qui,1)) PRINTDEBUG("err3");
      PRINTDEBUG(".. back to high\n");
    }
  }
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

  if (!ioexp_init(0xFF,0xFF)) //puts all high
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
    if(!sd_format_header(100))
      error("write data failed");
  }
  if (use_sd) {
    if(!sd_write_sample(1))
      error("write data failed");
  }
  if (use_sd) {
    logfile.close();
  }
}
void debug_Si7021(){
  uint16_t t;
  uint16_t rh;
  if(use_Si7021){
    uint32_t startms = millis();
    for (int wee=0;wee<150;wee++) {
      delay(1000);
      startms = millis();
      PRINTDEBUG("%06u\tAdafruit reading start\n",millis()-startms);
      PRINTDEBUG("%06u\tAdafuit fuctions reading - Temp,RH: \t%f\t%f\n",millis()-startms,Si7021.readTemperature(),Si7021.readHumidity());
      PRINTDEBUG("%06u\tRequest start\n",millis()-startms);
      if(!Si7021.RHrequest_measurement())
        PRINTDEBUG("%06u\tRequest failed\n",millis()-startms);
      while(1){
      if (use_veml){
        PRINTDEBUG("%06u\tMeantime read light %f Lux\n",millis()-startms,veml.readLux());
        PRINTDEBUG("%06u\tMeantime read ended\n",millis()-startms);
      }
      delay(150);
      if(!Si7021.read_requested(&t,&rh))
        PRINTDEBUG("%06u\tReadout failed\n",millis()-startms);
      else{
        PRINTDEBUG("%06u\tMy fuctions reading - Temp,RH: \t%u\t%u\n",millis()-startms,t,rh);
        break;
      }
      }
    }

  }else{
    PRINTDEBUG("No SI7021 debug, use_Si7021 is false\n");
  }
}

void debug_ioexp_AIport(){
  
  if(!ioexp_out_set(0x00,0x00)) PRINTDEBUG("err\n");
  PRINTDEBUG("set all AI to high\n");
  delay(2000);

  for (int qui=0;qui<10;qui++){
    if(!ioexp_out_set_AIport(qui,0)) PRINTDEBUG("err2");
    PRINTDEBUG("AI %d low ...",qui);
    delay(1000);
    if(!ioexp_out_set_AIport(qui,1)) PRINTDEBUG("err3");
    PRINTDEBUG(" high\n");
    delay(10000);
  }
}
void debug_lmp91000(){
  #define LMP_STATUS_REG 0x00
  #define LMP_LOCK_REG 0x01
  #define LMP_TIACN_REG 0x10
  #define LMP_REFCN_REG 0x11
  #define LMP_MODECN_REG 0x12
  const uint8_t tiacn[10] =    {0x18,0x18,0x18,0x19,0x09,0x14,0x12,0x0C,0x09,0x09}; //4:2 tia gain, 1:0 rload
  const uint8_t refcn[10] =    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x27,0x00};
  const uint8_t modecn[10] =   {0x01,0x03,0x03,0x03,0x03,0x03,0x01,0x03,0x03,0x03};
        
  //read ioexp so it can be put to original state should be done here
  uint8_t statusLMP;
  if(!ioexp_out_set(0x00,0x00)) PRINTDEBUG("err\n");
  PRINTDEBUG("set all AI to high\n");
  //to avoid 2 active LMPs on I2C bus
  for (int qui=0;qui<10;qui++){ //scan channels
    if(!ioexp_out_set_AIport(qui,0)) PRINTDEBUG("err2");
    PRINTDEBUG("AI %d low ...",qui);
    
    //check if LMP is there
    Wire.beginTransmission(LMP91000_ADR);
    Wire.write(LMP_STATUS_REG);//status register
    if (Wire.endTransmission()) {
      PRINTDEBUG("no LMP");
      if(!ioexp_out_set_AIport(qui,1)) PRINTDEBUG("err3");
      PRINTDEBUG(".. back to high\n");
      continue;
    }
    Wire.beginTransmission(LMP91000_ADR);
    Wire.requestFrom(LMP91000_ADR,1);
    if (Wire.available()){
      statusLMP = Wire.read();
      if (statusLMP) {
        PRINTDEBUG("LMP Ready\n");
        Wire.write(LMP_LOCK_REG);//0x11
        Wire.write(0x00);//disable write protection
        if (Wire.endTransmission()) PRINTDEBUG("Err r");
        //lock disabled
        //Write TIA
        Wire.beginTransmission(LMP91000_ADR);
        Wire.write(LMP_TIACN_REG);//0x10
        Wire.write(tiacn[qui]);//0x18 for A1.1
        if (Wire.endTransmission()) PRINTDEBUG("Err r");
        //Write REF
        Wire.beginTransmission(LMP91000_ADR);
        Wire.write(LMP_REFCN_REG);//0x11
        Wire.write(refcn[qui]);
        if (Wire.endTransmission()) PRINTDEBUG("Err r");
        //Write MODECN
        Wire.beginTransmission(LMP91000_ADR);
        Wire.write(LMP_MODECN_REG);//0x12
        Wire.write(modecn[qui]);
        //Wire.write(0b111);//temperature
        if (Wire.endTransmission()) PRINTDEBUG("Err r");
        //readback TIA
        Wire.beginTransmission(LMP91000_ADR);
        Wire.write(LMP_TIACN_REG);//0x10
        if (Wire.endTransmission()) PRINTDEBUG("Err r");
        Wire.beginTransmission(LMP91000_ADR);
        Wire.requestFrom(LMP91000_ADR,1);
        PRINTDEBUG("TIAREG was set: %02x\n",Wire.read());
        if (Wire.endTransmission()) PRINTDEBUG("Err r");
        //readback REFCN
        Wire.beginTransmission(LMP91000_ADR);
        Wire.write(LMP_REFCN_REG);//0x11
        if (Wire.endTransmission()) PRINTDEBUG("Err r");
        Wire.beginTransmission(LMP91000_ADR);
        Wire.requestFrom(LMP91000_ADR,1);
        PRINTDEBUG("REFCN was set: %02x\n",Wire.read());
        if (Wire.endTransmission()) PRINTDEBUG("Err r");
        //readback MODECN
        Wire.beginTransmission(LMP91000_ADR);
        Wire.write(LMP_MODECN_REG);//0x12
        if (Wire.endTransmission()) PRINTDEBUG("Err r");
        Wire.beginTransmission(LMP91000_ADR);
        Wire.requestFrom(LMP91000_ADR,1);
        PRINTDEBUG("MODECN was set: %02x\n",Wire.read());
      }
      else
        PRINTDEBUG("LMP not Ready");
    }
    if (Wire.endTransmission()) PRINTDEBUG("Err r");
    
    if(!ioexp_out_set_AIport(qui,1)) PRINTDEBUG("err3");
    PRINTDEBUG(".. back to high\n");
  }
}

