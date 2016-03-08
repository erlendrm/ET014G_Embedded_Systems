
#define		CPU_HZ				sysclk_get_cpu_hz()
#define		PBA_HZ				sysclk_get_pba_hz()

#define		IDLE				0
#define		MOUNT_SDMMC			1
#define		INT_TC				2
#define		INT_PB0				3
#define		INT_NONE			4
#define		UPDATE_INITIAL		5
#define		UPDATE_START		6
#define		UPDATE_STOP			7

#define		NO_COMMAND			0
#define		START_COMMAND		1
#define		STOP_COMMAND		2

#define		SAMPLING_TIME		20			// Sampling time in milliseconds
#define		POT_ADC_MAX_VALUE	1023
#define		POT_ADC_RESOLUTION	1024		// 10-bit ADC yields 1024 bit resolution


#include "asf.h"
#include "stdio.h"
#include "string.h"
#include "conf_sd_mmc_spi.h"
#include "erm_tc.h"
#include "erm_cdc.h"
#include "erm_fat.h"
#include "erm_sd_mmc.h"
#include "erm_adc_pot.h"
#include "erm_lcd_spi.h"
#include "erm_interrupts.h"



// Global variables
uint8_t			loop_identifier			= IDLE;
uint8_t			mounted					= false;
uint8_t			running					= false;
uint8_t			commence_program		= false;
uint8_t			command					= NO_COMMAND;
uint8_t			command_string_counter	= 0;
char			command_string[32];


#if __GNUC__
__attribute__((__interrupt__))
#elif __ICCAVR32__
__interrupt
#endif
static void erm_tc_handler(void)
{
	if (mounted && (command == START_COMMAND))
	{
		// Set loop_identifier
		loop_identifier = INT_TC;
	}
	
	// Clear TC interrupt flag
	tc_read_sr((&AVR32_TC), 0);
}


#if __GNUC__
__attribute__((__interrupt__))
#elif __ICCAVR32__
__interrupt
#endif
static void push_button_handler(void)
{
	if (gpio_get_pin_interrupt_flag(GPIO_PUSH_BUTTON_0))
	{
		if (!commence_program)
		{
			// Set commence_program identifier
			commence_program = true;
			
			// Set loop_identifier
			loop_identifier = UPDATE_INITIAL;
		}
		
		// Clear interrupt flag to allow new interrupts
		gpio_clear_pin_interrupt_flag(GPIO_PUSH_BUTTON_0);
	}
}



static void decode_command(void)
{
	// If received command was 'start'
	if (!strcmp(command_string, "start"))
	{
		if (running)
		{
			erm_cdc_println("\r\nCommand rejected!");
			erm_cdc_println("A measurement is currently running!");
			erm_cdc_println("Use 'stop' to terminate measurement:");
			erm_cdc_print("\r\n>>");
		}
		else
		{
			command = START_COMMAND;
			loop_identifier = UPDATE_START;
		}
		
	}
	
	// If received command was 'stop'
	else if (!strcmp(command_string, "stop"))
	{
		if (!running)
		{
			erm_cdc_println("\r\nCommand ineffective!");
			erm_cdc_println("Start new measurement with command 'start':");
			erm_cdc_print("\r\n>>");
		}
		else
		{
			command = STOP_COMMAND;
			loop_identifier = UPDATE_STOP;
		}
		
	}
	
	// Otherwise
	else
	{
		command	= NO_COMMAND;
		erm_cdc_println("\r\nInvalid command, try again!");
		erm_cdc_print("\r\n>>");
	}
}



static void build_cmd(void)
{
	char rx_character;
	
	if (udi_cdc_is_rx_ready())
	{
		udi_cdc_read_buf(&rx_character, 1);
		
		switch (rx_character)
		{
			case '\r':
			// Echo back to CDC
			udi_cdc_putc(rx_character);
			udi_cdc_putc('\n');
			// Add NULL to command string
			command_string[command_string_counter] = '\0';
			// Decode command
			decode_command();
			// Reset command string counter
			command_string_counter = 0;
			break;
			
			case '\b':
			if (command_string_counter > 0)
			{
				// Echo back to CDC
				udi_cdc_putc(rx_character);
				// Decrease command string counter
				command_string_counter--;
			}
			break;
			
			default:
			// Echo back to CDC
			udi_cdc_putc(rx_character);
			// Append to command string
			command_string[command_string_counter] = rx_character;
			command_string_counter++;
			break;
		}
	}
}



