#define F_CPU 1000000UL
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>

#define BAUD 9600
#define UBRR_VAL (F_CPU / 16 / BAUD - 1)

volatile uint8_t display_mode = 0; //0 for minutes, 1 for seconds
volatile uint8_t blink_state = 1; //allows blinking, 1 for on, 0 for off
volatile uint8_t set_time = 0; //0 for normal operation, 1 for blinking mode
volatile uint8_t chosen_row = 0; //0 for hours, 1 for minutes, 2 for seconds
volatile uint8_t pwm_values[] = {0, 100, 255}; //levels for brightness
volatile uint8_t pwm_level = 0; //will be devided by %3

//random values
volatile uint8_t hours = 11;
volatile uint8_t minutes = 38;
volatile uint8_t seconds = 27; 

//functions for programm
void LEDSetup(void);
void pwmSetup(void);
void buttonSetup(void);
void update_display(void);
void normal_display_operation(void);
void setting_time_LEDs(void);
void timer1Setup(void);
void timer2Setup(void);
void enter_sleep_mode(void);
void calibrate_time(void);
void usartSetup(void);
void parse_time_command(uint8_t*);
void usart_send_byte(uint8_t);
void usart_send_time(void);

int main(void){

	LEDSetup();
	timer1Setup();
	timer2Setup();
	buttonSetup();
	usartSetup();
	sei();

	update_display(); //for debug purposes

	while (1) {
		
		asm("nop");
		enter_sleep_mode(); //saves power when nothing is to do
		_delay_ms(5); //delay for prell-check
	}
	return 0;
	
}

void LEDSetup() {
	DDRD|= (1<<PD5) | (1<<PD6) | (1<<PD7); //top three of hours
	DDRB|= (1<<PB0) | (1<<PB2); //bottom two of hours
	DDRC|= (1<<PC5) | (1<<PC4) | (1<<PC3) | (1<<PC2) | (1<<PC1) | (1<<PC0); //minutes or seconds
}

void buttonSetup() {
	//set input Pins for Buttons
	DDRD&= ~(1<<PD2) & ~(1<<PD3) & ~(1<<PD4); //Set PD2, PD3, PD4
	PORTD|= (1<<PD2) | (1<<PD3) | (1<<PD4); //turn on Pull-up

	//manage Interrupts
	EIMSK|= (1<<INT1) | (1<<INT0); //turned on interrupt INT1/0
	EICRA|= (1<<ISC11) | (1<<ISC01); //INT1/0 on falling edge
	PCICR|= (1<<PCIE2); //i
	PCMSK2|= (1<<PCINT20);//interrupt on PCINT20
}

//updates Display according to current mode
void update_display() {

	calibrate_time(); //enforces time-rules

	if (set_time == 0) {
		normal_display_operation();
	}
	else {
		setting_time_LEDs(); 
	}
}

void normal_display_operation() {

	//hours
	PORTD = (PORTD & ~0xE0) | (0xE0 & (hours<<3)); //PD7, PD6, PD5
	PORTB = (PORTB & ~0x05) | (0x01 & hours) | (0x04 & (hours<<1));

	if (display_mode == 0) {
		PORTC = (PORTC & ~0x3F) | (0x3F & minutes);
	}
	else {
		PORTC = (PORTC &  ~0x3F) | (0x3f & seconds);
	}
}

//selected row when set_time==1 is supposed to blink
void setting_time_LEDs() {
	
	//bit mask for the diffrent Ports for easier access
	uint8_t bitMask_hoursD = (0xE0 & (hours<<3)); 
	uint8_t bitMask_hoursB = (0x01 & hours) | (0x04 & (hours<<1));
	uint8_t bitMask_minutes = (PORTC & ~0x3F) | (0x3F & minutes);
	uint8_t bitMask_seconds = (PORTC &  ~0x3F) | (0x3f & seconds);

    //non-chosen rows will be normaly displayed
    if (chosen_row != 0) {
        PORTD = bitMask_hoursD;
        PORTB = bitMask_hoursB;
    }
    
   //delteted chosen_row != 1 / 1= 2 because it will be set later

    //blink state is toggled
    blink_state ^= 1;

    if (chosen_row == 0) {
        if (blink_state) {
            PORTD = bitMask_hoursD; //turned on if blink state is one
            PORTB = bitMask_hoursB;
        } else {
            PORTD &= ~0xE0; //will be turned of if blink state is turned of
            PORTB &= ~0x05;
        }
    }
    if (chosen_row == 1) {
        if (blink_state) {
            PORTC = bitMask_minutes;
        } else {
            PORTC &= ~0x3F;
        }
    }
    if (chosen_row == 2) {
        if (blink_state) {
            PORTC = bitMask_seconds;
        } else {
            PORTC &= ~0x3F;
        }
    }
}

void calibrate_time() {
	
	if (seconds >= 60) {
		minutes++;
		seconds = 0;

		if (minutes >= 60) {
			hours++;
			minutes = 0;

			if (hours >= 24) {
				hours = 0;
			}
		}
	}
}

