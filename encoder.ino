/// @file stepper_control.ino
/// @brief Control a single stepper motor using "Arduino Leonardo" and stepper motor driver "Pololu A4988 Stepper Motor Driver Carrier, Black Edition"
/// @author Sarah Lindner
/// @date 2014/01/23

/**
for each stepper motor, one arduino board is required due to limited pins
each stepper motor is associated with 2 end buttons
each stepper motor has its own encoder

simple A4988 connection
jumper reset and sleep together
connect  VDD to Arduino 3.3v or 5v
connect  GND to Arduino GND (GND near VDD)
connect  1A and 1B to stepper coil 1
connect 2A and 2B to stepper coil 2
connect VMOT to power source 
connect GRD to power source

connect pin 11 to stepPin
connect pin 10 to dirPin
connect pin 12 to sleepPin


connect pin 7 to end buttons, forward
connect pin 6 to end buttons, backward

connect pin 2 to encoder phase A
connect pin 3 to encoder phase B
**/



 //libraries for high performance reads and writes e.g. fastDigitalRead( pin )
#include "DigitalIO.h"
#include "DigitalPin.h"
#include "SoftSPI.h"




// Stepper motor variables------------------------------
// pins for motor control
int stepPin = 11;  
int dirPin = 10;  
int sleepPin = 12;

int ms1Pin = 9;
int ms2Pin = 8;

int step_num = 20;     //  number of steps for "small" movement

// incoming serial data for controlling the motor state
char state = '~';   
char state_old = '~';

// incoming serial data for controlling microstepping
int microstepMode = 0;

// set speed of the motor
int speedRPM = 10; //desired speed in RPM
int stepsPerRevolution = 4000;
int delay_time = 100000;



//pins for end button control------------------------
int end_forward = 7; 
int end_backward = 6;



//Encoder specific variables-----------------------
#define encoderPinA 2
#define encoderPinB 3
#define encoderPinA1 0 // inverted channels
#define encoderPinB1 1 // inverted channels
#define encoderPinI 4

int encoderInterruptA = 0;
int encoderInterruptB = 1;

volatile long encoderPos = 0;
long oldPos = 0;
volatile long encoderRev = 0;
long oldRev = 0;

long Aold = 0;
long Bnew = 0;
long A1old = 0; // inverted channels
long B1new = 0; // inverted channels

// long Iold = 0;



//-----------------------------------------------------

// function prototypes (not actually needed for Arduino, because they are created by the compiler, but good coding practice)
//print standard directions
void print_directions();
void HandleInterruptA();
void HandleInterruptB();


void setup() 
{ 
  while(!Serial){;}       // needed for Ardiuno Leonardo to wait with executing until terminal is open
  Serial.begin(9600);           // set up Serial library at 9600 bps  
  
  Serial.println("Beginning stepper motor configuration.");               



  //set pin mode for all motor pins
  pinMode(stepPin, OUTPUT);// y axis, step
  pinMode(dirPin, OUTPUT);// y axis, dirPin
  pinMode(sleepPin, OUTPUT);

  pinMode(ms1Pin, OUTPUT);// microstepping
  pinMode(ms2Pin, OUTPUT);


  //set pin mode for all end button pins 
  pinMode(end_forward, INPUT);
  pinMode(end_backward, INPUT);


  // Quadrature encoders
  pinMode(encoderPinA, INPUT);      // sets pin A as input
  digitalWrite(encoderPinA, LOW);  // turn on pullup resistors
  pinMode(encoderPinB, INPUT);      // sets pin B as input
  digitalWrite(encoderPinB, LOW);  // turn on pullup resistors
  
  attachInterrupt(encoderInterruptA, HandleInterruptA, CHANGE);
  attachInterrupt(encoderInterruptB, HandleInterruptB, CHANGE);

  delay_time = 60L * 1000L / stepsPerRevolution / speedRPM; // calculate delay time to match desired speed in RPM

  print_directions();

}






