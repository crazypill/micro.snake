//
//  snake.c
//  
//
//  Created by Alex Lelievre on 12/26/18.
//

#include "snake.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

#define kMinDelay     5 
#define kLineWidth    2
#define kMaxSegments  100
#define kScreenWidth  tft.width() 
#define kScreenHeight tft.height()
#define kStartingPointX  80
#define kStartingPointY  40

// don't erase the screen on game over for debugging collisions
//#define KEEP_DISPLAY_FOR_DEBUG

#pragma mark -

/////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    // used by the eraser
    int16_t  x;         // position of where this segment turns
    int16_t  y;
    int16_t  dir_x;     // the turn we will take
    int16_t  dir_y;

    // mostly used for collision
    int16_t  start_x;   // position of where this segment starts
    int16_t  start_y;
    uint16_t length;    // how long it is...
} Segment;

#pragma mark -

static Segment snake_draw  = { kStartingPointX, kStartingPointY, 0, 1, 0, 0, 10 };    
static Segment snake_erase = { kStartingPointX, kStartingPointY, 0, 1, 0, 0, 1 };    

static int16_t  seg_start_x = kStartingPointX;
static int16_t  seg_start_y = kStartingPointY;

static int16_t  apple_x      = 0;
static int16_t  apple_y      = 0;
static int16_t  s_score      = 0;
static uint16_t s_delayTime  = 40;       // this gets shorter as the levels get higher
static bool     s_paused     = false;
static uint16_t s_counter    = 0;

static uint16_t s_segment_count  = 0;
static uint16_t s_segment_writer = 0;
static uint16_t s_segment_reader = 0;
static Segment  s_segments[kMaxSegments];

static Adafruit_ST7735 tft = Adafruit_ST7735( TFT_CS,  TFT_DC, TFT_RST );


/////////////////////////////////////////////////////////////////////////////////////////////////////

void check_for_apple();
void place_apple();
bool apple_in_segment();
void check_for_direction_change();
void boundary_clamp( Segment* );
void erase_snake();
bool snake_in_segment();
void print_error( const char* error );

#pragma mark -


int16_t fast_length( int16_t x, int16_t y )
{
    int16_t dx = abs( x );
    int16_t dy = abs( y );
    return dx + dy - (min( dx, dy ) >> 1);
}


// returns length from origin (or length/magnitude of vector)
int16_t fast_hvline_length( int16_t x, int16_t y )
{
    // it's assumed that this line is horizontal or vertical
    if( !x && !y )
        return 0;
    
    // this is easy :)   the idea here is that we can easily count the length by using the coorindate which is always h/v from origin
    if( !x )
        return abs( y );
    
    // line is assumed to be horizontal here
    return abs( x );
}


bool nearly_equals( int16_t p1, int16_t p2, int16_t errorTolerance )
{
    return abs( p1 - p2 ) <= errorTolerance && abs( p1 - p2 ) <= errorTolerance;
}


bool dot_in_segment( int16_t x, int16_t y, Segment* seg )
{
    // first determine if this line is vertical or horizontal
    int16_t v = abs( seg->x - seg_start_x );
    if( v == 0 )
    {   
        // line is vertical -- see if point is within the line somewhere (not necessarily the segment but a line is infinite)
//        if( nearly_equals( x, seg->x, /*kLineWidth*/ 0 ) )
        if( x == seg->x )
            return (y > seg->start_y && y < seg->y);
    }
    else
    {
        // horizontal
//        if( nearly_equals( y, seg->y, /*kLineWidth*/ 0 ) )
        if( y == seg->y )
            return (x > seg->start_x && x < seg->x);
    }

    return false;
}


void draw_grid( uint16_t color1, uint16_t color2 ) 
{
  tft.fillScreen( ST77XX_BLACK );
  for( int16_t y = 0; y < tft.height(); y += 10 )
    tft.drawFastHLine( 0, y, tft.width(), color1 );
  
  for( int16_t x = 0; x < tft.width(); x += 10 )
    tft.drawFastVLine( x, 0, tft.height(), color2 );
}


#pragma mark -

/////////////////////////////////////////////////////////////////////////////////////////////////////

bool initialize_graphics() 
{
  // Use this initializer (uncomment) if you're using a 0.96" 180x60 TFT
  tft.initR( INITR_MINI160x80 );   // initialize a ST7735S chip, mini display
  tft.setRotation( 3 );
  tft.fillScreen( ST77XX_BLACK );
  return true;
}


Adafruit_ST7735* get_tft()
{
    return &tft;
}


void print_error( const char* error )
{
  tft.setCursor(0, 0);
  tft.setTextColor( ST77XX_RED );
  tft.setTextSize( 1 );
  tft.println( error );
}


#pragma mark -


void start_game()
{  
  tft.fillScreen( ST77XX_BLACK );
//  draw_grid( 0x1111, 0x1111 );        // !!@ debug
  place_apple();
}


void pause()
{
    s_paused = !s_paused;
//    tft.println( s_paused ? "paused" : "running" );
    delay( 25 );
}


