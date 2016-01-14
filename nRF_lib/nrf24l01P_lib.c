/*
 * 	nRF24L01P library
 *
 */

#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "avr_spi.h"
#include "nrf24l01P_config.h"

/* 
 * 	nRF			ATmega8
	-------------------
	SCK			SCK(PB5)
	MISO		MISO(PB4)
	MOSI		MOSI(PB3)
	CSN			SS#(PB2)
	CE			PB1
	IRQ			INT1(PD3)
	VCC			VCC(3.3V)
	GND			GND
 */

#define CSN_LOW()	SS_LOW()
#define CSN_HIGH()	SS_HIGH()

#define CE_OUT()	(CE_DDR |= (1 << CE_PIN))
#define CE_LOW()	(CE_PORT &= ~(1 << CE_PIN))
#define CE_HIGH()	(CE_PORT |= (1 << CE_PIN))

#define CE_PULSE() CE_HIGH(); \
	_delay_us(100); \
	CE_LOW();
	
/* Verify the configurations */
#if (CONFIG_NRF_ADDR_LEN < 3) || (CONFIG_NRF_ADDR_LEN > 5)
#error "Incorrect CONFIG_NRF_ADDR_LEN: can be only 3, 4, or 5 - modify in nrf24l01P_config.h"
#endif

#if !defined CONFIG_NRF_AUTOACK_ENABLED
#error "CONFIG_NRF_AUTOACK_ENABLED not defined - define in nrf24l01P_config.h"
#endif

#if (CONFIG_NRF_STATIC_PL_LENGTH > 32)
#error "CONFIG_NRF_STATIC_PL_LENGTH cannot be >32 - modify in nrf24l01P_config.h"
#endif

#if (!CONFIG_NRF_AUTOACK_ENABLED) && (CONFIG_NRF_DYNAMIC_PL_ENABLED)
#error "CONFIG_NRF_AUTOACK_ENABLED should be defined to 1 for dynamic payload length"
#endif

#if !defined CONFIG_NRF_DATA_RATE
#error "CONFIG_NRF_DATA_RATE not defined - define in nrf24l01P_config.h"
#endif

#if !defined CONFIG_NRF_TX_PWR
#error "CONFIG_NRF_TX_PWR not defined - define in nrf24l01P_config.h"
#endif

#if !defined CONFIG_NRF_RF_CHANNEL
#error "CONFIG_NRF_RF_CHANNEL not defined - define in nrf24l01P_config.h"
#endif

#if (CONFIG_NRF_AUTOACK_ENABLED)
	#if ((!defined CONFIG_NRF_TX_RETRANSMITS) || (CONFIG_NRF_TX_RETRANSMITS == 0))
		#error "CONFIG_NRF_TX_RETRANSMITS not properly defined - modify in nrf24l01P_config.h"
	#endif
#endif


static volatile uint8_t tx_done;
static volatile uint8_t rx_ready;
static volatile uint8_t max_retries;


static void mcu_init(void)
{
	CE_OUT();
	SPI_Init(SPI_MODE0, SPI_CLKDIV_4);
	
	/* configure INT1 as LOW-level triggered */
	MCUCR &= ~(1 << ISC11);
	MCUCR &= ~(1 << ISC10);
	
	/* Enable INT1 interrupt for  IRQ */
	GICR |= (1 << INT1);
}

/* Reads from a register */
static uint8_t nrf_read_reg(uint8_t reg)
{
	uint8_t value;
	
	CSN_LOW(); 
	SPI_TxRx(reg);	/* Transmit register to read */
	value = SPI_TxRx(0);  /* Then get the register value */
	CSN_HIGH();
	
	return reg;
}

/* Writes to a register */
static uint8_t nrf_write_reg(uint8_t reg, uint8_t val)
{
	uint8_t status;
	
	CSN_LOW();
	if(reg < WRITE_REG) {  /* write register with data */
		status = SPI_TxRx(WRITE_REG|reg);  
		SPI_TxRx(val);
	}
	else { /* command with no data */
		status = SPI_TxRx(reg);
	}
	CSN_HIGH();
	
	return status;
}

static inline uint8_t nrf_nop(void)
{
	return nrf_write_reg(NOP, 0);
}

static inline uint8_t nrf_get_rx_pipe(void)
{
	return ((nrf_nop() >> 1) & 0x7);
}

