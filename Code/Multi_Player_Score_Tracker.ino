#include <LiquidCrystal.h>
#include <MsTimer2.h>
#include <EEPROM.h>

#define MAX_PLAYERS 50    // maximum allowed, depends on EEPROM size & memory available
#define MIN_PLAYERS 1     // minimum players needed
#define debounce 50       // switch debounce time, in milli seconds
#define HOLDTIME 2000     // for switch long click , in milli seconds
#define INT_SIZE 2
#define MAX_SCORE_STEP 10 // maximum number allowed for scoreStep variable

int players;
int scoreStep = 1 ;      // default value for score increase/decrease by this number
int currentPlayer;
int playerButtonState;
int upButtonState;
int dnButtonState;
int mode = 0 ; // operation modes:  0 = view score, 1 = update score , 2 = configure
int POINTS[MAX_PLAYERS+1] ;   // runtime score stored in an array
unsigned long configWait;
boolean configStarted = LOW;
int configMode = 1; // configuration modes: 1 = no of players, 2 = reset score , 3 = score STEP
boolean resetScore = LOW;
LiquidCrystal lcd(7, 8, 9, 10, 11, 12); // initialize LCD


// ++++++++++ BEGIN: Class sButton ++++++++++
// orginal button click logic is from http://jmsarduino.blogspot.com
// I did convert it into a Class so that I can create multiple objects
class sButton {
    const byte pin;
    int state;
    int prevState;
    int overallState; // 0 = not clicked , 1 = short click , 2 = long click (hold)
    boolean ignoreUp;
    boolean hold;
    unsigned long buttonUpMs;
    unsigned long buttonDownMs;
    

  public:
    sButton(byte attachTo) :
      pin(attachTo)
    {
    }

    void setup() {
      pinMode(pin, INPUT_PULLUP);
      state = LOW;
      hold = false;
      prevState = LOW;
      ignoreUp = false;
    }

    int stateCheck() {
      overallState = 0;
      state = digitalRead(pin);
      // Test for button pressed and store the down time
      if (state  == LOW && prevState == HIGH && (millis() - buttonUpMs ) > long(debounce))
      {
           buttonDownMs = millis();
      }
      // Test for button release and store the up time
      if (state == HIGH && prevState == LOW && (millis() - buttonDownMs) > long(debounce))
      {
        if (ignoreUp == false) {
            overallState = 1;
        }
        else ignoreUp = false;
        buttonUpMs = millis();
       }
       
    // Test for button held down for longer than the hold time
     if (state == LOW && (millis() - buttonDownMs) > long(HOLDTIME))  
     {
        ignoreUp = true;
        hold = true;
        buttonDownMs = millis();
        overallState = 2;
      }
      
      prevState = state;
      return overallState;
    }
};
// ------------END:  Class sButton --------------



// +++++++++++++ Main : Starts here ++++++++++++++++

sButton playerButton(4);      // initilaize didital pin 4 for Player sellection
sButton upButton(5);          // initialize digitla pin 5 for score increment operation
sButton dnButton(6);          // initialize digital pin 6 for score decrement operaiton

void setup() {
  playerButton.setup();
  upButton.setup();
  dnButton.setup();
  initScores();
  MsTimer2::set(10, powerLossDetector); // 10ms timer to check powerloss
  MsTimer2::start();
  lcd.begin(16, 2);
  currentPlayer=0;
  showWelcome();
}

