
#define		CPU_HZ				sysclk_get_cpu_hz()
#define		PBA_HZ				sysclk_get_pba_hz()

#define		ERM_IDLE			0
#define		ERM_INITIAL			1
#define		ERM_RUNNING			2
#define		ERM_STOPPED			3
#define		ERM_DEBUG			255
#define		INT_NONE			0
#define		INT_TC				1
#define		INT_PB0				2
#define		NO_COMMAND			0
#define		START_COMMAND		1
#define		STOP_COMMAND		2

#define		SAMPLING_TIME		20			// Sampling time in milliseconds
#define		POT_ADC_MAX_VALUE	1023
#define		POT_ADC_RESOLUTION	1024		// 10-bit ADC yields 1024 bit resolution

#include "asf.h"
#include "conf_sd_mmc_spi.h"
#include "erm_tc.h"
#include "erm_cdc.h"
#include "erm_sd_mmc.h"
#include "erm_adc_pot.h"
#include "erm_lcd_spi.h"
#include "erm_interrupts.h"
#include "stdio.h"
#include "string.h"


// Global variables
uint8_t		interrupt_identifier	= INT_NONE;
uint8_t		commence_program		= false;
uint8_t		update_identifier		= ERM_DEBUG;
uint8_t		command					= NO_COMMAND;
uint8_t		command_string_counter	= 0;
char		command_string[32];


#if __GNUC__
__attribute__((__interrupt__))
#elif __ICCAVR32__
__interrupt
#endif
static void erm_tc_handler(void)
{	
	// Set interrupt identifier
	interrupt_identifier = INT_TC;
	
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
		if (update_identifier != ERM_IDLE)
		{	
			// Set interrupt_identifier
			interrupt_identifier	= INT_PB0;
			
			// Set commence_program identifier
			commence_program	= true;
			
			// Set update_display to LCD_INITIAL
			update_identifier	= ERM_INITIAL;
		}
		
		// Clear interrupt flag to allow new interrupts
		gpio_clear_pin_interrupt_flag(GPIO_PUSH_BUTTON_0);
	}
}



static void decode_command(void)
{
	if (!strcmp(command_string, "start"))
	{
		command = START_COMMAND;
		update_identifier = ERM_RUNNING;
		erm_cdc_println("\r\nStart command recognized!");
		erm_cdc_println("Logging POT values to SD/MMC...");
		erm_cdc_print("\r\n>>");
	}
	
	else if (!strcmp(command_string, "stop"))
	{
		command = STOP_COMMAND;
		update_identifier = ERM_STOPPED;
		erm_cdc_println("\r\nStop command recognized!");
		erm_cdc_println("Logging has stopped!");
		erm_cdc_print("\r\n>>");
	}
	
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
	char		temp_string[9];
	uint32_t	i;
	uint32_t	temp_value;
	uint32_t	pot_value;
	
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
	
	// Initialize and start the TC with interrupts every SAMPLING_TIME
	erm_tc_init((&AVR32_TC), SAMPLING_TIME, PBA_HZ);
	
	// Start USB CDC
	udc_start();
	
	// Start ADC
	adc_start(&AVR32_ADC);
	
	// Select and mount the FAT partition on the SD/MMC
	nav_reset();
	nav_drive_set(0);
	nav_partition_mount();
	
	// Create the file test.txt on root if it doesn't exist already 
	nav_file_create((FS_STRING)"test.txt");
	
	// Main while loop
	while (1)
	{
		if (commence_program)
		{
			build_cmd();
		}
				
		if ((interrupt_identifier != INT_NONE) && commence_program)
		{
			if (interrupt_identifier == INT_TC && command == START_COMMAND)
			{
				// Read Potentiometer value
				pot_value = adc_get_value(&AVR32_ADC, ADC_POTENTIOMETER_CHANNEL);
				
				// Restart ADC
				adc_start(&AVR32_ADC);
				
				// Set navigator to desired file
				if (!nav_setcwd((FS_STRING)"test.txt", true, true))
				{
					erm_cdc_println("error selecting sd/mmc");
				}
				else
				{
					// Open file
					file_open(FOPEN_MODE_APPEND);
					
					// Convert pot_value to a string
					sprintf(temp_string, "%lu\x03", pot_value);
					
					// Write value to file
					i = 0;					
					while ((int) temp_string[i] != 0x03)
					{
						file_putc((int) temp_string[i]);
						i++;					
					}
					
					// Add CR and LF
					file_putc('\r');
					file_putc('\n');
					
					// Close file
					file_close();
				}
			}
			
			switch (update_identifier)
			{
				case ERM_INITIAL:
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
				
				break;
				
				case ERM_RUNNING:
				// Re-initialize LCD SPI
				lcd_spi_init(PBA_HZ);
				dip204_clear_display();
				dip204_set_cursor_position(2,2);
				dip204_write_string("Logging active...");
				// Re-initialize SD/MMC SPI
				erm_sd_mmc_init(PBA_HZ);
				break;
				
				case ERM_STOPPED:
				// Re-initialize LCD SPI
				lcd_spi_init(PBA_HZ);
				dip204_clear_display();
				dip204_set_cursor_position(2,2);
				dip204_write_string("Logging stopped...");
				// Re-initialize SD/MMC SPI
				erm_sd_mmc_init(PBA_HZ);
				break;
				
				default:
				// Do nothing
				break;
			}
			
			// Reset update_identifier
			update_identifier = ERM_IDLE;
					
			// Reset interrupt identifier
			interrupt_identifier = INT_NONE;
		}
		
		// Otherwise do nothing
	}
	
}
