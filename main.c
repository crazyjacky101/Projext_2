
//A- edit Date
//B- edit time
//* - back/reset (if in edit date/time mode resets it to 00/00/0000 or 00:00:00)
//#- conform (takes back to 'clock')
//D- AM/Pm -> Military (vice versa)


#include "avr.h"
#include "lcd.h"
#include <stdint.h>
 
 
#define DDR	DDRB
#define PORT	PORTB
#define RS_PIN 0
#define RW_PIN 1
#define EN_PIN 2


static const char MAP[4][4] = {
    {"1","2","3","A"},
    {"4","5","6","B"},
    {"7","8","9","C"},
    {"*","0","#","D"}
}; 


void keypad_init(void)
{
    DDRC = 0;	//PC0-3 outputs (rows)
    PORTC = 0;	//PC4-7 inputs w/pull-ups (columns)
}


typedef struct {
    int year;	//2000-2099
    unsigned char month; //1-12
    unsigned char day; //1-31
    unsigned char hour; //1-12 OR 0-24
    unsigned char minute; //1-59
    unsigned char second; //1-59
} DateTime;


bool is_leap(int y) 
{
    return (y % 4  == 0 && y % 100 != 0) || (y % 400 == 0);
}


static uint8_t days_in_month(uint8_t m, int y) 
{
    static const uint8_t MONTH_DAYS[12] = {
        31, /* Jan */
        28, /* Feb */
        31, /* Mar */
        30, /* Apr */
        31, /* May */
        30, /* Jun */
        31, /* Jul */
        31, /* Aug */
        30, /* Sep */
        31, /* Oct */
        30, /* Nov */
        31  /* Dec */
    };

    if (m == 2 && is_leap(y))	// feb in a leap year
        return 29;
    return MONTH_DAYS[m - 1];
}


int get_key()
{
    
    
}


int is_pressed()
{
    
}


int main(void)
{
    avr_init();
    lcd_init();
    keypad_init();
    
    
}
