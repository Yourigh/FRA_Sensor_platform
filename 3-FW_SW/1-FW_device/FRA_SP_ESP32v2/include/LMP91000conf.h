#ifndef LMP91000conf_h
#define LMP91000conf_h

//indexes:
//reg[configuration set for electrochemical][port]
// *port = A1.0-A1.3,A2.0-A2.3,A3.2    - A3.1 not used, override registers directly from labview
// *configuration set = 4 bits from Labview incremented by one
//index 8 MODECN turns on A3.1, but with custom parameters sent from labview
//index 8 TIA REF - A3.1 is ignored - takes values from start dtg

//4:2 tia gain, 1:0 rload

const uint8_t tiacn[15][10] = {
{0x18,0x18,0x18,0x19,0x1C,0x14,0x12,0x0C,0x00,0x09}, //conf set 1
{0x18,0x0E,0x15,0x11,0x19,0x19,0x11,0x04,0x04,0x04}, //conf set 2
{0x18,0x0E,0x15,0x11,0x19,0x19,0x04,0x04,0x04,0x04}, //conf set 3
{0x18,0x0E,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04}, //conf set 4
{0x18,0x0E,0x18,0x0E,0x04,0x04,0x04,0x04,0x04,0x04}, //conf set 5
{0x18,0x0E,0x18,0x0E,0x14,0x04,0x04,0x04,0x04,0x04}, //conf set 6
{0x0E,0x0E,0x0E,0x0E,0x04,0x04,0x04,0x04,0x04,0x04}}; //conf set 7
//1.0,1.1 ,1.2 ,1.3 ,2.0, 2.1 ,2.2 ,2.3 ,3.1, 3.2 ,
const uint8_t refcn[15][10] =    {
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},//1
{0x00,0x40,0x40,0x14,0x00,0x00,0x00,0x00,0x00,0x00},//2
{0x00,0x40,0x40,0x14,0x00,0x00,0x00,0x00,0x00,0x00},//3
{0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},//4
{0x00,0x40,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00},//5
{0x00,0x40,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00},//6
{0x40,0x40,0x40,0x40,0x00,0x00,0x00,0x00,0x00,0x00}};//7
const uint8_t modecn[15][10] =   {
{0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0xFF,0x00},//cs 1
{0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x00,0xFF,0x00},//cs 2
{0x03,0x03,0x03,0x03,0x03,0x03,0x00,0x00,0xFF,0x00},//cs 3
{0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00},//cs 4
{0x03,0x03,0x03,0x03,0x00,0x00,0x00,0x00,0xFF,0x00},//cs 5
{0x03,0x03,0x03,0x03,0x03,0x00,0x00,0x00,0x00,0x00},//cs 6
{0x03,0x03,0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x00},//cs 7
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00},//cs 8
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00},//cs 9
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00},//cs 10
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00},//cs 11
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00},//cs 12
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00},//cs 13
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00},//cs 14
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00}};//cs 15

#endif