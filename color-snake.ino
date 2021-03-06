//
//  color-snake.ino
//  
//
//  Created by Alex Lelievre on 12/26/18 (most of this source file from Adafruit).
//

#include "snake.h"
#include "Adafruit_miniTFTWing.h"

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
  // Required for Serial on Zero based boards
  #define Serial SERIAL_PORT_USBVIRTUAL
#endif


#define DISPLAY_INVERTED

static Adafruit_miniTFTWing ss;
static bool                 s_state_running = false;


void setup() 
{
  randomSeed( analogRead( 0 ) );

  Serial.begin(115200);
  
  if( !ss.begin() ) 
  {
    Serial.println( "seesaw init error!" );
    while(1);
  }

  ss.tftReset();
  ss.setBacklight( 0x0 ); //set the backlight fully on

  initialize_graphics();
  
  Serial.println( "Snake game initialized" );
  
  draw_intro();
}



void loop() 
{
    uint32_t buttons = ss.readButtons();

    if( !s_state_running && !((buttons & TFTWING_BUTTON_A) && (buttons & TFTWING_BUTTON_B) && (buttons & TFTWING_BUTTON_SELECT)) )
    {
        start_game();
        s_state_running = true;
    }

    if( !s_state_running )
        return;
        
    draw_snake();

    if( !(buttons & TFTWING_BUTTON_A) ) 
        pause();

#ifdef DISPLAY_INVERTED
    if( !(buttons & TFTWING_BUTTON_LEFT) ) 
        move_right();

    if( !(buttons & TFTWING_BUTTON_RIGHT) ) 
        move_left();

    if( !(buttons & TFTWING_BUTTON_DOWN) ) 
        move_up();

    if( !(buttons & TFTWING_BUTTON_UP) ) 
        move_down();
#else
    if( !(buttons & TFTWING_BUTTON_LEFT) ) 
        move_left();

    if( !(buttons & TFTWING_BUTTON_RIGHT) ) 
        move_right();

    if( !(buttons & TFTWING_BUTTON_DOWN) ) 
        move_down();

    if( !(buttons & TFTWING_BUTTON_UP) ) 
        move_up();
#endif

    move_snake();
}

// EOF
