/*
 * Project.c
 *
 * Created: 3/11/2018 2:00:55 PM
 * Author : harsh
 */ 
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

void startGame();
int updateTime = 1500;
volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.
unsigned char bluePins = 0x00;
unsigned char newBlue = 0x00;
unsigned char newGreen = 0x00;
unsigned char greenPins[8] = {255};
unsigned char redPins[8] = {255};

enum losers{on, off} loser;

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void updateShiftBlue(unsigned char update);
void updateShiftGreen(unsigned char update);
void updateShiftRed(unsigned char update);

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;    // Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

enum COLORS{red, green, blue, none} previous;

void startMenu(){
	bluePins = 0;
	unsigned i = rand() % 4;
	unsigned j = rand() % 4 + i;
	for (unsigned k = 0; k <= i; ++k){
		greenPins[k] = 0;
	}
	for (unsigned k = 0; k <= j; ++k){
		redPins[k] = 0;
	}
}

void updateShiftRed(unsigned char update){
	unsigned char b = 0x00;
	if (update == 255){
		PORTB = 0x00;
		for (int i = 0; i < 8; ++i){
			b = 0x09;
			PORTB = b;
			PORTB = 0x0C;
		}
		PORTB = 0x0A;
		return;
	}
	if (previous == blue){
		updateShiftBlue(255);
	}
	else if (previous == green){
		updateShiftGreen(255);
	}
	previous = red;
	PORTB = 0x00;
	for (int i = 0; i < 8; ++i){
		b = update & 0x01;
		b = b | 0x08;
		PORTB = b;
		PORTB = 0x0C;
		update = (update >> 1);
	}
	PORTB = 0x0A;
}

void updateShiftBlue(unsigned char update){
	PORTA = 0x01;
	unsigned char b = 0x00;
	if (update == 255){
		PORTA = 0x00;
		for (int i = 0; i < 8; ++i){
			b = 0x09;
			PORTA = b;
			PORTA = 0x0C;
		}
		PORTA = 0x0A;
		return;
	}
	if (previous == green){
		updateShiftGreen(255);
	}
	else if (previous == red){
		updateShiftRed(255);
	}
	previous = blue;
	PORTA = 0x00;
	for (int i = 0; i < 8; ++i){
		b = update & 0x01;
		b = b | 0x08;
		PORTA = b;
		PORTA = 0x0C;
		update = (update >> 1);
	}
	PORTA = 0x0A;
}

void updateShiftGreen(unsigned char update){
	unsigned char b = 0x00;
	if (update == 255){
		PORTC = 0x00;
		for (int i = 0; i < 8; ++i){
			b = 0x09;
			PORTC = b;
			PORTC = 0x0C;
		}
		PORTC = 0x0A;
		return;
	}
	if (previous == blue){
		updateShiftBlue(255);
	}
	else if (previous == red){
		updateShiftRed(255);
	}
	previous = green;
	PORTC = 0x00;
	for (int i = 0; i < 8; ++i){
		b = update & 0x01;
		b = b | 0x08;
		PORTC = b;
		PORTC = 0x0C;
		update = (update >> 1);
	}
	PORTC = 0x0A;
}


void updateGame(){ //will update locations and locate any collisions
	unsigned char points = 0;
	for (unsigned i = 0; i < 8; ++i){
		unsigned char g = ~greenPins[i];
		unsigned char r = ~redPins[i];
		if ((r & 0x80) == 0x80){
			loser = on;
			return;
		}
		else if ((r & g)){
			unsigned char temp = r;
			r = r & ~g;
			g = g & ~temp;
			points = points + 1;
		}
		g = (g >> 1);
		if ((r & g)){
			unsigned char temp = r;
			r = r & ~g;
			g = g & ~temp;
			points = points + 1;
		}
		r = (r << 1);
		redPins[i] = ~r;
		greenPins[i] = ~g;
	}
	PORTC = (points << 3) | 0x0A;
	while(!TimerFlag);
	TimerFlag = 0;
	PORTC = 0x0A;
}

void spawnEnemy(){
	unsigned char j = rand() % 8;
	unsigned char k = j;
	for (j = j; j <= k; ++j){
		unsigned char row = ~redPins[j];
		if ((row & 0x02) != 0x02){
			row = row | 0x01;
			redPins[j] = ~row;
		}
	}
	
}

