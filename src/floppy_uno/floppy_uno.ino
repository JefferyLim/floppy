// floppy - Independent Study Spring 2017 with Professor Shalom Ruben

#include <TimerOne.h>  

#define NUMDRIVES 7 //Number of Drives being used

#define MAXSTEPS 150 //Maximum number of steps the head can go
#define RESOLUTION 100 //us resolution of timer

//(125 cycles) * (128 prescaler) / (16MHz clock speed) = 1ms
//(1 * 128) / 16MHz

volatile int stepPins[NUMDRIVES + 1] = { A1, A3, A5, 1, 3, 5, 7, 9}; //ODD PINS
volatile int dirPins[NUMDRIVES + 1]  = { A0, A2, A4, 2, 4, 6, 8, 10}; //EVEN PINS

volatile int flag = 1;

//drive head position
volatile int headPos[NUMDRIVES + 1];

//period counter to match note period
volatile int periodCounter[NUMDRIVES + 1];

//the note period 
volatile int notePeriod[NUMDRIVES + 1];

//State of each drive
volatile boolean driveState[NUMDRIVES + 1]; //1 or 0

//Direction of each drive
volatile boolean driveDir[NUMDRIVES + 1]; // 1 -> forward, 0 -> backwards

//MIDI bytes
byte midiStatus, midiChannel, midiCommand, midiNote, midiVelocity;

//Freq = 1/(RESOLUTION * Tick Count)
static int noteLUT[127];

void setup()  { 

  //Init parameters
  int i,j;
  for(i = 0; i < NUMDRIVES + 1; i++){
    driveDir[i] = 1; //Set initially at 1 to reset all drives
    driveState[i] = 0;

    periodCounter[i] = 0;
    notePeriod[i] = 0;

    headPos[i] = 0;

  }

  //Midi note setup found on wikipedia page on tuning standards
  double midi[127];
  int a = 440; // a is 440 hz...
  for (i = 0; i < 128; ++i)
  {
    midi[i] = a*pow(2, ((double)(i-69)/12));
  }

  //1/(resolution * noteLUT) = midi -> midi*resolution = 1/noteLUT
  for(i = 0; i < 127; i++){
    noteLUT[i] = 1/(2*midi[i]*RESOLUTION*.000001);
  }

  //Pin Setup 
  for(i = 0; i < NUMDRIVES + 1; i++){
    pinMode(stepPins[i], OUTPUT);
    pinMode(dirPins[i], OUTPUT);
  }

  //Drive Reset
  for(i = 0; i < 80; i++){
    for(j = 0; j < NUMDRIVES + 1; j++){
      digitalWrite(dirPins[j], driveDir[j]);
      digitalWrite(stepPins[j], 0);
      digitalWrite(stepPins[j], 1);
    } 

    delay(50);
  } 

  //Set Drive Pins to forward direction
  for(i = 0; i < NUMDRIVES + 1; i++){
    driveDir[i] = 0; 
    digitalWrite(dirPins[i], driveDir[i]);
  }  

  delay(1000);

  //Set timer1 interrupt and initialize
  Timer1.initialize(RESOLUTION);
  Timer1.attachInterrupt(count);


  //Serial for MIDI to Serial Drivers
  Serial.begin(115200);   
} 

void loop(){

  //Only looking for 3 byte command
  if(Serial.available() == 3){
    midiStatus = Serial.read(); //MIDI Status
    midiNote = Serial.read(); //MIDI Note
    midiVelocity = Serial.read(); //MIDI Velocity
     
    //Parse out the channel and the command
    midiChannel = midiStatus & B00001111;
    midiCommand = midiStatus & B11110000;
    
    //Ensure we are not going over the number of drives we have
    if(midiChannel < NUMDRIVES + 1){
    //Note On, set notePeriod to the midi note period
      if(midiCommand == 0x90 && midiVelocity != 0){ 
        notePeriod[midiChannel] = noteLUT[midiNote];
      }
      //Note off, Channel Off, velocity = 0
      else if(midiCommand == 0x80 || (midiCommand == 0xB0 && midiNote == 120) || midiVelocity == 0){
        notePeriod[midiChannel] = 0;
      }
    }
}  

}

void count(){
  int i;
  //For each drive
  for(i = 0; i < NUMDRIVES + 1; i++){
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
    }
    else{
      //Set Counter to 0
      periodCounter[i] = 0;

    }
  } 
}


