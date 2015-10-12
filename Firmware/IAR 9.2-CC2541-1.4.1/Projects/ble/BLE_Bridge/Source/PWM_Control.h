#ifndef _PWM_CONTROL_H_
#define _PWM_CONTROL_H_

#ifndef BV
#define BV(n)      (1 << (n))
#endif

// PWM ¾«¶ÈÅäÖÃ
#define PWM_PRECISION     255


typedef uint8  uint8_t;
typedef int8   int8_t;
typedef uint16 uint16_t;
typedef int16  int16_t;
typedef uint32 uint32_t;
typedef int32  int32_t;

/*-------------------------EXPORT APIs-----------------------------------------*/
#define M_PWM_INIT(fre)             do{PWM_init();PWM_SetFrequency(fre);}while(0)
#define M_PWM_SET_FREQUENCY(fre)    do{PWM_SetFrequency(fre);}while(0)
#define M_PWM_SET_DUTY(ch, duty)    do{                    \
uint16_t _duty = ((float)duty/(float)PWM_PRECISION)*1000.0;\
PWM_SetPercent(ch, _duty);                                 \
}while(0)

/*--------------------------EXAMPLES-------------------------------------------
    M_PWM_INIT(4000);         // Set PWM Frequency 4KHz
    M_PWM_SET_DUTY(1, 25);    // Ch1 duty ---- 10% * 256 = 25
    M_PWM_SET_DUTY(2, 51);    // Ch2 duty ---- 20% * 256 = 51
    M_PWM_SET_DUTY(3, 76);    // Ch3 duty ---- 30% * 256 = 76
    M_PWM_SET_DUTY(4, 102);   // Ch4 duty ---- 40% * 256 = 102
-*----------------------------------------------------------------------------*/

extern void PWM_init(void);
extern void PWM_SetPercent(uint8_t ch, uint16_t duty);
extern uint16_t PWM_GetPercent(uint8_t ch, uint16_t* duty);
extern void PWM_SetFrequency(uint16 frequency);
extern uint16_t PWM_GetFrequency(void);

#endif // _PWM_CONTROL_H_