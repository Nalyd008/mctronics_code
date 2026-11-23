/**
 * @file main.c
 * @author nalyd
 * @date 2026-10-05
 * @brief Main function
 */

#include <stdlib.h>
#include <math.h>
#include <xc.h>
#pragma config FNOSC = FRCDIV // 8MHz Fast RC with Postscaler
#pragma config OSCIOFNC = OFF
#pragma config SOSCSRC = DIG

#define TRUE 1
#define FALSE 0
#define FORWARD 0
#define BACKWARD 1
#define LEFT 0
#define RIGHT 1

// IF YOU CHANGE THESE VALUES MAKE SURE YOU CHANGE THE VALUES FOR AD1CSSL AND IN pinSetup()
#define LEFT_SHARP_ANALOG ADC1BUF10
#define RIGHT_SHARP_ANALOG ADC1BUF9

#define LEFT_QRD_ANALOG ADC1BUF4
#define MIDDLE_QRD_ANALOG ADC1BUF13
#define RIGHT_QRD_ANALOG ADC1BUF14

#define BALL_QRD_ANALOG ADC1BUF1
#define SATELITE_IR_ANALOG ADC1BUF11
#define BALL_IR_ANALOG ADC1BUF12

//Threshold values REMEMBER BLACK = HIGH ON QRDs
#define THRESHOLD_BALL_VOLTAGE 2.5

    // Trigger for canyon
#define THRESHOLD_CANYON_LEFT_QRD_VOLTAGE 1.1
#define THRESHOLD_CANYON_MIDDLE_QRD_VOLTAGE 1.1
#define THRESHOLD_CANYON_RIGHT_QRD_VOLTAGE 1.1
#define THRESHOLD_CANYON_RIGHT_SHARP_VOLTAGE 1.32
#define THRESHOLD_CANYON_LEFT_SHARP_VOLTAGE 1.32

    // Trigger for ball drop off
#define THRESHOLD_CHECK_BOX_LEFT_QRD_VOLTAGE 2.64
#define THRESHOLD_CHECK_BOX_MIDDLE_QRD_VOLTAGE .33
#define THRESHOLD_CHECK_BOX_RIGHT_QRD_VOLTAGE 2.64
#define THRESHOLD_CHECK_BOX_LEFT_SHARP_VOLTAGE 1
#define THRESHOLD_CHECK_BOX_RIGHT_SHARP_VOLTAGE 1

    // Ball sorter color
#define THRESHOLD_BALL_COLOR_QRD_VOLTAGE 2

    // Line Follow
#define LINE_FOLLOW_BASE_SPEED 1000
#define KP_SPEED_UP_ONE_LIGHT_OFF 0
#define KP_SPEED_DOWN_ONE_LIGHT_OFF .3
#define KP_SPEED_UP_ONE_LIGHT_OFF 0
#define KP_SPEED_DOWN_ONE_LIGHT_OFF .3
#define KP_SPEED_UP_TWO_LIGHT_OFF .5
#define KP_SPEED_DOWN_TWO_LIGHT_OFF .5

    // Canyon Navigation
#define CANYON_BASE_SPEED 700
#define KP_CANYON 1

    // Check for Line (exit canyon)
#define THRESHOLD_CHECK_LINE_LEFT_QRD_VOLTAGE 1.1
#define THRESHOLD_CHECK_LINE_MIDDLE_QRD_VOLTAGE 1.1
#define THRESHOLD_CHECK_LINE_RIGHT_QRD_VOLTAGE 1.1

    // Satellite Alignment
#define THRESHOLD_SATELLITE_LEFT_SHARP_VOLTAGE .4 // greater than
#define THRESHOLD_SATELLITE_RIGHT_SHARP_VOLTAGE .4 // greater than
#define SATELLITE_SERVO_STARTING_DUTY_CYCLE .1 // varies from .1 to .2 for max and min of servo
#define THRESHOLD_SATELLITE_IR_VOLTAGE .4 // less than


unsigned char timer1Done = 0;
unsigned char timer2Done = 0;

double oscFreq;
int oscDiv;
int timerPrescaler;
int microSteps = 4;
int FULL_STEPS_PER_REVOLUTION = 200;
unsigned int ball = 1;

enum ROBOT_STATE{
    STOPPED,
    FOLLOW_LINE,
    NAVIGATE_CANYON,
};

