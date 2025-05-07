// A - Enter date mode (MM/DD/YYY)
// B - Enter time mode
// C - Backspace
// D - Toggle military time
// * - Cancel edit and return to RUN (no changes)
// # - Confirm edit and return to RUN


#include "avr.h"
#include "lcd.h"

#define DDR    DDRB
#define PORT   PORTB
#define RS_PIN 0 //PB0
#define RW_PIN 1 //PB1
#define EN_PIN 2 //PB2

#include <stdbool.h>
#include <stdio.h>

static uint8_t show24 = 1;  // 1 = military, 0 = AM/PM
#define CHAR_TO_DIGIT(c) ((c) - '0') // keypad input char -> int


static inline void
set_data(unsigned char x)
{
	PORTD = x;
	DDRD = 0xff;
}

static inline unsigned char
get_data(void)
{
	DDRD = 0x00;
	return PIND;
}

static inline void
sleep_700ns(void)
{
	NOP();
	NOP();
	NOP();
}

static unsigned char
input(unsigned char rs)
{
	unsigned char d;
	if (rs) SET_BIT(PORT, RS_PIN); else CLR_BIT(PORT, RS_PIN);
	SET_BIT(PORT, RW_PIN);
	get_data();
	SET_BIT(PORT, EN_PIN);
	sleep_700ns();
	d = get_data();
	CLR_BIT(PORT, EN_PIN);
	return d;
}

static void
output(unsigned char d, unsigned char rs)
{
	if (rs) SET_BIT(PORT, RS_PIN); else CLR_BIT(PORT, RS_PIN);
	CLR_BIT(PORT, RW_PIN);
	set_data(d);
	SET_BIT(PORT, EN_PIN);
	sleep_700ns();
	CLR_BIT(PORT, EN_PIN);
}

static void
write(unsigned char c, unsigned char rs)
{
	while (input(0) & 0x80);
	output(c, rs);
}

void
lcd_init(void)
{
	SET_BIT(DDR, RS_PIN);
	SET_BIT(DDR, RW_PIN);
	SET_BIT(DDR, EN_PIN);
	avr_wait(16);
	output(0x30, 0);
	avr_wait(5);
	output(0x30, 0);
	avr_wait(1);
	write(0x3c, 0);
	write(0x0c, 0);
	write(0x06, 0);
	write(0x01, 0);
}

void
lcd_clr(void)
{
	write(0x01, 0);
}

void
lcd_pos(unsigned char r, unsigned char c)
{
	unsigned char n = r * 40 + c;
	write(0x02, 0);
	while (n--) {
		write(0x14, 0);
	}
}

void
lcd_put(char c)
{
	write(c, 1);
}

void
lcd_puts1(const char *s)
{
	char c;
	while ((c = pgm_read_byte(s++)) != 0) {
		write(c, 1);
	}
}

void
lcd_puts2(const char *s)
{
	char c;
	while ((c = *(s++)) != 0) {
		write(c, 1);
	}
}



/*** data structures and helpers ***/
typedef struct
{
	int year;
	uint8_t month, day; // 1–12, 1–31
	uint8_t hour, minute, second;   // 0–23, 0–59, 0–59
} DateTime;


static const char MAP[16] = {
	'1','2','3','A',
	'4','5','6','B',
	'7','8','9','C',
	'*','0','#','D'
};

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


void advance_dt(DateTime *dt)
{
	dt->second++;
	if (dt->second == 60)
	{
		dt->second = 0;
		dt->minute++;

		if (dt->minute == 60)
		{
			dt->minute = 0;
			dt->hour++;

			if (dt->hour == 24)
			{
				dt->hour = 0;
				dt->day++;

				if (dt->day > days_in_month(dt->month, dt->year))
				{
					dt->day = 1;
					dt->month++;

					if (dt->month > 12)
					{
						dt->month = 1;
						dt->year++;
					}
				}
			}
		}
	}
}


void normalize_datetime(DateTime* dt)
{
	// seconds -> minutes
	if (dt->second >= 60)
	{
		dt->minute += dt->second / 60;
		dt->second %= 60;
	}

	// minutes -> hours
	if (dt->minute >= 60)
	{
		dt->hour += dt->minute / 60;
		dt->minute %= 60;
	}

	// hours -> days
	if (dt->hour >= 24)
	{
		dt->day += dt->hour / 24;
		dt->hour %= 24;
	}

	// days -> months
	while (dt->day > days_in_month(dt->month, dt->year))
	{
		dt->day -= days_in_month(dt->month, dt->year);
		dt->month++;
		if (dt->month > 12)
		{
			dt->month = 1;
			dt->year++;
		}
	}
}



/*** keypad helpers ***/
int is_pressed(int r, int c)
{
	// set all 8 GPIOs to high impedance (N/C state)
	DDRC = 0x0F;     // PC0–PC3 = output (rows), PC4–PC7 = input (columns)
	PORTC = 0xF0;    // pull-up resistors ON for PC4–PC7

	// drive row r LOW, others HIGH
	PORTC = (PORTC & 0xF0) | ((~(1 << r)) & 0x0F);

	avr_wait(1);    // small delay

	// check if column c (PC4 to PC7) is LOW
	if (!(PINC & (1 << (c + 4))))
	{
		return 1;   // button (r, c) is pressed
	}

	return 0;   // not pressed
}



