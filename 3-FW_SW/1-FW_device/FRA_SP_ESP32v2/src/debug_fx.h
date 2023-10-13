#ifndef DEBUG_FX_H
#define DEBUG_FX_H

#define DEBUG 1 //UART logs
#if DEBUG == 1
  #define PRINTDEBUG Serial.printf
#else
  #define PRINTDEBUG
#endif 

#include "NDIR_module.h"

void debug_NDIR(void)
{
    PRINTDEBUG("\nNDIR test read");
    uint16_t NDIRstatus = 0xFFFF;
    uint16_t NDIRgas = 0;
    uint16_t NDIRth = 0;
    if (readNDIRRegisters(NDIR_ADDR, &NDIRstatus, &NDIRgas, &NDIRth))
    {
        PRINTDEBUG("\nFail read, NDIR");
    }
    if(NDIRstatus == 0xFFFF && NDIRgas == 0xFFFF && NDIRth == 0xFFFF){
        PRINTDEBUG("\nFail, all 0xFFFF, NDIR");
    }
    PRINTDEBUG("\nStatus: 0x%04X\tGas: %d\tTemp: %d", NDIRstatus, NDIRgas, NDIRth);
    float converted_gas = (float)NDIRgas / 10000.0;
    float converted_temp = ((((float)NDIRth) / 10.0) - 200.0);
    PRINTDEBUG("\nConverted Gas: %.4f\tConverted Temp: %.2f\n\n", converted_gas, converted_temp);
}

void debug_NDIR_UDP_datagram(uint8_t *sendUDP_Buffer,uint8_t sendUDP_len){
    PRINTDEBUG("\nDebug sent UDP NDIR data: 0x");
    for(uint8_t di=sendUDP_len-5;di<=sendUDP_len;){
        PRINTDEBUG("%02X",sendUDP_Buffer[di++]);
    }
    PRINTDEBUG(" UDP message length %d",sendUDP_len+3);  //debug check of overflow.
}

/*
//debug functions
void debug_UDP_receive(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, const char *data, uint16_t len);
void debug_sd_log();
uint8_t debug_adc();
void debug_GPIOexp();
void debug_Si7021();
void debug_ioexp_AIport();
*/

////////////////////////////DEBUG FUNCTIONS, copied out from main. To use, copy back or edit so they work.
/*

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
}

uint8_t debug_adc(ADS1115 adc1,ADS1115 adc2, ADS1115 adc3){
  uint32_t m=micros();
  uint32_t n=micros();
   
 // Serial.println(adc1.testConnection() ? "ADC1 connection successful" : "ADC1 connection failed");
 // adc1.initialize(); // initialize ADS1115 16 bit A/D chip
 // adc1.setMode(ADS1115_MODE_SINGLESHOT);
 // adc1.setRate(ADS1115_RATE_64); //16 SPS is enough even for 100ms sampling
 // adc1.setGain(ADS1115_PGA_6P144); //6.144V range
  //ALERT/RDY pin already disabled in INIT
  
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


void debug_Si7021(bool use_Si7021,bool use_veml){
  uint16_t t;
  uint16_t rh;
  if(use_Si7021){
    uint32_t startms = millis();
    for (int wee=0;wee<150;wee++) {
      delay(1000);
      startms = millis();
      PRINTDEBUG("%06lu\tAdafruit reading start\n",millis()-startms);
      PRINTDEBUG("%06lu\tAdafuit fuctions reading - Temp,RH: \t%f\t%f\n",millis()-startms,Si7021.readTemperature(),Si7021.readHumidity());
      PRINTDEBUG("%06lu\tRequest start\n",millis()-startms);
      if(!Si7021.RHrequest_measurement())
        PRINTDEBUG("%06lu\tRequest failed\n",millis()-startms);
      while(1){
      if (use_veml){
        PRINTDEBUG("%06lu\tMeantime read light %f Lux\n",millis()-startms,veml.readLux());
        PRINTDEBUG("%06lu\tMeantime read ended\n",millis()-startms);
      }
      delay(150);
      if(!Si7021.read_requested(&t,&rh))
        PRINTDEBUG("%06lu\tReadout failed\n",millis()-startms);
      else{
        PRINTDEBUG("%06lu\tMy fuctions reading - Temp,RH: \t%u\t%u\n",millis()-startms,t,rh);
        break;
      }
      }
    }

  }else{
    PRINTDEBUG("No SI7021 debug, use_Si7021 is false\n");
  }
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
void debug_sd_log(bool use_sd,ofstream logfile){
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
*/

#endif