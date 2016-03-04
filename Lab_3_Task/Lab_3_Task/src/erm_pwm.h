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


pwm_opt_t			erm_pwm_opt;
avr32_pwm_channel_t erm_pwm_channel;


static void erm_pwm_init(uint32_t duty_cycle, uint32_t freq, uint32_t pba_clk)
{
	float		temp;
	uint32_t	period;
	uint32_t	cycle;
	
	temp	= ((float) pba_clk) / ((float)freq);
	period	= (uint32_t) temp;
	
	switch (duty_cycle)
	{
		case 0:
		cycle	= period - 1;
		break;
		
		case 100:
		cycle	= 1;
		break;
		
		default:
		temp	= ((float) period) / (100.0 / (100.0 - ((float)duty_cycle)));
		cycle	= (uint32_t) temp;
		break;
	}
	
	// Define the PWM controller options
	erm_pwm_opt.diva = AVR32_PWM_DIVA_CLK_OFF;
	erm_pwm_opt.divb = AVR32_PWM_DIVB_CLK_OFF;
	erm_pwm_opt.prea = AVR32_PWM_PREA_MCK;
	erm_pwm_opt.preb = AVR32_PWM_PREB_MCK;
	
	// Define the PWM channel options
	erm_pwm_channel.cdty		= cycle;						// Channel duty cycle
	erm_pwm_channel.cprd		= period;						// Channel period
	erm_pwm_channel.cupd		= 0;							// Channel update
	erm_pwm_channel.CMR.calg	= PWM_MODE_LEFT_ALIGNED;		// CMR mode
	erm_pwm_channel.CMR.cpol	= PWM_POLARITY_LOW;				// CMR polarity
	erm_pwm_channel.CMR.cpd		= PWM_UPDATE_DUTY;				// CMR update
	erm_pwm_channel.CMR.cpre	= AVR32_PWM_CPRE_MCK;			// CMR prescaler
	
	// Enable GPIO with output on pin PB22
	gpio_enable_module_pin(ERM_PWM_PIN, ERM_PWM_FUNCTION);

	// Initialize the PWM module
	pwm_init(&erm_pwm_opt);

	// Configure the PWM channel
	pwm_channel_init(ERM_PWM_CHANNEL_ID, &erm_pwm_channel);

	// Start the PWM
	pwm_start_channels(1 << ERM_PWM_CHANNEL_ID);
}

static void erm_pwm_update_config(uint32_t duty_cycle, uint32_t freq, uint32_t pba_clk, uint32_t duty_or_freq)
{
	float		temp;
	uint32_t	period;
	uint32_t	cycle;
	
	temp	= ((float) pba_clk) / ((float)freq);
	period	= (uint32_t) temp;
	
	switch (duty_cycle)
	{
		case 0:
		cycle	= period - 1;
		break;
		
		case 100:
		cycle	= 1;
		break;
		
		default:
		temp	= ((float) period) / (100.0 / (100.0 - ((float)duty_cycle)));
		cycle	= (uint32_t) temp;
		break;
	}
	
	erm_pwm_channel.cprd	= period;
	erm_pwm_channel.cdty	= cycle;
	
	/*
	// Normally, it is favorable to use an update function that waits until the
	// end of a PWM period before applying the update when the user wants to change 
	// the frequency or the duty cycle of a running PWM. However, this function and 
	// the hardware it utilizes, is designed to only update one of these values at a time, 
	// causing unwanted behavior in my "real-time" application. That is why I am using 
	// a re-initialization function instead. It is capable of changing all parameters at
	// once, but unfortunately results in a slightly jerky PWM waveform because it applies
	// the changes immediately. The general PWM behavior is a lot better though. 
	
	// The code below is used to set update parameters for the aforementioned update function.
	// I have left this code in the program because I want it for future reference.    
	
	if (duty_or_freq == PWM_UPDATE_PERIOD)
	{
		// Re-configure the PWM channel
		// CPUD is set with the new frequency value and on the next waveform update
		// the contents of CPUD is loaded into CPRDx.
		erm_pwm_channel.cupd		= period;					// Load CPUD with update value
		erm_pwm_channel.CMR.cpd		= PWM_UPDATE_PERIOD;		// Define CPUD to be loaded into CPRDx
	}
	else if (duty_or_freq == PWM_UPDATE_DUTY)
	{
		// Re-configure the PWM channel
		// CPUD is set with the new duty cycle value and on the next waveform update
		// the contents of CPUD is loaded into CDTYx.
		erm_pwm_channel.cupd		= cycle;					// Load CPUD with update value
		erm_pwm_channel.CMR.cpd		= PWM_UPDATE_DUTY;			// Define CPUD to be loaded into CDTYx
	}
	*/
	
	
}

static void erm_pwm_update_channel(void)
{
	// Configure the PWM channel
	pwm_channel_init(ERM_PWM_CHANNEL_ID, &erm_pwm_channel);
	
	/*
	// The function below is the update function mentioned in the text above.
	// As mentioned, I have left it in because I want it for future reference.
	
	pwm_async_update_channel(ERM_PWM_CHANNEL_ID, &erm_pwm_channel);
	
	*/
}


#endif /* ERM_PWM_H_ */