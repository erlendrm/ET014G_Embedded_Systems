/*
 * erm_cdc.h
 *
 * Created: 07.03.2016 19:01:46
 *  Author: Erlend
 */ 


#ifndef ERM_CDC_H_
#define ERM_CDC_H_

static void erm_cdc_print(char *input_string)
{
	while (*input_string)
	{
		udi_cdc_putc(*input_string);
		*input_string++;
	}
}


static void erm_cdc_println(char *input_string)
{
	while (*input_string)
	{
		udi_cdc_putc(*input_string);
		*input_string++;
	}
	udi_cdc_putc('\r');
	udi_cdc_putc('\n');
}



#endif /* ERM_CDC_H_ */