void spawnEnemy2(){
	unsigned char j = rand() % 7;
	unsigned char k = j + 1;
	for (j = j; j <= k; ++j){
		unsigned char row = ~redPins[j];
		if ((row & 0x02) != 0x02){
			row = row | 0x01;
			redPins[j] = ~row;
		}
	}
	
}

void updatePlayer(){
	if (newBlue != 0){
		bluePins = newBlue;
		newBlue = 0;
	}
}

void updateShot(){
	if (newGreen == 0){
		return;
	}
	unsigned char row = ~bluePins;
	if (row == 0x01){
		row = ~greenPins[0];
		if (row == 255){
			row = 0;
		}
		row = row | 0x40;
		greenPins[0] = ~row;
	}
	else if (row == 0x02){
		row = ~greenPins[1];
		if (row == 255){
			row = 0;
		}
		row = row | 0x40;
		greenPins[1] = ~row;
	}
	else if (row == 0x04){
		row = ~greenPins[2];
		if (row == 255){
			row = 0;
		}
		row = row | 0x40;
		greenPins[2] = ~row;
	}
	else if (row == 0x08){
		row = ~greenPins[3];
		if (row == 255){
			row = 0;
		}
		row = row | 0x40;
		greenPins[3] = ~row;
	}
	else if (row == 0x10){
		row = ~greenPins[4];
		if (row == 255){
			row = 0;
		}
		row = row | 0x40;
		greenPins[4] = ~row;
	}
	else if (row == 0x20){
		row = ~greenPins[5];
		if (row == 255){
			row = 0;
		}
		row = row | 0x40;
		greenPins[5] = ~row;
	}
	else if (row == 0x40){
		row = ~greenPins[6];
		if (row == 255){
			row = 0;
		}
		row = row | 0x40;
		greenPins[6] = ~row;
	}
	else if (row == 0x80){
		row = ~greenPins[7];
		if (row == 255){
			row = 0;
		}
		row = row | 0x40;
		greenPins[7] = ~row;
	}

	newGreen = 0;
}

int matrixUpdate(){ //return the total amount of period used
	int period = 0;
	PORTD = 16;
	updateShiftBlue(bluePins);
	while(!TimerFlag);
	TimerFlag = 0;
	period = period + 2;

	for (unsigned i = 0; i < 8; ++i){
		unsigned char g = ~greenPins[i];
		unsigned char update = 0x01;
		if (g != 0){
			if ((g & 0x40) == 0x40){
				PORTD = 32;
				update = ~(update << i);
				updateShiftGreen(update);
				while(!TimerFlag);
				TimerFlag = 0;
				period = period + 2;
				update = 0x01;
			}
			if ((g & 0x20) == 0x20){
				PORTD = 64;
				update = ~(update << i);
				updateShiftGreen(update);
				while(!TimerFlag);
				TimerFlag = 0;
				period = period + 2;
				update = 0x01;
			}
			if ((g & 0x10) == 0x10){
				PORTD = 128;
				update = ~(update << i);
				updateShiftGreen(update);
				while(!TimerFlag);
				TimerFlag = 0;
				period = period + 2;
				update = 0x01;
			}
			if ((g & 0x08) == 0x08){
				PORTD = 1;
				update = ~(update << i);
				updateShiftGreen(update);
				while(!TimerFlag);
				TimerFlag = 0;
				period = period + 2;
				update = 0x01;
			}
			if ((g & 0x04) == 0x04){
				PORTD = 2;
				update = ~(update << i);
				updateShiftGreen(update);
				while(!TimerFlag);
				TimerFlag = 0;
				period = period + 2;
				update = 0x01;
			}
			if ((g & 0x02) == 0x02){
				PORTD = 4;
				update = ~(update << i);
				updateShiftGreen(update);
				while(!TimerFlag);
				TimerFlag = 0;
				period = period + 2;
				update = 0x01;
			}
			if ((g & 0x01) == 0x01){
				PORTD = 8;
				update = ~(update << i);
				updateShiftGreen(update);
				while(!TimerFlag);
				TimerFlag = 0;
				period = period + 2;
				update = 0x01;
			}
		}
	}

	for (unsigned i = 0; i < 8; ++i){
		unsigned char g = ~redPins[i];
		unsigned char update = 0x01;
		if (g != 0){
			if ((g & 0x40) == 0x40){
				PORTD = 32;
				update = ~(update << i);
				updateShiftRed(update);
				while(!TimerFlag);
				TimerFlag = 0;
				period = period + 2;
				update = 0x01;
			}
			if ((g & 0x20) == 0x20){
				PORTD = 64;
				update = ~(update << i);
				updateShiftRed(update);
				while(!TimerFlag);
				TimerFlag = 0;
				period = period + 2;
				update = 0x01;
			}
			if ((g & 0x10) == 0x10){
				PORTD = 128;
				update = ~(update << i);
				updateShiftRed(update);
				while(!TimerFlag);
				TimerFlag = 0;
				period = period + 2;
				update = 0x01;
			}
			if ((g & 0x08) == 0x08){
				PORTD = 1;
				update = ~(update << i);
				updateShiftRed(update);
				while(!TimerFlag);
				TimerFlag = 0;
				period = period + 2;
				update = 0x01;
			}
			if ((g & 0x04) == 0x04){
				PORTD = 2;
				update = ~(update << i);
				updateShiftRed(update);
				while(!TimerFlag);
				TimerFlag = 0;
				period = period + 2;
				update = 0x01;
			}
			if ((g & 0x02) == 0x02){
				PORTD = 4;
				update = ~(update << i);
				updateShiftRed(update);
				while(!TimerFlag);
				TimerFlag = 0;
				period = period + 2;
				update = 0x01;
			}
			if ((g & 0x01) == 0x01){
				PORTD = 8;
				update = ~(update << i);
				updateShiftRed(update);
				while(!TimerFlag);
				TimerFlag = 0;
				period = period + 2;
				update = 0x01;
			}
		}
	}


	return period;
}

