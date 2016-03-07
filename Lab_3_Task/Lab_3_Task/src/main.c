
// ----------------------------------------------------
// NOTE: The LCD will only work if the code is compiled
//		 with Optimization level 0 (-O0)!
//		 One or more of the ASF drivers are causing
//		 problems when attempting optimization.
// ----------------------------------------------------

#define CPU_HZ				48000000	// CPU clock = 48 MHz
#define PBA_HZ				12000000	// PBA clock = 12 MHz
#define SAMPLING_TIME		20			// Sampling time in milliseconds
#define POT_ADC_MAX_VALUE	1023
#define POT_ADC_RESOLUTION	1024		// 10-bit ADC yields 1024 bit resolution
#define F_CPU				CPU_HZ

#define INT_NONE				0
#define INT_TC					1
#define INT_PB0					2
#define CHANGE_PWM_FREQUENCY	1
#define CHANGE_PWM_DUTY_CYCLE	2


#include "asf.h"
#include "erm_lcd_spi.h"
#include "erm_interrupts.h"
#include "erm_tc.h"
#include "erm_adc_pot.h"
#include "erm_pwm.h"
#include "stdio.h"

// Pointer to SDRAM start address
volatile unsigned long *sdram = SDRAM;

// Global variables
uint32_t			interrupt_identifier	= INT_NONE;
uint32_t			change_identifier		= CHANGE_PWM_FREQUENCY;


#if __GNUC__
__attribute__((__interrupt__))
#elif __ICCAVR32__
__interrupt
#endif
static void erm_tc_handler(void)
{
	gpio_toggle_pin(LED0_GPIO);
	
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
		gpio_toggle_pin(LED1_GPIO);
		
		// Switch change_identifier to the next value
		switch (change_identifier)
		{
			case CHANGE_PWM_FREQUENCY:
			change_identifier = CHANGE_PWM_DUTY_CYCLE;
			break;
			
			case CHANGE_PWM_DUTY_CYCLE:
			change_identifier = CHANGE_PWM_FREQUENCY;
			break;
		}
		
		// Set interrupt identifier
		interrupt_identifier = INT_PB0;
		
		// Clear interrupt flag to allow new interrupts
		gpio_clear_pin_interrupt_flag(GPIO_PUSH_BUTTON_0);
	}
}


int main (void)
{
	// Declare variables
	char temp_string[9];
	uint32_t i, j;
	uint32_t pot_value;
	uint32_t pwm_frequency	= 100;	// Initial value 100 Hz
	uint32_t pwm_duty_cycle = 50;	// Initial value 50 % duty cycle
	
	// Configure main CPU clock and peripheral bus speed
	pm_freq_param_t System_Clock = {
		.cpu_f = CPU_HZ,
		.pba_f = PBA_HZ,
		.osc0_f = FOSC0,
		.osc0_startup = OSC0_STARTUP
	};
	pm_configure_clocks(&System_Clock);
		
	// Initialize the EVK1100 and its PIN config
	board_init();
	
	// Enable LED1 and LED2 as GPIO output
	gpio_enable_gpio_pin(LED0_GPIO);
	gpio_enable_gpio_pin(LED1_GPIO);
	gpio_configure_pin(LED0_GPIO, GPIO_DIR_OUTPUT);
	gpio_configure_pin(LED1_GPIO, GPIO_DIR_OUTPUT);
	
	// Initialize the LCD display
	lcd_spi_pin_init();
	lcd_spi_init(PBA_HZ);
	
	// Write start string to LCD display
	dip204_set_cursor_position(2,2);
	dip204_write_string("FREQ: ");
	sprintf(temp_string, "%lu",pwm_frequency);
	dip204_write_string(temp_string);
	dip204_set_cursor_position(15,2);
	dip204_write_string("Hz");
	dip204_set_cursor_position(2,3);
	dip204_write_string("DUTY: ");
	sprintf(temp_string, "%lu",pwm_duty_cycle);
	dip204_write_string(temp_string);
	dip204_set_cursor_position(15,3);
	dip204_write_string("%");
	
	// Initialize the external SDRAM chip.
	sdramc_init(CPU_HZ);
	
	// Add a lookup table for PWM frequency in SDRAM
	for (i = 0; i < POT_ADC_RESOLUTION; i++)
	{
		if (i == 0)
		{
			sdram[i] = 100;
		}
		else if (i == POT_ADC_MAX_VALUE)
		{
			sdram[i] = 100000;
		}
		else
		{
			sdram[i] = sdram[i-1] + 100;
		}	
	}
	
	// Add a lookup table for PWM duty cycle in SDRAM
	for (i = 0, j = 0; i < POT_ADC_RESOLUTION; i++)
	{
		sdram[i + POT_ADC_RESOLUTION]= j;
		
		if ( (!(i % 10)) && (j<100))
		{
			j++;
		}
	}
	
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
	
	// Initialize and enable ADC for the Potentiometer
	adc_pot_init();
	
	// Initialize and start the PWM with initial values (output on pin PB22)
	erm_pwm_init(pwm_duty_cycle, pwm_frequency, PBA_HZ);
	
	// Initialize and start the TC with interrupts every SAMPLING_TIME
	erm_tc_init((&AVR32_TC), SAMPLING_TIME, PBA_HZ);
	
	// Start ADC
	adc_start(&AVR32_ADC);
	
	while (1)
	{
		// Check if an interrupt has occurred
		if (interrupt_identifier != INT_NONE)
		{ 
			if (interrupt_identifier == INT_TC)
			{
				// Read Potentiometer value
				pot_value = adc_get_value(&AVR32_ADC, ADC_POTENTIOMETER_CHANNEL); 
				
				// Restart ADC
				adc_start(&AVR32_ADC);
				
				switch (change_identifier)
				{
					case CHANGE_PWM_FREQUENCY:
					// Look up corresponding PWM frequency based on pot_value
					pwm_frequency	= sdram[pot_value];
					// Reset the PWM config and enable PWM update on the next period
					erm_pwm_update_config(pwm_duty_cycle, pwm_frequency, PBA_HZ, PWM_UPDATE_PERIOD);
					erm_pwm_update_channel();
					// Update the LCD display
					dip204_set_cursor_position(8,2);
					sprintf(temp_string, "%lu   ", pwm_frequency);
					dip204_write_string(temp_string);
					dip204_set_cursor_position(15,2);
					dip204_write_string("Hz *");
					dip204_set_cursor_position(15,3);
					dip204_write_string("%   ");
					break;
					
					case CHANGE_PWM_DUTY_CYCLE:
					// Look up corresponding PWM duty cycle based on pot_value
					pwm_duty_cycle	= sdram[pot_value + POT_ADC_RESOLUTION];
					// Reset the PWM config and enable PWM update on the next period
					erm_pwm_update_config(pwm_duty_cycle, pwm_frequency, PBA_HZ, PWM_UPDATE_DUTY);
					erm_pwm_update_channel();
					// Update the LCD display
					dip204_set_cursor_position(8,3);
					sprintf(temp_string, "%lu   ", pwm_duty_cycle);
					dip204_write_string(temp_string);
					dip204_set_cursor_position(15,2);
					dip204_write_string("Hz  ");
					dip204_set_cursor_position(15,3);
					dip204_write_string("%  *");
					break;
					
					default:
					// Impossible state, only here for debugging
					break;
				}
			}
			
			// Reset interrupt identifier
			interrupt_identifier = INT_NONE;
		}
		
		// Otherwise, do nothing
	}
}
