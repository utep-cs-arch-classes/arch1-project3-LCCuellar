#include <libTimer.h>
#include <msp430.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include "abCircle.h"

AbRect rect10 = {abRectGetBounds, abRectCheck, {10,10}};; /**< 10x10 rectangle */

u_int bgColor = COLOR_BLUE;
char pacState;

Layer layer1 = {
  (AbShape *)&rect10,
  {screenWidth/2, screenHeight/2},
  {0,0},{0,0},
  COLOR_BLUE,
  0
};


Layer layer0 = {		/**< Layer with an orange circle */
  (AbShape *)&circle25,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* next & last pos */
  COLOR_ORANGE,
  &layer1,
};

main()
{
  configureClocks();
  lcd_init();

  enableWDTInterrupts();
  
  clearScreen(COLOR_BLUE);
  drawString5x7(20,20, "hello", COLOR_GREEN, COLOR_RED);

  pacState = 1;
  layerDraw(&layer0);

  or_sr(0x8);

}
__interrupt(WDT_VECTOR) WDT() {
  static char blink_count = 0;
  if(++blink_count == 1) {
    blink_count = 0;    
    switch(pacState) {
    case 1:
      pacState = 2;
      break;
    case 2:
      pacState = 4;
      break;
    case 4:
      pacState = 250;
      break;
    case 250:
      pacState = 1;
      break;
    }
    layerDraw(&layer0);
  }
}
