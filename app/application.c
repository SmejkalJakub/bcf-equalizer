#include <application.h>
#include "Adafruit_ZeroFFT.h"
#include "signal.h"
#include "time.h"

#define APPLICATION_TASK_ID 0


//the signal in signal.h has 2048 samples. Set this to a value between 16 and 2048 inclusive.
//this must be a power of 2
#define DATA_SIZE 512

//the sample rate
#define FS 8000

int sampleCount;

int freqBand[8];

bc_gfx_t *pgfx;

uint16_t endTick;


void generateRandomSignal();
void fillTheDisplay();

void lcd_button_right_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) event_param;

	if (event == BC_BUTTON_EVENT_CLICK)
	{
        generateRandomSignal(25);
        freqBand[3] = rand()%128;
        freqBand[2] = rand()%128;
        bc_scheduler_plan_from_now(APPLICATION_TASK_ID, 500);
    }
}

void generateRandomSignal(int randomNumber)
{
    int i = 0;
    int randSign = 0;

    /*for(int j = 0; j < 1400; j++)
    {
        srand(randomNumber);
        signal[j] = (q15_t)rand() % 21000;
        randSign = rand() % 2;
        if(randSign)
        {
            signal[j] *= -1;
        }
    }*/

    ZeroFFT(signal, DATA_SIZE);

    for(int j = 0; j < 8; j++)
    {
        int count = 0;
        float average = 0;
        sampleCount = (pow(2, j) * 2);
        for(; i < sampleCount; i++)
        {
            bc_log_debug("%.1f Hz: %d", FFT_BIN(i, FS, DATA_SIZE), signal[i]);
            //bc_log_debug("cislo: %d", signal[i]);
            average += signal[i];
            count++;;
        }
        bc_log_debug("----------");
        average /= count;
        freqBand[j] = (int)average;
        //bc_log_debug("průměr: %d", freqBand[j]);
    }
}


void application_init(void)
{
    bc_log_init(BC_LOG_LEVEL_DUMP, BC_LOG_TIMESTAMP_ABS);
    
    // Initialize logging

    bc_module_lcd_init();
    pgfx = bc_module_lcd_get_gfx();

    bc_gfx_clear(pgfx);
    bc_gfx_set_font(pgfx, &bc_font_ubuntu_13);
    bc_timer_start();

    generateRandomSignal(30);

    bc_timer_stop();
    endTick = bc_timer_get_microseconds();

    bc_scheduler_plan_from_now(APPLICATION_TASK_ID, 500);

    // Initialize LCD button right
    static bc_button_t lcd_right;
    bc_button_init_virtual(&lcd_right, BC_MODULE_LCD_BUTTON_RIGHT, bc_module_lcd_get_button_driver(), false);
    bc_button_set_event_handler(&lcd_right, lcd_button_right_event_handler, NULL);
  
}


void application_task(void)
{
    int lastPos = 0;
    bc_gfx_clear(pgfx);
    if (!bc_gfx_display_is_ready(pgfx))
    {
        bc_scheduler_plan_from_now(APPLICATION_TASK_ID, 2000);
        return;
    }
    bc_system_pll_enable();

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
}

