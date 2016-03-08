/*
 * erm_interrupts.h
 *
 * Created: 03.03.2016 14:13:43
 *  Author: Erlend
 */ 


#ifndef INTERRUPTS_INIT_H_
#define INTERRUPTS_INIT_H_

static void push_button_0_interrupt_init(__int_handler handler, uint32_t priority)
{
	// Define handler and configure interrupt with correct priority
	INTC_register_interrupt(handler,
							AVR32_GPIO_IRQ_0 + (GPIO_PUSH_BUTTON_0/8),
							priority);
	
	// Enable falling edge interrupt on Push Button 1
	gpio_enable_pin_interrupt(GPIO_PUSH_BUTTON_0, GPIO_FALLING_EDGE);
}


static void push_button_1_interrupt_init(__int_handler handler, uint32_t priority)
{
	// Define handler and configure interrupt with correct priority
	INTC_register_interrupt(handler,
							AVR32_GPIO_IRQ_0 + (GPIO_PUSH_BUTTON_1/8),
							priority);
	
	// Enable falling edge interrupt on Push Button 1
	gpio_enable_pin_interrupt(GPIO_PUSH_BUTTON_1, GPIO_FALLING_EDGE);
}


static void erm_tc_interrupt_init(__int_handler handler, uint32_t priority)
{
	// Define handler and configure interrupt with correct priority
	INTC_register_interrupt(handler, AVR32_TC_IRQ0, priority);
}


#endif /* INTERRUPTS_INIT_H_ */