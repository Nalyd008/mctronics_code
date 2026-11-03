/**
 * @file main.c
 * @author nalyd
 * @date 2026-10-05
 * @brief Main function
 */

#include <stdlib.h>
#include <xc.h>
#pragma config FNOSC = FRCDIV // 8MHz Fast RC with Postscaler

#define TRUE 1
#define FALSE 0
#define FORWARD 1
#define BACKWARD 0
#define LEFT 0
#define RIGHT 1

#define LEFT_LIGHT AN4


unsigned char timer1Done = 0;
unsigned char timer2Done = 0;

long double kp = 1;

double oscFreq;
int oscDiv;
int timerPrescaler;
int microSteps = 4;
int FULL_STEPS_PER_REVOLUTION = 200;

enum ROBOT_STATE{
    STOPPED,
    FOLLOW_LINE,
    NAVIGATE_CANYON,
    COLLECT_BALL,
    DEPOSIT_BALL,
    ADJUST_LASER
};

enum ROBOT_STATE lastState = STOPPED;
enum ROBOT_STATE currentState = STOPPED;
enum ROBOT_STATE nextState = STOPPED;
// INTERRUPTS
void _ISR _T1Interrupt(void){
    _T1IF = 0;
    timer1Done = 1;
}
void _ISR _T2Interrupt(void){
    _T2IF = 0;
    timer2Done = 1;
}

// HELPER FUNCTIONS
int countsPerRevolutionPerSecond(double revolutionsPerSecond){
    int clockCounts = (oscFreq / oscDiv) / (FULL_STEPS_PER_REVOLUTION * microSteps * revolutionsPerSecond);
    return clockCounts; 
}
int countsPerDegreesPerSecond(double degreesPerSecond){
    return countsPerRevolutionPerSecond(degreesPerSecond / 360);
}
int countsPerSecond(double seconds){
    int clockCounts = oscFreq * seconds / oscDiv;
    return clockCounts;
}
int timerCountsPerSecond(double seconds){
    int clockCounts = oscFreq * seconds / oscDiv / timerPrescaler;
    return clockCounts;
}
int countsPerFrequency(double hertz){
    int clockCounts = (oscFreq/oscDiv) / hertz;
    return clockCounts;
}

void setMotorSpeed(double counts, unsigned char direction, unsigned char motor){
    double dutyCycle =  .5;
    // char signedDirection = (direction) ? 1 : -1;

    // static long double currentRightSpeed = 0;
    // static long double currentLeftSpeed = 0;

    int speed = counts;

    
    // speed is percentage of frequency of the oscilltor

    // if forward == 1, drives forward forward == 0, backwards
    // motor 0 = left
    // Pin 4 OC2 - PWM
    // Pin 3 RA1 - Direction

    // motor 1 = right
    // Pin 5 OC3 - PWM
    // Pin 6 RB2 - Direction

    

    if(motor){ // Right motor
        // Proportional Controller

        // long double desiredSpeed = counts * signedDirection;
        // long double error = desiredSpeed - currentRightSpeed;
        // long double delta = error / kp;
        // currentRightSpeed += delta;

        // int speed = abs(currentRightSpeed);
        OC3RS = speed;
        OC3R = (speed + 1) * dutyCycle;
        _LATA4 = (direction ^ 1);
    }
    else{ 
        // Proportional Controller
        // long double desiredSpeed = counts * signedDirection;
        // long double error = desiredSpeed - currentLeftSpeed;
        // long double delta = error / kp;
        // currentLeftSpeed += delta;
        // int speed = abs(currentLeftSpeed);

        OC2RS = speed;
        OC2R = (speed+1) * dutyCycle;
        _LATB4 = direction;
         // reverses the right so it will be in the same direction as the left
    }
    
}
// SETUP FUNCTIONS
void oscSetup(){
    oscFreq = 8000000;
    _RCDIV = 0b001; // Postscaler = 2
    oscDiv = 4;
}
void clearRegisters(){
    ANSA = 0;
    ANSB = 0;
    TRISA = 0;
    TRISB = 0;
    T1CON = 0;

    OC1CON1 = 0;
    OC1CON2 = 0;
    OC2CON1 = 0;
    OC2CON2 = 0;
    OC3CON1 = 0;
    OC3CON2 = 0;
}

