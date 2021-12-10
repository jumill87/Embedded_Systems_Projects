// R2R.c
// Runs on LM4F120 or TM4C123
// Use SysTick interrupts to implement a single 440Hz sine wave
// Daniel Valvano, Jonathan Valvano
// September 15, 2013

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2013
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013

 Copyright 2013 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// Port B bits 2-0 have the 3-bit DAC
// Port F is onboard LaunchPad switches and LED
// Port F bit 4 is negative logic switch to play sound, SW1
// SysTick ISR: PF3 ISR heartbeat


#include "tm4c123gh6pm.h"
#include "Sound.h"
#include "SwitchLed.h"
#include "PLL.h"

struct Note {
	unsigned char tone_index;
	unsigned char delay;
};
typedef const struct Note NTyp;
// initial values for piano major tones.
// Assume SysTick clock frequency is 50MHz.
const unsigned long Tone_Tab[] =
// Notes:  C, D, E, F, G, A, B
// Offset: 0, 1, 2, 3, 4, 5, 6
{ 47778, 42565, 37921, 35793, 31888, 28409, 25309, // C5 Major
  23889, 21282, 18960, 17896, 15944, 14204, 12654, // C6 Major
  11944, 10641, 9480, 8948, 7972, 7102, 6327 };// C7 Major

// indexes for notes used in music scores
#define C4 95556
#define D4 85130
#define E4 75842
#define F4 71586
#define G4 64776
#define A4 56818
#define B4 50618
	
#define C5 0+7
#define D5 1+7
#define E5 2+7
#define F5 3+7
#define G5 4+7
#define A5 5+7
#define B5 6+7
#define C6 0+2*7
#define D6 1+2*7
#define E6 2+2*7
#define F6 3+2*7
#define G6 4+2*7
#define A6 5+2*7
#define B6 6+2*7
//Number of samples for sine wave / 2
#define SAMPLES 32

struct Note littleLamb[] =
	// score table for Mary Had A Little Lamb
{ E6, 4, D6, 4, C6, 4, D6, 4, E6, 4, E6, 4, E6, 8,
 D6, 4, D6, 4, D6, 8, E6, 4, G6, 4, G6, 8,
 E6, 4, D6, 4, C6, 4, D6, 4, E6, 4, E6, 4, E6, 8,
 D6, 4, D6, 4, E6, 4, D6, 4, C6, 8, 0, 0 };

// basic functions defined at end of startup.s
extern void DisableInterrupts(void); // Disable interrupts
extern void EnableInterrupts(void);  // Enable interrupts
extern void WaitForInterrupt(void);  // low power mode
 
void play_a_song(NTyp scoretab[]);
void Delay(void);

unsigned int debounceCounter;//Counter for debounce
unsigned long playFlag;//flag that signals playing 1 is on, 0 is off
unsigned long modeFlag;//flag for either 0 is piano mode or  1 is song mode
unsigned long input;//just pressed flag
unsigned long previous;//just released flag
unsigned long note;//note played

// need to generate a 100 Hz sine wave
// table size is 64, so need 100Hz*64=6.4 kHz interrupt
// bus is 80MHz, so SysTick period is 80000kHz/6.4kHz = 12500
int main(void){    
  DisableInterrupts();
	PLL_Init();	
  SwitchLed_Init();       // Port F is onboard switches, LEDs, profiling
	PORT_F_LED_Init();
	SwitchC_Init();
	SwitchD_Init();
  DAC_Init();          // Port B0-5 is DAC
	EnableInterrupts();
	playFlag = 0;//piano inits off
	modeFlag = 0;// piano starts in piano mode
 
  while(1){     
		if(playFlag == 1){//piano is on
				GPIO_PORTF_DATA_R = 0x08; //green LED for piano on
			if(modeFlag == 0){//piano mode
				
				if(note !=0){
				GPIO_PORTF_DATA_R = 0x04;	//blue for note being played
				Sound_Init(note/SAMPLES);
				note = 0;
				}
			}
				if(modeFlag == 1){
					play_a_song(littleLamb);
		
	}
		}
		else GPIO_PORTF_DATA_R = 0x02;//red LED for piano off
  }  

}

