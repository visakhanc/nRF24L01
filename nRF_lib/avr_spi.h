/*****************************************
 *
 * 	spi.h
 *
 *  SPI polled driver for AVR
 *
 ************************************************/
 
 #include <stdint.h>
 
/* SPI Chip select pin */
#define 	SS_DDR		DDRB
#define 	SS_PORT		PORTB
#define 	SS_PIN		2		

typedef enum spi_mode
{
	SPI_MODE0,
	SPI_MODE1,
	SPI_MODE2,
	SPI_MODE3
} SPI_MODE_t;


typedef enum spi_clk_div 
{
	SPI_CLKDIV_4,
	SPI_CLKDIV_16,
	SPI_CLKDIV_64,
	SPI_CLKDIV_128,
	SPI_CLKDIV_2,
	SPI_CLKDIV_8,
	SPI_CLKDIV_32
} SPI_CLKDIV_t;


#define SS_HIGH()		(SS_PORT |= (1 << SS_PIN))
#define SS_LOW()		(SS_PORT &= ~(1 << SS_PIN))

	
/* Prototypes */

void SPI_Init(SPI_MODE_t mode, SPI_CLKDIV_t clk_div);
uint8_t SPI_TxRx(uint8_t data);
void SPI_TxBuf(uint8_t *buf, uint16_t length);
void SPI_RxBuf(uint8_t *buf, uint16_t length);