void getUserInput(){
	PORTA = 0xEA;
	unsigned char input = 0;
	PORTA = 0xCA;
	for (unsigned i = 0; i < 8; ++i){
		PORTA = 0xCA;
		if ((~PINA & 0x40) == 0x40){
			input = input + 0x01;
			input = (input << i);
		}
		PORTA = 0xDA;
	}
	if (input == 0x08){
		startGame();
		return;
	}
	if (input == 0x01){
		newGreen = 1;
	}
	else{
		newGreen = 0;
	}
	if (input == 0x10){
		newBlue = ~bluePins;
		newBlue = (newBlue << 1);
		if (newBlue == 0){
			newBlue = 0x80;
		}
		newBlue = ~newBlue;
	}
	else if (input == 0x20){
		newBlue = ~bluePins;
		newBlue = (newBlue >> 1);
		if (newBlue == 0){
			newBlue = 0x01;
		}
		newBlue = ~newBlue;
	}
	else{
		newBlue = bluePins;
	}
}

void startGame(){
	bluePins = 0x7F;
	newBlue = 0x00;
	newGreen = 0x00;
	for(unsigned i = 0; i < 8; ++i){
		greenPins[i] = 255;
		redPins[i] = 255;
	}
	loser = off;
	previous = none;
	updateShiftGreen(255);
	updateShiftBlue(255);
	updateShiftRed(255);
	PORTC = 0xFA;
	updateTime = 1300;
}
//update Locations and look for collisions, if any ship passes through then game over
//add new ships or shots
//update matrix

int main(void) //game will update 
{
	DDRC = 0xFF;
	PORTC = 0x00;
	DDRB = 0xFF;
	PORTB = 0x00;
	DDRD = 0xFF;
	PORTD = 0xDD;
	DDRA = 0x3F;
	PORTA = 0xC0;
	srand(4);
	loser = off;
	previous = none;
	TimerSet(2);
	TimerOn();
	updateShiftGreen(255);
	updateShiftBlue(255);
	updateShiftRed(255);
	int periodOfGame = 0; //should be 1000 ms
	int inputPeriod = 0;
	int shotPeriod = 0;
    while (1) 
    {
		while (loser == on){
			getUserInput();
			startMenu();
			matrixUpdate();
		}
		if (periodOfGame >= updateTime){
			if (updateTime <= 1000){
				spawnEnemy2();
			}else{
				spawnEnemy();
			}
			updateGame();
			periodOfGame = 0;
			if (updateTime > 1000){
				updateTime = updateTime - 5;
			}
		}
		if (inputPeriod >= 100){
			getUserInput();
			updateShot();
			updatePlayer();
			inputPeriod = 0;
		}
		int updatePeriod = matrixUpdate();
		periodOfGame = periodOfGame + updatePeriod;
		inputPeriod = inputPeriod + updatePeriod;
		shotPeriod = shotPeriod + updatePeriod;
    }
}

