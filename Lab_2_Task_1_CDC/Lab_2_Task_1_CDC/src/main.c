
#include <asf.h>

#define STATE_1		1
#define STATE_2		2
#define	TRUE		1
#define FALSE		0

uint8_t state_indicator	= STATE_1;
uint8_t	run_once		= FALSE;

void push_button_interrupt_handler(void);
void write_print_to_cdc(char *input_string);
void write_println_to_cdc(char *input_string);


#if __GNUC__
__attribute__((__interrupt__))
#elif __ICCAVR32__
__interrupt
#endif
void push_button_interrupt_handler(void)
{
	if (gpio_get_pin_interrupt_flag(GPIO_PUSH_BUTTON_1))
	{
		// Switch to next state based on current state
		switch (state_indicator)
		{
			case STATE_1:
				state_indicator = STATE_2;
			break;
			case STATE_2:
				state_indicator = STATE_1;
			break;
		}
		
		// Set run_once variable to true
		run_once = TRUE;
		
		// Clear interrupt flag to allow new interrupts
		gpio_clear_pin_interrupt_flag(GPIO_PUSH_BUTTON_1);
	}
}


void write_print_to_cdc(char *input_string)
{
	while (*input_string)
	{
		udi_cdc_putc(*input_string);
		*input_string++;
	}
}


void write_println_to_cdc(char *input_string)
{
	while (*input_string)
	{
		udi_cdc_putc(*input_string);
		*input_string++;
	}
	udi_cdc_putc('\r');
	udi_cdc_putc('\n');
}


int main (void)
{
	// Initialize CPU clock to 48 MHz (configured in conf_clock.h)
	sysclk_init();
	
	// Initialize the EVK1100 and its pin configuration
	board_init();
	
	// Enable LED1 and LED2 as GPIO output
	gpio_enable_gpio_pin(LED0_GPIO);
	gpio_enable_gpio_pin(LED1_GPIO);
	gpio_configure_pin(LED0_GPIO, GPIO_DIR_OUTPUT);
	gpio_configure_pin(LED1_GPIO, GPIO_DIR_OUTPUT);
	
	// Configure IRQs
	irq_initialize_vectors();
	cpu_irq_enable();
	
	// Disable all interrupts
	Disable_global_interrupt();
	
	// Initialize interrupt module
	INTC_init_interrupts();
	
		// Define handler and configure interrupt with INT1 priority
	INTC_register_interrupt(&push_button_interrupt_handler,
							AVR32_GPIO_IRQ_0 + (GPIO_PUSH_BUTTON_1/8),
							AVR32_INTC_INT1);
	
	// Enable falling edge interrupt on Push Button 1
	gpio_enable_pin_interrupt(GPIO_PUSH_BUTTON_1, GPIO_FALLING_EDGE);
	
	// Enable global interrupts
	Enable_global_interrupt();
	
	// Start USB CDC
	udc_start();
	
	// Set initial state
	// STATE_1 = LED0 On, LED1 Off
	// STATE_2 = LED0 Off, LED1 On
	gpio_set_pin_low(LED0_GPIO);
	gpio_set_pin_high(LED1_GPIO);
	
	while (1)
	{
		// If an interrupt has happened and run_once is true...
		if (run_once)
		{
			switch (state_indicator)
			{
				// ... and if current state is STATE_1...
				case STATE_1:
					// Activate LED0 and deactivate LED1
					gpio_set_pin_low(LED0_GPIO);
					gpio_set_pin_high(LED1_GPIO);
					// Send debug message over CDC
					write_println_to_cdc("--------------");
					write_println_to_cdc("Interrupt detected on PB1");
					write_println_to_cdc("STATE_1 engaged!");
					write_println_to_cdc("LED1 = ON");
					write_println_to_cdc("LED2 = OFF");
					write_println_to_cdc("--------------");
				break;
				
				// ... and if current state is STATE_2...
				case STATE_2:
					// Activate LED1 and deactivate LED0
					gpio_set_pin_low(LED1_GPIO);
					gpio_set_pin_high(LED0_GPIO);
					// Send debug message over CDC
					write_println_to_cdc("--------------");
					write_println_to_cdc("Interrupt detected on PB1");
					write_println_to_cdc("STATE_2 engaged!");
					write_println_to_cdc("LED1 = OFF");
					write_println_to_cdc("LED2 = ON");
					write_println_to_cdc("--------------");
				break;
			}
			
			// Reset run_once to false
			run_once = FALSE;
		}
		
		// Otherwise, do nothing!
	}
}