void GPIOPortF_Handler(void) { // called on touch of either SW1 or SW2
	if (GPIO_PORTF_RIS_R & 0x01) {  // SW2 touch
		GPIO_PORTF_ICR_R = 0x01;  // acknowledge flag0
		if (debounceCounter >= 1) {
			modeFlag = (modeFlag + 1) & 1; // changes to autoplay mode
			debounceCounter = 0;

		}
		else
			debounceCounter++;
	}
	if (GPIO_PORTF_RIS_R & 0x10) {  // SW1 touch
		GPIO_PORTF_ICR_R = 0x10;  // acknowledge flag4
		if (debounceCounter >= 1) {
			playFlag = (playFlag + 1) & 1;
			debounceCounter = 0;
		}
		else
			debounceCounter++;
	}
}
void GPIOPortC_Handler(void){
		if (GPIO_PORTC_RIS_R & 0x10) {  // PC4 triggered
			GPIO_PORTC_ICR_R = 0x10;  // acknowledge flag PC4
			Delay10ms();//debounce
			if(playFlag && (GPIO_PORTC_DATA_R & 0x10)) note = G4;
			else Sound_stop();
			
}
		if (GPIO_PORTC_RIS_R & 0x20) {  // PC5 triggered
			GPIO_PORTC_ICR_R = 0x20;  // acknowledge flag PC5
			Delay10ms();//debounce
			if(playFlag && (GPIO_PORTC_DATA_R & 0x20)) note = A4;
			else Sound_stop();
}
		if (GPIO_PORTC_RIS_R & 0x40) {  // PC6 triggered
			GPIO_PORTC_ICR_R = 0x40;  // acknowledge flag PC6
			Delay10ms();//debounce
			if(playFlag && (GPIO_PORTC_DATA_R & 0x40)) note = B4;
			else Sound_stop();
}
}

void GPIOPortD_Handler(void){
		if (GPIO_PORTD_RIS_R & 0x01) {  // PD0 triggered
			GPIO_PORTD_ICR_R = 0x01;  // acknowledge flag PD0
			Delay10ms();//debounce
			if(playFlag && (GPIO_PORTD_DATA_R & 0x01)) note = C4;
			else Sound_stop();
}
		if (GPIO_PORTD_RIS_R & 0x02) {  // PD1 triggered
			GPIO_PORTD_ICR_R = 0x02;  // acknowledge flag PD1
			Delay10ms();//debounce
			if(playFlag && (GPIO_PORTD_DATA_R & 0x02)) note = D4;
			else Sound_stop();
}
		if (GPIO_PORTD_RIS_R & 0x04) {  // PD2 triggered
			GPIO_PORTD_ICR_R = 0x04;  // acknowledge flag PD2
			Delay10ms();//debounce
			if(playFlag && (GPIO_PORTD_DATA_R & 0x04)) note = E4;
			else Sound_stop();
}
		 if (GPIO_PORTD_RIS_R & 0x08) {  // PD3 triggered
			GPIO_PORTD_ICR_R = 0x08;  // acknowledge flag PD3
			Delay10ms();//debounce
			if(playFlag && (GPIO_PORTD_DATA_R & 0x08)) note = F4;
			else Sound_stop();
}
}

void play_a_song(NTyp scoretab[])
{
	unsigned char i = 0, j;
	//unsigned char songPlaying = currentSong;
	while (scoretab[i].delay) {
		if(playFlag < 1 || modeFlag < 1)
			return;

		if (!scoretab[i].tone_index)
			Sound_stop(); // silence tone, turn off SysTick timer
		else {
			Sound_Init(Tone_Tab[scoretab[i].tone_index]/SAMPLES);
		}

		// tempo control: 
		// play current note for duration 
		// specified in the music score table
		for (j = 0; j < scoretab[i].delay; j++)
			Delay();

		Sound_stop();
		i++;  // move to the next note
	}

	// pause after each play
	for (j = 0; j < 15; j++)
		Delay();
}

void Delay(void) {
	unsigned long volatile time;
	time = 2272625 * 20 / 91;  // 0.1sec
	while (time) {
		time--;
	}
}
