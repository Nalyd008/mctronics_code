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
unsigned char timer2Done = 0;

long double kp = 1;

double oscFreq;
int oscDiv;
int timerPrescaler;
int microSteps = 4;
int FULL_STEPS_PER_REVOLUTION = 200;

// enum ROBOT_STATE{
//     STOPPED,
//     DRIVE_FORWARD,
//     TURN_LEFT_90,
//     TURN_LEFT_180,
//     DRIVE_BACKWARD
// };
enum ROBOT_STATE{
    STOPPED,
    FOLLOW_LINE
};

enum LINE_FOLLOW_STATES{
    MIDDLE_LIT,
    LEFT_LIT,
    RIGHT_LIT,
    LEFT_2_LIT,
    RIGHT_2_LIT,
    NONE_LIT
}


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
int main(){
    unsigned char notFinished = TRUE;
    enum ROBOT_STATE robotState = FOLLOW_LINE;
    enum LINE_FOLLOW_STATES lineFollowState = NONE_LIT;
    
    oscSetup();
    clearRegisters();
    pwmSetup();
    timerSetup();
    interruptSetup();
    // while(1) {
    //     currentState = nextState;
    //     switch(currentState){
    //         case STOPPED:
    //             setMotorSpeed(countsPerDegreesPerSecond(0),FORWARD,LEFT);
    //             setMotorSpeed(countsPerDegreesPerSecond(0),FORWARD,RIGHT);
    //             if(timer1Done){
    //                 timer1Done = 0;
    //                 static int stopCount = 0;
    //                 PR1 = timerCountsPerSecond(.25);
    //                 if(stopCount % 4 == 1){
    //                     nextState = TURN_LEFT_90;
    //                     PR1 = timerCountsPerSecond(.64);
    //                     setMotorSpeed(countsPerDegreesPerSecond(360),FORWARD,LEFT);
    //                     setMotorSpeed(countsPerDegreesPerSecond(360),BACKWARD,RIGHT);
                        
    //                 }
    //                 else if(stopCount % 4 == 3){
    //                     nextState = TURN_LEFT_180;
    //                     PR1 = timerCountsPerSecond(1.29);
    //                     setMotorSpeed(countsPerDegreesPerSecond(360),FORWARD,LEFT);
    //                     setMotorSpeed(countsPerDegreesPerSecond(360),BACKWARD,RIGHT);
    //                 }
    //                 else{
    //                     PR1 = timerCountsPerSecond(1.5);
    //                     setMotorSpeed(countsPerDegreesPerSecond(720),FORWARD,LEFT);
    //                     setMotorSpeed(countsPerDegreesPerSecond(720),FORWARD,RIGHT);
    //                     nextState = DRIVE_FORWARD;
    //                 }
    //                 // else{
    //                 //     PR1 = timerCountsPerSecond(1);
    //                 //     setMotorSpeed(countsPerDegreesPerSecond(720),BACKWARD,RIGHT);
    //                 //     setMotorSpeed(countsPerDegreesPerSecond(720),BACKWARD,LEFT);
    //                 // }
    //                 TMR1 = 0;
    //                 stopCount++;
    //             }
    //             break;
    //         case DRIVE_FORWARD:
    //             if(timer1Done){
    //                 timer1Done = 0;
    //                 nextState = STOPPED;
    //             }
    //             break;
    //         case TURN_LEFT_90:
    //             if(timer1Done){
    //                 timer1Done = 0;
    //                 nextState = STOPPED;
    //             }
    //             break;
    //         case TURN_LEFT_180:
    //             if(timer1Done){
    //                 timer1Done = 0;
    //                 nextState = STOPPED;
    //             }
    //             break;
    //         case DRIVE_BACKWARD:
    //             if(timer1Done){
    //                 timer1Done = 0;
    //                 nextState = STOPPED;
    //             }
    //             break;
    //     }
    // }
    while(notFinished){
        switch(robotState){
            case FOLLOW_LINE:
                switch(lineFollowState){
                    case NONE_LIT:
                        if(1==1){ //Check if 1 light is lit
                            // set specific motor to default turn speed (set both motors to go forward but 1 goes a little faster)
                            // set state to correct state based on lights that are on
                            // potentially start timer to let us know how long we've been in the next state for PID?
                        }
                        break;
                    case MIDDLE_LIT:
                        break;
                    case LEFT_LIT:

                        break;
                    case RIGHT_LIT:

                        break;
                    case LEFT_2_LIT:

                        break;
                    case RIGHT_2_LIT:

                        break;

                }
                break;
        }
    }
    return 0;
}