static inline uint8_t nrf_get_address_width(void)
{
	return (nrf_read_reg(SETUP_AW) + 2);
}


/* Reads Multibyte register 
 * return value: MSByte - Rx payload pipe number. LSByte - read length
 */
static uint16_t nrf_read_multibyte_reg(uint8_t reg, uint8_t *buf)
{
	uint8_t ctr = 0, length = 0;
	
	switch(reg)
	{
		case NRF_PIPE0:
		case NRF_PIPE1:
		case NRF_TX_PIPE:  /* Read pipe address */
			length = ctr = nrf_get_address_width();
			CSN_LOW();
			SPI_TxRx(RX_ADDR_P0 + reg);
			break;
      
		case NRF_RX_PLOAD:  /* Read Rx payload */
			if( (reg = nrf_get_rx_pipe()) < 7) {
				length = ctr = nrf_read_reg(RD_RX_PLOAD_W);
				CSN_LOW();
				SPI_TxRx(RD_RX_PLOAD);
			}
			break;

		default:
			break;
	}

	while(ctr--) {
		*buf++ = SPI_TxRx(0);
	}

	CSN_HIGH();

	return (((uint16_t) reg << 8) | length);
}


/* Writes to Multibyte register 
 * 
 */
static void nrf_write_multibyte_reg(uint8_t reg, const uint8_t *buf, uint8_t length)
{
	switch(reg)
	{
		case NRF_PIPE0:
		case NRF_PIPE1:
		case NRF_TX_PIPE:  /* Write pipe address */
			length = nrf_get_address_width();
			CSN_LOW();
			SPI_TxRx(WRITE_REG + RX_ADDR_P0 + reg);
			break;
		case NRF_TX_PLOAD:  /* Write Tx payload */
			CSN_LOW();
			SPI_TxRx(WR_TX_PLOAD);
			break;
		default:
			break;
	}

	while(length--) {
		SPI_TxRx(*buf++);
	}

	CSN_HIGH();
}


/* Set address of Pipes 
 * Tx address, Rx Pipe0,Pipe1 address are 5 bytes long
 * Rx pipe2-5 are given 1 byte address
 */
static void nrf_set_address(nrf_pipe_t pipe, const uint8_t *addr)
{
  switch(pipe)
  {
    case NRF_TX_PIPE:
    case NRF_PIPE0:
    case NRF_PIPE1:
      nrf_write_multibyte_reg((uint8_t) pipe, addr, 0);
      break;

    case NRF_PIPE2:
    case NRF_PIPE3:
    case NRF_PIPE4:
    case NRF_PIPE5:
      nrf_write_reg(RX_ADDR_P0 + (uint8_t) pipe, *addr);
      break;

    default:
      break;
  }
}



/*
 *  Initialize NRF24L01+ radio
 *		Set address
 *		Enable pipes (with/without auto ack)
 * 		Set data rate, power, payload length and other things
 *		Set mode to Tx/Rx
 */
