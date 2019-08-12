#include <application.h>
#include "Adafruit_ZeroFFT.h"

#include "bc_adc_dma.h"
#include "bc_adc.h"


#define APPLICATION_TASK_ID 0

/*
If no PLL is NOT enabled, then the core clock is 2 MHz
Prescaler:
32 MHz / 1000 = 32 kHz = 32000 Hz
Autoreload:
32000 Hz / 4 = 8000 Hz
*/
#define ADC_DMA_8KHZ_PRESCALER 1000
#define ADC_DMA_8KHZ_AUTORELOAD 4

//the signal in signal.h has 2048 samples. Set this to a value between 16 and 2048 inclusive.
//this must be a power of 2
#define DATA_SIZE 512

//the sample rate
#define FS 8000

uint8_t buffer[512];

q15_t signal[512];

int sampleCount;

int freqBand[8];

bc_gfx_t *pgfx;

void fillTheDisplay();

void byte_to_int16(uint8_t *src, int16_t *dst, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        dst[i] = src[i] << 8;
    }
}

void calculate_fft()
{
    int i = 1;


    ZeroFFT(signal, DATA_SIZE);

    for(int j = 0; j < 8; j++)
    {
        int count = 0;
        float average = 0;
        sampleCount = (pow(2, j) * 2);
        for(; i <= sampleCount; i++)
        {
            bc_log_debug("%.1f Hz: %d", FFT_BIN(i, FS, DATA_SIZE), signal[i]);
            //bc_log_debug("cislo: %d", signal[i]);
            average += signal[i];
            count++;
        }
        bc_log_debug("----------");
        average /= count;
        freqBand[j] = (int)average;
        //bc_log_debug("průměr: %d", freqBand[j]);
    }
}


static void adc_dma_event_handler(bc_dma_channel_t channel, bc_dma_event_t event, void *event_param)
{
    (void) event_param;
    (void) channel;

   /* if (event == BC_DMA_EVENT_HALF_DONE)
    {

        byte_to_int16(&buffer[0], signal, sizeof(buffer) / 2);
        calculate_fft();
        bc_scheduler_plan_now(APPLICATION_TASK_ID);
    }
    else*/ if (event == BC_DMA_EVENT_DONE)
    {
        byte_to_int16(buffer, signal, sizeof(buffer));
        calculate_fft();
        bc_scheduler_plan_now(APPLICATION_TASK_ID);

        bc_adc_dma_stop();
        bc_adc_dma_start(BC_ADC_CHANNEL_A0, ADC_DMA_8KHZ_PRESCALER, ADC_DMA_8KHZ_AUTORELOAD);
    }
}


void application_init(void)
{
    bc_log_init(BC_LOG_LEVEL_DUMP, BC_LOG_TIMESTAMP_ABS);

    bc_system_pll_enable();

    // Initialize logging

    bc_module_lcd_init();
    pgfx = bc_module_lcd_get_gfx();

    bc_gfx_set_font(pgfx, &bc_font_ubuntu_13);

    bc_adc_dma_init(buffer, sizeof(buffer));
    bc_adc_dma_set_event_handler(adc_dma_event_handler, NULL);
    //bc_adc_oversampling_set(BC_ADC_CHANNEL_A0, BC_ADC_OVERSAMPLING_8);

    // Start circular ADC sampling on P0/A0 to the buffer
    bc_adc_dma_start(BC_ADC_CHANNEL_A0, ADC_DMA_8KHZ_PRESCALER, ADC_DMA_8KHZ_AUTORELOAD);

    bc_scheduler_plan_from_now(APPLICATION_TASK_ID, BC_TICK_INFINITY);

}


void application_task(void)
{
    int lastPos = 0;

    if (!bc_gfx_display_is_ready(pgfx))
    {
        bc_scheduler_plan_from_now(APPLICATION_TASK_ID, 2000);
        return;
    }

    bc_system_pll_enable();
    bc_gfx_clear(pgfx);

    for(int j = 0; j < 8; j++)
    {
        if(freqBand[j] > 128)
        {
            freqBand[j] = 127;
        }
        bc_gfx_draw_rectangle(pgfx, lastPos, 128, (lastPos+14), (128 - (log_lookup[freqBand[j]])), true);
        lastPos+=16;
    }
    bc_log_debug("konec");
    bc_gfx_update(pgfx);
    bc_system_pll_disable();

    bc_scheduler_plan_current_relative(100);
}

