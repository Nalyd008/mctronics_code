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

unsigned char timer1Done = 0;

long double kp = 1;

double oscFreq;
int oscDiv;
int timerPrescaler;
int microSteps = 2;
int FULL_STEPS_PER_REVOLUTION = 200;

enum ROBOT_STATE{
    STOPPED,
    DRIVE_FORWARD,
    TURN_LEFT_90,
    TURN_LEFT_180,
    DRIVE_BACKWARD
};

enum ROBOT_STATE currentState = STOPPED;
enum ROBOT_STATE nextState = STOPPED;
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
        _LATB2 = (direction ^ 1);
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
        _LATA1 = direction;
         // reverses the right so it will be in the same direction as the left
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
void _ISR _T1Interrupt(void){
    _T1IF = 0;
    timer1Done = 1;
}
void timerSetup(){
   
    T1CONbits.TCS = 0; // Select internal clock (Osc/2)

    T1CONbits.TCKPS = 0b11; // Prescale value 1:256 ~ 4s with no Postscale
    timerPrescaler = 256; // Make this match above
    PR1 = timerCountsPerSecond(4);

    T1CONbits.TON = 1; // Turn on timer 1
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
    oscDiv = 4;

    clearRegisters();
    pwmSetup();
    timerSetup();
    interruptSetup();
    while(1) {
        currentState = nextState;
        switch(currentState){
            case STOPPED:
                setMotorSpeed(countsPerDegreesPerSecond(0),FORWARD,LEFT);
                setMotorSpeed(countsPerDegreesPerSecond(0),FORWARD,RIGHT);
                if(timer1Done){
                    static int stopCount = 0;
                    PR1 = timerCountsPerSecond(.5);
                    if(stopCount % 4 == 1){
                        PR1 = timerCountsPerSecond(2);
                        nextState = TURN_LEFT_90;
                    }
                    else if(stopCount % 4 == 3){
                        PR1 = timerCountsPerSecond(2);
                        nextState = TURN_LEFT_180;
                    }
                    else{
                        PR1 = timerCountsPerSecond(2);
                        nextState = DRIVE_FORWARD;
                    }
                    TMR1 = 0;
                    stopCount++;
                }
                break;
            case Canyon:
                OC1R = oscFreq/300;
                OC1RS = OC1R*.5; // Look straight (angle servo = 90)
                // configure comparator interrupt
                OC1R = oscFreq/300;
                OC1RS = OC1R*0; // Look left (angle servo = 0)
                if (SHARP >= dist_thresh){
                    OC1RS = OC1R*.5 // Look straight
                    // Turn 180
                    // Clear flag return to normal state
                    }
                else{
                    OC1RS = OC1R*.5 // Look straight
                    // Turn left 90
                    // Clear flag return to normal state
                }
                // Check for line to exit Canyon state
                if (QRD1 == HIGH){
                    OC1R = oscFreq/300;
                    OC1RS = OC1R*0; // Look left (angle servo = 0)
                    if (SHARP >= dist_thresh){
                        //Turn Right 90
                        nextState = LineFollowing;
                    }
                    else{
                        //Turn Left 90
                        nextState = LineFollowing;
                    }
                    
                }
                break;
            case DRIVE_FORWARD:
                setMotorSpeed(countsPerDegreesPerSecond(1240),FORWARD,LEFT);
                setMotorSpeed(countsPerDegreesPerSecond(1240),FORWARD,RIGHT);
                if(timer1Done){
                    nextState = STOPPED;
                }
                break;
            case TURN_LEFT_90:
                setMotorSpeed(countsPerDegreesPerSecond(720),FORWARD,LEFT);
                setMotorSpeed(countsPerDegreesPerSecond(720),BACKWARD,RIGHT);
                if(timer1Done){
                    nextState = STOPPED;
                }
                break;
            case TURN_LEFT_180:
                setMotorSpeed(countsPerDegreesPerSecond(720),FORWARD,LEFT);
                setMotorSpeed(countsPerDegreesPerSecond(720),BACKWARD,RIGHT);
                if(timer1Done){
                    nextState = STOPPED;
                }
                break;
            case DRIVE_BACKWARD:
                PR1 = timerCountsPerSecond(1);
                setMotorSpeed(countsPerDegreesPerSecond(720),BACKWARD,RIGHT);
                setMotorSpeed(countsPerDegreesPerSecond(720),BACKWARD,LEFT);
                if(timer1Done){
                    nextState = STOPPED;
                }
                break;
        }
            
    }

    return 0;
}
