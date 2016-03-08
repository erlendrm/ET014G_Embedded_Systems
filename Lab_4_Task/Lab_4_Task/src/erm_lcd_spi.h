/*
 * erm_lcd_spi.h
 *
 * Created: 02.02.2016 12:54:05
 *  Author: Erlend
 */ 


#ifndef LCD_SPI_H_
#define LCD_SPI_H_

void lcd_spi_pin_init(void);
void lcd_spi_init(uint32_t pba_freq);


// The DIP204B is connected to the SPI1 channel on the EVK1100 as opposed to the SPI0 channel
// (which is mostly for the optional external SPI connector), and must be configured in
// accordance with this fact.

spi_options_t erm_lcd_spiOptions =
{
	.reg          = DIP204_SPI_NPCS,	// Set SPI channel
	.baudrate     = 384000,				// Set desired baud rate
	.bits         = 8,					// Set data character length
	.spck_delay   = 0,					// Set delay for first clock after slave select
	.trans_delay  = 0,					// Set delay between each transfer/character
	.stay_act     = 1,					// Set chip to stay active after last transfer
	.spi_mode     = SPI_MODE_0,			// Select SPI mode
	.modfdis      = 1					// Disable mode fault detection
};

void lcd_spi_pin_init(void)
{
	// Define LCD Display SPI GIPO options
	static const gpio_map_t DIP204_SPI_GPIO_MAP =
	{
		{DIP204_SPI_SCK_PIN,  DIP204_SPI_SCK_FUNCTION },  // SPI Clock.
		{DIP204_SPI_MISO_PIN, DIP204_SPI_MISO_FUNCTION},  // MISO.
		{DIP204_SPI_MOSI_PIN, DIP204_SPI_MOSI_FUNCTION},  // MOSI.
		{DIP204_SPI_NPCS_PIN, DIP204_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
	};
	
	// Assign I/Os to SPI
	gpio_enable_module(DIP204_SPI_GPIO_MAP,
	sizeof(DIP204_SPI_GPIO_MAP) / sizeof(DIP204_SPI_GPIO_MAP[0]));
}

void lcd_spi_init(uint32_t pba_freq)
{
	spi_initMaster(DIP204_SPI, &erm_lcd_spiOptions);				// Initialize the AVR32 as SPI MASTER
	spi_selectionMode(DIP204_SPI, 0, 0, 0);							// Set selection mode: variable_ps, pcs_decode, delay
	spi_enable(DIP204_SPI);											// Enable SPI
	spi_setupChipReg(DIP204_SPI, &erm_lcd_spiOptions, pba_freq);	// Configure registers on MASTER
	dip204_init(backlight_IO, true);								// Initialize LCD
	dip204_hide_cursor();											// Hide cursor
}


#endif /* LCD_SPI_H_ */