void loop() 
{ 


  // send data only when you receive data:
  if (Serial.available() > 0) { //check for incoming data
    
    state = Serial.read(); // read the incoming byte:

  }






  switch (state) {

    case '~': // no incoming data -> do nothing
      break;
      

    case 'F': // run forward until next command

      if( state != state_old ) { // if you got something new, say what you're doing
        
        Serial.println("Motor running forward.");
     
        state_old = state;
      }

      if( digitalRead(end_forward) == HIGH ){ // is end button pressed?
        
        digitalWrite(sleepPin, LOW);// stop motor

        state = 's'; // stop

      } else {

        digitalWrite(sleepPin, HIGH); //wake up!


        digitalWrite(dirPin, HIGH); //run forward

        digitalWrite(stepPin, HIGH); //step!  
        delayMicroseconds(10);               
        digitalWrite(stepPin, LOW);  
        delay(delay_time);
    
      }

      break;

    case 'B': // run backward until next command

      if( state != state_old ) { // if you got something new, say what you're doing    
        
        Serial.println("Motor running backward.");
     
        state_old = state;
      }

      if( digitalRead(end_backward) == HIGH ){ // is end button pressed?
        
        digitalWrite(sleepPin, LOW);// stop motor

        state = 's'; // equals an "s" -> stop

      } else {

      digitalWrite(sleepPin, HIGH); //wake up!

      digitalWrite(dirPin, LOW); // run backward

      digitalWrite(stepPin, HIGH); //step!  
      delayMicroseconds(10);               
      digitalWrite(stepPin, LOW);  
      delay(delay_time);
    
      }

      break;

    case 's': //equals an s

      if( state != state_old ) { // if you got something new, say what you're doing

        Serial.println("Motor stopping.");
        state_old = state;
    
        digitalWrite(ms1Pin, setMs1Mode(1) );
        digitalWrite(ms2Pin, setMs2Mode(1) );
      }


    digitalWrite(sleepPin, LOW); // stop
    break;

    case 'f': //small movement forward

      if( state != state_old ) { // if you got something new, say what you're doing

        Serial.println("Motor making a small movement forward.");

      }

      digitalWrite(sleepPin, HIGH); //wake up!

      digitalWrite(dirPin, HIGH); //run forward


      for (int i =0; i < step_num; i++) {

        digitalWrite(stepPin, HIGH); //step!  
        delayMicroseconds(100);               
        digitalWrite(stepPin, LOW);  
        delay(delay_time);
      } 

      state = 's'; // stop after small movement

      break;

    case 'b': //small movement backward

      if( state != state_old ) { // if you got something new, say what you're doing

        Serial.println("Motor making a small movement backward.");

      }

      digitalWrite(sleepPin, HIGH); //wake up!

      digitalWrite(dirPin, LOW); //run backward


      for (int i =0; i < step_num; i++) {

        digitalWrite(stepPin, HIGH); //step!  
        delayMicroseconds(10);               
        digitalWrite(stepPin, LOW);  
        delay(delay_time);
      } ;

      state = 's'; // stop after one step

      break;

    case 'm': //microstepping

      if( state != state_old ) { // if you got something new, say what you're doing    
        
        state_old = state;
        
        Serial.println("To switch to half-step mode, enter 2.");
        Serial.println("To switch to quarter-step mode, enter 4.");
        Serial.println("To switch to eighth-step mode, enter 8.");
        
        while(Serial.available() == 0) { }

        if (Serial.available() > 0) { //check for incoming data
          microstepMode = ( Serial.read() - '0' ); // read the incoming byte:
          
          // Serial.print("microstepMode: ");
          // Serial.println(microstepMode, DEC);
        }
        
        digitalWrite(sleepPin, HIGH); //wake up!
        digitalWrite(dirPin, HIGH); //run forward

        digitalWrite(ms1Pin, setMs1Mode(microstepMode));
        digitalWrite(ms2Pin, setMs2Mode(microstepMode));


        while(Serial.available() == 0) { 

          digitalWrite(stepPin, LOW);
          digitalWrite(stepPin, HIGH);
          // delay(delay_time/microstepMode);
          delay(1600/microstepMode);

        }

      }



      



      

    default:
      Serial.println("Wrong input.");
      print_directions();

      //print instructions only once
      while(Serial.available() == 0) { }
  }

// output encoder tick
  if (oldPos != encoderPos) {

    int revolution = encoderPos/4000;
    int encoderTick = encoderPos%4000;

    Serial.print("Encoder Revolution: ");
    Serial.print(revolution, DEC);
    Serial.print("\t Encoder Tick: ");
    Serial.println(encoderTick, DEC);
    oldPos = encoderPos;
  }

  // if (oldRev != encoderRev) {
  //   Serial.print("Revolution: ");
  //   Serial.println(encoderRev, DEC);
  //   oldRev = encoderRev;
  // }




}





//print standard directions
void print_directions(){

  Serial.println("To run motor forward, enter 'F'.");
  Serial.println("To run motor backward, enter 'B'.");
  Serial.println("To stop motor, enter 's'.");
  Serial.println("To make a small movement forward, enter 'f'.");
  Serial.println("To make a small movement backward, enter 'b'.");
  Serial.println("To do microstepping, enter 'm'.");

}

// microstep functions-------------------------------

void microstepping(){



}

int setMs1Mode(int ms1StepMode){              // A function that returns a High or Low state number for MS1 pin
  switch(ms1StepMode){                      // Switch statement for changing the MS1 pin state
                                             // Different input states allowed are 1,2,4 or 8
  case 1:
    ms1StepMode = 0;
    Serial.println("Step Mode is Full...");
    break;
  case 2:
    ms1StepMode = 1;
    Serial.println("Step Mode is Half...");
    break;
  case 4:
    ms1StepMode = 0;
    Serial.println("Step Mode is Quarter...");
    break;
  case 8:
    ms1StepMode = 1;
    Serial.println("Step Mode is Eighth...");
    break;
  }
  return ms1StepMode;
}
 

int setMs2Mode(int ms2StepMode){              // A function that returns a High or Low state number for MS2 pin
  switch(ms2StepMode){                      // Switch statement for changing the MS2 pin state
                                             // Different input states allowed are 1,2,4 or 8
  case 1:
    ms2StepMode = 0;
    break;
  case 2:
    ms2StepMode = 0;
    break;
  case 4:
    ms2StepMode = 1;
    break;
  case 8:
    ms2StepMode = 1;
    break;
  }
  return ms2StepMode;
}


// Encoder Functions--------------------------------------------------


void HandleInterruptA(){

  //heart of the step
  (Bnew^Aold && B1new^A1old) ? encoderPos++ : encoderPos-- ; // XOR for normal and inverted channels and comparison 
  
  Aold = fastDigitalRead(encoderPinA);
  A1old = fastDigitalRead(encoderPinA1);
  // Iold = fastDigitalRead(encoderPinI);
  // if ( Iold && Aold ) encoderRev++ ;

  // Serial.println( Iold );

}

// Interrupt on B changing state
void HandleInterruptB(){

  Bnew=fastDigitalRead(encoderPinB);
  B1new=fastDigitalRead(encoderPinB1);

  //heart of the step
  (Bnew^Aold && B1new^A1old) ? encoderPos++:encoderPos--;// XOR for normal and inverted channels and comparison

}