void timerSetup(){
    T1CONbits.TCS = 0; // Select internal clock (Osc/2)

    T1CONbits.TCKPS = 0b11; // Prescale value 1:256 ~ 4s with no Postscale
    timerPrescaler = 256; // Make this match above
    PR1 = timerCountsPerSecond(4);

    T1CONbits.TON = 1; // Turn on timer 1

    T2CONbits.TCS = 0; // Select internal clock (Osc/2)

    T2CONbits.TCKPS = 0b11; // Prescale value 1:256 ~ 4s with no Postscale
    PR2 = timerCountsPerSecond(.1);

    T2CONbits.TON = 1; // Turn on timer 1
}
void pwmSetup(){
    OC2CON1bits.OCTSEL = 0b111; // System clock
    OC3CON1bits.OCTSEL = 0b111;
    
    OC2CON1bits.OCM = 0b110; //Edge aligned PWM
    OC3CON1bits.OCM = 0b110;

    OC2CON2bits.OCTRIG = 0; // Sychronize with source in SYNCSEL
    OC3CON2bits.OCTRIG = 0;

    OC2CON2bits.SYNCSEL = 0b11111; // System clock (In datasheet it says this output compare module)
    OC3CON2bits.SYNCSEL = 0b11111; 
}
void interruptSetup(){
    _T1IP = 4;
    _T1IF = 0;
    _T1IE = 1;
}
void configADC(void)
{
    _ADON = 0;    // AD1CON1<15> -- Turn off A/D during config
    // AD1CON1 register
    _ADSIDL = 0;  // AD1CON1<13> -- A/D continues in idle mode
    _MODE12 = 1;  // AD1CON1<10> -- 12-bit A/D operation
    _FORM = 0;    // AD1CON1<9:8> -- Unsigned integer output
    _SSRC = 7;    // AD1CON1<7:4> -- Auto conversion (internal counter)
    _ASAM = 1;    // AD1CON1<2> -- Auto sampling
    // AD1CON2 register
    _PVCFG = 0;   // AD1CON2<15:14> -- Use VDD as positive
                  // ref voltage
    _NVCFG = 0;   // AD1CON2<13> -- Use VSS as negative
                  // ref voltage
    _BUFREGEN = 1;// AD1CON2<11> -- Result appears in buffer
                  // location corresponding to channel
    _CSCNA = 1;   // AD1CON2<10> -- Does not scan inputs
                  // specified in AD1CSSx registers (instead
                  // uses channels specified by CH0SA bits in
                  // AD1CHS register) -- Selecting '0' here
                  // probably makes writing to the AD1CSSL
                  // register unnecessary.
    _SMPI = 2;    // AD1CON2<6:2> -- Each conversion sent to 
                  // buffer
    _ALTS = 0;    // AD1CON2<0> -- Sample MUXA only
    // _CN2PDE = 1; // Do we need this?
    // AD1CON3 register
    _ADRC = 0;    // AD1CON3<15> -- Use system clock
    _SAMC = 0;    // AD1CON3<12:8> -- Auto sample every A/D
                  // period TAD
    _ADCS = 0b00000011;    // AD1CON3<7:0> -- TAD needs to be at least 750 ns. Thus, _ADCS = 0b00000010 will give us the fastest AD clock given a 4 MHz system clock.
    // AD1CHS register
    // _CH0NA = 0;   // AD1CHS<7:5> -- Use VDD as negative input
    // _CH0SA = 0;  // AD1CHS<4:0> -- Use AN0 as positive input
    // AD1CSSL register
    AD1CSSL = 0b0110000000010000;  // AD1CSSL<15:0> -- Skip all channels on 
                  // input scan -- see the CSCNA bits in
                  // AD1CON2
    _ADON = 1;    // AD1CON1<15> -- Turn on A/D

}
void pinSetup(){
    _TRISB2 = 1;
    _TRISA2 = 1;
    _TRISA3 = 1;

    _ANSB2 = 1;
    _ANSA2 = 1;
    _ANSA3 = 1;
}

void lineFollow(){
    double leftLight = (ADC1BUF13 / 4096.);
    double middleLight = (ADC1BUF14 / 4096.);
    double rightLight = (ADC1BUF4 / 4096.);

    int leftSpeed = 840 - (rightLight * 480) - ((rightLight * middleLight)*360) + (leftLight * 540);
    int rightSpeed = 840 - (leftLight * 480) - ((leftLight * middleLight)*360) + (leftLight * 540);

    setMotorSpeed(countsPerDegreesPerSecond(leftSpeed),FORWARD,LEFT);
    setMotorSpeed(countsPerDegreesPerSecond(rightSpeed),FORWARD,RIGHT);
}
int main(){
    unsigned char notFinished = TRUE;    
    currentState = FOLLOW_LINE;
    oscSetup();
    clearRegisters();
    pwmSetup();
    timerSetup();
    interruptSetup();
    configADC();
    pinSetup();
    


    while(notFinished){
        switch(currentState){
            case (FOLLOW_LINE):
                lineFollow();
                break;
            case (STOPPED):
                break;
            case (NAVIGATE_CANYON):
                break;
            case (COLLECT_BALL):
                break;
            case (DEPOSIT_BALL):
                break;
            case (ADJUST_LASER):
                break;
        }
    }
    return 0;
}
