/*
 initial run at doing an arduino sequencer
 */
 
// special characters
int off = 0; // have to use int to prevent thinking it's a null
int on =  1;

// These conflict with the TR808 trigger shield and will need to be changed.
// don't use A0 for them either :-)
#define CLOCKOUT 12
#define RESET A4
#define RUN A5
#define BUTTONS A1
#define VOICES 7

// just to be sure we remember NOT to use these
#define ACCENT 13
// same as CLOCKOUT but that's ok, same basic meaning
#define TRIG 12
// BDGate can be assigned anywhere

// Analog input for BPM same as trigger shield
#define BPM 0

// include the library code:
#include <LiquidCrystal.h>
#include <Encoder.h>
#include <avr/interrupt.h>  
#include <avr/io.h>


// initialize the libraries with the numbers of the interface pins
LiquidCrystal lcd(9, 10, 5, 6, 7, 8);
Encoder knob(3, 4);

// special characters
byte onChar[8] = {
  B00000,
  B01110,
  B11111,
  B11111,
  B11111,
  B01110,
  B00000,
  B00000,
};

byte offChar[8] = {
  B00000,
  B01110,
  B10001,
  B10001,
  B10001,
  B01110,
  B00000,
  B00000,
};

int width = 16; // width of sequencer, not width of display
int prevWidth = 16; // track changes so we update display correctly
char outputString[7]; // so we get justified output; +1 for null terminator
int runMode = 1;
int resetFlag = 1; // normally high input
int currentCol = 0;
int rawBPM = 0;
int bpm = 10;
int bpmTime = 6000;
int prevMS = 0;
int curMS = 0;
int button = 0;
int voice = 0;
int pos = 0;

volatile unsigned int count = 0;   //used to keep count of how many interrupts were fired

//Timer2 Overflow Interrupt Vector, called every 1ms
ISR(TIMER2_OVF_vect) {
  count++;               //Increments the interrupt counter
  
  TCNT2 = 130;           //Reset Timer to 130 out of 255
  TIFR2 = 0x00;          //Timer2 INT Flag Reg: Clear Timer Overflow Flag
};  


void setup() {
  //Serial.begin(9600);
  //Serial.println("tracking BPM actual times");
  pinMode(RUN, INPUT); // read run/stop switch
  pinMode(RESET, INPUT); // reset momentary can't use INPUT_PULLUP for non-digital pins, apparently
  digitalWrite(RESET, HIGH); // pull up
  pinMode(CLOCKOUT, OUTPUT); // send a clock to other devices
  pinMode(BPM, INPUT); // huh, how does this work?
  //pinMode(13,OUTPUT); // temporary
  // set up the LCD's number of columns and rows: 
  lcd.begin(24, 2);
  lcd.createChar(off,offChar);
  lcd.createChar(on,onChar);
  
    lcd.setCursor(0,0);
    for (int i=0; i< width; i++) {
      lcd.write(off);
    }
    lcd.setCursor(0,1);
    for (int i=0; i< width; i++) {
      lcd.write(off);
    }
    lcd.setCursor(0,1);
    lcd.write(on);
    currentCol = -1; // make sure we start with beat 1 not beat 2

  //Setup Timer2 to fire every 1ms
  TCCR2B = 0x00;        //Disbale Timer2 while we set it up
  TCNT2  = 130;         //Reset Timer Count to 130 out of 255
  TIFR2  = 0x00;        //Timer2 INT Flag Reg: Clear Timer Overflow Flag
  TIMSK2 = 0x01;        //Timer2 INT Reg: Timer2 Overflow Interrupt Enable
  TCCR2A = 0x00;        //Timer2 Control Reg A: Normal port operation, Wave Gen Mode normal
  TCCR2B = 0x05;        //Timer2 Control Reg B: Timer Prescaler set to 128
  //lcd.cursor();
  //lcd.blink();
}

