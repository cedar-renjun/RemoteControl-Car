#include <ioCC2540.h>
#include "hal_types.h"
#include "ioCC254x_bitdef.h"
#include "PWM_Control.h"


static uint16_t __frequency;

static uint16_t __duty_100;
static uint16_t __ch_duty[4];

uint16 period = 4000 - 1;
uint16 ch1    = 0;
uint16 ch2    = 2000 - 1;
uint16 ch3    = 3000 - 1;
uint16 ch4    = 4000 - 1;

void PWM_init(void)
{
    // Timer Clock 4MHz
    CLKCONCMD &= 0xC3;

    // Ch1 -- P1.1
    // Ch2 -- P1.0
    // Ch3 -- P0.7
    // Ch4 -- P0.6
    P1DIR|= BV(0)|BV(1);
    P1SEL|= BV(0)|BV(1);
    P0DIR|= BV(6)|BV(7);
    P0SEL|= BV(6)|BV(7);

    PERCFG |= 0x40;

    // Initialize Timer
    T1CTL   = 0x04;
    T1CCTL1 = 0x3C; // Not used
    T1CCTL2 = 0x3C; // Not Used
    T1CCTL3 = 0x3C; // Not Used
    T1CCTL4 = 0x3C; // Not Used
    T1CNTL  = 0;    // Reset timer to 0;

    T1CCTL0 = 0x30;

    // PWM Frequency & Duty

    // Reconfigure 4-PWM Channel
    for(uint8_t i = 0; i < 4; i++)
    {
        PWM_SetPercent(i, __ch_duty[i]);
    }

    //T1CTL  |= 0x02;

    // Configure Interrupt
    //EA=1;
    //IEN1 |= 0x02;
}


#if 0
#pragma vector = T1_VECTOR
__interrupt void pwmISR (void)
{
    uint8 flag = T1STAT;

    /*
    if(T1STAT_OVFIF == (T1STAT_OVFIF & flag))
    {
        // TODO
    }

    if(T1STAT_CH4IF == (T1STAT_CH4IF & flag))
    {
        // TODO
    }

    if(T1STAT_CH3IF == (T1STAT_CH3IF & flag))
    {
        // TODO
    }

    if(T1STAT_CH2IF == (T1STAT_CH2IF & flag))
    {
        // TODO
    }

    if(T1STAT_CH1IF == (T1STAT_CH1IF & flag))
    {
        // TODO
    }

    if(T1STAT_CH0IF == (T1STAT_CH0IF & flag))
    {
        // TODO
    }
    */

    T1STAT = ~ flag;
}
#endif

void PWM_SetPercent(uint8_t ch, uint16_t duty)
{
    uint16_t value;
    uint32_t tmp;

    if(--ch > 4)
    {
        return;
    }

    if(duty > 1000)
    {
        duty = 1000;
    }
    __ch_duty[ch++] = duty;

    tmp = (uint32_t)duty;
    tmp *= (uint32_t)__duty_100;
    tmp /= 1000;
    if(tmp)
    {
        tmp -= 1;
    }
    else
    {
        duty = 0;
    }

    value = (uint16_t)tmp;

    if(1 == ch)
    {
        T1CC1L = value%256;
        T1CC1H = value/256;

        if(duty == 0)
        {
            T1CCTL1 = 0x0C;
        }
        else if(duty == 1000)
        {
            T1CCTL1 = 0x04;
        }
        else
        {
            T1CCTL1 = 0x34;
        }
    }

    if(2 == ch)
    {
        T1CC2L = value%256;
        T1CC2H = value/256;

        if(duty == 0)
        {
            T1CCTL2 = 0x0C;
        }
        else if(duty == 1000)
        {
            T1CCTL2 = 0x04;
        }
        else
        {
            T1CCTL2 = 0x34;
        }
    }

    if(3 == ch)
    {
        T1CC3L = value%256;
        T1CC3H = value/256;

        if(duty == 0)
        {
            T1CCTL3 = 0x0C;
        }
        else if(duty == 1000)
        {
            T1CCTL3 = 0x04;
        }
        else
        {
            T1CCTL3 = 0x34;
        }
    }

    if(4 == ch)
    {
        T1CC4L = value%256;
        T1CC4H = value/256;

        if(duty == 0)
        {
            T1CCTL4 = 0x0C;
        }
        else if(duty == 1000)
        {
            T1CCTL4 = 0x04;
        }
        else
        {
            T1CCTL4 = 0x34;
        }
    }
}

uint16_t PWM_GetPercent(uint8_t ch, uint16_t* duty)
{
    if(--ch > 3)
    {
        return 0;
    }

    return __ch_duty[ch];
}

void PWM_SetFrequency(uint16 frequency)
{
    uint16_t value;

    // Frequency 62Hz~8KHz
    if(frequency < 62 || frequency > 8000)
    {
        return;
    }

    // 4MHz/Counter
    __duty_100 = 4000000/((uint32_t)frequency);
    value      = __duty_100 - 1;
    __frequency = frequency;

    T1CTL  &= ~T1CTL_MODE;
    T1CNTL  = 0;           // Reset timer counter

    // Configure PWM Frequency
    T1CC0L  = value%256;
    T1CC0H  = value/256;

    // Reconfigure 4-PWM Channel
    for(uint8_t i = 0; i < 4; i++)
    {
        PWM_SetPercent(i, __ch_duty[i]);
    }

    T1CTL |= T1CTL_MODE_MODULO;
}

uint16_t PWM_GetFrequency(void)
{
    return __frequency;
}


