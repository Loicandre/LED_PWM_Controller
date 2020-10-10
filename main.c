////////////////////////////////////////////////////////////////////////////////
// LED_PWM_Controller                                                         //
// Platform : PIC12F1572                                                      //
// Compiler : XC 2.10, IDE : MPLAB X V5.35, PicKit3                           //
//                                                                            //
// Loic Andre                                                                 //
// Creative Commons CC-BY-NC                                                  //
////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <xc.h>

// CONFIG1
#pragma config FOSC = INTOSC    //  (INTOSC oscillator; I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is digital input)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable (Brown-out Reset disabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = OFF      // PLL Enable (4x PLL disabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will not cause a Reset)
#pragma config BORV = HI        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), high trip point selected.)
#pragma config LPBOREN = OFF    // Low Power Brown-out Reset enable bit (LPBOR is disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

// IO definition :
// RA0  |   NC, PGD
// RA1  |   ADC_VIN, PGC
// RA2  |   PWM_OUT
// RA4  |   ADC_POT
// RA5  |   RX_IN

#define _ADDR 1 

#define _VIN_UNDERVOLTAGE_CUTOFF 1000 // with error correction >  10.25 V
#define _VIN_UNDERVOLTAGE_MIN 1066    // with error correction >  11.00 V
#define _VIN_UNDERVOLTAGE_MAX 1122    // with error correction >  11.50 V
#define _PMAXOFFSET 20 // Potentiometer max offset
#define _PMINOFFSET 90

#define ADC_CHANNEL_POT 0
#define ADC_CHANNEL_VIN 1

char LOW_VOLTAGE =0;
char UPDATE_JOB =0;
char UART_MODE =0;
int UART_MODE_last_POT =0;

long VIN_Value=0;
long VIN_Value_filter=_VIN_UNDERVOLTAGE_MIN;
long LED_Value=0;
int  POT_Value=0;
long POT_Value_filter=1024;
    
int RX_VALUE =0;
unsigned char RX_CHEKSUM =0;
int RX_VALUE_temp =0;
char RX_DECODE_SEQ =0;

int ADC_result(unsigned char channel)
{
    if(channel == ADC_CHANNEL_POT){
        ADCON0bits.CHS=0b11;            // Analog channel RA4 (POT)
    }
    else if(channel == ADC_CHANNEL_VIN){
        ADCON0bits.CHS=0b01;            // Analog channel RA1 (VIN)
    }
    ADCON0bits.GO=1;                // ADC start conversion
    while(ADCON0bits.GO);           // wait for conversion done
    return ADRES;
}

void PWM()
{
    //PWM3CLKCON = 0b00000001;          // Set HFINTOSC clock, no prescaler.
    PWM3CLKCON = 0b00000000;          // Set FOSC clock, no prescaler.
    PWM3CON = 0b01100000;             // PWM control (standard mode).
    PWM3PH = 0;                       // Phase.
    PWM3PR = 1400;                    // Period. (ADC is max 1024, 13.6v-12v = 1.33 * 1024=1160 -> Duty cycle max is 88%)
    PWM3DC =0;
    PWMLDbits.PWM3LDA_A=1; // trig new value
    PWMENbits.PWM3EN_A=1;
}

void ADC_set()
{
    FVRCONbits.FVREN = 1;           // enable FVR (fixed VReference module)
    FVRCONbits.ADFVR = 0b10;        // ADC FVR gain is 2 --> 2x1.024V
    
    ANSELAbits.ANSA4=1;             // Analog input on PIN RA4 // ADC_POT
    ANSELAbits.ANSA1=1;             // Analog input on PIN RA1 // ADC_VIN 
    
    ADCON1bits.ADFM=1;              // ADC set to ADRESL
    ADCON0bits.ADON=1;              // ADC enabled
    ADCON1bits.ADCS=0b11;           // ADC conversion clock select as FRC
    ADCON1bits.ADPREF=0b11;         // ADC Positive VREF is FVR
}

