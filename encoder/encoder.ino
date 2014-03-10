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

int step_num = 20;     //  number of steps for "small" movement

// incoming serial data for controlling the motor state
char state = '~';   
char state_old = '~';

int microstep = 0;   
int microstep_old = 0;   

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
#define encoderPinA1 4 //0 // inverted channels
#define encoderPinB1 5 //1 // inverted channels

#define MS1 9
#define MS2 8

// int encoderInterruptA = 0;
// int encoderInterruptB = 1;
int encoderInterruptA = 1;
int encoderInterruptB = 0;

volatile long encoderPos = 0;
volatile long oldPos = 0;
volatile long encoderRev = 0;
volatile long oldRev = 0;

volatile long Aold = 0;
volatile long Bnew = 0;
volatile long A1old = 1; // inverted channels
volatile long B1new = 1; // inverted channels

volatile long ASet = 0;
volatile long BSet = 0;
volatile long APrev = 0;
volatile long BPrev = 0;
volatile long A1Set = 1; // inverted channels
volatile long B1Set = 1; // inverted channels




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


  pinMode(MS1, OUTPUT);   // set pin 13 to output
  pinMode(MS2, OUTPUT);   // set pin 9 to output
  // delay_time = 60L * 1000L / stepsPerRevolution / speedRPM; // calculate delay time to match desired speed in RPM
                                                            // seconds*milliseconds/ (stepsPerRevolution * speedRPM )
  delay_time = 100000;

  print_directions();

}






void loop() 
{ 

  delay(1000);
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

        if( ( microstep != 0 ) && ( microstep != microstep_old ) ) {
          digitalWrite(MS1, MS1_MODE(microstep));
          digitalWrite(MS2, MS2_MODE(microstep));
        }

        digitalWrite(stepPin, HIGH); //step!  
        delayMicroseconds(10);               
        digitalWrite(stepPin, LOW);  
        // delayMicroseconds(1000);
        delayMicroseconds(delay_time);
    
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

      if( ( microstep != 0 ) && ( microstep != microstep_old ) ) {
        digitalWrite(MS1, MS1_MODE(microstep));
        digitalWrite(MS2, MS2_MODE(microstep));
      }

      digitalWrite(stepPin, HIGH); //step!  
      delayMicroseconds(10);               
      digitalWrite(stepPin, LOW);  
      delayMicroseconds(delay_time);
      // delayMicroseconds(1000);
    
      }

      break;

    case 's': //equals an s

      if( state != state_old ) { // if you got something new, say what you're doing

        Serial.println("Motor stopping.");
        state_old = state;
    
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
        delayMicroseconds(delay_time);
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
        delayMicroseconds(delay_time);
      } ;

      state = 's'; // stop after one step

      break;




    case '2': 

      if( state != state_old ) { // if you got something new, say what you're doing

        Serial.println("Step Mode is Half.");
        state_old = state;

      }

      microstep =2;


      break;

    case '4': 

      if( state != state_old ) { // if you got something new, say what you're doing

        Serial.println("Step Mode is Quarter.");
        state_old = state;

      }

      microstep =4;


      break;

    case '8': 

      if( state != state_old ) { // if you got something new, say what you're doing

        Serial.println("Step Mode is Eigth.");
        state_old = state;

      }

      microstep =8;


      break;

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
    Serial.print(encoderTick, DEC);
    Serial.print("\t Encoder Position: ");
    Serial.println(encoderPos, DEC);
    oldPos = encoderPos;
  }



}





//print standard directions
void print_directions(){

  Serial.println("To run motor forward, enter 'F'.");
  Serial.println("To run motor backward, enter 'B'.");
  Serial.println("To stop motor, enter 's'.");
  Serial.println("To make a small movement forward, enter 'f'.");
  Serial.println("To make a small movement backward, enter 'b'.");

}




// Encoder Functions--------------------------------------------------