enum ROBOT_STATE lastState = STOPPED;
enum ROBOT_STATE currentState = STOPPED;
enum ROBOT_STATE nextState = STOPPED;

unsigned char isCanyonDone = FALSE;
unsigned char hasBall = FALSE;
unsigned char isBallDropped = FALSE;

// FORWARD DECLARATONS
void adjustSatellite();
void depositBall();
void getBall();

// INTERRUPTS
// Comparator on AN0
// void _ISR //*Comparator for when 4th qrd goes high(void){
//     _flag = 0;
//     if(currentState = FOLLOW_LINE && isBallDropped){
//         adjustSatellite();
//     }
// }
// Comparator on AN12
// void _ISR //*Comparator for when ball dropper detected goes high(void){
//     _flag = 0;
//     if(currentState = FOLLOW_LINE && !hasBall){
//         getBall();
//     }
// }
void _ISR _CompInterrupt(void){
    _CMIF =0;
    if(CM3CONbits.CEVT){
        CM1CONbits.CEVT = 0;
        // GetBall();
    }
}
void _ISR _T1Interrupt(void){
    _T1IF = 0;
    timer1Done = 1;
}
void _ISR _T2Interrupt(void){
    _T2IF = 0;
    timer2Done = 1;
}


// HELPER FUNCTIONS
double convertAnalogToVoltage(double analogInput){
    return (analogInput / 4096.) * 3.3;
}
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
    // Max is 4.16 seconds
    int clockCounts = oscFreq * seconds / oscDiv / timerPrescaler;
    return clockCounts;
}
int timerCountsPerFrequncy(double hertz){
    // Max is 4.16 seconds
    int clockCounts = oscFreq / oscDiv / timerPrescaler / hertz;
    return clockCounts;
}
int countsPerFrequency(double hertz){
    int clockCounts = (oscFreq/oscDiv) / hertz;
    return clockCounts;
}

void setMotorSpeed(double speedInDegreesPerSecond, unsigned char direction, unsigned char motor){
    double counts = countsPerDegreesPerSecond(speedInDegreesPerSecond);
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
        _LATB7 = direction;
        OC3RS = speed;
        OC3R = (speed + 1) * dutyCycle;
        
    }
    else{ 
        // Proportional Controller
        // long double desiredSpeed = counts * signedDirection;
        // long double error = desiredSpeed - currentLeftSpeed;
        // long double delta = error / kp;
        // currentLeftSpeed += delta;
        // int speed = abs(currentLeftSpeed);
        _LATB8 = direction;
        OC2RS = speed;
        OC2R = (speed+1) * dutyCycle;
        
         // reverses the right so it will be in the same direction as the left
    }
    
}
// void lineFollowPID(){
//     double leftLight = (ADC1BUF4 / 4096.);
//     double middleLight = (ADC1BUF13 / 4096.);
//     double rightLight = (ADC1BUF12 / 4096.);
    
