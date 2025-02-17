#include "SPI.h"
#include "U8g2lib.h"

U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 15, /* dc=*/ 2, /* reset=*/ 4);

byte cx=64;           // x center
byte cy=45;           // y center
byte radius=40;       // radius outer circle
byte radius2=30;      // radius inner circle
byte radius3=6;       // radius needle mount
int value;
int skip = 0;

void Display_function(float output) {
  u8g2.clearBuffer();				                  // clear the internal memory
  u8g2.setFont(u8g2_font_unifont_h_symbols);  // just for the up/down arrows

  value = (int(output));                      // cast float to int
  if (skip<2) {                               // Prevent needle starting at "0" first round,
    skip++;                                   // skips first iterations
    return;
  }

  if (value<1) { value = 0; }                 // prevent needle below '0'
  
  u8g2.setFont(u8g2_font_t0_12b_te);
  u8g2.drawCircle(cx,cy,radius, U8G2_DRAW_UPPER_LEFT|U8G2_DRAW_UPPER_RIGHT );
  u8g2.drawCircle(cx,cy,radius2, U8G2_DRAW_UPPER_LEFT|U8G2_DRAW_UPPER_RIGHT );
  u8g2.drawCircle(cx,cy,radius3, U8G2_DRAW_UPPER_LEFT|U8G2_DRAW_UPPER_RIGHT );
  u8g2.drawLine(cx-radius, cy, cx-radius2, cy);
  u8g2.drawLine(cx+radius, cy, cx+radius2, cy);
  u8g2.drawLine(cx, cy-radius, cx, cy-radius2-6);   // 50% 
  u8g2.drawLine(cx+radius * sin(3.14 * 2 * 315/360)+1, cy-radius * cos(3.14 * 2 * 315 / 360)+1 , (cx+radius2 * sin(3.14 * 2 * 315/360))-4, (cy-radius2 * cos(3.14 * 2 * 315/360))-4);   // 25%
  u8g2.drawLine(cx+radius * sin(3.14 * 2 * 45/360), cy-radius * cos(3.14 * 2 * 45 / 360) , (cx+radius2 * sin(3.14 * 2 * 45 / 360))+5, (cy-radius2 * cos(3.14 * 2 * 45 / 360))-5);       // 75%
  u8g2.setFont(u8g2_font_t0_11_tn);
  u8g2.drawStr(12,48,"0"); u8g2.drawStr(110,48,"100");
  u8g2.setFont(u8g2_font_t0_13b_te);
  u8g2.drawStr(2,64,"Vuilwater:");
  u8g2.setCursor(72,64); u8g2.print(value); u8g2.print("% vol");
  if (value>85) { u8g2.drawStr(0,10,"VOL!"); }
  else if (value>80) { u8g2.drawStr(0,10,"Bijna"); u8g2.drawStr(0,20,"vol"); }
  else if (value<5) { u8g2.drawStr(0,10,"Leeg"); }
  float val = map(value, 0, 100, 0, 180);
  val = val*3.14/180 -1.572;
  int xp = cx+(sin(val) * radius);
  int yp = cy-(cos(val) * radius);
  u8g2.drawLine(cx,cy,xp,yp);
  u8g2.drawLine(cx-1,cy-1,xp,yp);
  u8g2.drawLine(cx+1,cy-1,xp,yp);
  u8g2.drawLine(cx-1,cy,xp,yp);
  u8g2.drawLine(cx+1,cy,xp,yp);

  u8g2.sendBuffer();					            // transfer internal memory to the display
}
