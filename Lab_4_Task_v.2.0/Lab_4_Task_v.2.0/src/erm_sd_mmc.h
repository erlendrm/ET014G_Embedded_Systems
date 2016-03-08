/*
 * erm_sd_mmc.h
 *
 * Created: 07.03.2016 22:26:26
 *  Author: Erlend
 */ 


#ifndef ERM_SD_MMC_H_
#define ERM_SD_MMC_H_


// SPI options
spi_options_t erm_sd_mmc_spiOptions =
{
	.reg          = SD_MMC_SPI_NPCS,
	.baudrate     = SD_MMC_SPI_MASTER_SPEED,
	.bits         = SD_MMC_SPI_BITS,          // Defined in conf_sd_mmc_spi.h.
	.spck_delay   = 0,
	.trans_delay  = 0,
	.stay_act     = 1,
	.spi_mode     = 0,
	.modfdis      = 1
};


static void erm_sd_mmc_pin_init(void)
{
	// GPIO pins used for SD/MMC interface
	static const gpio_map_t SD_MMC_SPI_GPIO_MAP =
	{
		{SD_MMC_SPI_SCK_PIN,  SD_MMC_SPI_SCK_FUNCTION },  // SPI Clock.
		{SD_MMC_SPI_MISO_PIN, SD_MMC_SPI_MISO_FUNCTION},  // MISO.
		{SD_MMC_SPI_MOSI_PIN, SD_MMC_SPI_MOSI_FUNCTION},  // MOSI.
		{SD_MMC_SPI_NPCS_PIN, SD_MMC_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
	};


	// Assign I/Os to SPI.
	gpio_enable_module(SD_MMC_SPI_GPIO_MAP,
	sizeof(SD_MMC_SPI_GPIO_MAP) / sizeof(SD_MMC_SPI_GPIO_MAP[0]));
}

static void erm_sd_mmc_init(uint32_t pba_clk)
{
	// Initialize as master.
	spi_initMaster(SD_MMC_SPI, &erm_sd_mmc_spiOptions);

	// Set SPI selection mode: variable_ps, pcs_decode, delay.
	spi_selectionMode(SD_MMC_SPI, 0, 0, 0);

	// Enable SPI module.
	spi_enable(SD_MMC_SPI);

	// Initialize SD/MMC driver with SPI clock (PBA).
	sd_mmc_spi_init(erm_sd_mmc_spiOptions, pba_clk);
}



#endif /* ERM_SD_MMC_H_ */