//change displayed row or increase time
ISR(INT0_vect) {
	_delay_ms(5); //wait for the bounce
	if (set_time == 0) {
		display_mode = (display_mode+1)%2; //changes displaymode
		update_display(); //updates display
		return; //exits function
	}
	if (set_time == 1) {
		//selects according to row
		if (chosen_row == 0) {
			hours++;
		}
		if (chosen_row == 1) {
			minutes++;
		}
		if (chosen_row == 2) {
			seconds++;
		}
		update_display(); //sets LEDs
		return; //exits
	}
}

ISR(INT1_vect) {
	_delay_ms(5); //wait for the bounce
	if (set_time == 0) {
		set_time = 1; //changes mode
		chosen_row = 0; //resets chosen blinking row to hours
		blink_state = 1; //resets blinking
		update_display(); //updates LEDs
		return; //exits
	}
	if (set_time == 1) {
		chosen_row = (chosen_row+1)%3; //changes selected row
		update_display(); //updates LEDs
		return; //exits
	}
}

//change pwm-level ofr exit time-set mode
ISR(PCINT2_vect) {
	if (PIND & (1<<PD4)) {
		return; //ignores Button releases
	}
	_delay_ms(5); //wait for the bounce
	if (set_time == 0) { //not in Time-Set-Mode
		pwm_level++;
		OCR1A = pwm_values[pwm_level%3]; //switches right-round 
		return; //exits after pwm is switched
	}
	if (set_time == 1) {
		set_time = 0; //exits Time-setting-Mode
		display_mode = 0; 
		update_display(); //updates Display accordingly 
		return; //exits after mode is switched
	}
}

//increases seconds every time timer2 overflows, calibrates time if seconds reach 60 and updates display
ISR(TIMER2_OVF_vect) {
	if (set_time == 1) {
		update_display(); //updates display to ensure blinking
		return; //exits without changing time
	}
	seconds++;
	update_display(); //just to be sure
}

void timer2Setup() {
    // Activate asynchronous mode with an external clock 
    ASSR |= (1 << AS2);

    // Set Timer2 to Normal mode
    TCCR2A = 0;                     	    // Normal mode
    TCCR2B = (1 << CS22) | (1 << CS20);     // Prescaler = 128

    // Enable Timer2 Overflow interrupt
    TIMSK2 = (1 << TOIE2);

    // Wait for asynchronous clock stabilization
    while (ASSR & ((1 << TCR2AUB) | (1 << TCR2BUB))) {
        // Wait until Timer2 registers are updated
    }
}

void timer1Setup() {

    DDRB|= (1<<PB1); //Ausgang für PWM

	//Registerbelegung für PWM
	TCCR1A|= (1<<COM1A1); //Clear-on Compare Match
	TCCR1A|= (1<<WGM10); //8-Bit fast PWM
	TCCR1B|= (1<<WGM12);
	TCCR1B|= (1<<CS11); //Prescaler 8

	OCR1A = pwm_values[pwm_level]; //sets brightness
}

void enter_sleep_mode() {
	if(OCR1A == 255) {
        set_sleep_mode(SLEEP_MODE_PWR_SAVE); // Set Power-Down mode --> avr/sleep.h
                             // Put the CPU to sleep
    } 
	else {
		set_sleep_mode(SLEEP_MODE_IDLE); // Set Idle mode --> avr/sleep.h
	}
    sleep_enable();                      // Enable sleep mode
    sleep_cpu();                         // Put the CPU to sleep	
    
    // CPU wakes up here after an interrupt
    sleep_disable(); // Disable sleep mode after wake-up
}

void usartSetup(void) {
    
    UBRR0H = (uint8_t)(UBRR_VAL >> 8); //set rate
    UBRR0L = (uint8_t)(UBRR_VAL);

    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0); //activate receiver/transmitter and aktivate rx interrupt

    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // sets frame format: 8 bits, 1 stop bit
}

ISR(USART_RX_vect) {
    static uint8_t buf[12];
    static uint8_t idx = 0;
    
    uint8_t c = UDR0; //reads input

    if (c == '\n' || idx >= 11) {
        buf[idx] = '\0';
        parse_time_command(buf);
        idx = 0;
    } else {
        buf[idx++] = c;
    }
}

void parse_time_command(uint8_t* buf) {
    
	//expectet format: "H08M30S00"
    if (buf[0] == 'H' && buf[3] == 'M' && buf[6] == 'S') {
        uint8_t h = (buf[1] - '0') * 10 + (buf[2] - '0');
        uint8_t m = (buf[4] - '0') * 10 + (buf[5] - '0');
        uint8_t s = (buf[7] - '0') * 10 + (buf[8] - '0');

        // checks plausability
        if (h < 24 && m < 60 && s < 60) {
            hours   = h;
            minutes = m;
            seconds = s;
            update_display();
        }
		return;
    }

	 if (buf[0] == 'R' && buf[1] == '?') {
        usart_send_time();
        return;
    }
}

//sends byte and waits for buffer to clear
void usart_send_byte(uint8_t data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

//sends two digit number as ASCII
static void send_two_digits(uint8_t val) {
    usart_send_byte('0' + (val / 10));
    usart_send_byte('0' + (val % 10));
}

//sends time in format Th:min:s
void usart_send_time(void) {
    
	usart_send_byte('T');
    usart_send_byte(' ');
    send_two_digits(hours);
    usart_send_byte(':');
    send_two_digits(minutes);
    usart_send_byte(':');
    send_two_digits(seconds);
    usart_send_byte('\n');
}

