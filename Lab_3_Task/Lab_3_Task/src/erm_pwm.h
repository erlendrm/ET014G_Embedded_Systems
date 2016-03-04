/*
 * erm_pwm.h
 *
 * Created: 03.03.2016 18:14:04
 *  Author: Erlend
 */ 


#ifndef ERM_PWM_H_
#define ERM_PWM_H_

#define ERM_PWM_PIN			AVR32_PWM_3_PIN
#define ERM_PWM_FUNCTION	AVR32_PWM_3_FUNCTION
#define ERM_PWM_CHANNEL_ID	3


static void erm_pwm_init(uint32_t duty_cycle, uint32_t freq, uint32_t pba_clk)
{
	uint32_t period;
	uint32_t cycle;
	
	switch (duty_cycle)
	{
		case 0:
		period	= 1;
		cycle	= 1;
		break;
		
		case 100:
		period	= 1;
		cycle	= 0;
		break;
		
		default:
		period	= pba_clk / freq;
		cycle	= period / (100 / (100 - duty_cycle));
		break;
	}
	
	
	
	/* PWM controller configuration. */
	pwm_opt_t pwm_opt ={
		.diva = AVR32_PWM_DIVA_CLK_OFF,
		.divb = AVR32_PWM_DIVB_CLK_OFF,
		.prea = AVR32_PWM_PREA_MCK,
		.preb = AVR32_PWM_PREB_MCK
	};
	
	/* PWM channel configuration structure. */
	avr32_pwm_channel_t pwm_channel ={
		.ccnt		= 0,							// Channel counter
		.cdty		= cycle,						// Channel duty cycle
		.cprd		= period,						// Channel period
		.cupd		= 0								// Channel update
	};
	
	pwm_channel.CMR.calg	= PWM_MODE_LEFT_ALIGNED;			// CMR mode
	pwm_channel.CMR.cpol	= PWM_POLARITY_LOW;					// CMR polarity
	pwm_channel.CMR.cpd		= PWM_UPDATE_DUTY;					// CMR update
	pwm_channel.CMR.cpre	= AVR32_PWM_CPRE_MCK;				// CMR prescaler
	
	/* Enable the alternative mode of the output pin to connect it to the PWM
	 * module within the device. */
	gpio_enable_module_pin(ERM_PWM_PIN, ERM_PWM_FUNCTION);

	/* Initialize the PWM module. */
	pwm_init(&pwm_opt);

	/* Set channel configuration to channel 0. */
	pwm_channel_init(ERM_PWM_CHANNEL_ID, &pwm_channel);

	/* Start channel 0. */
	pwm_start_channels(1 << ERM_PWM_CHANNEL_ID);
}


#endif /* ERM_PWM_H_ */