//     static error = 0;
// }
void turnForSeconds(double speed, double seconds, unsigned char direction){
    setMotorSpeed(speed,direction,LEFT);
    setMotorSpeed(speed,direction ^ 1,RIGHT);
    
    TMR1 = 0;
    PR1 = timerCountsPerSecond(seconds);
    timer1Done = 0;
    while(!timer1Done);

    setMotorSpeed(0,FORWARD ^ direction,LEFT);
    setMotorSpeed(0,FORWARD ^ direction,RIGHT);
}
void turnForDegrees(double speed, double degrees, unsigned char direction){
    double seconds = 360. * degrees / (speed * 140); // this math assuming a full revolution 360 of both wheels = 140 degrees of robot
    turnForSeconds(speed, seconds, direction);
}
void driveForSeconds(double speed, double seconds, unsigned char direction){
    setMotorSpeed(speed,FORWARD ^ direction,LEFT);
    setMotorSpeed(speed,FORWARD ^ direction,RIGHT);
    TMR1 = 0;
    PR1 = timerCountsPerSecond(seconds);
    timer1Done = 0;
    while(!timer1Done);

    setMotorSpeed(0,FORWARD ^ direction,LEFT);
    setMotorSpeed(0,FORWARD ^ direction,RIGHT);
}
void stopForSeconds(double seconds){
    driveForSeconds(0,seconds,1);
}
int checkBallColor(){
    /*Returns 1 for BLACK, 0 for WHITE*/ 
    int color;
    double QRDcolor = convertAnalogToVoltage(BALL_QRD_ANALOG);
    
    if (QRDcolor < THRESHOLD_BALL_COLOR_QRD_VOLTAGE) {
        color = 1;
    }
    else {
        color = 0;
    }

    return color;
}
void getBall(){
    setMotorSpeed(0, FORWARD, LEFT);
    setMotorSpeed(0, FORWARD, RIGHT);
    turnForDegrees(540, 90, LEFT);
    driveForSeconds(360, 7, BACKWARD); //change this
    stopForSeconds(4); //this too
    driveForSeconds(360, 7, FORWARD); //also this
    turnForDegrees(540, 90, RIGHT);
    hasBall = TRUE;
}
void depositBall() {
    //move forward
     // white is 1
    

    stopForSeconds(2);
    int color;
    color = checkBallColor();
    TMR1 = 0;
    PR1 = timerCountsPerSecond(1.25);
    timer1Done = 0;

//        while(!timer1Done){
//            lineFollow(1);
//        }
//        stopForSeconds(2);
    driveForSeconds(200,2.4,FORWARD);
    
    if(color == 0){
        
        turnForDegrees(540,120,LEFT);
        stopForSeconds(2);
        driveForSeconds(200,.8,BACKWARD);
        stopForSeconds(2);
        _LATB14 = 1; // solenoid trigger on 
        stopForSeconds(4);
        _LATB14 = 0; // solenoid trigger off
        stopForSeconds(2);
        driveForSeconds(200,.45,FORWARD);
        stopForSeconds(2);
        turnForDegrees(540,140,RIGHT);
        stopForSeconds(2);
    }
    else{
      
        turnForDegrees(540,140,RIGHT);
        stopForSeconds(2);
        driveForSeconds(200,.8,BACKWARD);
        stopForSeconds(2);
        _LATB14 = 1; // solenoid trigger on 
        stopForSeconds(4);
        _LATB14 = 0; // solenoid trigger off
        stopForSeconds(2);
        driveForSeconds(200,.45,FORWARD);
        stopForSeconds(2);
        turnForDegrees(540,140,LEFT);
        stopForSeconds(2);
    }
    
    ball = 0;
    
    
}
void navigateCanyon(){
    double rightSenseVoltage = convertAnalogToVoltage(RIGHT_SHARP_ANALOG); // adjust these values for differences in robots
    double leftSenseVoltage = convertAnalogToVoltage(LEFT_SHARP_ANALOG);
    double error = leftSenseVoltage - rightSenseVoltage;

    int leftSpeed = 700 - (error * KP_CANYON);
    int rightSpeed = 700 + (error * KP_CANYON);
    
    if (rightSpeed < 120){
        rightSpeed = 120;
    }
    if(leftSpeed < 120){
        leftSpeed = 120;
    }
    setMotorSpeed(leftSpeed,FORWARD,LEFT);
    setMotorSpeed(rightSpeed,FORWARD,RIGHT);
}
void lineFollow(){
    double leftLight = convertAnalogToVoltage(LEFT_QRD_ANALOG);
    double middleLight = convertAnalogToVoltage(MIDDLE_QRD_ANALOG);
    double rightLight = convertAnalogToVoltage(RIGHT_QRD_ANALOG);

    unsigned char leftDir = FORWARD;
    unsigned char rightDir = FORWARD;

    // light goes high on black, low on white
    int leftSpeed = LINE_FOLLOW_BASE_SPEED * (1 + (leftLight * KP_SPEED_UP_ONE_LIGHT_OFF) - (rightLight * KP_SPEED_DOWN_ONE_LIGHT_OFF) - ((rightLight * middleLight) * KP_SPEED_DOWN_TWO_LIGHT_OFF) + (leftLight * middleLight * KP_SPEED_UP_TWO_LIGHT_OFF));
    int rightSpeed = LINE_FOLLOW_BASE_SPEED * (1 + (rightLight * KP_SPEED_UP_ONE_LIGHT_OFF) - (leftLight * KP_SPEED_DOWN_ONE_LIGHT_OFF) - ((leftLight * middleLight) * KP_SPEED_DOWN_TWO_LIGHT_OFF) + (rightLight * middleLight * KP_SPEED_UP_TWO_LIGHT_OFF));
    if(leftSpeed < 0){
        leftSpeed = abs(leftSpeed);
        leftDir = BACKWARD;
    }
    if(rightSpeed < 0){
        rightSpeed = abs(rightSpeed);
        rightDir = BACKWARD;
    }

    if(rightSpeed < 100){
        rightSpeed = 0;
    }
    if(leftSpeed < 100){
        leftSpeed = 0;
    }
    
    setMotorSpeed(leftSpeed,leftDir,LEFT);
    setMotorSpeed(rightSpeed,rightDir,RIGHT);
}
void checkForLine(){
    double leftLight = convertAnalogToVoltage(LEFT_QRD_ANALOG);
    double middleLight = convertAnalogToVoltage(MIDDLE_QRD_ANALOG);
    double rightLight = convertAnalogToVoltage(RIGHT_QRD_ANALOG);
   
    if(leftLight < THRESHOLD_CHECK_LINE_LEFT_QRD_VOLTAGE|| middleLight < THRESHOLD_CHECK_LINE_MIDDLE_QRD_VOLTAGE || rightLight < THRESHOLD_CHECK_LINE_RIGHT_QRD_VOLTAGE){ // adjust value?
        currentState = FOLLOW_LINE;
    }
}
void checkForBall(){
    double ballIR = convertAnalogToVoltage(BALL_IR_ANALOG);
    if(ballIR > THRESHOLD_BALL_VOLTAGE){
        getBall();
    }
}
void checkForCanyon(){
    double leftLight = convertAnalogToVoltage(LEFT_QRD_ANALOG);
    double middleLight = convertAnalogToVoltage(MIDDLE_QRD_ANALOG);
    double rightLight = convertAnalogToVoltage(RIGHT_QRD_ANALOG);
    double rightSenseVoltage = convertAnalogToVoltage(RIGHT_SHARP_ANALOG);
    double leftSenseVoltage = convertAnalogToVoltage(LEFT_SHARP_ANALOG);

    if(leftLight > THRESHOLD_CANYON_LEFT_QRD_VOLTAGE && middleLight > THRESHOLD_CANYON_MIDDLE_QRD_VOLTAGE && rightLight > THRESHOLD_CANYON_RIGHT_QRD_VOLTAGE && rightSenseVoltage > THRESHOLD_CANYON_RIGHT_SHARP_VOLTAGE && leftSenseVoltage > THRESHOLD_CANYON_LEFT_SHARP_VOLTAGE){ // adjust value?
        currentState = NAVIGATE_CANYON;
    }
}
void checkForBox(){    
    double leftLight = convertAnalogToVoltage(LEFT_QRD_ANALOG);
    double middleLight = convertAnalogToVoltage(MIDDLE_QRD_ANALOG);
    double rightLight = convertAnalogToVoltage(RIGHT_QRD_ANALOG);
    double rightSenseVoltage = convertAnalogToVoltage(RIGHT_SHARP_ANALOG);
    double leftSenseVoltage = convertAnalogToVoltage(LEFT_SHARP_ANALOG);
    if(middleLight < THRESHOLD_CHECK_BOX_MIDDLE_QRD_VOLTAGE && leftLight < THRESHOLD_CHECK_BOX_LEFT_QRD_VOLTAGE && rightLight < THRESHOLD_CHECK_BOX_RIGHT_QRD_VOLTAGE && rightSenseVoltage > THRESHOLD_CHECK_BOX_RIGHT_SHARP_VOLTAGE && leftSenseVoltage > THRESHOLD_CHECK_BOX_LEFT_SHARP_VOLTAGE){ // NEED TO MESS WITH THESE TO GET IT TO  TRANSITION
        depositBall();
    }
}
//tell it to go back to line follow
//code to read the ping pong ball here
void adjustSatellite(){
    driveForSeconds(360, 1, FORWARD);
    turnForDegrees(540,90,LEFT);
    setMotorSpeed(540,FORWARD,LEFT);
    setMotorSpeed(540,FORWARD,RIGHT);
    while(convertAnalogToVoltage(LEFT_SHARP_ANALOG) > THRESHOLD_SATELLITE_LEFT_SHARP_VOLTAGE && convertAnalogToVoltage(RIGHT_SHARP_ANALOG) > THRESHOLD_SATELLITE_RIGHT_SHARP_VOLTAGE){} //ADJUST THESE VALUES

    setMotorSpeed(0,FORWARD,LEFT);
    setMotorSpeed(0,FORWARD,RIGHT);

    currentState = STOPPED;

    // CHANGE THESE NUMBERS TO BE ALIGNED WITH 50 HZ PERIOD 
    
    OC1R = timerCountsPerFrequncy(50); // Period of 20 ms
    OC1RS = OC1R * SATELLITE_SERVO_STARTING_DUTY_CYCLE; // Duty cylce varies from 1-2 ms for 0 - 180 degrees of servo (.1 - .2)
    while(convertAnalogToVoltage(SATELITE_IR_ANALOG) < THRESHOLD_SATELLITE_IR_VOLTAGE){ // ADJUST THESE VALUES
        OC1R+= 1; // Add increasing duty cycle
    }

}