void game_over()
{
    // flash quickly first
    for( int i = 0; i < 15; i++ )
    {
        tft.invertDisplay( true );
        delay( 50 );
        tft.invertDisplay( false );
        delay( 50 );
    }

#ifndef KEEP_DISPLAY_FOR_DEBUG    
    delay( 1500 );  // 1.5 secs
    tft.fillScreen( ST77XX_BLACK );

    tft.setCursor( 0, 0 );
    tft.setTextColor( ST77XX_RED );
    tft.setTextSize( 3 );
    tft.println( "Game Over" );

    tft.setCursor( 38, 34 );
    tft.setTextColor( ST77XX_BLUE );
    tft.setTextSize( 1 );
    tft.print( "Your score: " );
    tft.println( s_score );
#endif

    while( 1 )
    ;
}


#pragma mark -


void draw_intro()
{
  tft.setTextWrap( false );
  tft.setCursor(0, 0);
  
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(1);
  tft.println("Far Out Labs");
  
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.println("Far Out Labs");

  delay( 850 );
  tft.fillScreen( ST77XX_BLACK );

  // now draw the press any key to start text
  tft.setCursor( 0, 0 );
  tft.setTextColor(ST77XX_BLUE);
  tft.setTextSize(3);
  tft.println("Snake 1.0");

  tft.setCursor( 13, 34 );
  tft.setTextColor( ST77XX_YELLOW );
  tft.setTextSize( 1 );
  tft.println("Press any key to start");
}


void draw_dot( int16_t x_pos, int16_t y_pos, uint16_t color )
{
    tft.fillCircle( x_pos, y_pos, 1, color );
}


void draw_snake()
{
    draw_dot( snake_draw.x, snake_draw.y, ST77XX_GREEN );
    erase_snake();
}


void erase_snake()
{
    if( s_counter < snake_draw.length )
        return;

    draw_dot( snake_erase.x, snake_erase.y, ST77XX_BLACK );
}


void move_snake()
{
    if( s_paused )
        return;
        
    snake_draw.x += snake_draw.dir_x;
    snake_draw.y += snake_draw.dir_y;
    check_for_apple();
    if( snake_in_segment() )
        game_over();
    
    if( s_counter < snake_draw.length )
        ++s_counter;
    else
    {
        snake_erase.x += snake_erase.dir_x;
        snake_erase.y += snake_erase.dir_y;
        check_for_direction_change();
    }

    boundary_clamp( &snake_draw );
    boundary_clamp( &snake_erase );
    delay( s_delayTime );
}


void dump_segments()
{
    for( int i = 0; i < s_segment_count; i++ )
    {
        // first segment is at reader index
        int index = s_segment_reader + i;
        Serial.print( "segment: " ); 
        Serial.print( index );
        Serial.print( "[" );
        Serial.print( i );
        Serial.print( "], (" );
        Serial.print( s_segments[index].start_x );
        Serial.print( ", " );
        Serial.print( s_segments[index].start_y );
        Serial.print( ") -> (" );
        Serial.print( s_segments[index].x );
        Serial.print( ", " );
        Serial.print( s_segments[index].y );
        Serial.print( "), dir: (" );
        Serial.print( s_segments[index].dir_x );
        Serial.print( ", " );
        Serial.print( s_segments[index].dir_y );
        Serial.print( "), length: " );
        Serial.println( s_segments[index].length );
    }
    Serial.print( "count: " );
    Serial.println( s_segment_count );
}


bool snake_in_segment()
{
    // we need at least 4 segments in order to intersect with ourselves
    if( s_segment_count < 4 )
        return false;
        
    // go thru all the segments and see if we intersect any
    for( int i = 0; i < s_segment_count; i++ )
    {
        // first segment is at reader index
        int index = s_segment_reader + i;
        if( dot_in_segment( snake_draw.x, snake_draw.y, &s_segments[index] ) )
        {
            Serial.print( "snake_in_segment: " ); 
            Serial.print( index );
            Serial.print( "[" );
            Serial.print( i );
            Serial.print( "], snake head (" );
            Serial.print( snake_draw.x );
            Serial.print( ", " );
            Serial.print( snake_draw.y );
            Serial.print( "), snake dir: (" );
            Serial.print( snake_draw.dir_x );
            Serial.print( ", " );
            Serial.print( snake_draw.dir_y );
            Serial.print( "), segment: (" );
            Serial.print( s_segments[index].start_x );
            Serial.print( ", " );
            Serial.print( s_segments[index].start_y );
            Serial.print( ") -> (" );
            Serial.print( s_segments[index].x );
            Serial.print( ", " );
            Serial.print( s_segments[index].y );
            Serial.print( "), length: " );
            Serial.print( s_segments[index].length );
            Serial.print( ", writer index: " );
            Serial.print( s_segment_writer );
            Serial.print( ", segment count: " );
            Serial.println( s_segment_count );
            
            dump_segments();
            return true;
        }
    }
    
    return false;
}



#pragma mark -

void draw_apple()
{
    draw_dot( apple_x, apple_y, ST77XX_RED );
}


void erase_apple()
{
    draw_dot( apple_x, apple_y, ST77XX_BLACK );
}


