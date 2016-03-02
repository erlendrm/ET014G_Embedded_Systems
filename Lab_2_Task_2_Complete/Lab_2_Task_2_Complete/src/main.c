
//  - PC USART terminal settings:
//    - 57600 bps,
//    - 8 data bits,
//    - no parity bit,
//    - 1 stop bit,
//    - no flow control.


#include "asf.h"
#include "stdio.h"
#include "conf_access.h"
#include "conf_sd_mmc_spi.h"
#include "extra_pdca_functions.h"

#define AVR32_PDCA_CHANNEL_USED_RX AVR32_PDCA_PID_SPI1_RX
#define AVR32_PDCA_CHANNEL_USED_TX AVR32_PDCA_PID_SPI1_TX
#define AVR32_PDCA_CHANNEL_SPI_RX	0
#define AVR32_PDCA_CHANNEL_SPI_TX	1
#define CPU_HZ						60000000	// Define desired main clock speed as 60 MHz
#define PBA_HZ						30000000	// Define maximum peripheral bus speed as 30 MHz
#define UINT32_MAX_VALUE			4294967295

// PDCA Channel pointer
volatile avr32_pdca_channel_t* pdca_channelrx ;
volatile avr32_pdca_channel_t* pdca_channeltx ;

// Used to indicate the end of PDCA transfer
volatile bool end_of_transfer;

// Local RAM buffer for the example to store data received from the SD/MMC card
volatile char ram_buffer[1000];

// Pointer to SDRAM start address
volatile unsigned long *sdram = SDRAM;



static void local_pdca_init(void)
{
	// this PDCA channel is used for data reception from the SPI
	pdca_channel_options_t pdca_options_SPI_RX ={ // pdca channel options

		.addr = ram_buffer,						  // memory address.

		.size = 512,                              // transfer counter: here the size of the string
		.r_addr = NULL,                           // next memory address after 1st transfer complete
		.r_size = 0,                              // next transfer counter not used here
		.pid = AVR32_PDCA_CHANNEL_USED_RX,        // select peripheral ID - data are on reception from SPI1 RX line
		.transfer_size = PDCA_TRANSFER_SIZE_BYTE  // select size of the transfer: 8,16,32 bits
	};

	// this channel is used to activate the clock of the SPI by sending a dummy variables
	pdca_channel_options_t pdca_options_SPI_TX ={ // pdca channel options

		.addr = sdram,							  // memory address.
		.size = 512,                              // transfer counter: here the size of the string
		.r_addr = NULL,                           // next memory address after 1st transfer complete
		.r_size = 0,                              // next transfer counter not used here
		.pid = AVR32_PDCA_CHANNEL_USED_TX,        // select peripheral ID - data are on reception from SPI1 RX line
		.transfer_size = PDCA_TRANSFER_SIZE_WORD  // select size of the transfer: 8,16,32 bits
	};

	// Init PDCA transmission channel
	pdca_init_channel(AVR32_PDCA_CHANNEL_SPI_TX, &pdca_options_SPI_TX);

	// Init PDCA Reception channel
	pdca_init_channel(AVR32_PDCA_CHANNEL_SPI_RX, &pdca_options_SPI_RX);
}


