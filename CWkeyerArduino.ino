// Arduino Morse Keyer by Charlie Davy M0PZT  www.m0pzt.com
// Last revised: 16th April 2015
// CW Routines by SM4XAS

// Very slightly modified by Roger, EA3ALZ

int         pinSidetone = 12;   // Tone output pin
int         pinTX       = 11;   // Key/PTT output pin
int         pinDit      = 2;    // Left paddle input
int         pinDah      = 3;    // Right paddle input
int         pinKey      = 4;    // Straight Key
int         pinLED      = 13;   // LED
int         pinMsg1      = 5;   // LED
int         pinMsg2      = 6;   // LED
int         pinMsg3      = 7;   // LED
int         pinMsg4      = 8;   // LED
int         pinWPM      = 10;   // Gnd for 18wpm, +v for 22wpm
// above line not used now due to WPM Pot

int         pinWPMPot = A0;     // WPM Speed Adjust
int MorseWPM = 20;

#define     DIT_L      0x01     // Dit latch
#define     DAH_L      0x02     // Dah latch
#define     DIT_PROC   0x04     // Dit is being processed
#define     PDLSWAP    0x08     // 0 for normal, 1 for swap
#define     IAMBICB    0x10     // 0 for Iambic A, 1 for Iambic B
 
unsigned long       ditTime;                    // No. milliseconds per dit
unsigned char       keyerControl;
unsigned char       keyerState;
 
#define freqSidetone  700

#define MyMsg1 "TEST EA3ALZ EA3ALZ"
#define MyMsg2 "5NN A4"
#define MyMsg3 "CECILIA"
#define MyMsg4 "V V V"
 
enum KSTYPE {IDLE, CHK_DIT, CHK_DAH, KEYED_PREP, KEYED, INTER_ELEMENT };
 
int incomingByte = 0;   // for incoming serial data
String inputString = "";         // a string to hold incoming data
//String COMdata = "";
boolean stringComplete = false;  // whether the string is complete
boolean KeyDown = false;  // whether the string is complete

// Bung Morse into an array
char* MorseCodeCharacters[] = {
"A", ".-", 
"B","-...", 
"C","-.-.", 
"D","-..", 
"E",".", 
"F","..-.", 
"G","--.", 
"H","....", 
"I","..", 
"J",".---", 
"K","-.-", 
"L",".-..", 
"M","--", 
"N","-.", 
"O","---", 
"P",".--.", 
"Q","--.-", 
"R",".-.", 
"S","...", 
"T","-", 
"U","..-", 
"V","...-", 
"W",".--", 
"X","-..-", 
"Y","-.--",
"Z","--..",
"0","-----", 
"1",".----", 
"2","..---",
"3","...--",
"4","....-",
"5",".....",
"6","-....",
"7","--...",
"8","---..",
"9","----.",
"/","-..-.",
"*","-.-.-",
".",".-.-.",
"$","...-.-",
"&","-...-.-",
"?","..--..",
"<",".-.-.",
"(","-.--.",
"=","-...-",
" "," "
};

void setup() {
 
  pinMode(pinLED, OUTPUT);      // sets the digital pin as output
  pinMode(pinDit, INPUT_PULLUP);        // sets Left Paddle digital pin as input
  pinMode(pinDah, INPUT_PULLUP);        // sets Right Paddle digital pin as input
  pinMode(pinKey, INPUT_PULLUP);        // sets Straight Key digital pin as input
  pinMode(pinMsg1, INPUT_PULLUP);        // sets WPM Pin
  pinMode(pinMsg2, INPUT_PULLUP);        // sets WPM Pin
  pinMode(pinMsg3, INPUT_PULLUP);        // sets WPM Pin
  pinMode(pinMsg4, INPUT_PULLUP);        // sets WPM Pin
  pinMode(pinWPM, INPUT_PULLUP);        // sets WPM Pin
  digitalWrite(pinLED, LOW);   // turn the LED off
  keyerState = IDLE;
  keyerControl = IAMBICB;      // Or 0 for IAMBICA

  if (digitalRead(pinWPM) == LOW) loadWPM(16);        // Fix speed at 18 WPM
  if (digitalRead(pinWPM) == HIGH) loadWPM(20);        // Fix speed at 18 WPM

  if (digitalRead(pinWPM) == LOW) MorseWPM = 16;        // Fix speed at 18 WPM
  if (digitalRead(pinWPM) == HIGH) MorseWPM = 20;        // Fix speed at 18 WPM

  loadWPM( (int)(analogRead(pinWPMPot) * (40/1023.) + .5) );  // vary WPM

  Serial.begin(9600);
  inputString.reserve(200);
  //Serial.println((int)(analogRead(pinWPMPot) * (35/1023.) + .5)); 
}
 
