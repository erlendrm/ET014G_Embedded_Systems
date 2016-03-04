
// ----------------------------------------------------
// NOTE: The LCD will only work if the code is compiled
//		 with Optimization level 0 (-O0)!
//		 One or more of the ASF drivers are causing
//		 problems when attempting optimization.
// ----------------------------------------------------

#define CPU_HZ			12000000	// CPU clock = 12 MHz
#define PBA_HZ			12000000	// PBA clock = 12 MHz
#define SAMPLING_TIME	20			// Sampling time in milliseconds

#include "asf.h"
#include "erm_lcd_spi.h"
#include "erm_interrupts.h"
#include "erm_tc.h"
#include "erm_adc_pot.h"
#include "erm_pwm.h"
#include "stdio.h"

char temp[9];
uint32_t timer = 0;
uint32_t pot_value = 0;
volatile static bool update_timer = true;
volatile static uint32_t tc_tick = 0;


#if __GNUC__
__attribute__((__interrupt__))
#elif __ICCAVR32__
__interrupt
#endif
static void erm_tc_handler(void)
{
	tc_tick++;
	
	tc_read_sr((&AVR32_TC), 0);
	
	pot_value = adc_get_value(&AVR32_ADC, ADC_POTENTIOMETER_CHANNEL);
	
	// Start ADC
	adc_start(&AVR32_ADC);
	
	update_timer = true;
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
		// 		dip204_clear_display();
		// 		dip204_set_cursor_position(2,2);
		// 		dip204_write_string("lulz");
		
		// Clear interrupt flag to allow new interrupts
		gpio_clear_pin_interrupt_flag(GPIO_PUSH_BUTTON_0);
	}
}


int main (void)
{
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
	
	// Initialize the LCD display
	lcd_spi_pin_init();
	lcd_spi_init(PBA_HZ);
	
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
	
	// Initialize and start the PWM with output on PB22
	erm_pwm_init(75, 100, PBA_HZ);
	
	// Initialize and start the TC with interrupts every SAMPLING_TIME
	erm_tc_init((&AVR32_TC), SAMPLING_TIME, PBA_HZ);
	
	// Start ADC
	adc_start(&AVR32_ADC);
	
	while (1)
	{
		if ((update_timer) && (!(tc_tick%50)))
		{
			timer++;
			
			sprintf(temp,"%lu", timer);
			dip204_set_cursor_position(2,2);
			dip204_write_string("Timer: ");
			dip204_write_string(temp);
			dip204_write_string(" s");
			
			update_timer = false;
			
		}
		
		sprintf(temp, "%lu", pot_value);
		dip204_set_cursor_position(2,3);
		dip204_write_string("Pot value: ");
		dip204_write_string(temp);
		dip204_write_string("   ");
		
		// Otherwise, do nothing
	}
}
