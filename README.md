# mctronics_code
The Official Codebase for the ME 330 Team, McTronics 

PWM and Timer cheatsheet: https://docs.google.com/document/d/1OdBD8_Dj_gzsTFDm1PUNO_qmoAui-if9DcfJTbtV0rc/edit?usp=sharing


USING PIC24F16KA301 WITH THIS PIN CONFIGURATION

Pin out:
- PIN 1 - Solenoid triggering for ball dispenser (RA5)
- PIN 2 - Line following QRD 4 (AN0)
- PIN 3 - Ball color detection QRD (AN1)
- PIN 4 - PWM out for drive left (OC2)
- PIN 5 - PWM out for drive right (OC3)
- PIN 6 - Line following QRD 1 (AN4)
- PIN 7 - Line following QRD 2 (AN13)
- PIN 8 - Line following QRD 3 (AN14)
- PIN 9 - Motor directionality 1 (RB4)
- PIN 10 - Motor directionality 1 (RA4)
- PIN 11 - PWM switch for servo satellite (RB7)
- PIN 12 - PWM switch for servo canyon (RB8)
- PIN 13 - 
- PIN 14 - Servo for canyon and satellite (OC1)
- PIN 15 - 
- PIN 16 - IR photodiode satellite (AN11)
- PIN 17 - IR photodiode ball (AN10)
- PIN 18 -  Sharp sensor for canyon (AN9)
- PIN 19 - VSS (GND)
- PIN 20 - VDD (3.3V)


Wheel width : 3/8
Outer edge to outer edge: 9 3/8
Diameter: 3 1/2
(inches)

1 full revolution of both wheels goes 11"
1 full revolution of one wheel turns robot 70 degrees
1 full revelution of two wheels turns robot 140 degrees (one forward one backward)
1 full revolution of the motor = 200 full steps 
