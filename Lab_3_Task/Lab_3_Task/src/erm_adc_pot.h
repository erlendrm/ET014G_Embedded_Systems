/*
 * erm_adc_pot.h
 *
 * Created: 03.03.2016 17:30:19
 *  Author: Erlend
 */ 


#ifndef ADC_POT_H_
#define ADC_POT_H_

static void adc_pot_init(void)
{
	// Define Potentiometer GIPO options
	const gpio_map_t ADC_GPIO_MAP ={
		{ADC_POTENTIOMETER_PIN, ADC_POTENTIOMETER_FUNCTION}
	};
	
	// Assign I/O
	gpio_enable_module(ADC_GPIO_MAP, 
	sizeof(ADC_GPIO_MAP) / sizeof(ADC_GPIO_MAP[0]));
	
	// Configure the ADC peripheral module.
	// Lower the ADC clock to match the ADC characteristics (because we
	// configured the CPU clock to 12MHz, and the ADC clock characteristics are
	// usually lower; cf. the ADC Characteristic section in the datasheet).
	AVR32_ADC.mr |= 0x1 << AVR32_ADC_MR_PRESCAL_OFFSET;
	adc_configure(&AVR32_ADC);
	
	// Enable ADC
	adc_enable(&AVR32_ADC, ADC_POTENTIOMETER_CHANNEL);
}


#endif /* ADC_POT_H_ */