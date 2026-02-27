#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <avr/delay.h>
#include <avr/wdt.h>

#define STATE_COOLING 0
#define STATE_DEFROST_ACTIVE 1
#define STATE_DEFROST_PASSIVE  2

// timer constants
#define DEFROST_ACTIVE_TIME  40*60    // 40 minutes active defrost cycle
#define DEFROST_PASSIVE_TIME 2*60    // 2 minutes 'passive' defrost (when defrost thermostat opened)
#define COOLING_TIME 8*60*60           // 8 hours cooling cycle
#define SAVE_TIME 57	     // interval to save last value to EEPROM

// EEPROM storage for the latest value of timer when main thermostat is in 'off' position
uint16_t EEMEM timerSave[2];


volatile uint16_t timer = COOLING_TIME;
volatile uint8_t state = STATE_COOLING;
volatile uint8_t timerSaveTimer = 0;

// timer to check defrost thermostat state
volatile uint8_t tempSensor = 0;

// when defrost thermostat is closed tempSensor goes to 0
ISR(PCINT0_vect)
{
	if (!(PINB && _BV(2)))
	return;
	if (tempSensor>0)
	{
		tempSensor--;
	}
	else
	{
		if (state==STATE_DEFROST_ACTIVE)
		// Switch mode to 'passive' when defrost started and defrost thermostat is opened for 5+ seconds
		{
			state = STATE_DEFROST_PASSIVE;
			timer = DEFROST_PASSIVE_TIME;
			GIMSK &= ~(1<<PCIE);
		}
	}
}

// Button 'test' pressed
ISR(INT0_vect)
{
	
	
	timer = 5;

	
	
}

// Timer 0 interrupt. Interval 1 second
ISR(TIM0_OVF_vect)
{
	if (timerSaveTimer>0)
	timerSaveTimer--;
	
	if (timer>0)
	{
		timer--;
	}
	else
	{
		// Switch mode cooling/defrost when timer==0
		if (state==STATE_COOLING)
		{
			state = STATE_DEFROST_ACTIVE;
			timer = DEFROST_ACTIVE_TIME;
			// turn on interrupt by defrost thermostat
			tempSensor = 255;
			cli();
			GIMSK |= (1<<PCIE);

			eeprom_update_word(&timerSave[0], COOLING_TIME);
			eeprom_update_word(&timerSave[1], COOLING_TIME);
			sei();
			PORTB |= _BV(0);
			
		}
		else
		if (state==STATE_DEFROST_ACTIVE)
		{
			state = STATE_DEFROST_PASSIVE;
			timer = DEFROST_PASSIVE_TIME;
			// turn on interrupt by defrost thermostat
			GIMSK &= ~(1<<PCIE);
			
		}
		else
		{
			PORTB &= ~(_BV(0));
			state = STATE_COOLING;
			timer = COOLING_TIME;
			GIMSK &= ~(1<<PCIE);
		}
	}
	
}


inline void Init()
{
	// Input/Output Ports initialization
	// Port B initialization
	// Function: Bit5=In Bit4=Out Bit3=Out Bit2=In Bit1=In Bit0=Out
	DDRB=(0<<DDB5) | (1<<DDB4) | (1<<DDB3) | (0<<DDB2) | (0<<DDB1) | (1<<DDB0);
	// State: Bit5=T Bit4=0 Bit3=T Bit2=T Bit1=P Bit0=0
	PORTB=(0<<PORTB5) | (0<<PORTB4) | (0<<PORTB3) | (0<<PORTB2) | (1<<PORTB1) | (0<<PORTB0);

	//   PORTB.0 - Output to mosfet+relay. 0 == compressor, 1 == defrost heater
	//   PORTB.1 - Input. 'Test' button. 0 == pressed
	//   PORTB.2 - Input. Defrost thermostat. 1 == -20<t<-8, 0 == t>+10
	//   PORTB.3 - Output. Active defrost LED. 1 == active
	//   PORTB.4 - Output. Passive defrost LED. 1 == active
	
	// Timer/Counter 0 initialization
	// Clock source: System Clock
	// Clock value: 0,125 kHz
	// Mode: Fast PWM top=OCR0A
	// OC0A output: Disconnected
	// OC0B output: Disconnected
	// Timer Period: 1 s
	TCCR0A=(0<<COM0A1) | (0<<COM0A0) | (0<<COM0B1) | (0<<COM0B0) | (1<<WGM01) | (1<<WGM00);
	TCCR0B=(1<<WGM02) | (1<<CS02) | (0<<CS01) | (1<<CS00);
	TCNT0=0x00;
	OCR0A=0x7C;
	OCR0B=0x00;

	// Timer/Counter 0 Interrupt(s) initialization
	TIMSK0=(0<<OCIE0B) | (0<<OCIE0A) | (1<<TOIE0);

	// External Interrupt(s) initialization
	// INT0: On
	// INT0 Mode: Low level
	// Interrupt on any change on pins PCINT0-5: On
	GIMSK=(1<<INT0);//| (1<<PCIE);
	MCUCR=(0<<ISC01) | (0<<ISC00);
	PCMSK=(0<<PCINT5) | (0<<PCINT4) | (0<<PCINT3) | (1<<PCINT2) | (0<<PCINT1) | (0<<PCINT0);
	GIFR=(1<<INTF0) | (1<<PCIF);

	// Analog Comparator initialization
	// Analog Comparator: Off
	// The Analog Comparator's positive input is
	// connected to the AIN0 pin
	// The Analog Comparator's negative input is
	// connected to the AIN1 pin
	//ACSR=(1<<ACD) | (0<<ACBG) | (0<<ACO) | (0<<ACI) | (0<<ACIE) | (0<<ACIS1) | (0<<ACIS0);
	//ADCSRB=(0<<ACME);
	// Digital input buffer on AIN0: On
	// Digital input buffer on AIN1: On
	//DIDR0=(0<<AIN0D) | (0<<AIN1D);

	// ADC initialization
	// ADC disabled
	//ADCSRA=(0<<ADEN) | (0<<ADSC) | (0<<ADATE) | (0<<ADIF) | (0<<ADIE) | (0<<ADPS2) | (0<<ADPS1) | (0<<ADPS0);


	uint16_t timer0 = eeprom_read_word(&timerSave[0]);
	uint16_t timer1 = eeprom_read_word(&timerSave[1]);
	//check two eeprom constants - should be equal

	if (timer1 == timer0)
	timer = timer0;

	//wrong value
	if (timer>COOLING_TIME)
	timer = COOLING_TIME;

	// if timer value <5 minutes ->  wait for compressor for 5 minutes before turn off
	if (timer<(2*SAVE_TIME))
	timer = COOLING_TIME;

	sei();
}

int main()
{
	Init();
	wdt_enable(WDTO_2S);
	while (1)
	{
		wdt_reset();
		
		uint8_t i;
		for (i=0;i<=state;i++)
		{
			PORTB |=  _BV(4);
			_delay_ms(40);
			PORTB &= ~( _BV(4));
			_delay_ms(100);
		}
		_delay_ms(300);
		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_mode();
		if ((timerSaveTimer==0) && (state == STATE_COOLING))
		{
			cli();
			eeprom_update_word(&timerSave[0], timer);
			eeprom_update_word(&timerSave[1], timer);
			
			timerSaveTimer = SAVE_TIME;
			sei();
		}
		
		//  wdt_reset();
	}
	return 0;
}