void Timer2init()
{
    T2CONbits.T2CKPS = 0b10;   // 16bits prescaler
    T2CONbits.T2OUTPS = 0b0000; // no postscaler
    
    PR2 = 250; // --> 32'000'000Mhz / 4 / prescaler (16) / PR2 (250)  = 2Khz
    TMR2 = 0;
    T2CONbits.TMR2ON = 1; // Enable timer
    PIR1bits.TMR2IF = 0; // Reset timer 2 interrupt
    PIE1bits.TMR2IE = 1; // Enable timer 2 interrupt
}

void UARTinit()
{
    APFCONbits.RXDTSEL = 1;
    RCSTA = 0;
    
    TXSTAbits.BRGH = 0;   // High Speed Baud Rate Select bit ( 1 = High)( 0 = low)
    BAUDCONbits.BRG16 = 0; // 16-Bit BRG register Enable bit ( 1 = 16bit)( 0 = 8bit)
    SPBRG  = 51; // = ((FOSC/Desired Baud Rate)/64) ? 1 = (32000000/9600)/64 - 1
    SPBRGH = 0;
    
    RCSTAbits.CREN = 1;   // Enable Receiver ( 1 = ON )
    RCSTAbits.SPEN = 1;   // Enable Receiver ( 1 = ON )
    PIR1bits.RCIF = 0;
    PIE1bits.RCIE = 0;  // Interrupt on receipt ( 1 = ON)
}

void main(void) 
{
   
    // init GPIO
    TRISAbits.TRISA0=1;             // I/O RA0 Input    // NC
    WPUAbits.WPUA0=1;               // RA0 Input pull-up
    TRISAbits.TRISA1=1;             // I/O RA1 Input    // ADC_VIN
    TRISAbits.TRISA2=0;             // I/O RA2 Ouptut   // PWM_OUT
    PORTAbits.RA2=0;                // SET PWM_OUT to 0b
    TRISAbits.TRISA4=1;             // I/O RA4 Input    // ADC_POT
    TRISAbits.TRISA5=1;             // I/O RA5 Input    // RX_IN
    WPUAbits.WPUA5=1;               // RA5 Input pull-up
    
    //other init
    CM1CON0bits.C1ON=0;             // Disable Comaprator
    //OSCCON = 0b01111010;           // 16 Mhz oscillator.
    OSCCON = 0b11110000;            // 8x4pll > 32Mhz oscillator.
    INTCON=0;
    INTCONbits.GIE =1; // global interrupt enable
    INTCONbits.PEIE =1; // Peripherals interrupt enable
    
    //init
    ADC_set();
    PWM();
    Timer2init();
    UARTinit();
    
    // wait 100 ms to be sure ( 2Khz / 200 => 100ms)
    while(UPDATE_JOB >= 200){ 
        UPDATE_JOB == 0;
    }
    
    while(1)
    {
        if(UPDATE_JOB > 10) //  2kHz / 10 = 200 Hz
        {
            // VIN Measure and filter
            VIN_Value_filter = ((VIN_Value_filter *31) + ADC_result(ADC_CHANNEL_VIN) + ADC_result(ADC_CHANNEL_VIN)) / 33;
            VIN_Value = (((VIN_Value_filter*2) * 725) / 1000); // VIN_Value = 1024 * 2(=2048v) * (14.85v/2.048v)->7.25->725
            
            // Undervoltage Tests
            if(VIN_Value <= _VIN_UNDERVOLTAGE_CUTOFF){ // 10.250V
                //INTCONbits.GIE =0; // global interrupt disable
                //INTCONbits.PEIE =0; // Peripherals interrupt disable
                //PORTAbits.RA2=0;                // SET PWM_OUT to 0ff
                //SLEEP(); // no wakeup possible than powercycle / reset
                LOW_VOLTAGE = 2;
            }
            else  if(VIN_Value <= _VIN_UNDERVOLTAGE_MIN){ // 11.00V
                LOW_VOLTAGE = 1;
            }
            else if(VIN_Value >= _VIN_UNDERVOLTAGE_MAX){ // 11.5V
                LOW_VOLTAGE = 0;
            }
            
                
            // POT Measure and filter
            POT_Value= (long)((((long)POT_Value_filter *62) + ADC_result(ADC_CHANNEL_POT) + ADC_result(ADC_CHANNEL_POT)) / 64);
            POT_Value_filter = POT_Value;
            
            // POT remap and filter
            POT_Value = 1024 - (long)POT_Value - (long)_PMAXOFFSET;    // return Value because POTmax = 0 and POTmin = ~1024
            if(POT_Value < _PMINOFFSET)       // make sure than no minus value is there (due to _PMAXOFFSET)
                POT_Value = _PMINOFFSET;

            
            // UART MODE (UART is prioritary ! To quit uart mode, stop UART frames, and turn the potentiometer)
            if(UART_MODE == 1) 
            {
                int calc = UART_MODE_last_POT;

                calc -= POT_Value ;
                if ((calc > 50) ||(calc < -50))
                {
                    UART_MODE =0; // Potentiometer has moved, quit UART mode
                }
                else{
                    POT_Value = RX_VALUE;   
                }
            }
            else
            {
                UART_MODE_last_POT = POT_Value;
            }

            // y=f(x)=(x^2)/2048 Exponential remap for better illumination control
            LED_Value = POT_Value;
            LED_Value = LED_Value*LED_Value;
            LED_Value = (LED_Value >> 10);
            
            
            //in case of LOW_VOLTAGE cutoff, PWM = off
            if(LOW_VOLTAGE == 2){
                PWM3DC = 0;
            }
            //in case of LOW_VOLTAGE, PWM = 7% max
            else if(LOW_VOLTAGE == 1){
                if(LED_Value <= 100){
                    PWM3DC = LED_Value;
                }
                else{
                    PWM3DC = 100;
                }
            }
            else{
                PWM3DC = LED_Value; // normal condition
            }
            
            PWMLDbits.PWM3LDA_A=1; // trig new value
            UPDATE_JOB =0;
        }
    }
}

