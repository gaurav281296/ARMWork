/*
 * main.c
 *
 *  Created on: 2012/10/08
 *      Author: sin
 */

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

#include <stm32f4xx.h>
#include <stm32f4xx_usart.h>

#include "stm32f4xx_it.h"

#include "armcore.h"
#include "gpio.h"
#include "delay.h"
#include "usart.h"
#include "I2CWire.h"

#include "ST7032i.h"
#include "USARTSerial.h"
#include "DS1307.h"

USARTSerial Serial6(USART6);
I2CWire Wire1(I2C1);
ST7032i lcd(Wire1);
DS1307 rtc(Wire1);

int main(void) {
//	uint16_t bits;
//	uint32_t intval = 40;
	uint32_t tnow;
	uint32 halfsec;
	uint32 millis_offset;
	uint32 oldtime;
	char tmp[128];
	uint16_t i = 0;
	
	TIM2_timer_start();

	Serial6.begin(PC7, PC6, 19200);

	const char message[] = 
			"This royal throne of kings, this scepter'd isle, \n"
			"This earth of majesty, this seat of Mars, \n"
			"This other Eden, demi-paradise, \n"
			"This fortress built by Nature for herself\n"
			"Against infection and the hand of war, \n"
			"This happy breed of men, this little world,\n" 
			"This precious stone set in the silver sea, \n"
			"Which serves it in the office of a wall, \n"
			"Or as a moat defensive to a house, \n"
			"Against the envy of less happier lands, \n"
			"This blessed plot, this earth, this realm, this England,";
	const uint16 messlen = strlen(message);
	
	Serial6.println(message);
	Serial6.flush();

	RCC_ClocksTypeDef RCC_Clocks;
	RCC_GetClocksFreq(&RCC_Clocks);

	Serial6.print( "SYSCLK = ");
	Serial6.print(RCC_Clocks.SYSCLK_Frequency);
	Serial6.print( ", HCLK = ");
	Serial6.print( RCC_Clocks.HCLK_Frequency);
	Serial6.print( ", PCLK1 = ");
	Serial6.print( RCC_Clocks.PCLK1_Frequency);
	Serial6.print( ", PCLK2 = ");
	Serial6.print(RCC_Clocks.PCLK2_Frequency);
	Serial6.println();
	Serial6.flush();

	GPIOMode(PinPort(PD12),
			(PinBit(PD12) | PinBit(PD13) | PinBit(PD14) | PinBit(PD15)), OUTPUT,
			FASTSPEED, PUSHPULL, NOPULL);

	//i2c_begin(&I2C1Buffer, I2C1, PB9, PB8, 100000);
	Wire1.begin(PB9, PB8, 100000);
	
//	lcd.init(&I2C1Buffer);
	lcd.begin();
	lcd.setContrast(46);
	lcd.print("Hello!");       // Classic Hello World

	tnow = millis();
	rtc.begin();
	rtc.updateTime();
	oldtime = rtc.time;
	while ( millis() > tnow + 1000 ) {
		millis_offset = millis();
		rtc.updateTime();
		if ( rtc.time != oldtime ) {
			oldtime = rtc.time;
			break;
		}
	}
	millis_offset %= 1000;
	halfsec = millis_offset / 500;
	
	while (1) {

		tnow = (millis() - millis_offset) % 1000;
		
		switch(tnow/100) {
			case 0:
			case 5:
				digitalWrite(PD13, HIGH);
				break;
			case 1:
				digitalWrite(PD13, LOW);
				digitalWrite(PD14, HIGH);
				break;
			case 2:
				digitalWrite(PD14, LOW);
				digitalWrite(PD15, HIGH);
				break;
			case 3:
				digitalWrite(PD15, LOW);
				digitalWrite(PD12, HIGH);
				break;
			case 4:
				digitalWrite(PD12, LOW);
				digitalWrite(PD13, HIGH);
				break;
			case 6:
				digitalWrite(PD13, LOW);
				digitalWrite(PD12, HIGH);
				break;
			case 7:
				digitalWrite(PD12, LOW);
				digitalWrite(PD15, HIGH);
				break;
			case 8:
				digitalWrite(PD15, LOW);
				digitalWrite(PD14, HIGH);
				break;
			case 9:
				digitalWrite(PD14, LOW);
				digitalWrite(PD13, HIGH);
				break;
		}
		//
		
		if ( halfsec != tnow / 500 ) {
			halfsec = tnow / 500;
			rtc.updateTime();
			if ( oldtime != rtc.time ) {
				oldtime = rtc.time;

				Serial6.print("Real Time Clock: ");
				Serial6.printByte((uint8)(rtc.time>>16));
				Serial6.print(':');
				Serial6.printByte((uint8)(rtc.time>>8));
				Serial6.print(':');
				Serial6.printByte((uint8)rtc.time);
				rtc.updateCalendar();
				Serial6.print(" ");
				Serial6.printByte( (uint32)rtc.cal+0x20000000 );
				Serial6.println();
			}
			//Serial6.print(tmp);
//			Serial6.print((float)millis()/500, 3);
//			Serial6.write((uint8_t *)message+((millis()/500)%(messlen-16)), 16);
			//Serial6.println();
		
			//		lcd.clear();
			lcd.setCursor(0, 0);
			lcd.write((uint8_t *)message+((millis()/500)%(messlen-16)), 16);
			if ( I2C1Buffer.flagstatus & 0x80000000 ) {
				Serial6.print(" I2C Status ");
				Serial6.println(I2C1Buffer.flagstatus, HEX);
			}
			lcd.setCursor(0, 1);
			lcd.printByte((uint8)(rtc.time>>16));
			lcd.print(':');
			lcd.printByte((uint8)(rtc.time>>8));
			lcd.print(':');
			lcd.printByte((uint8)rtc.time);
			lcd.print(' ');
			lcd.printByte((uint8)(rtc.cal>>8));
			lcd.print('/');
			lcd.printByte((uint8)rtc.cal);
		}

		char c;
		if (Serial6.available() > 0) {
			while (Serial6.available() > 0 && i < 92) {
				c = (char) Serial6.read();
				if ( c == '\n' || c == '\r' || c == ' ' ) {
					tmp[i] = 0;
					break;
				}
				tmp[i++] = c;
			}
			if ( strlen(tmp) and (c == 0x0d or c == 0x0a) ) {

				if (tmp[0] == 't') {
					rtc.time = strtol(tmp+1, 0, 16);
					rtc.setTime(rtc.time);
				} else if (tmp[0] == 'c') {
					rtc.cal = strtol(tmp+1, 0, 16);
					rtc.setCalendar(rtc.cal);
				}
				Serial6.print("> ");
				Serial6.print(rtc.time, HEX);
				Serial6.print(" ");
				Serial6.print(rtc.cal, HEX);
				Serial6.print("\n");
				i = 0;
			}
		}
		delay_ms(1);
	}
	return 0;
}

