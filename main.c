/**
 * @file main.c
 * @author nalyd
 * @date 2026-10-05
 * @brief Main function
 */

#include <xc.h>
#pragma config FNOSC = FRCDIV // 8MHz Fast RC with Postscaler

double kp = .01;


int oscFreq;
int oscDiv;

enum ROBOT_STATE{
    STOPPED,
    DRIVE_FORWARD,
    TURN_LEFT_90,
    TURN_LEFT_180,
    DRIVE_BACKWARD
};

enum ROBOT_STATE currentState = STOPPED;

void _ISR _T1Interrupt(void){
    static int loopCount = 0;
    _T1IF = 0;
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

int setMotorSpeed(double speed, int forward, int motor){
    int direction = (forward == 1) ? 1 : -1;
    // Proportional controller
    static double currentSpeed = 0;
    double desiredSpeed = speed * direction;
    double error = desiredSpeed - currentSpeed;
    double delta = error * kp;

    currentSpeed += delta;
    // speed is percentage of frequency of the oscilltor

    // if forward == 1, drives forward forward == 0, backwards
    // motor 0 = left
    // Pin 4 OC2 - PWM
    // Pin 3 RA1 - Direction

    // motor 1 = right
    // Pin 5 OC3 - PWM
    // Pin 6 RB2 - Direction
    int realFreq = (oscFreq / oscDiv);
    int period = currentSpeed * (realFreq/1000.);
    double dutyCycle = .5;

    if(!motor){
        OC2RS = period;
        OC2R = period * dutyCycle;
        _LATA1 = forward;
    }
    else{
        OC3RS = period;
        OC3R = period * dutyCycle;
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
    timerSteup();
    interruptSetup();

    while(1) {
        switch(currentState){
            case STOPPED:
                setMotorSpeed(0,1,0);
                setMotorSpeed(1,1,0);
                break;
            case DRIVE_FORWARD:
                setMotorSpeed(1,1,0);
                setMotorSpeed(1,1,1);
                break;
            case TURN_LEFT_90:
                setMotorSpeed(.25,1,0);
                setMotorSpeed(.25,-1,0);
                break;
            case TURN_LEFT_180:
                setMotorSpeed(.5,1,0);
                setMotorSpeed(.5,-1,0);
                break;
            case DRIVE_BACKWARD:
                setMotorSpeed(1,0,0);
                setMotorSpeed(1,0,1);
                break;
        }
            
    }

    return 0;
}
