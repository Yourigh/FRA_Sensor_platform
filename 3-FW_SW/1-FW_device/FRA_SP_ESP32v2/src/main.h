#ifndef MAIN_H
#define MAIN_H

//debug macro
#define DEBUG 1 //UART logs

#if DEBUG == 1
  #define PRINTDEBUG Serial.printf
#else
  #define PRINTDEBUG
#endif 


#endif