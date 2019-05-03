/*
 * UIPEthernet UdpServer example.
 *
 * UIPEthernet is a TCP/IP stack that can be used with a enc28j60 based
 * Ethernet-shield.
 *
 * UIPEthernet uses the fine uIP stack by Adam Dunkels <adam@sics.se>
 *
 *      -----------------
 *
 * This UdpServer example sets up a udp-server at 192.168.0.6 on port 5000.
 * send packet via upd to test
 *
 * Copyright (C) 2013 by Norbert Truchsess (norbert.truchsess@t-online.de)
 */

#include <UIPEthernet.h>
#include "utility/logging.h"

EthernetUDP udp;

void setup() {
  #if ACTLOGLEVEL>LOG_NONE
    LogObject.begin(115200);
  #endif

  uint8_t mac[6] = {0x00,0x01,0x02,0x03,0x04,0x05};

  Ethernet.begin(mac,IPAddress(192,168,0,200));

  int success = udp.begin(5000);

  #if ACTLOGLEVEL>=LOG_INFO
    LogObject.uart_send_str(F("initialize: "));
    LogObject.uart_send_strln(success ? "success" : "failed");
  #endif
}

void loop() {
  //check for new udp-packet:
  int size = udp.parsePacket();
  if (size > 0) {
    do
      {
        char* msg = (char*)malloc(size+1);
        int len = udp.read(msg,size+1);
        msg[len]=0;
        #if ACTLOGLEVEL>=LOG_INFO
          LogObject.uart_send_str(F("received: '"));
          LogObject.uart_send_str(msg);
          LogObject.uart_send_str(F(" \n"));
        #endif
        free(msg);
      }
    while ((size = udp.available())>0);
    //finish reading this packet:
    udp.flush();
    #if ACTLOGLEVEL>=LOG_INFO
      LogObject.uart_send_strln(F("'"));
    #endif
    int success;
    do
      {
      #if ACTLOGLEVEL>=LOG_INFO
        LogObject.uart_send_str(F("remote ip: "));
        LogObject.println(udp.remoteIP());
        LogObject.uart_send_str(F("remote port: "));
        LogObject.uart_send_decln(udp.remotePort());
      #endif
        //send new packet back to ip/port of client. This also
        //configures the current connection to ignore packets from
        //other clients!
        success = udp.beginPacket(udp.remoteIP(),udp.remotePort());
      #if ACTLOGLEVEL>=LOG_INFO
        LogObject.uart_send_str(F("beginPacket: "));
        LogObject.uart_send_strln(success ? "success" : "failed");
      #endif
    //beginPacket fails if remote ethaddr is unknown. In this case an
    //arp-request is send out first and beginPacket succeeds as soon
    //the arp-response is received.
      }
    while (!success);

    static int a=0;
    a++;
    success = udp.print(F("hello world from arduino "));
    success+= udp.println(a);
    #if ACTLOGLEVEL>=LOG_INFO
      LogObject.uart_send_str(F("bytes written: "));
      LogObject.uart_send_decln(success);
    #endif

    success = udp.endPacket();

    #if ACTLOGLEVEL>=LOG_INFO
      LogObject.uart_send_str(F("endPacket: "));
      LogObject.uart_send_strln(success ? "success" : "failed");
    #endif

    udp.stop();
    //restart with new connection to receive packets from other clients
    #if ACTLOGLEVEL>=LOG_INFO
      LogObject.uart_send_str(F("restart connection: "));
      LogObject.uart_send_strln(udp.begin(5000) ? "success" : "failed");
    #endif
  }
}