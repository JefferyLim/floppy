// floppy - Independent Study Spring 2017 with Professor Shalom Ruben
// Needs to be used with a MiDi to Serial driver http://www.varal.org/ttymidi/ 
//ttymidi -b 115200 -s /dev/ttyACM0 -v
//aconnect -i and aconnect -o to find output and input drivers

#include <TimerOne.h>
#include <MIDI.h>

#define NUMDRIVES 2
#define MAXSTEPS 130
#define LED 13
#define RESOLUTION 50

//Pin 13 is reserved for LED

//drive Count starts at 12 i.e Drive 1: stepPins[1] = 12, dirPins[1] = 11
volatile int stepPins[NUMDRIVES]; //EVEN PINS
volatile int dirPins[NUMDRIVES]; //ODD PINS

//drive head position
volatile int headPos[NUMDRIVES];

//period counter to match note period
volatile int periodCounter[NUMDRIVES];

//the note period 
volatile int notePeriod[NUMDRIVES];

//State of each drive
volatile boolean driveState[NUMDRIVES]; //1 or 0

//Direction of each drive
volatile boolean driveDir[NUMDRIVES]; // 1 -> forward, 0 -> backwards

//MIDI bytes
byte midiStatus, midiChannel, midiCommand, midiNote;

static const int noteLUT[50] = {
 727, // 0
 727, // 1
 686, // 2
 648, // 3
 612, // 4
 577, // 5
 545, // 6
 514, // 7
 485, // 8
 458, // 9
 432, // 10
 408, // 11
 385, // 12
 364, // 13
 343, // 14
 324, // 15
 306, // 16
 289, // 17
 272, // 18
 257, // 19
 243, // 20
 229, // 21
 216, // 22
 204, // 23
 193, // 24
 182, // 25
 172, // 26
 162, // 27
 153, // 28
 144, // 29
 136, // 30
 129, // 31
 121, // 32
 115, // 33
 108, // 34
 102, // 35
 96, // 36
 91, // 37
 86, // 38
 81, // 39
 76, // 40
 72, // 41
 68, // 42
 64, // 43
 61, // 44
 57, // 45
 54, // 46
 51, // 47
 48, // 48 (that's the highest note we can play (FLOPPY_HIGHES_NOTE)!)
 48 // 49
};

void setup()  { 
 
  
  //Init parameters
  int i,j;
  for(i = 0; i < NUMDRIVES; i++){
     driveDir[i] = 1; //Set initially at 1 to reset all drives
     driveState[i] = 0;
     
     periodCounter[i] = 0;
     notePeriod[i] = 0;
     
     headPos[i] = 0;
     
     stepPins[i] = (12-2*i);
     dirPins[i] = (12-2*i-1);
  }
  
  
  //Pin Setup 
  pinMode(LED, OUTPUT); 
  
  for(i = 0; i < NUMDRIVES; i++){
    pinMode(stepPins[i], OUTPUT);
    pinMode(dirPins[i], OUTPUT);
  }
  
  //Drive Reset
  for(i = 0; i < 80; i++){
    for(j = 0; j < NUMDRIVES; j++){
      digitalWrite(dirPins[j], driveDir[j]);
      digitalWrite(stepPins[j], 0);
      digitalWrite(stepPins[j], 1);
    } 
    
    delay(50);
  } 
  
  //Set Drive Pins to forward direction
  for(i = 0; i < NUMDRIVES; i++){
    driveDir[i] = 0; 
    digitalWrite(dirPins[i], driveDir[i]);
  }  
  
  delay(1000);
  
  Timer1.initialize(RESOLUTION); //1200 microseconds * 1 = 800 Hz, but 400 Hz output.
  Timer1.attachInterrupt(count);
  
  Serial.begin(115200); //Serial for MIDI to Serial Drivers
} 

void loop()  {  
  
  if(Serial.available() > 2){
      midiStatus = Serial.read(); //MIDI Status
      midiNote = Serial.read(); //MIDI Note
      Serial.read(); //Ignoring MIDI Velocity
      
      midiChannel = midiStatus & B00001111;
      midiCommand = midiStatus & B11110000;
     
     if(midiChannel == 0){
       if(midiCommand == 0x90 && midiNote < 50){
         notePeriod[0] = noteLUT[midiNote];
       }else if(midiCommand == 0x80){
         notePeriod[0] = 0;
       }
    }else if(midiChannel == 1){
       if(midiCommand == 0x90 && midiNote < 50){
         notePeriod[1] = noteLUT[midiNote];
       }else if(midiCommand == 0x80){
         notePeriod[1] = 0;
       }
    }
  }
}

void count(){
  
  int i;
  //For each drive
  for(i = 0; i < NUMDRIVES; i++){
    //If the desired drive is suppose to be ticking
    if(notePeriod[i] > 0){
      //tick the drive
      periodCounter[i]++;
        
      //If the drive has reached the desired period
      if(periodCounter[i] >= notePeriod[i]){
        
        //Flip the drive
        driveState[i] ^= 1;
        digitalWrite(stepPins[i], driveState[i]);
                
        //reset the counter
        periodCounter[i] = 0; //IT WAS == AND I COULDN'T FIGURE OUT WHY IT WASN'T WORKING
        
        headPos[i]++;
        //If the drive is at the maximum step, reset its direction
        if(headPos[i] >= MAXSTEPS){
          headPos[i] = 0;
          driveDir[i] ^= 1;
          digitalWrite(dirPins[i], driveDir[i]);
        }
        

      }
    }else{
      //Set Counter to 0
      periodCounter[i] = 0;
    
    }
  }
 
}

