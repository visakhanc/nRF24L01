/**
 * @file main.c
 * @brief NRF24L01+ transmit test
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


static uint8_t tx_buf[CONFIG_NRF_STATIC_PL_LENGTH];
static uint8_t addr[CONFIG_NRF_ADDR_LEN] = CONFIG_NRF_ADDRESS;

int main(void) 
{
	RED_LED_OUT();
	RED_LED_ON();
	nrf_init(NRF_MODE_PTX, addr);
	sei();
	
	_delay_ms(100);
	while(1) {
		RED_LED_ON();
		_delay_ms(50);
		RED_LED_OFF();
		tx_buf[0]++;
		if(nrf_transmit_packet(tx_buf, sizeof(tx_buf)) != 0) {
			_delay_ms(50);
			RED_LED_ON();
			_delay_ms(50);
			RED_LED_OFF();
		}
		_delay_ms(1000);
	}
}