void nrf_init(nrf_opmode_t mode, const uint8_t *address)
{
	uint8_t 	reg_val;
	
	/* low level initialization */
	mcu_init();
	
	/* Set Address */
	nrf_write_reg(SETUP_AW, (uint8_t)(CONFIG_NRF_ADDR_LEN-2));  /* Address width */
	nrf_set_address(NRF_TX_PIPE, address);  /* Set same address for Tx and Pipe0, for auto ack */
	nrf_set_address(NRF_PIPE0, address);
	
	/* Open channels */
	reg_val = (1 << 0); /* open only Pipe0 */
	nrf_write_reg(EN_RXADDR, reg_val);
	if(CONFIG_NRF_AUTOACK_ENABLED) {
		reg_val = (1 << 0);  /* Enable auto ack (only for Pipe 0) */
		nrf_write_reg(EN_AA, reg_val);
	}
	
	/* Set payload length (only for Pipe 0)*/
	if(mode == NRF_MODE_PRX) {
		nrf_write_reg(RX_PW_P0, CONFIG_NRF_STATIC_PL_LENGTH);
	}
	
	/* FEATURE reg */
	reg_val = 0;
	if(CONFIG_NRF_DYNAMIC_PL_ENABLED) {
		reg_val = (1 << 2);
	}
	if(CONFIG_NRF_ACK_PL_ENABLED) {
		reg_val |= (1 << 1);
	}
	if(!CONFIG_NRF_AUTOACK_ENABLED) {
		reg_val |= (1 << 0);
	}
	nrf_write_reg(LOCK_UNLOCK,0);  
	nrf_write_reg(0x73,0);			/* Unlock FEATURE register */
	nrf_write_reg(FEATURE, reg_val);
	

	/* Retransmit reg */
	reg_val = 0;
	if(CONFIG_NRF_AUTOACK_ENABLED) {
		reg_val = (((CONFIG_NRF_RETRANS_DELAY/250)-1) << 4)|(CONFIG_NRF_TX_RETRANSMITS & 0xF);
	}
	nrf_write_reg(SETUP_RETR, reg_val);
	
	/* RF setup reg */
	reg_val = nrf_read_reg(RF_SETUP);
	reg_val &= ~(RF_DR_LOW|RF_DR_HIGH|RF_PWR1|RF_PWR0);
	if(CONFIG_NRF_DATA_RATE == NRF_RATE_250KBPS) {
		reg_val |= RF_DR_HIGH;
	} else if(CONFIG_NRF_DATA_RATE == NRF_RATE_2MBPS) {  
		reg_val |=  RF_DR_LOW;
	}
	reg_val |= (CONFIG_NRF_TX_PWR << 1);
	nrf_write_reg(RF_SETUP, reg_val);


	/* RF Channel reg*/
	nrf_write_reg(RF_CH, CONFIG_NRF_RF_CHANNEL);
	
	/* Config reg */
	reg_val = (CONFIG_EN_CRC|CONFIG_CRCO|CONFIG_PWR_UP);
	if(mode == NRF_MODE_PRX) {
		reg_val |= 1;
	}
	nrf_write_reg(CONFIG, reg_val);
	
	/* Set CE High for Rx mode */
	if(mode == NRF_MODE_PRX) {
		CE_HIGH();
	}
	
	_delay_ms(5);
}

/* Transmit a packet 
 *	Returns :
 *		0 - If successfully transmitted
 *		1 - Not successful (ACK not received)
 *		2 - Cannot transmit (Tx FIFO full)
 */
uint8_t nrf_transmit_packet(uint8_t *packet, uint8_t length)
{
	uint8_t 	ret;
	
	if(nrf_read_reg(FIFO_STATUS) & TX_FIFO_FULL) {
		return 2;
	}
	
	nrf_write_multibyte_reg(NRF_TX_PLOAD, packet, length);
	CE_PULSE();
	do {
		if(tx_done == true) {
			tx_done = false;
			ret = 0;
			break;
		}
		if(max_retries == true) {
			max_retries = false;
			ret = 1;
			break;
		}
	} while (1);
	
	return ret;
}

/* 
 *  Receive a packet if available
 *	Returns: 16 bit
 *		MSB byte: pipe number
 *		LSB byte: length of received packet
 */
uint8_t nrf_receive_packet(uint8_t *buf, uint8_t *length)
{
	uint8_t rx_pipe = 0;
	uint16_t ret;
	
	*length = 0;
	if(rx_ready == true) {
		rx_ready = false;
		do {
			ret = nrf_read_multibyte_reg(NRF_RX_PLOAD, buf);
		} while (!(nrf_read_reg(FIFO_STATUS) & RX_EMPTY));
		*length = (uint8_t)ret;
		rx_pipe = (uint8_t)(ret >> 8);
	}
	return rx_pipe;
}


/* 
 * 	Write ACK payload for a pipe
 *		
 */
void nrf_set_ack_payload(uint8_t pipe, uint8_t *buf, uint8_t length)
{
	CSN_LOW();
	SPI_TxRx(WR_ACK_PLOAD|pipe);
	while(length--) {
		SPI_TxRx(*buf++);
	}
	CSN_HIGH();
}


/* ISR for  IRQ */
ISR(INT1_vect)
{
	uint8_t status;
	
	status = nrf_write_reg(STATUS, (STAT_MAX_RT|STAT_TX_DS|STAT_RX_DR)); /* Get and clear interrupt flags */
	if(status & STAT_RX_DR)
	{
		/* received data */
		rx_ready = true;
	}
	
	if(status & STAT_TX_DS)
	{
		/* transmit done */
		tx_done = true;
	}
	
	if(status & STAT_MAX_RT)
	{
		/* Maxtimum reties exceeded */
		max_retries = true;
	}
}