static void sd_mmc_resources_init(void)
{
	// GPIO pins used for SD/MMC interface
	static const gpio_map_t SD_MMC_SPI_GPIO_MAP =
	{
		{SD_MMC_SPI_SCK_PIN,  SD_MMC_SPI_SCK_FUNCTION },  // SPI Clock.
		{SD_MMC_SPI_MISO_PIN, SD_MMC_SPI_MISO_FUNCTION},  // MISO.
		{SD_MMC_SPI_MOSI_PIN, SD_MMC_SPI_MOSI_FUNCTION},  // MOSI.
		{SD_MMC_SPI_NPCS_PIN, SD_MMC_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
	};

	// SPI options.
	spi_options_t spiOptions =
	{
		.reg          = SD_MMC_SPI_NPCS,
		.baudrate     = SD_MMC_SPI_MASTER_SPEED,  // Defined in conf_sd_mmc_spi.h.
		.bits         = SD_MMC_SPI_BITS,          // Defined in conf_sd_mmc_spi.h.
		.spck_delay   = 0,
		.trans_delay  = 0,
		.stay_act     = 1,
		.spi_mode     = 0,
		.modfdis      = 1
	};

	// Assign I/Os to SPI.
	gpio_enable_module(SD_MMC_SPI_GPIO_MAP,
	sizeof(SD_MMC_SPI_GPIO_MAP) / sizeof(SD_MMC_SPI_GPIO_MAP[0]));

	// Initialize as master.
	spi_initMaster(SD_MMC_SPI, &spiOptions);

	// Set SPI selection mode: variable_ps, pcs_decode, delay.
	spi_selectionMode(SD_MMC_SPI, 0, 0, 0);

	// Enable SPI module.
	spi_enable(SD_MMC_SPI);

	// Initialize SD/MMC driver with SPI clock (PBA).
	sd_mmc_spi_init(spiOptions, PBA_HZ);
}

int main (void)
{
	uint32_t	number_of_sd_sectors;
	uint32_t	progress_inc;
	uint32_t	sdram_size;
	uint32_t	i;
	uint32_t	j;
	uint32_t	k;
	
	// Configure main CPU clock and peripheral bus speed
	pm_freq_param_t System_Clock = {
		.cpu_f = CPU_HZ,
		.pba_f = PBA_HZ,
		.osc0_f = FOSC0,
		.osc0_startup = OSC0_STARTUP
	};
	pm_configure_clocks(&System_Clock);
	
	// Initialize debug RS232 with PBA clock
	init_dbg_rs232(PBA_HZ);

	// Initialize the EVK1100 and its PIN config
	board_init();
	
	// Initialize SD/MMC driver resources: GPIO, SPI and SD/MMC.
	sd_mmc_resources_init();
	
	// Write start string to USART
	print_dbg("\x0C---------------------------------------------------------------------\r\n");
	print_dbg("\r\nLab 2 Task 2 - Erlend R. Myklebust\r\n");
	print_dbg("\r\n---------------------------------------------------------------------\r\n");
	
	// The SDRAM is 32 MB in total, meaning:
	// Number of bits = 268,435,456
	// Number of bytes = 33,554,432
	// Number of 4 byte words = 8,388,608
	// Defining SDRAM size in 32-bit words and write to USART:
	sdram_size = SDRAM_SIZE >> 2;
	print_dbg("\r\nSDRAM size: ");
	print_dbg_ulong(SDRAM_SIZE >> 20);
	print_dbg(" MB\r\n");
	
	// Initialize the external SDRAM chip.
	sdramc_init(CPU_HZ);
	print_dbg("SDRAM initialized\r\n\n");	
	
	// Setting EBI slave to have a fixed default master
	AVR32_HMATRIX.SCFG[AVR32_HMATRIX_SLAVE_EBI].defmstr_type	= AVR32_HMATRIX_DEFMSTR_TYPE_FIXED_DEFAULT;

	// Set EBI slave to have PDCA as master
	AVR32_HMATRIX.SCFG[AVR32_HMATRIX_SLAVE_EBI].fixed_defmstr	= AVR32_HMATRIX_MASTER_PDCA;
	
	// Determine the increment of steps for progress indicator
	progress_inc = (sdram_size + 50) / 100;
	
	// Fill SDRAM with test pattern: 0x00, 0x01, ... 0xFF
	for (i = 0, j = 0, k = 0; i < sdram_size;	i++)
	{
		// Write progress indicator to USART
		if (i == k * progress_inc)
		{
			print_dbg("\rFilling SDRAM with test pattern: ");
			print_dbg_ulong(k++);
			print_dbg_char('%');
		}
		
		// Fill SDRAM byte i with value j, and then increment j
		sdram[i] = j;
		j++;
		
		// Debugger for ensuring the pattern
		if (j > 0xFF)
		{
			j = 0;
		}
	}
	
	// Write confirmation to USART
	print_dbg("\r\nSDRAM filled with test pattern: 0x00, 0x01 ... 0xFF\r\n");
	
	// Ask user to insert SD/MMC
	print_dbg("\r\nInsert SD/MMC...");
	
	// Wait for a card to be inserted
	while (!sd_mmc_spi_mem_check());
	print_dbg("\r\nCard detected!");
	
	// Read Card capacity
	sd_mmc_spi_get_capacity();
	print_dbg("\r\nCapacity = ");
	print_dbg_ulong(capacity >> 20);
	print_dbg(" MBytes\r\n\n");

	// Initialize PDCA controller before starting a transfer
	local_pdca_init();
		
	// Calculate amount of SD/MMC sectors needed to fit entire SDRAM (512 Bytes per sector)
	number_of_sd_sectors = sdram_size / 512;
	
	// Determine the increment of steps for progress indicator
	progress_inc = number_of_sd_sectors / 100;
	
	// Loop for writing entire SDRAM to SD/MMC one sector at a time
	for (i = 0, j = 0; i < number_of_sd_sectors; i++)
	{
		// Open PCDA write session to SD/MMC sector i
		if (sd_mmc_spi_write_open_PDCA(i))
		{
			// Load contents of SDRAM on the SPI_TX channel
			pdca_load_channel( AVR32_PDCA_CHANNEL_SPI_TX,sdram,512);
			
			// Enable PDCA
			pdca_enable(AVR32_PDCA_CHANNEL_SPI_TX);
			
			// Wait for transmission to end
			while (!(pdca_get_transfer_status(AVR32_PDCA_CHANNEL_SPI_TX)&2));
			
			// Disable PDCA
			pdca_disable(AVR32_PDCA_CHANNEL_SPI_TX);
			
			// Close PCDA write session
			sd_mmc_spi_write_close_PDCA();
		}
		
		// Write progress indicator to USART
		if (i == j * progress_inc)
		{
			print_dbg("\rTransfering SDRAM to SD/MMC: ");
			print_dbg_ulong(j++);
			print_dbg_char('%');
		}
		
	}
	
	// Write end string to USART
	print_dbg("\r\nTransfer complete!");
	print_dbg("\r\nYou can now power down the board and check the contents of the SD/MMC\r\n");
	print_dbg("\r\n---------------------------------------------------------------------\r\n");
	
	
	while (1)
	{
		// Do nothing
	}
	
}	// END
