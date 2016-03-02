/*
 * lcd_spi.h
 *
 * Created: 02.02.2016 12:54:05
 *  Author: Erlend
 */ 


#ifndef LCD_SPI_H_
#define LCD_SPI_H_

void lcd_spi_pin_init(void);
void lcd_spi_init(void);


// The DIP204B is connected to the SPI1 channel on the EVK1100 as opposed to the SPI0 channel
// (which is mostly for the optional external SPI connector), and must be configured in
// accordance with this fact.

spi_options_t spiOptions =
{
	.reg          = DIP204_SPI_NPCS,	// Set SPI channel, in this case it's 1 (INT1, not INT0)
	.baudrate     = 1000000,			// Set desired baud rate
	.bits         = 8,					// Set data character length
	.spck_delay   = 0,					// Set delay for first clock after slave select
	.trans_delay  = 0,					// Set delay between each transfer/character
	.stay_act     = 1,					// Set chip to stay active after last transfer
	.spi_mode     = SPI_MODE_0,			// Select SPI mode
	.modfdis      = 1					// Disable mode fault detection
};

void lcd_spi_pin_init(void)
{
	// The DIP204B LCD display on the EVK1100 board is connected on the following PINs:
	// PA15 (MUX function B: SPI1_CLK) = LCD Pin 6 (CLK)
	// PA16 (MUX function B: SPI1_MOSI) = LCD Pin 5 (MOSI)
	// PA17 (MUX function B: SPI1_MISO) = LCD Pin 7 (MISO)
	// PA19 (MUX function B: SPI1_CS2) = LCD Pin 4 (CS)
	// PB18 (MUX function B: PWM_6) = LCD pin 18 (Backlight)
	
	// Disabling the GPIO seems a bit counter-intuitive, but it's not to be used as
	// General Purpoise IO (GPIO) anymore, it's now supposed to be driven by the clock
	// or the SPI.
	
	AVR32_GPIO.port[0].gperc	= 1<<15;	// Disable GPIO on PA15 (LCD CLK)
	AVR32_GPIO.port[0].pmr1c	= 1<<15;	// Clear PMR1
	AVR32_GPIO.port[0].pmr0s	= 1<<15;	// Set PMR0 = MUX mode B
	
	AVR32_GPIO.port[0].gperc	= 1<<16;	// Disable GPIO on PA16 (LCD MOSI)
	AVR32_GPIO.port[0].pmr1c	= 1<<16;	// Clear PMR1
	AVR32_GPIO.port[0].pmr0s	= 1<<16;	// Set PMR0 = MUX mode B
	
	AVR32_GPIO.port[0].gperc	= 1<<17;	// Disable GPIO on PA17 (LCD MISO)
	AVR32_GPIO.port[0].pmr1c	= 1<<17;	// Clear PMR1
	AVR32_GPIO.port[0].pmr0s	= 1<<17;	// Set PMR0 = MUX mode B
	
	// The DIP204B is connected to the SPI1 channel on the EVK1100 along with the SD/MMC
	// card slot, and is selected as slave by using the second chip select line PA19 (SPI1_CS2).
	// PA18 (SPI1_CS1) is used to select the SD/MMC card slot as slave.
	
	AVR32_GPIO.port[0].gperc	= 1<<19;	// Disable GPIO on PA19 (LCD CS2)
	AVR32_GPIO.port[0].pmr1c	= 1<<19;	// Clear PMR1
	AVR32_GPIO.port[0].pmr0s	= 1<<19;	// Set PMR0 = MUX mode B
}

void lcd_spi_init(void)
{
	spi_initMaster(DIP204_SPI, &spiOptions);			// Initialize the AVR32 as SPI MASTER
	spi_selectionMode(DIP204_SPI, 0, 0, 0);				// Set selection mode: variable_ps, pcs_decode, delay
	spi_enable(DIP204_SPI);								// Enable SPI
	spi_setupChipReg(DIP204_SPI, &spiOptions, FOSC0);	// Configure registers on MASTER
	dip204_init(backlight_IO, true);					// Initialize LCD
	dip204_hide_cursor();								// Hide cursor
}


#endif /* LCD_SPI_H_ */