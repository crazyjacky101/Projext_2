// A - Enter date mode (MM/DD/YYY)
// B - Enter time mode
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


/*** set the date setting ***/
void set_date(const DateTime *dt)
{
	char buf[8];
	int pressed;
	for(int i = 0; i < 8; ++i)
	{
		while(1)
		{
			pressed = keypad_get_key();
			if(pressed)
			{
				break;
			}
		}
		buf[i] = pressed;
	}
	dt->month = {MAP[buf[0]], MAP[buf[1]]};
	dt->day = {MAP[buf[2]], MAP[buf[3]]};
	dt->year = {MAP[buf[4]], MAP[buf[5]], MAP[buf[6]], MAP[buf[7]]};
}


int is_pressed(int r, int c)
{
	DDRA = 0x00;
	PORTA = 0x00;

	SET_BIT(DDRA, r);
	CLR_BIT(PORTA, r);
	SET_BIT(PORTA, c + 4);

	avr_wait(5);
	if (GET_BIT(PINA, c + 4) == 0)
	{
		return 1;
	}
	return 0;
}


// Return the index (0–15) of the key pressed, or -1 if none
int keypad_get_key(void)
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			if (is_pressed(i, j))
			{
				avr_wait(20);
				if (is_pressed(i, j))
				{
					return i * 4 + j; // return index (0–15)
				}
			}
		}
	}
	return -1;
}



/*** set the date/time setting ***/
void enter_value(char *buf, int len, const char *format) 
{
	int index = 0;
	for (int i =0; i<len; i++) buf[i] = '0';
	buf[index] = '/0';
	

	while (index < len) 
	{
		
		int k = keypad_get_key();
		if (k <= 0) continue;

		char key = MAP[k];	
		if (key >= '0' && key <= '9') 
		{
			buf[index++] = key;
			buf[0] = '\0';
			
		} 
		else if (key == '*') 
		{
			for (int i =0; i<len; i++) buf[i] = '0';
			buf[len] = '\0'; // clear
			return;  // cancel
		} 
		else if (key == '#' && index == len) 
		{
			break;  // confirm
		}
		
		char display[17];
		int b = 0, d = 0;
		while (format[b] != '\0' && d < 16) 
		{
			if (format[b] == 'X') 
			{
				display[d++] = buf[index > d ? d : index];
				} 
			else 
			{
				display[d++] = format[b];
			}
			b++;
		}
		display[d] = '\0';
		
		
		lcd_clr();
		lcd_pos(0, 0);
		lcd_puts2("Enter: ");
		lcd_pos(1, 0);
		lcd_puts2(buf);
		avr_wait(200);
		
	}
}


void set_date(DateTime *dt)
{
	char buf[8] = {0};

	while (1) 
	{
		enter_value(buf, 8, "XX/XX/XXXX");

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
		enter_value(buf, 6, "XX:XX:XX");

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

	//example date/time
	DateTime now = {2025,4,30,12,0,0};
	print_display(&now);

	while (1)
	{
		int k = keypad_get_key();

		if      (k==4) { set_date(&now); print_display(&now); } //A - change date setting
		else if (k==8) { set_time(&now); print_display(&now); } //B - change time setting
		else if (k==16) { show24 = !show24; print_display(&now); } //D - military on/off
		else //no option chosen CLOCK MODE
		{
			avr_wait(1000); // wait 1 second
			advance_dt(&now); // increment second
			print_display(&now); // print new second
		}
	}
}