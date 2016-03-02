
#include <asf.h>

#define F_CPU FOSC0

int main (void)
{
	pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);		// Set Oscillator 0 (FOSC0 @ 12 MHz) as main CPU clock

	board_init();											// Initialize the EVK1100 and its pin configuration 
	
	gpio_enable_gpio_pin(GPIO_PUSH_BUTTON_0);				// Enable GPIO on pin PX16 (Push Button 0 on the EVK1100)
	gpio_enable_gpio_pin(LED6_GPIO);						// Enable GPIO on pin PB21 (LED 6 on the EVK1100)
	gpio_configure_pin(LED6_GPIO, GPIO_DIR_OUTPUT);			// Set PB21 (LED 6 on the EVK1100) as output
		

	while (1)												// Main while loop
	{
		if (gpio_pin_is_low(GPIO_PUSH_BUTTON_0))			// If Push Button 0 is pressed (active low) ...
		{
			gpio_set_pin_low(LED6_GPIO);					// ... activate LED 6
		} 
		else												// Otherwise ...
		{
			gpio_set_pin_high(LED6_GPIO);					// ... deactivate LED 6
		}
	}
}															// END
