// floppy - Independent Study Spring 2017 with Professor Shalom Ruben

#include <TimerOne.h>  
#include <TimerThree.h>

#include <SD.h>

#define NUMDRIVES 7 //Number of Drives being used

#define MAXSTEPS 150 //Maximum number of steps the head can go
#define RESOLUTION 100 //us resolution of timer

//(125 cycles) * (128 prescaler) / (16MHz clock speed) = 1ms
//(1 * 128) / 16MHz

volatile int stepPins[NUMDRIVES + 1] = {
  A0, A2, A4, 9, A5, A3, A1}; //ODD PINS
volatile int dirPins[NUMDRIVES + 1]  = {
  A1, A3, A5, 8, A4, A2, A0}; //EVEN PINS

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

volatile int tick = 0;

volatile int wait = 0;

File song;
byte input[100];

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

  //Serial for MIDI to Serial Drivers
  Serial.begin(115200);   

  pinMode(53, OUTPUT);
  SD.begin(53);

  song = SD.open("getLucky.mid");

  if(song){
    Serial.print("File Found");
  }

  int header = 1;

  int trackNum;
  int fileType;
  int timeSig;
  int keySig;

  uint32_t PPQN=0; //Parts Per Quarter Note
  double BPM =0; //Beats Per Minute
  double usPerTick=0; //usPerTick

  int length = 0; 

  if(header == 1){
    //Header
    song.read(input, 4);
    Serial.print(input[0], HEX);
    Serial.print(" ");
    Serial.print(input[1], HEX);
    Serial.print(" ");
    Serial.print(input[2], HEX);
    Serial.print(" ");
    Serial.print(input[3], HEX);
    Serial.print("\n");

    //Length of Data
    song.read(input, 4);
    Serial.print(input[0], HEX);
    Serial.print(" ");
    Serial.print(input[1], HEX);
    Serial.print(" ");
    Serial.print(input[2], HEX);
    Serial.print(" ");
    Serial.print(input[3], HEX);
    Serial.print("\n");

    //MIDI Format
    song.read(input, 2);
    Serial.print(input[0], HEX);
    Serial.print(" ");
    Serial.print(input[1], HEX);
    Serial.print("\n");

    fileType = input[1];

    //Number of Tracks
    song.read(input, 2);
    Serial.print(input[0], HEX);
    Serial.print(" ");
    Serial.print(input[1], HEX);
    Serial.print("\n");

    trackNum = input[1];

    //Time Divison or PPQN
    song.read(input, 2);
    Serial.print(input[0], HEX);
    Serial.print(" ");
    Serial.print(input[1], HEX);
    Serial.print("\n");

    PPQN = ((input[0]) << 8) + input[1];

header:
    //Track Header
    song.read(input, 4);
    Serial.print(input[0], HEX);
    Serial.print(" ");
    Serial.print(input[1], HEX);
    Serial.print(" ");
    Serial.print(input[2], HEX);
    Serial.print(" ");
    Serial.print(input[3], HEX);
    Serial.print("\n");

    //Track Length
    song.read(input, 4);
    Serial.print(input[0], HEX);
    Serial.print(" ");
    Serial.print(input[1], HEX);
    Serial.print(" ");
    Serial.print(input[2], HEX);
    Serial.print(" ");
    Serial.print(input[3], HEX);
    Serial.print(" ");
    Serial.print("\n");

    song.read(input, 1);
    Serial.print(input[0], HEX);
    Serial.print("\n");  

    while(song.peek() == 0xFF){
      song.read(input, 1);
      Serial.print(input[0], HEX);
      Serial.print(" ");
      song.read(input, 1);
      Serial.print(input[0], HEX);
      Serial.print(" ");

      if(input[0] == 0x51){

        song.read(input, 1);

        Serial.print(input[0], HEX);
        Serial.print(" ");

        length = input[0];

        song.read(input, length);

        usPerTick = ((long)input[0] << 16) + ((long)input[1] << 8) + input[2];
        BPM = 60000000/usPerTick;


        for (int i = 0; i < length; i++){
          Serial.print(" ");
          Serial.print(input[i], HEX);
        }
        Serial.print("\n");
      }
      else if(input[0] == 0x59){
        song.read(input, 1);

        Serial.print(input[0], HEX);
        Serial.print(" ");

        length = input[0];

        song.read(input, length);

        for (int i = 0; i < length; i++){
          Serial.print(" ");
          Serial.print(input[i], HEX);
        }
        Serial.print("\n");
      }
      else if(input[0] == 0x2F){
        song.read(input, 1); // 0x00
        Serial.print(input[0], HEX);
        Serial.print("\n");

        goto header;


      }
      else{
        song.read(input, 1);

        Serial.print(input[0], HEX);
        Serial.print(" ");

        length = input[0];

        song.read(input, length);
        for (int i = 0; i < length; i++){
          Serial.print(" ");
          Serial.print(input[i], HEX);
        }
        Serial.print("\n");
      }

      song.read(input, 1); // 0x00
      Serial.print(input[0], HEX);
      Serial.print("\n");

    }
  }  


  header = 0;
  BPM = 120;

  PPQN = 192;
  //This is the calculation needed to determine how much each tick lasts
  //A good source was found here:
  //http://stackoverflow.com/questions/24717050/midi-tick-to-millisecond
  usPerTick = round(1000*60000/(BPM*PPQN)); 


  //Set timer1 inter60000/(BPM*PPQN) rupt and initialize
  Timer1.initialize(RESOLUTION);
  Timer1.attachInterrupt(count);

  Timer3.initialize(usPerTick);
  Timer3.attachInterrupt(parse);


} 

void loop(){

  while(flag == 0);

  song.read(input, 2);
  midiStatus = input[0];
  midiNote = input[1];

  Serial.print(midiStatus , HEX);
  Serial.print(" ");
  Serial.print(midiNote, HEX);
  Serial.print(" ");


  midiChannel = midiStatus & B00001111;
  midiCommand = midiStatus & B11110000;

  //This is the message for the end of a track
  //This part is where I left on because I realized
  //that the midi file would need to be parsed through entirely
  if(midiStatus == 0xFF && midiNote == 0x2F){
    while(1);
  }

  if(midiCommand == 0xC0 || midiCommand == 0xD0){
  }
  else{


    song.read(input, 1);
    midiVelocity = input[0];

    Serial.println(midiVelocity, HEX); 

  }

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

  int temp = 0;
  wait = 0;
  while(song.peek() >= 0x80){
    song.read(input, 1);
    Serial.print(input[0], HEX);
    Serial.print(" ");
    wait = input[0] & 0x7F;
    wait = wait << 7;
  }
  song.read(input ,1);
  Serial.println(input[0], HEX);

  wait |= input[0];
  flag = 0;

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

//ISR for parsing through the MIDI tracks
void parse(){  
  //If the flag is 0, we should be ticking
  if(flag == 0){
    tick++;

    //If the wait time from the delta time is reached
    if(tick >= wait){
      tick = 0;
      //Raise the flag to let the main loop that it can get the next message
      flag = 1;
    }
  }
  else{
    tick = 0;
  }

}





