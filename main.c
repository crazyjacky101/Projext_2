// A - Enter date mode (MM/DD/YYY)
// B - Enter time mode
// C - Backspace
// D - Toggle military time
// * - Cancel edit and return to RUN (no changes)
// # - Confirm edit and return to RUN

#include "avr.h"
#include "lcd.h"
#include <stdint.h>
 
 
#define DDR	DDRB
#define PORT	PORTB
#define RS_PIN 0
#define RW_PIN 1
#define EN_PIN 2
static uint8_t show24 = 1;  // 1 = military, 0 = AM/PM


/*** data structures and helpers ***/
typedef struct 
{
    int year;   
    uint8_t month, day; // 1–12, 1–31
    uint8_t hour, minute, second;   // 0–23, 0–59, 0–59
} DateTime;


static bool is_leap(int y) 
{
    return (y%4==0 && y%100!=0) || (y%400==0);
}


static uint8_t days_in_month(uint8_t m, int y) 
{
    static const uint8_t MONTH_DAYS[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m==2 && is_leap(y)) return 29;
    return MONTH_DAYS[m-1];
}


/*** keypad helpers ***/
static const char MAP[16] = 
{
    {"1","2","3","A","4","5","6","B","7","8","9","C","*","0","#","D"}
}; 


void keypad_init(void)
{
    DDRC = 0x0F;	//PC0-3 outputs, PC4-PC7 inputs
    PORTC = 0xF0;	//PC4-7 inputs w/pull-ups
}


int keypad_get_key(void)
{
    int i,j;
      for(i = 0; i < 4; ++i) 
      {
         for(j = 0; j < 4; ++j) 
         {
            if(is_pressed(i,j)) 
            {
                return i*4+j+1;
            }
         }
         return 0;
      }
}


/*** display current time/date ***/
void print_display(const DateTime *dt) 
{
    char buf[17];
    lcd_clr(); //clear LCD

    // top row: MM/DD/YYYY
    sprintf(buf, "%02d/%02d/%04d", dt->month, dt->day, dt->year); //write it

    // bottom row: HH:MM:SS
    if (show24) //military time ON
    {
        sprintf(buf, "%02d:%02d:%02d", dt->hour, dt->minute, dt->second); //write it
    }
    else //military time OFF have to state PM/AM
    {
        uint8_t h = dt->hour % 12;  //get time in 12 hour
        if (h == 0) h = 12;
        char ampm = (dt->hour < 12) ? 'A' : 'P';    //less than 12 = A(M)  more than 12 = P(M)
        sprintf(buf, "%02d:%02d:%02d %cM", h, dt->minute, dt->second, ampm); //write it
    }   
    lcd_pos(1,0); 
    lcd_puts2(buf);
}


/*** set the date setting ***/



/*** set the time setting ***/



/*** demo only settings to make sure LCD works***/
int main(void) 
{
    avr_init();
    lcd_init();
    keypad_init();

    //example date/time
    DateTime now = {2025,4,30,12,0,0};
    print_display(&now);

    while (1) 
    {
        char k = keypad_get_key();

        if      (k=='A') { set_date(&now); print_display(&now); } //A - change date setting
        else if (k=='B') { set_time(&now); print_display(&now); } //B - change time setting
        else if (k=='D') { show24 = !show24; print_display(&now); } //D - miltary on/off
        else 
        {
            avr_wait(1000); // only tick when not entering
            advance_dt(&now);
            print_display(&now);
        }
    }
}