void loop() {
  playerButtonState = playerButton.stateCheck();
  upButtonState = upButton.stateCheck();
  dnButtonState = dnButton.stateCheck();

  // +++ BEGIN: Block for handling configuration  +++++++++
  // In confure mode number of players can be set & can reset every player's score to 0
  //
  // config mode begins by holding upbutton for HOLDTIME
  if (upButtonState == 2 || mode == 2 ) { 
    if (mode != 2 ) {
      configWait = millis();            // config mode begins, up button in hold, waititng for dn button
    }
    mode = 2;
    if (dnButtonState == 2) {           // dn button also held for HOLDTIME
        configStarted = HIGH;           // now we enter into configuraiton mode
        lcd.clear();
    }
    
    // to return back to view mode if the dn button wasnt long-clicked within HOLDTIME window
    if (millis() - configWait > HOLDTIME && configStarted == LOW) mode = 0;
    
    if (configStarted == HIGH) {          // now we are in configuraiton mode
       switch(configMode){
        case 1: doPlayerCountConfig();  break;
        case 2: doResetScore(); break;
        case 3: doScoreStep(); break;
        default: break;
       } 

      if ( playerButtonState == 1) {      // changing config mode
        switch (configMode) {
          case 1: configMode = 2 ; playerButtonState = 0 ; lcd.clear(); break;
          case 2: configMode = 3 ; playerButtonState = 0 ; lcd.clear(); break;
          case 3: configMode = 1 ; playerButtonState = 0 ; lcd.clear(); break;
          default:  break;
        }
      }

        if ( playerButtonState == 2) {    // exit from configuration mode
          mode = 0;
          configMode = 1 ;
          showWelcome();
        }
    }

    // returning to the begining op loop as we are in configuraiton mode
    return ;          
  }
  configStarted = LOW;
  // ---- END: Block for handling configuration (number of players) ---------

 
    
  // +++ BEGIN: Block for updating score of each player  +++++++++ 
  if ( playerButtonState == 1) {    // jump to update score mode if update button pressed
    lcd.clear();
    if (currentPlayer == 0) currentPlayer++; 
    if (mode == 1 ) currentPlayer++;  // move to next player
    mode = 1;
    if (currentPlayer > players ) currentPlayer = 1 ;
  }

  if (mode == 1 ) {               // to stay in update score mode, show score on display
    doUpdateScore();
  }
  // ----  END: Block for updating score of each player  -------  


  
  // +++ BEGIN: Block for displaying score of each player  +++++++++ 
   if (mode == 0 && (upButtonState == 1 || dnButtonState == 1)) { // to view score
    lcd.clear();
    showPlayerScore();
   }
 // --- END : Block for displaying score of each player  --------


  // +++ BEGIN: Block for exiting from any mode to welcome screen  +++++++++    
    if ( playerButtonState == 2) {    // to exit from current  mode to  home screen
        mode = 0 ;
        currentPlayer = 0 ; 
        showWelcome();
    }
  // ---- END: Block for exiting from any mode to welcome screen  --------     
  
}

// -------------  Main : Ends here ---------------


// function for showing welcome screen
void showWelcome() {
  if (resetScore == HIGH ) {
    for (int p = 1 ; p <= players ; p++ ) {
      POINTS[p]=0;
    }

    resetScore = LOW;
  }
  
  lcd.clear();
  lcd.print ("*Score Tracker*");
  lcd.setCursor(0, 1);
  lcd.print ("No of players ");
  lcd.print (players);
  lcd.noBlink();
}


// function for showing individual player's score
void showPlayerScore () {
    if (upButtonState == 1 ) currentPlayer++ ; 
    if (dnButtonState == 1) currentPlayer--;
    if (currentPlayer > players) currentPlayer = 1;
    if (currentPlayer < 1) currentPlayer = players;
    lcd.setCursor(0, 0);
    lcd.print ("View score:");
    lcd.setCursor(0, 1);
    lcd.print ("Player ");
    lcd.print (currentPlayer);
    lcd.print (" = ");
    lcd.print (POINTS[currentPlayer]);
    lcd.print (" ");
}

// function for updating individual player's score
void doUpdateScore () {
    lcd.setCursor(0, 0);
    lcd.print ("Update score:");
    lcd.setCursor(0, 1);
    lcd.print ("Player ");
    lcd.print (currentPlayer);
    lcd.print (" = ");
    lcd.print (POINTS[currentPlayer]);
    lcd.print (" ");
  
   if (mode == 1 && upButtonState == 1) {   // increment score if up button pressed
    POINTS[currentPlayer] += scoreStep;
   }
   if (mode == 1 && dnButtonState == 1) {   // decrement point if down button pressed
    if (POINTS[currentPlayer] != 0 )  
      POINTS[currentPlayer] -= scoreStep;
   }
}

