/** DMA PWM 
 *  
 *  In this example we will use the DMA to generate a PWM sequence
 *  This can be common for things like Digital LEDs (WS8212), motor control, etc.
 * 
 *  For this we're going to use Daisy Seed pin D17 as TIM3 Channel 4 (unless it has issues because of the LED attached..)
 * 
 *  TODO: This won't work until this stuff is resolved:
 *  * Hardcoding for TIM3Ch4 is in the libDaisy stuff. Needs to get updated so we can use TIM4 Ch1 for the devboard
 *  * Some sort of stop/reset between transactions to prep LEDs for next signal
 */
#include "daisy_seed.h"

using namespace daisy;

/** Hardware object for communicating with Daisy */
DaisySeed                       hw;
const size_t                    kOutBufferSize = 512;
uint32_t DMA_BUFFER_MEM_SECTION outbuffer[kOutBufferSize];

const int kOneTime  = 20;
const int kZeroTime = 8;

const int kNumLeds = 12;
uint8_t   led_data[kNumLeds][3]; /**< RGB data */

const size_t kOutDataSize = kNumLeds * 3 * 8;
uint32_t DMA_BUFFER_MEM_SECTION
    output_data[kNumLeds * 3
                * 8]; /**< PWM lengths data, one "duration" per bit */

/** buff expects that 8 elements are available for the one 8-bit color val*/
static void populate_bits(uint8_t color_val, uint32_t *buff)
{
    for(int i = 0; i < 8; i++)
    {
        buff[i] = (color_val & i) > 0 ? kOneTime : kZeroTime;
    }
}

static void fill_led_data()
{
    /** TODO fix these to be accurate for necessary timing */
    for(int i = 0; i < kNumLeds; i++)
    {
        /** Grab G, R, B for filling bytes */
        uint8_t g          = led_data[i][1];
        uint8_t r          = led_data[i][0];
        uint8_t b          = led_data[i][2];
        auto    data_index = i * 3 * 8;
        populate_bits(g, &output_data[data_index]);
        populate_bits(r, &output_data[data_index + 8]);
        populate_bits(b, &output_data[data_index + 16]);
    }
}

static void set_led(int index, uint8_t r, uint8_t g, uint8_t b)
{
    led_data[index][0] = r;
    led_data[index][1] = g;
    led_data[index][2] = b;
}

static void set_led_f(int index, float r, float g, float b)
{
    set_led(index, r * 255, g * 255, b * 255);
}

int main(void)
{
    /** Initialize hardware */
    hw.Init();

    /** Initialize timer */
    TimerHandle::Config tim_cfg;
    TimerHandle         timer;
    tim_cfg.periph = TimerHandle::Config::Peripheral::TIM_4;
    timer.Init(tim_cfg);

    /** Generate period for timer 
     *  This is a marvelously useful little tidbit that should be put into a TimerHandle function or something.
     */
    uint32_t prescaler         = 8;
    uint32_t tickspeed         = (System::GetPClk2Freq() * 2) / prescaler;
    uint32_t target_pulse_freq = 833333; /**< 1.2 microsecond symbol length */
    uint32_t period            = (tickspeed / target_pulse_freq) - 1;
    timer.SetPrescaler(prescaler - 1); /**< ps=0 is divide by 1 and so on.*/
    timer.SetPeriod(period);

    TimChannel::Config chn_cfg;
    chn_cfg.tim  = &timer;
    chn_cfg.chn  = TimChannel::Config::Channel::ONE;
    chn_cfg.mode = TimChannel::Config::Mode::PWM;
    chn_cfg.pin  = seed::D13;
    TimChannel pwm;
    /** Fill Buffer */
    for(size_t i = 0; i < kOutBufferSize; i++)
    {
        float t      = (float)i / (float)(kOutBufferSize - 1); /**< 0.0->1.0 */
        float ts     = 0.5f + (cos(t * 6.28) * 0.5f);
        outbuffer[i] = (uint32_t)(ts * period);
    }
    /** Initialize PWM */
    pwm.Init(chn_cfg);
    timer.Start();
    //pwm.Start();
    // System::Delay(1000);
    // pwm.StartDma(outbuffer, kOutBufferSize, nullptr);
    // bool led_state = true;

    uint32_t now, tled;
    now = tled = System::GetNow();

    while(1)
    {
        // System::Delay(2000);
        // hw.SetLed(led_state);
        // if(led_state)
        //     led_state = false;
        // else
        //     led_state = true;
        // pwm.StartDma(outbuffer, kOutBufferSize, nullptr);

        now = System::GetNow();
        if(now - tled > 33)
        {
            tled = now;

            /* Lets set some LED stuff */
            for(int i = 0; i < kNumLeds; i++)
            {
                float bright = (float)(now & 1023) / 1023.f;
                set_led_f(i, bright, bright, bright);
            }
            fill_led_data();

            /** And transmit */
            pwm.Start();
            pwm.StartDma(output_data, kOutDataSize, nullptr);
            /** when its done..... 
             *  we need to set pwm to 0% or pull low for >=80us
             *  this is hacky, gross, and would be the perfect thing to setup in the callback...
             * 
             *  It should _never_ take more than 1ms nevermind 3..
             *  You'd need at least 36 LEDs for it to be 1ms of transmission.
             */
            System::Delay(3);
            pwm.Stop();
        }
    }
}