void HandleInterruptA(){

  //heart of the step
  // (Bnew^Aold && B1new^A1old) ? encoderPos++ : encoderPos-- ; // XOR for normal and inverted channels and comparison 
  // Bnew^Aold ? encoderPos++ : encoderPos-- ; // XOR for normal and inverted channels and comparison 
  
  // if (  ( Bnew != B1new ) && ( Aold != A1old ) ) {
    
    if ( Bnew^Aold ) {

      if ( B1new^A1old ) 
        encoderPos++;

    } else if( !(Bnew^Aold) ) {

      if( !(B1new^A1old) ) 
        encoderPos--;
    } 
    
  // }


  // if (  ( Bnew != B1new ) && ( Aold != A1old ) ) {
    
  //   Bnew^Aold ? encoderPos++ : encoderPos-- ;
    
  // }



  Aold = fastDigitalRead(encoderPinA);
  A1old = fastDigitalRead(encoderPinA1);

}

// Interrupt on B changing state
void HandleInterruptB(){

  Bnew=fastDigitalRead(encoderPinB);
  B1new=fastDigitalRead(encoderPinB1);

  //heart of the step
  // (Bnew^Aold && B1new^A1old) ? encoderPos++ : encoderPos--;// XOR for normal and inverted channels and comparison
  // Bnew^Aold ? encoderPos++ : encoderPos--;// XOR for normal and inverted channels and comparison
  
  // if (  ( Bnew != B1new ) && ( Aold != A1old ) ) {
    
    if ( Bnew^Aold ) {

      if ( B1new^A1old ) 
        encoderPos++;

    } else if( !(Bnew^Aold) ) {

      if( !(B1new^A1old) ) 
        encoderPos--;
    } 

  // }

  // if (  ( Bnew != B1new ) && ( Aold != A1old ) ) {
    
  //   Bnew^Aold ? encoderPos++ : encoderPos-- ;
    
  // }

  


}




// // Interrupt service routines for the left motor's quadrature encoder
// void HandleInterruptA(){
//   BSet = fastDigitalRead(encoderPinB);
//   ASet = fastDigitalRead(encoderPinA);
//   B1Set = fastDigitalRead(encoderPinB1);
//   A1Set = fastDigitalRead(encoderPinA1);
  
//   if ( (BSet == B1Set) || (ASet == A1Set) ) return;

//   encoderPos+=ParseEncoder();
  
//   APrev = ASet;
//   BPrev = BSet;
// }

// // Interrupt service routines for the right motor's quadrature encoder
// void HandleInterruptB(){
//   // Test transition;
//   BSet = fastDigitalRead(encoderPinB);
//   ASet = fastDigitalRead(encoderPinA);
//   B1Set = fastDigitalRead(encoderPinB1);
//   A1Set = fastDigitalRead(encoderPinA1);

//   if ( (BSet == B1Set) || (ASet == A1Set) ) return;

//   encoderPos+=ParseEncoder();
  
//   APrev = ASet;
//   BPrev = BSet;
// }

// int ParseEncoder(){
//   if(APrev && BPrev){
//     if(!ASet && BSet) return 1;
//     if(ASet && !BSet) return -1;
//   }else if(!APrev && BPrev){
//     if(!ASet && !BSet) return 1;
//     if(ASet && BSet) return -1;
//   }else if(!APrev && !BPrev){
//     if(ASet && !BSet) return 1;
//     if(!ASet && BSet) return -1;
//   }else if(APrev && !BPrev){
//     if(ASet && BSet) return 1;
//     if(!ASet && !BSet) return -1;
//   }
// }

int MS1_MODE(int MS1_StepMode){              // A function that returns a High or Low state number for MS1 pin
   switch(MS1_StepMode){                      // Switch statement for changing the MS1 pin state
                                              // Different input states allowed are 1,2,4 or 8 
                                              //http://danthompsonsblog.blogspot.de/2010_05_01_archive.html
   case 1:
     MS1_StepMode = 0;
     break;
   case 2:
     MS1_StepMode = 1;
     break;
   case 4:
     MS1_StepMode = 0;
     break;
   case 8:
     MS1_StepMode = 1;
     break;
   }
   return MS1_StepMode;
 }
  
  
  
 int MS2_MODE(int MS2_StepMode){              // A function that returns a High or Low state number for MS2 pin
   switch(MS2_StepMode){                      // Switch statement for changing the MS2 pin state
                                              // Different input states allowed are 1,2,4 or 8
   case 1:
     MS2_StepMode = 0;
     break;
   case 2:
     MS2_StepMode = 0;
     break;
   case 4:
     MS2_StepMode = 1;
     break;
   case 8:
     MS2_StepMode = 1;
     break;
   }
   return MS2_StepMode;
 }