void loop()
{
  static long ktimer;
  int debounce;
 
  if (digitalRead(pinMsg1) == LOW) TransmitMorse(MyMsg1, MorseWPM, freqSidetone);
  if (digitalRead(pinMsg2) == LOW) TransmitMorse(MyMsg2, MorseWPM, freqSidetone);
  if (digitalRead(pinMsg3) == LOW) TransmitMorse(MyMsg3, MorseWPM, freqSidetone);
  if (digitalRead(pinMsg4) == LOW) TransmitMorse(MyMsg4, MorseWPM, 1750);

  if (stringComplete) {
    TransmitMorse(inputString, MorseWPM, freqSidetone); 
    //Serial.println(inputString); 
    // clear the string:
    inputString = "";
    stringComplete = false;
  }

  //Serial.println((int)(analogRead(pinWPMPot) / 25 )); 
  loadWPM( (int)(analogRead(pinWPMPot) * (40/1023.) + .5) );  // vary WPM
 
//  rKey: {
//  if (pinKey == LOW) {
//    KeyDown = true;
//    digitalWrite(pinLED,HIGH);
//    tone(pinSidetone,freqSidetone);
//    goto rKey;
//  }
//  else {
//    KeyDown = false;
//    digitalWrite(pinLED,LOW);
//    noTone(pinSidetone);
//  }
//  }
//
//  if (KeyDown) {
//    goto rKey;
//  }

  switch (keyerState) {
    case IDLE:
        if ((digitalRead(pinDit) == LOW) ||
                (digitalRead(pinDah) == LOW) ||
                    (keyerControl & 0x03)) {
            update_PaddleLatch();
            keyerState = CHK_DIT;
        }
        break;
 
    case CHK_DIT:
        if (keyerControl & DIT_L) {
            keyerControl |= DIT_PROC;
            ktimer = ditTime;
            keyerState = KEYED_PREP;
        }
        else {
            keyerState = CHK_DAH;
        }
        break;
 
    case CHK_DAH:
        if (keyerControl & DAH_L) {
            ktimer = ditTime*3;
            keyerState = KEYED_PREP;
        }
        else {
            keyerState = IDLE;
        }
        break;
 
    case KEYED_PREP:
        digitalWrite(pinLED, HIGH);         // turn the LED on
        tone( pinSidetone, freqSidetone );
        ktimer += millis();                 // set ktimer to interval end time
        keyerControl &= ~(DIT_L + DAH_L);   // clear both paddle latch bits
        keyerState = KEYED;                 // next state
        break;
 
    case KEYED:
        if (millis() > ktimer) {            // are we at end of key down ?
            digitalWrite(pinLED, LOW);      // turn the LED off
            noTone( pinSidetone );
            ktimer = millis() + ditTime;    // inter-element time
            keyerState = INTER_ELEMENT;     // next state
        }
        else if (keyerControl & IAMBICB) {
            update_PaddleLatch();           // early paddle latch in Iambic B mode
        }
        break;
 
    case INTER_ELEMENT:
        // Insert time between dits/dahs
        update_PaddleLatch();               // latch paddle state
        if (millis() > ktimer) {            // are we at end of inter-space ?
            if (keyerControl & DIT_PROC) {             // was it a dit or dah ?
                keyerControl &= ~(DIT_L + DIT_PROC);   // clear two bits
                keyerState = CHK_DAH;                  // dit done, check for dah
            }
            else {
                keyerControl &= ~(DAH_L);              // clear dah latch
                keyerState = IDLE;                     // go idle
            }
        }
        break;
    }
}
 
void update_PaddleLatch()
{
    if (digitalRead(pinDah) == LOW) {
        keyerControl |= DIT_L;
    }
    if (digitalRead(pinDit) == LOW) {
        keyerControl |= DAH_L;
    }
}
 
void loadWPM (int wpm)
{
    MorseWPM = wpm;
    ditTime = 1200/wpm;
    int ditLength = 1200 / wpm;
    int dahLength = (1200 / wpm)*3;
}

  void TransmitMorse(String MorseString, int MorseWPM,int MorseToneFreq) {
  
  Serial.print("Message: ");
  Serial.println(MorseString);
  
  loadWPM( (int)(analogRead(pinWPMPot) * (40/1023.) + .5) );  // vary WPM
  int CWdot = 1200 / MorseWPM;
  int CWdash = (1200 / MorseWPM)*3;
  int istr;
  for (istr=0; istr < MorseString.length(); istr++) {
  loadWPM( (int)(analogRead(pinWPMPot) * (40/1023.) + .5) );  // vary WPM
  int CWdot = 1200 / (analogRead(pinWPMPot) * (40/1023.) + .5);
  int CWdash = (1200 / (analogRead(pinWPMPot) * (40/1023.) + .5) )*3;
    String currentchar = MorseString.substring(istr,istr+1);
    int ichar = 0;
    while(ichar < sizeof(MorseCodeCharacters)) {
      String currentletter = MorseCodeCharacters[ichar];   // letter
      String currentcode = MorseCodeCharacters[ichar+1];   // corresponding morse code
      if (currentletter.equalsIgnoreCase(currentchar)) {
        int icp=0;
        for (icp=0;icp < currentcode.length(); icp++) {
          // Transmit Dit
          if (currentcode.substring(icp,icp+1).equalsIgnoreCase(".")) { 
            digitalWrite(pinLED, HIGH);
            tone(pinSidetone, MorseToneFreq);
            delay(CWdot);
            digitalWrite(pinLED, LOW);
            noTone(pinSidetone);
            delay(CWdot);
          }
          // Transmit Dah
          else if (currentcode.substring(icp,icp+1).equalsIgnoreCase("-")) {
            digitalWrite(pinLED, HIGH);
            tone(pinSidetone, MorseToneFreq);
            delay(CWdash);
            digitalWrite(pinLED, LOW);
            noTone(pinSidetone);
            delay(CWdot); 
          }
          else if (currentcode.substring(icp,icp+1).equalsIgnoreCase(" ")) {
            delay(CWdot*3);
          };
          }
        }
        ichar=ichar+2;
     }
      delay(CWdot*3);
      }    
    delay(CWdot*7);
  }

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {

    // get the new byte:
    char inChar = (char)Serial.read(); 
    // add it to the inputString:

    inputString += inChar;

//    if (inChar.startsWith("**", 2)) {
      // do it
//      loadWPM(20);        // Fix speed at 20 WPM
//      MorseWPM = 16;        // Fix speed at 16 WPM
//    }

    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    } 
  }
} 
