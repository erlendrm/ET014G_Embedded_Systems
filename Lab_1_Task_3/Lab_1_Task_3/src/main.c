
#include "asf.h"
#include "board.h"
#include "compiler.h"
#include "dip204.h"
#include "intc.h"
#include "gpio.h"
#include "pm.h"
#include "delay.h"
#include "spi.h"
#include "conf_clock.h"
#include "lcd_spi.h"
#include "stdlib.h"
#include "stdio.h"


uint8_t global_counter = 0;

#if __GNUC__
__attribute__((__interrupt__))
#elif __ICCAVR32__
__interrupt
#endif
static void push_button_0_interrupt_handler(void)
{
	if (gpio_get_pin_interrupt_flag(GPIO_PUSH_BUTTON_0))
	{
		float		c				= 0;
		float		a				= 1234.5678;
		float		b				= 8765.4321;
		uint64_t	z				= 0;
		uint32_t	x				= 12345678;
		uint32_t	y				= 87654321;
		uint32_t	cycle_count		= 0;
		uint32_t	cycle_in_ms		= 0;
		char		cycle_count_string[9];
		char		cycle_in_ms_string[9];
		
		dip204_clear_display();											// Clear LCD
		
		switch (global_counter)
		{
			case 0 :
			cycle_count = Get_sys_count();								// Get the current cycle count
			
			z = x * y;													// Calculate x * y
			
			cycle_count = ((Get_sys_count()) - cycle_count);			// Put cycle count difference in cycle_count
			
			cycle_in_ms = cpu_cy_2_us(cycle_count, F_CPU);				// Convert cycle count to microseconds
			
			sprintf(cycle_count_string, "%lu", cycle_count);			// Convert count value from int to char string
			sprintf(cycle_in_ms_string, "%lu", cycle_in_ms);			// Convert millisecond value from int to char string
			
			dip204_set_cursor_position(4,1);							// Move LCD cursor
			dip204_write_string("Case: z = x * y");						// Write string to LCD
			dip204_set_cursor_position(4,3);							// Move LCD cursor
			dip204_write_string("Cycles: ");							// Write string to LCD
			dip204_write_string(cycle_count_string);					// Write string to LCD
			dip204_set_cursor_position(4,4);							// Move LCD cursor
			dip204_write_string("In us:  ");							// Write string to LCD
			dip204_write_string(cycle_in_ms_string);					// Write string to LCD
			
			global_counter = 1;											// Increment global counter to 1
			break;
			
			case 1 :
			cycle_count = Get_sys_count();								// Get the current cycle count
			
			c = a * b;													// Calculate a * b
			
			cycle_count = ((Get_sys_count()) - cycle_count);			// Put cycle count difference in cycle_count
			
			cycle_in_ms = cpu_cy_2_us(cycle_count, F_CPU);				// Convert cycle count to microseconds
			
			sprintf(cycle_count_string, "%lu", cycle_count);			// Convert count value from int to char string
			sprintf(cycle_in_ms_string, "%lu", cycle_in_ms);			// Convert millisecond value from int to char string
			
			dip204_set_cursor_position(4,1);							// Move LCD cursor
			dip204_write_string("Case: c = a * b");						// Write string to LCD
			dip204_set_cursor_position(4,3);							// Move LCD cursor
			dip204_write_string("Cycles: ");							// Write string to LCD
			dip204_write_string(cycle_count_string);					// Write string to LCD
			dip204_set_cursor_position(4,4);							// Move LCD cursor
			dip204_write_string("In us:  ");							// Write string to LCD
			dip204_write_string(cycle_in_ms_string);					// Write string to LCD
			
			global_counter = 0;											// Reset global counter
			break;
		}
		
		gpio_clear_pin_interrupt_flag(GPIO_PUSH_BUTTON_0);				// Clear interrupt flag to allow new interrupts
	}
}



int main (void)
{
	pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);					// Set Oscillator 0 (FOSC0 @ 12 MHz) as main CPU clock
	
	board_init();														// Initialize the EVK1100 and its pin configuration
	
	lcd_spi_pin_init();													// Initialize correct pins for the SPI
	lcd_spi_init();														// Initialize SPI MASTER, enable SPI and initialize LCD
	
	Disable_global_interrupt();											// Disable all interrupts
	
	INTC_init_interrupts();												// Initialize interrupt module
	
	gpio_enable_pin_interrupt(GPIO_PUSH_BUTTON_0, GPIO_FALLING_EDGE);	// Enable rising edge interrupt on Push Button 0
	
	INTC_register_interrupt( &push_button_0_interrupt_handler,
	AVR32_GPIO_IRQ_0 + (GPIO_PUSH_BUTTON_0/8),
	AVR32_INTC_INT1);							// Define handler and configure interrupt with INT1 priority
	
	Enable_global_interrupt();											// Enable global interrupts
	
	dip204_set_cursor_position(2,2);									// Move LCD cursor
	dip204_write_string("Press PB0 to start");							// Write string to LCD
	dip204_set_cursor_position(2,3);									// Move LCD cursor
	dip204_write_string("Use PB0 to scroll");							// Write string to LCD
	
	while (1)															// Main while loop
	{
		// Do nothing
	}
}																		// END
