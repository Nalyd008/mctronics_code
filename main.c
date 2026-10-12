/**
 * @file main.c
 * @author nalyd
 * @date 2026-10-05
 * @brief Main function
 */

#include <xc.h>
#pragma config FNOSC = FRCDIV // 8MHz Fast RC with Postscaler

#define TRUE 1
#define FALSE 0
#define FORWARD 1
#define BACKWARD 0
#define LEFT 0
#define RIGHT 1

double kp = .01;

double oscFreq;
int oscDiv;
int microSteps = 1;
int FULL_STEPS_PER_REVOLUTION = 200;

enum ROBOT_STATE{
    STOPPED,
    DRIVE_FORWARD,
    TURN_LEFT_90,
    TURN_LEFT_180,
    DRIVE_BACKWARD
};

enum ROBOT_STATE currentState = STOPPED;
int countsPerRevolutionPerSecond(double revolutionsPerSecond){
    int clockCounts = (oscFreq / oscDiv) / (FULL_STEPS_PER_REVOLUTION * microSteps * revolutionsPerSecond);
    return clockCounts; 
}
int countsPerDegreesPerSecond(double degreesPerSecond){
    return countsPerRevolutionPerSecond(degreesPerSecond / 360);
}
int countsPerSeconds(double seconds){
    int clockCounts = oscFreq * seconds / oscDiv;
    return clockCounts;
}
int countsPerFrequency(double hertz){
    int clockCounts = (oscFreq/oscDiv) / hertz;
    return clockCounts;
}
void _ISR _T1Interrupt(void){
    static int loopCount = 0;
    _T1IF = 0;
    setMotorSpeed(0,FORWARD,LEFT);
    setMotorSpeed(0,FORWARD,RIGHT);
    switch(currentState){
        case STOPPED:
            currentState = DRIVE_FORWARD;
            break;
        case DRIVE_FORWARD:
            if(loopCount % 6 == 1){
                currentState = TURN_LEFT_90;
            }
            else if(loopCount % 6 == 3){
                currentState = TURN_LEFT_180;
            }
            else if(loopCount % 6 == 5){
                currentState = TURN_LEFT_180;
            }
            else{
                currentState = DRIVE_FORWARD;
            }
            break;
        case TURN_LEFT_90:
            currentState = DRIVE_FORWARD;
            break;
        case TURN_LEFT_180:
            currentState = DRIVE_FORWARD;
            break;
        case DRIVE_BACKWARD:
            currentState = DRIVE_FORWARD;
            break;
    }
    loopCount++;
}

void setMotorSpeed(double period, unsigned char forward, unsigned char motor){
    // char direction = (forward) ? 1 : -1;
    // // Proportional controller
    // static double currentSpeed = 0;
    // double desiredSpeed = speed * direction;
    // double error = desiredSpeed - currentSpeed;
    // double delta = error * kp;

    // currentSpeed += delta;
    // speed is percentage of frequency of the oscilltor

    // if forward == 1, drives forward forward == 0, backwards
    // motor 0 = left
    // Pin 4 OC2 - PWM
    // Pin 3 RA1 - Direction

    // motor 1 = right
    // Pin 5 OC3 - PWM
    // Pin 6 RB2 - Direction
    // int realFreq = (oscFreq / oscDiv)-1;
    // int period = currentSpeed * (realFreq/1000.);
    double dutyCycle = .5;
    OC2RS = period;
    OC2R = period * dutyCycle;

    if(!motor){
        _LATA1 = forward;
    }
    else{
        _LATA2 = (forward ^ 1); // reverses the right so it will be in the same direction as the left
    }
    
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
    T1CONbits.TON = 1; // Turn on timer 1
    T1CONbits.TCS = 0; // Select internal clock (Osc/2)
    T1CONbits.TCKPS = 0b11; // Prescale value 1:256 ~ 4s with no Postscale
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
int main(){
    oscFreq = 8000000;
    _RCDIV = 0b001; // Postscaler = 2
    oscDiv = 2;

    clearRegisters();
    pwmSetup();
    timerSetup();
    interruptSetup();

    while(1) {
        switch(currentState){
            case STOPPED:
                setMotorSpeed(0,FORWARD,LEFT);
                setMotorSpeed(0,FORWARD,RIGHT);
                break;
            case DRIVE_FORWARD:
                setMotorSpeed(countsPerDegreesPerSecond(45),FORWARD,LEFT);
                setMotorSpeed(countsPerDegreesPerSecond(45),FORWARD,RIGHT);
                break;
            case TURN_LEFT_90:
                setMotorSpeed(countsPerDegreesPerSecond(22.5),FORWARD,LEFT);
                setMotorSpeed(countsPerDegreesPerSecond(22.5),BACKWARD,RIGHT);
                break;
            case TURN_LEFT_180:
                setMotorSpeed(countsPerDegreesPerSecond(22.5),FORWARD,LEFT);
                setMotorSpeed(countsPerDegreesPerSecond(22.5),BACKWARD,RIGHT);
                break;
            case DRIVE_BACKWARD:
                setMotorSpeed(countsPerDegreesPerSecond(45),BACKWARD,RIGHT);
                setMotorSpeed(countsPerDegreesPerSecond(45),BACKWARD,LEFT);
                break;
        }
            
    }

    return 0;
}
