/**
 * @file main.c
 * @brief Receiver test for nRF24L01+
 *
 * @author 
 * @version 
 * @date Jan 8, 2015
 * @copyright
 */

#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include "nrf24l01.h"

#define RED_LED					PB0
#define RED_LED_DDR				DDRB
#define RED_LED_PORT			PORTB

#define RED_LED_OUT()			(RED_LED_DDR |= (1 << RED_LED))
#define RED_LED_ON()			(RED_LED_PORT |= (1 << RED_LED))
#define RED_LED_OFF()			(RED_LED_PORT &= ~(1 << RED_LED))
#define RED_LED_TOGGLE()		(RED_LED_PORT ^= (1 << RED_LED))

static uint8_t rx_buf[CONFIG_NRF_STATIC_PL_LENGTH];
static uint8_t addr[CONFIG_NRF_ADDR_LEN] = CONFIG_NRF_ADDRESS;

int main(void) 
{
	uint8_t length, old = 0;
	
	RED_LED_OUT();
	RED_LED_OFF();
	nrf_init(NRF_MODE_PRX, addr);
	sei();
	
	_delay_ms(100);
	while(1) {
		nrf_receive_packet(rx_buf, &length);
		if(length > 0) {
			if((old + 1) == rx_buf[0]) {
				RED_LED_ON();
				_delay_ms(50);
				RED_LED_OFF();
			}
			old = rx_buf[0];
		}
	}
}