// SETUP FUNCTIONS
void oscSetup(){
    oscFreq = 8000000;
    _RCDIV = 0b000; // Postscaler = 2
    oscDiv = 2;
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
    PR2 = timerCountsPerSecond(4);

    T3CONbits.TON = 1; // Turn on timer 1

    T3CONbits.TCS = 0; // Select internal clock (Osc/2)

    T3CONbits.TCKPS = 0b11; // Prescale value 1:256 ~ 4s with no Postscale
    PR3 = timerCountsPerSecond(4);

    T3CONbits.TON = 1; // Turn on timer 1
}
void pwmSetup(){
    // CHANGE OC1 TO USE DIFFERENT CLOCK SOURCE FOR 50 HZ
    OC1CON1bits.OCTSEL = 0b000;
    OC2CON1bits.OCTSEL = 0b111; // System clock
    OC3CON1bits.OCTSEL = 0b111;
    
    OC1CON1bits.OCM = 0b110;
    OC2CON1bits.OCM = 0b110; //Edge aligned PWM
    OC3CON1bits.OCM = 0b110;

    OC1CON2bits.OCTRIG = 0;
    OC2CON2bits.OCTRIG = 0; // Sychronize with source in SYNCSEL
    OC3CON2bits.OCTRIG = 0;

    OC1CON2bits.SYNCSEL = 0b11111;  
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
    _SMPI = 6;    // AD1CON2<6:2> -- Each conversion sent to 
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
    AD1CSSL = 0b0111111000010010;  // AD1CSSL<15:0> -- Skip all channels on 
                  // input scan -- see the CSCNA bits in
                  // AD1CON2
    _ADON = 1;    // AD1CON1<15> -- Turn on A/D

}
void configInterrupt(){
 // Compare comparator 3 on C3INC to Voltage
}
void pinSetup(){
    _TRISA1 = 1; //AN1
    _TRISB2 = 1; //AN4
    _TRISA2 = 1; //AN13
    _TRISA3 = 1; //AN14

    _TRISB15 = 1; //AN9
    _TRISB14 = 1; //AN10
    _TRISB13 = 1; //AN11
    _TRISB12 = 1; //AN12


    _ANSA1 = 1; //AN1
    _ANSB2 = 1; //AN4
    _ANSA2 = 1; //AN13
    _ANSA3 = 1; //AN14

    _ANSB15 = 1; //AN9
    _ANSB14 = 1; //AN10
    _ANSB13 = 1; //AN11
    _ANSB12 = 1; //AN12
}
int main(){
    unsigned char notFinished = TRUE;    
    currentState = FOLLOW_LINE;

    oscSetup();
    clearRegisters();
    pwmSetup();
    timerSetup();
    interruptSetup();
    pinSetup();
    configADC();
    configInterrupt();

    while(notFinished){        
        switch(currentState){
            case (FOLLOW_LINE):
                lineFollow();
                checkForCanyon();
                if(hasBall){
                    checkForBox();
                }
                else{
                    checkForBall();
                }
                
                break;
            case (NAVIGATE_CANYON):
                navigateCanyon();
                checkForLine();
                break;
            case (STOPPED):
                setMotorSpeed(0,FORWARD,LEFT);
                setMotorSpeed(0,FORWARD,RIGHT);
                notFinished = TRUE;
                break;
        }
    }
    return 0;
}