int keypad_get_key(void)
{
	lcd_clr();
	lcd_pos(0,0);
	lcd_puts2("Keypad...");
	avr_wait(500);
	lcd_clr();
	
	
	int i,j;
	for(i = 0; i < 4; ++i)
	{
		for(j = 0; j < 4; ++j)
		{
			if(is_pressed(i,j))
			{
				lcd_clr();
				lcd_pos(0,0);
				lcd_puts2("Being pressed...");
				avr_wait(500);
				lcd_clr();
				
				
				while (is_pressed(i, j))
				{
					lcd_clr();
					 // in case holding button
				}
				lcd_clr();
				lcd_pos(0,0);
				lcd_puts2("Returning press...");
				avr_wait(500);
				lcd_clr();
				return i * 4 + j;
			}
		}
	}
	lcd_clr();
	lcd_pos(0,0);
	lcd_puts2("Returning -1...");
	avr_wait(500);
	lcd_clr();
	
	return -1; // not pressed
}


void keypad_init(void)
{
	DDRC = 0x0F;   // rows output (PC0–3), Cols input (PC4–7)
	PORTC = 0xF0;  // pull-ups on PC4–7
}


/*** display current time/date ***/
void print_display(const DateTime *dt)
{
	char buf[17];
	lcd_clr(); //clear LCD

	// top row: MM/DD/YYYY
	sprintf(buf, "%02d/%02d/%04d", dt->month, dt->day, dt->year); // write it
	lcd_pos(0, 0);
	lcd_puts2(buf);

	// bottom row: HH:MM:SS
	if (show24) //military time ON
	{
		sprintf(buf, "%02d:%02d:%02d", dt->hour, dt->minute, dt->second); // write it
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


/*** set the date/time setting ***/
void enter_value(char *buf, int len) 
{
	int index = 0;
	while (index < len) 
	{
		int k = keypad_get_key();
		if (k < 0) continue;

		char key = MAP[k];
		if (key >= '0' && key <= '9') 
		{
			buf[index++] = key;
			} 
		else if (key == 'C' && index > 0) 
		{
			index--;
		} 
		else if (key == '*') 
		{
			return;  // cancel
		} 
		else if (key == '#' && index == len) 
		{
			break;  // confirm
		}
	}
}


void set_date(DateTime *dt)
{
	char buf[8] = {0};

	while (1) 
	{
		enter_value(buf, 8);

		uint8_t month = 10 * CHAR_TO_DIGIT(buf[0]) + CHAR_TO_DIGIT(buf[1]);
		uint8_t day = 10 * CHAR_TO_DIGIT(buf[2]) + CHAR_TO_DIGIT(buf[3]);
		int year =	1000 * CHAR_TO_DIGIT(buf[4]) +
					100 * CHAR_TO_DIGIT(buf[5]) +
					10  * CHAR_TO_DIGIT(buf[6]) +
					CHAR_TO_DIGIT(buf[7]);

		if (month >= 1 && month <= 12 && day >= 1 && day <= days_in_month(month, year)) 
		{
			dt->month = month;
			dt->day = day;
			dt->year = year;
			break; // valid input
		}
		else 
		{
			lcd_clr();
			lcd_puts2("Invalid date.");
			avr_wait(1000);
			lcd_clr();
			lcd_puts2("Try again:");
		}
	}
}

void set_time(DateTime *dt)
{
	char buf[6] = {0};

	while (1) 
	{
		enter_value(buf, 6);

		uint8_t hour   = 10 * CHAR_TO_DIGIT(buf[0]) + CHAR_TO_DIGIT(buf[1]);
		uint8_t minute = 10 * CHAR_TO_DIGIT(buf[2]) + CHAR_TO_DIGIT(buf[3]);
		uint8_t second = 10 * CHAR_TO_DIGIT(buf[4]) + CHAR_TO_DIGIT(buf[5]);

		if (hour < 24 && minute < 60 && second < 60) 
		{
			dt->hour = hour;
			dt->minute = minute;
			dt->second = second;
			break; // input is valid
		}
		else 
		{
			lcd_clr();
			lcd_puts2("Invalid time.");
			avr_wait(1000);
			lcd_clr();
			lcd_puts2("Try again:");
		}
	}
}




/*** demo only settings to make sure LCD works***/
int main(void)
{
	avr_init();
	lcd_init();
	keypad_init();
	
	lcd_clr();
	lcd_pos(0,0);
	lcd_puts2("Initializing...");
	avr_wait(500);
	lcd_clr();
	

	// preset example date/time
	DateTime now = {2025, 5, 7, 12, 0, 0};
	print_display(&now);
		
	while (1)
	{
		lcd_clr();
		lcd_pos(0,0);
		lcd_puts2("In loop...");
		avr_wait(500);
		lcd_clr();
		
		int k = keypad_get_key();
		
		lcd_clr();
		lcd_pos(0,0);
		lcd_puts2("Out loop...");
		avr_wait(500);
		lcd_clr();
		
		if (k >= 0) 
		{
			lcd_clr();
			lcd_pos(0,0);
			lcd_puts2("Keypad...");
			avr_wait(500);
			lcd_clr();
		}
		else
		{
			lcd_clr();
			lcd_pos(0,0);
			lcd_puts2("No keypad...");
			avr_wait(500);
			lcd_clr();
			
			// no key pressed run clock
			avr_wait(1000);           // wait 1 second
			advance_dt(&now);         // increment time
			print_display(&now);      // refresh display
		}
	}
}
