/*
 * erm_tc.h
 *
 * Created: 03.03.2016 15:14:23
 *  Author: Erlend
 */ 


#ifndef TC_INIT_H_
#define TC_INIT_H_

#define ERM_TC						(&AVR32_TC)
#define ERM_TC_CHANNEL_ID			0
#define ERM_TC_CHANNEL_PIN			AVR32_TC_A0_0_0_PIN
#define ERM_TC_CHANNEL_FUNCTION		AVR32_TC_A0_0_0_FUNCTION

static void erm_tc_init(volatile avr32_tc_t *tc, uint32_t tick_ms, uint32_t pba_clk)
{
	float		temp;
	uint16_t	RC;
	
	// Defining waveform generation options.
	tc_waveform_opt_t waveform_opt =
	{
		.channel  = ERM_TC_CHANNEL_ID,					// Channel selection.

		.bswtrg   = TC_EVT_EFFECT_NOOP,					// Software trigger effect on TIOB.
		.beevt    = TC_EVT_EFFECT_NOOP,					// External event effect on TIOB.
		.bcpc     = TC_EVT_EFFECT_NOOP,					// RC compare effect on TIOB.
		.bcpb     = TC_EVT_EFFECT_NOOP,					// RB compare effect on TIOB.

		.aswtrg   = TC_EVT_EFFECT_NOOP,					// Software trigger effect on TIOA.
		.aeevt    = TC_EVT_EFFECT_NOOP,					// External event effect on TIOA.
		.acpc     = TC_EVT_EFFECT_NOOP,					// RC compare effect on TIOA: toggle.
		.acpa     = TC_EVT_EFFECT_NOOP,					// RA compare effect on TIOA: toggle (other possibilities are none, set and clear).

		.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,	// Waveform selection: Up mode without automatic trigger on RC compare.
		.enetrg   = false,								// External event trigger enable.
		.eevt     = 0,									// External event selection.
		.eevtedg  = TC_SEL_NO_EDGE,						// External event edge selection.
		.cpcdis   = false,								// Counter disable when RC compare.
		.cpcstop  = false,								// Counter clock stopped with RC compare.

		.burst    = false,								// Burst signal selection.
		.clki     = false,								// Clock inversion.
		.tcclks   = TC_CLOCK_SOURCE_TC4					// Internal source clock 3, connected to fPBA / 32.
	};
	
	// Defining TC interrupt options
	static const tc_interrupt_t tc_interrupt = {
		.etrgs = 0,
		.ldrbs = 0,
		.ldras = 0,
		.cpcs  = 1, // Enable interrupt on RC compare alone
		.cpbs  = 0,
		.cpas  = 0,
		.lovrs = 0,
		.covfs = 0
	};
	
	// Initialize the timer/counter.
	tc_init_waveform(tc, &waveform_opt);
	
	// Internal clock source 4 is active, hence tc_clk = pba_clk / 32
	// Determine value of RC based on desired tick_time_ms
	temp = ( ((float) pba_clk) * ((float) tick_ms) ) / 32000.0;
	RC = (uint16_t) temp;
	
	// Set RC value
	tc_write_rc(tc, ERM_TC_CHANNEL_ID, RC);
	
	// Configure the timer interrupt
	tc_configure_interrupts(tc, ERM_TC_CHANNEL_ID, &tc_interrupt);
	
}

static void erm_tc_start(volatile avr32_tc_t *tc)
{
	// Start the timer/counter.
	tc_start(tc, ERM_TC_CHANNEL_ID);
}


#endif /* TC_INIT_H_ */