// function for  reset all  player's score
void doResetScore() {
  lcd.setCursor(0, 0);
  lcd.print("Configure: Reset");
  lcd.setCursor(0, 1);
  lcd.print("score ? ");
  if (resetScore == LOW ) {
    lcd.print("NO ");
  } else {
    lcd.print("YES");
  }
  
  if (upButtonState == 1) {   // increment score if up button pressed
    resetScore = HIGH;
   }
  if (dnButtonState == 1) {   // decrement point if down button pressed
    resetScore = LOW;
   }
}

// function for configuring number of players
void doPlayerCountConfig () {
     lcd.setCursor(0, 0);
     lcd.print ("Configure: No of ");
     lcd.setCursor(0, 1);
     lcd.print("Players = ");
     lcd.print(players);
     lcd.print(" ");
     if (upButtonState == 1) {         // increment number of players
         players++;
         if (players > MAX_PLAYERS) { 
           players = MAX_PLAYERS;        // limiting players to MAX_PLAYERS
         }
     }
     if (dnButtonState == 1) {         // decement number of players
        players--;
        if (players < MIN_PLAYERS) {
          players = MIN_PLAYERS;
        }
            
     }
}


// function for configuring sccore increase/decrease step
void doScoreStep () {
     lcd.setCursor(0, 0);
     lcd.print ("Configure: Score ");
     lcd.setCursor(0, 1);
     lcd.print("Step  = ");
     lcd.print(scoreStep);
     lcd.print(" ");
     if (upButtonState == 1) {         // increment number of players
         scoreStep++;
         if (scoreStep > MAX_SCORE_STEP ) {
            scoreStep = MAX_SCORE_STEP;
         }
     }
     if (dnButtonState == 1) {         // decement number of players
        scoreStep--;
        if (scoreStep < 1 ) {
          scoreStep = 1;              // minimum step value is 1
        }
            
     }
}


// function called by timer to detect power loss and save into EEPROM
// coutrsey to Darieee's youtube video on power losss detection
void powerLossDetector () {
  int eeAddress = 0;
  int eeValue = 0;
  digitalWrite(LED_BUILTIN, LOW);
  if (analogRead(A3) < 920) {
        pinMode(A3, OUTPUT);
        digitalWrite (A3, HIGH);
        return;
  }

  pinMode(A3, INPUT);
  if (analogRead(A3) > 1000) {  // this happens only when power fails
    eeAddress = 0;
    EEPROM.put( eeAddress, players);
    eeAddress+=INT_SIZE;
    EEPROM.put( eeAddress, scoreStep);
    eeAddress+=INT_SIZE;
    for(int i=1; i<=players; i++) {
      EEPROM.put( eeAddress, POINTS[i]);    // save score into EEPROM
      eeAddress+=INT_SIZE;
    }
    MsTimer2::stop();
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

// function to read from EEPROM and initailize score 
void initScores() {
  int eeAddress = 512;        // address 512  for tracking EEPROM usage
  int eeValue = 0;
  EEPROM.get( eeAddress, eeValue);
  if (eeValue != 89 ) {       // looks like a fresh EEPROM
    EEPROM.put( eeAddress, 89);
    players = MIN_PLAYERS;
    eeAddress = 0;
    EEPROM.put( eeAddress, players);
    eeAddress+=INT_SIZE;
    EEPROM.put( eeAddress, scoreStep);
    for(int i=1; i<=players; i++) {
        eeAddress+=INT_SIZE;
        EEPROM.put( eeAddress, POINTS[i]);
    }
    return;
  }
  eeAddress = 0;
  EEPROM.get( eeAddress, players);
  eeAddress+=INT_SIZE;
  EEPROM.get( eeAddress, scoreStep);
  
  for(int i=1; i<=players; i++) {
    eeAddress+=INT_SIZE;
    EEPROM.get( eeAddress, eeValue);
    POINTS[i]=eeValue; 
  }
}