void place_apple()
{
    // erase the old one first
    erase_apple();
    
    // make sure we never put an apple on top of the snake
    do
    {
        apple_x = random( 0, kScreenWidth );
        apple_y = random( 0, kScreenHeight );
    } while( apple_in_segment() );
    
    draw_apple();
}


void check_for_apple()
{
    // see if we hit an apple!
    if( nearly_equals( apple_x, snake_draw.x, kLineWidth ) && nearly_equals( apple_y, snake_draw.y, kLineWidth ) )
    {
        // make snake longer and the game faster and faster
        if( s_delayTime > kMinDelay )
            --s_delayTime;
        snake_draw.length += 20;
        place_apple();
        ++s_score;
    }
}


bool apple_in_segment()
{
    // go thru all the segments and see if we intersect any
    for( int i = 0; i < s_segment_count; i++ )
    {
        // first segment is at reader index
        int index = s_segment_reader + i;
        if( dot_in_segment( apple_x, apple_y, &s_segments[index] ) )
            return true;
    }
    
    return false;
}

#pragma mark -


void boundary_clamp( Segment* segment )
{
    if( segment->x < 0 || segment->y < 0 || segment->x >= kScreenWidth || segment->y >= kScreenHeight )
        game_over();
}


void add_segment()
{
    // take current position and create a new segment
    s_segments[s_segment_writer].x       = snake_draw.x;
    s_segments[s_segment_writer].y       = snake_draw.y;
    s_segments[s_segment_writer].dir_x   = snake_draw.dir_x;
    s_segments[s_segment_writer].dir_y   = snake_draw.dir_y;

    // if this is the first segment, the end of it is not the starting point, it's the eraser's head
    if( !s_segment_writer )
    {
        seg_start_x = snake_erase.x;
        seg_start_y = snake_erase.y;
    }

    // used for collision testing of this segment
    s_segments[s_segment_writer].start_x = seg_start_x;
    s_segments[s_segment_writer].start_y = seg_start_y;
    s_segments[s_segment_writer].length  = fast_hvline_length( snake_draw.x - seg_start_x, snake_draw.y - seg_start_y );
    ++s_segment_writer;
    ++s_segment_count;
    
    // update segment start for next segment
    seg_start_x = snake_draw.x;
    seg_start_y = snake_draw.y;

    // s_segments is a circuler buffer...
    if( s_segment_writer >= kMaxSegments )
        s_segment_writer = 0;  // wrap around to the front of the buffer
    
//    Serial.print( "push- s_segment_count: " );
//    Serial.print( s_segment_count );
//    Serial.print( ", s_segment_writer: " );
//    Serial.println( s_segment_writer );
}


void check_for_direction_change()
{
    if( !s_segment_count )
        return;
        
    // see if we are standing on a direction change
    if( s_segments[s_segment_reader].x == snake_erase.x && s_segments[s_segment_reader].y == snake_erase.y )
    {
        // read the data and pop it off
        snake_erase.dir_x = s_segments[s_segment_reader].dir_x;
        snake_erase.dir_y = s_segments[s_segment_reader].dir_y;
                
        // pop the segment too!
        ++s_segment_reader;
        --s_segment_count;

        if( s_segment_reader >= kMaxSegments )
            s_segment_reader = 0;  // wrap around to the front of the buffer
    
//        Serial.print( "pop- s_segment_count: " );
//        Serial.print( s_segment_count );
//        Serial.print( ", s_segment_reader: " );
//        Serial.println( s_segment_reader );
    }
}


#pragma mark -

void move_left()
{
    // check to see if we are already moving in that direction
    if( snake_draw.dir_x == 1 && snake_draw.dir_y == 0 )
        return;
    
    // you can't move left if you are already moving right...
    if( snake_draw.dir_x == -1 && snake_draw.dir_y == 0 )
        return;

    snake_draw.dir_x = 1;
    snake_draw.dir_y = 0;
    add_segment();
}


void move_right()
{
    // check to see if we are already moving in that direction
    if( snake_draw.dir_x == -1 && snake_draw.dir_y == 0 )
        return;

    // you can't move right if you are already moving left...
    if( snake_draw.dir_x == 1 && snake_draw.dir_y == 0 )
        return;

    snake_draw.dir_x = -1;
    snake_draw.dir_y = 0;
    add_segment();
}


void move_up()
{
    // check to see if we are already moving in that direction
    if( snake_draw.dir_x == 0 && snake_draw.dir_y == 1 )
        return;

    // you can't move up if you are already moving down...
    if( snake_draw.dir_x == 0 && snake_draw.dir_y == -1 )
        return;

    snake_draw.dir_x = 0;
    snake_draw.dir_y = 1;
    add_segment();
}


void move_down()
{
    // check to see if we are already moving in that direction
    if( snake_draw.dir_x == 0 && snake_draw.dir_y == -1 )
        return;

    // you can't move down if you are already moving up...
    if( snake_draw.dir_x == 0 && snake_draw.dir_y == 1 )
        return;

    snake_draw.dir_x = 0;
    snake_draw.dir_y = -1;
    add_segment();
}



// EOF