int main (void)
{
	// Declare variables
	uint32_t	pot_value;
	char		current_file[32];
	
	// Initialize CPU clock to 48 MHz and PBA clock to 12 MHz (conf_clock.h)
	sysclk_init();
	
	// Initialize the EVK1100 and its pin config
	board_init();
	
	// Initialize the LCD display
	lcd_spi_pin_init();
	lcd_spi_init(PBA_HZ);
	
	// Write start string to the LCD display
	dip204_set_cursor_position(2,2);
	dip204_write_string("Open PC terminal");
	dip204_set_cursor_position(2,3);
	dip204_write_string("Press PB0 to start");
	
	// Configure IRQs (needed for the USB CDC)
	irq_initialize_vectors();
	cpu_irq_enable();
	
	// Disable all interrupts
	Disable_global_interrupt();
	
	// Initialize interrupt module
	INTC_init_interrupts();
	
	// Interrupt priority for this program should be:
	// INT3 (highest):	TC event (ADC measurement)
	// INT0 (lowest):	Push button event
	
	// Initialize TC interrupt with INT3 priority
	erm_tc_interrupt_init(&erm_tc_handler, AVR32_INTC_INT3);
	
	// Initialize Push Button 0 (PB0) interrupt with INT0 priority
	push_button_0_interrupt_init(&push_button_handler, AVR32_INTC_INT0);
	
	// Enable global interrupts
	Enable_global_interrupt();
	
	// Initialize the SD/MMC
	erm_sd_mmc_pin_init();
	erm_sd_mmc_init(PBA_HZ);
	
	// Initialize and enable ADC for the Potentiometer
	adc_pot_init();
	
	// Initialize the TC with interrupts every SAMPLING_TIME
	erm_tc_init((&AVR32_TC), SAMPLING_TIME, PBA_HZ);
	
	// Start USB CDC
	udc_start();
	
	// Start ADC
	adc_start(&AVR32_ADC);
	
	
	// Main while loop
	while (1)
	{
		if (commence_program)
		{
			// Main switch case structure
			switch (loop_identifier)
			{
				// -------------------------------------------------------------------
				case INT_TC:
				// Read Potentiometer value
				pot_value = adc_get_value(&AVR32_ADC, ADC_POTENTIOMETER_CHANNEL);
				// Save value to file
				erm_fat_file_append(current_file, pot_value);
				// Restart ADC
				adc_start(&AVR32_ADC);
				// Reset loop_identifier
				loop_identifier = IDLE;
				break;
				// -------------------------------------------------------------------
				
				// -------------------------------------------------------------------
				case UPDATE_INITIAL:
				// Select and mount the FAT partition on the SD/MMC
				erm_fat_mount_sdmmc();
				// Start TC
				erm_tc_start(&AVR32_TC);
				// Write initial string to CDC
				erm_cdc_println("\x0C---------------------------------------------------------------------");
				erm_cdc_println("\r\nLab 4 - Erlend R. Myklebust");
				erm_cdc_println("\r\n---------------------------------------------------------------------");
				erm_cdc_println("\r\nCommand list:");
				erm_cdc_println("start  = start logging POT values to sd/mmc");
				erm_cdc_println("stop   = stop logging POT values to sd/mmc");
				erm_cdc_println("\r\nType command followed by enter:");
				erm_cdc_print(">>");
				// Re-initialize LCD SPI
				lcd_spi_init(PBA_HZ);
				// Update LCD display
				dip204_clear_display();
				dip204_set_cursor_position(2,2);
				dip204_write_string("Enter command...");
				// Re-initialize SD/MMC SPI
				erm_sd_mmc_init(PBA_HZ);
				// Reset loop_identifier
				loop_identifier = IDLE;
				break;
				// -------------------------------------------------------------------
				
				// -------------------------------------------------------------------
				case UPDATE_START:
				// Create the measurement file
				erm_fat_create_file(current_file);
				// Set mounted and running variable
				mounted = true;
				running = true;
				// Write start string to CDC
				erm_cdc_println("\r\nStart command recognized!");
				erm_cdc_print("File: ");
				erm_cdc_println(current_file);
				erm_cdc_println("Logging POT values to SD/MMC...");
				erm_cdc_print("\r\n>>");
				// Re-initialize LCD SPI
				lcd_spi_init(PBA_HZ);
				dip204_clear_display();
				dip204_set_cursor_position(2,2);
				dip204_write_string("Logging active...");
				// Re-initialize SD/MMC SPI
				erm_sd_mmc_init(PBA_HZ);
				// Reset loop_identifier
				loop_identifier = IDLE;
				break;
				// -------------------------------------------------------------------
				
				// -------------------------------------------------------------------
				case UPDATE_STOP:
				// Reset mounted and running variable
				mounted = false;
				running = false;
				// Write stop string to CDC
				erm_cdc_println("\r\nStop command recognized!");
				erm_cdc_println("Logging has stopped!");
				erm_cdc_print("\r\n>>");
				// Re-initialize LCD SPI
				lcd_spi_init(PBA_HZ);
				dip204_clear_display();
				dip204_set_cursor_position(2,2);
				dip204_write_string("Logging stopped!");
				dip204_set_cursor_position(2,3);
				dip204_write_string("Enter command...");
				// Re-initialize SD/MMC SPI				
				erm_sd_mmc_init(PBA_HZ);
				// Reset loop_identifier
				loop_identifier = IDLE;
				break;
				// -------------------------------------------------------------------
				
				// -------------------------------------------------------------------
				default:
				// Do nothing
				break;
				// -------------------------------------------------------------------
			}
			
			// Build commands from characters received from CDC
			build_cmd();
		}
		
		// Otherwise do nothing
	}
	
}
