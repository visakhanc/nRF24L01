/* Here you define the configurations to initialize nRF24L01P module 
 *
 *	These are:
 *		MODE: Transmitter(PTX) or Receiver(PRX)
 *		Tx Address (For PTX, if auto ack is enabled, RX_ADDR_P0 is set to the same)
 *		Rx Addresses: P0, P1, (P2, P3, P4, P5 are set as increments from P1)
 *		Address width (5/4/3, should match with size of above Tx/Rx address)
 *		Tx power: 
 * 		Data rate:
 *		CRC mode:
 *		RF channel: eg 40
 *		Static Payload length :
 *		Dynamic payload
 *		Retransmit delay (250 if PL>18, else 500 us)
 *		Retransmit count eg 15
 *		TX_PAYLOAD_NOACK	
 *		
 */
#ifndef NRF24L01P_CONFIG_H
#define NRF24L01P_CONFIG_H 



/******** I/O PIN DEFINITIONS FOR AVR *********/

/* Define which AVR pin is connected to RFM70 Chip Enable (CE) */
#define CE_DDR		DDRB
#define CE_PORT		PORTB
#define CE_PIN   	1

/**********************************************/

 
 /* Address of the radio and Address length 
  * example: 
  *	#define NRF_ADDRESS		{0x11, 0x22, 0x33, 0x44, 0x55}
  *	#define NRF_ADDR_LEN	5
  *Note: Length can be 3, 4, 5
  *	  If Automatic ack is enabled and if mode is PTX, this address is used to set RX_P0 address
  */
#define CONFIG_NRF_ADDRESS		{0x11, 0x22, 0x33, 0x44, 0x55}
#define CONFIG_NRF_ADDR_LEN		5


/* Enable or Disable Automatic Retransmit/Acknowledgement feature
 * To enable define to 1, otherwise to 0
 * NOTE: In PTX/PRX, EN_AA=0 for No ack; In PRX/PTX, NO_ACK is checked in the packet
 */
 #define CONFIG_NRF_AUTOACK_ENABLED 	1
 

/* Whether to enable or disable dynamic payload width
 * Define to 1 to enable, 0 to disable
 */
#define CONFIG_NRF_DYNAMIC_PL_ENABLED		0


/* Length of the static payload
 * Define this from 0 to 32 (bytes)
 */
#define CONFIG_NRF_STATIC_PL_LENGTH			16

/* Whether to enable Payload in the ACK
   If defined to 1, also define ACK payload length */
#define CONFIG_NRF_ACK_PL_ENABLED			0
#define CONFIG_NRF_ACK_PL_LENGTH			0


/* Output power
 * Define this to:
 * 	NRF_PWR_0DBM : 0dBm
 * 	NRF_PWR_6DBM : -6dBm
 * 	NRF_PWR_12DBM : -12dBm
 * 	NRF_PWR_18DBM : -18dBm
 */
#define CONFIG_NRF_TX_PWR		NRF_PWR_0DBM


/* Data rate
 *	Define this to:
 *		NRF_RATE_250KBPS
 *		NRF_RATE_1MBPS
 *		NRF_RATE_2MBPS
 */
#define CONFIG_NRF_DATA_RATE	NRF_RATE_250KBPS


/* Define RF channel
 */
#define CONFIG_NRF_RF_CHANNEL 	40


/* Define how many retransmitts that should be performed */
#define CONFIG_NRF_TX_RETRANSMITS	15


/* Auto Retransmission delay (us) */
#if (CONFIG_NRF_DATA_RATE == NRF_RATE_250KBPS)
	#if (CONFIG_NRF_ACK_PL_ENABLED) /* ACK with payload */
		#if (CONFIG_NRF_ACK_PL_LENGTH < 8)
		#define CONFIG_NRF_RETRANS_DELAY 750
		#elif (CONFIG_NRF_ACK_PL_LENGTH < 16)
		#define CONFIG_NRF_RETRANS_DELAY 1000
		#elif (CONFIG_NRF_ACK_PL_LENGTH < 24)
		#define CONFIG_NRF_RETRANS_DELAY 1250
		#else
		#define CONFIG_NRF_RETRANS_DELAY 1500
		#endif
	#else
		#define CONFIG_NRF_RETRANS_DELAY 500  /* Empty ACK */
	#endif
#else /* 1Mbps or 2Mbps */
	#if (CONFIG_NRF_ACK_PL_ENABLED) /* ACK with payload */
		#define CONFIG_NRF_RETRANS_DELAY 500
	#else /* Empty ACK */
		#define CONFIG_NRF_RETRANS_DELAY 250
	#endif
#endif

/** 
 * Pipes to enable (EN_RXADDR) - Rx only
   pipes to auto_ack (EN_AA) - Tx and Rx (if EN_AA=0, Enhanced shockburst is disabled)
	each pipe payload width (RX_PW_Px) - Rx only
*/

 #endif  // NRF24L01P_CONFIG_H
 