void loop() {
  runMode = digitalRead(RUN);
  resetFlag = digitalRead(RESET);
  rawBPM = analogRead(BPM);
  bpm = (rawBPM / 4) + 10;
  bpmTime = 60000/bpm; // one minute in milliseconds
  button = analogRead(BUTTONS) / 100; // rounding, currently 3 buttons, 4, 6, 8 (0 is off)
  if (resetFlag == 0) {
    //knob.write(0);
    lcd.setCursor(currentCol, 1);
    lcd.write(off);  // clear the current step
    lcd.setCursor(0,1);
    lcd.write(on);
    currentCol = -1; // want next cycle to start at 0 after increment
  }
  if (runMode == 1) {
    // CANNOT DO LCD STUFF INSIDE THE INTERRUPT CONTEXT
    if(count >= bpmTime) {
      lcdUpdate();
      count = 0;      //Resets the interrupt counter
      /************
      digitalWrite(BDGate, HIGH);
      if (alternate == 1) {
        digitalWrite(ACGate, HIGH);
      }
      pulse(Trig);  // should pass an array of pins to pulse
      digitalWrite(BDGate, LOW);
      if (alternate == 1) {
        digitalWrite(ACGate, LOW);
        alternate = 0;
      } else {
        alternate = 1;
      }
      digitalWrite(ACGate, LOW);
      *************/
      //Serial.print("BPM = ");
      //Serial.print(bpm);
      //Serial.print(" bpmTime = ");
      //Serial.println(bpmTime);
      //Serial.print("Actual time: ");
      //curMS = millis();
      //Serial.println(curMS - prevMS);
      //prevMS =curMS;
    }
    lcd.setCursor(17,0);
    sprintf(outputString, "BPM%3d", bpm); // DO NOT scroll off the end here!
    lcd.print(outputString);
  }
  if (button == 0) {
    prevWidth = width;
    width += int(knob.read()/2);
    if (width > 16)
      width = 16;
    if (width < 1)
      width = 1;
    knob.write(0);
    if (width != prevWidth) {
      if (width < prevWidth) {
        for (int i = width; i<16; i++) {
          lcd.setCursor(i,0);
          lcd.write(' ');
          lcd.setCursor(i,1);
          lcd.write(' ');
        }
      } else {
        for (int i= prevWidth; i<width; i++) {
          lcd.setCursor(i,0);
          lcd.write(off);
          lcd.setCursor(i,1);
          lcd.write(off);
        }
      }
    }
  } else if ((runMode == 0) && (button < 5)) { // change voice
    voice += int(knob.read()/2);
    if (voice >= VOICES)
      voice = VOICES - 1;
    if (voice < 0)
      voice = 0;
    knob.write(0);
    lcd.setCursor(17,0);
    sprintf(outputString, "Vox%3d", voice); // DO NOT scroll off the end here!
    lcd.print(outputString);
  } else if ((runMode = 0) && (button < 7)) { // change position in sequence
  } else if ((runMode = 0) && (button < 9)) { // toggle current position
  } else { 
    lcd.setCursor(17,1);
    sprintf(outputString, "%6d", button); // do not scroll off the end here!
    // itoa(loopCount, loopCountString, 10); // smaller, still needs to be justified
    lcd.print(outputString); // needs to be right justified and space padded
  }
}

void lcdUpdate() {
  if (runMode == 1) {
    lcd.setCursor(currentCol, 1);
    if (currentCol < width) {
      lcd.write(off);
    } else {
      lcd.write(' ');
    }
    currentCol = (currentCol + 1) % width;
    lcd.setCursor(currentCol, 1);
    lcd.write(on);
  }
}

// 1ms pulse output-- need to test how this will interact with the interrupt service routines
void pulse(int pin)
{
  digitalWrite(pin,HIGH);
  delay(1); // ms
  /* or possibly...
   all appropriate pins high
   check count to see if it's >1 (zero it before calling the pulse)
   all pins low
   */
  digitalWrite(pin,LOW);
}