void __interrupt() ISR()
{    
    if (PIR1bits.TMR2IF){
        // interruption every 2kHz
        UPDATE_JOB ++;
        
        PIR1bits.TMR2IF = 0; // reset interrupt flag
    } 
    
    if(PIR1bits.RCIF){
        if((RX_DECODE_SEQ == 0) && (RCREG == 0xCA)){
            RX_CHEKSUM = RCREG;
            RX_DECODE_SEQ =1;
        }
        else if((RX_DECODE_SEQ == 1) && (RCREG == 0xFE)){
            RX_CHEKSUM += RCREG;
            RX_DECODE_SEQ =2;
        }
        else if((RX_DECODE_SEQ == 2) && ((RCREG == _ADDR) || (RCREG == 0xFF))){
            RX_CHEKSUM += RCREG;
            RX_DECODE_SEQ =3;
        }
        else if(RX_DECODE_SEQ == 3){
            RX_CHEKSUM += RCREG;
            RX_VALUE_temp = RCREG << 8;
            RX_DECODE_SEQ =4;
        }
        else if(RX_DECODE_SEQ == 4){
            RX_CHEKSUM += RCREG;
            RX_VALUE_temp = RX_VALUE_temp | RCREG;
            RX_DECODE_SEQ =5;
        }
        else if((RX_DECODE_SEQ == 5) && (RX_CHEKSUM == RCREG)){
            if(RX_VALUE_temp > 1023){
                RX_VALUE_temp = 1023;
            }
            RX_VALUE = RX_VALUE_temp;   
            UART_MODE = 1;
            RX_DECODE_SEQ =0;
        }
        else{ 
            RX_DECODE_SEQ =0;
        }
        
        PIR1bits.RCIF  =0; // reset interrupt flag
    }
}

