#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET 4     // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3C for 128x32, 0x3D for 128x64
#define SENSORPIN 5
#define GAME_END_SENSORPIN 6 // New IR sensor pin for game end

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// variables will change:
int sensorState = 0, lastState = 0;
int gameEndSensorState = 0;
int score = 0;
bool jackpotReached = false;
bool gameOver = false;
bool gameStarted = false;
bool gameResume = false;
unsigned long lastScoreUpdate = 0;
unsigned long lastJackpotTime = 0;
unsigned long countdownStartTime = 0;
int countdownDuration = 5000; // 5 seconds

void setup() {
  // initialize the sensor pin as an input:
  pinMode(SENSORPIN, INPUT);
  digitalWrite(SENSORPIN, HIGH); // turn on the pullup

  // initialize the game end sensor pin as an input:
  pinMode(GAME_END_SENSORPIN, INPUT);
  digitalWrite(GAME_END_SENSORPIN, HIGH); // turn on the pullup

  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();

  // Display the title
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println(" Pinball");
  display.display();

  countdownStartTime = millis();
  countdownDuration = 5000; // 5 seconds countdown to start the game
}

void loop() {
  // read the state of the sensors
  sensorState = digitalRead(SENSORPIN);
  gameEndSensorState = digitalRead(GAME_END_SENSORPIN);

  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - countdownStartTime;

  // check if the countdown is done
  if (!gameStarted && elapsedTime >= countdownDuration) {
    gameStarted = true;
  }

  // check if the sensor beam is broken
  if (sensorState == LOW && gameStarted) {
    if (!jackpotReached) {
      jackpotReached = true;
      lastJackpotTime = millis();
      score *= 2; // Double the score for jackpot
      gameResume = true;
      countdownStartTime = millis();
      countdownDuration = 5000; // 5 seconds countdown to resume the game
    }
  } else {
    if (jackpotReached && millis() - lastJackpotTime >= 5000) {
      jackpotReached = false;
      gameResume = false;
    }
  }

  // check if the game end sensor beam is broken
  if (gameEndSensorState == LOW) {
    if (jackpotReached) {
      gameResume = false;
      jackpotReached = false;
      countdownStartTime = millis();
      countdownDuration = 5000; // 5 seconds countdown to resume the game
    } else {
      gameOver = true;
      countdownStartTime = millis();
      countdownDuration = 10000; // 10 seconds countdown to start a new game
    }
  }

  // Update the score every 0.1 seconds if not in jackpot pause or game over, and game has started
  if (millis() - lastScoreUpdate >= 100 && !jackpotReached && !gameOver && gameStarted) {
    score++;
    lastScoreUpdate = millis();
  }

  // Update the scoreboard or display countdown
  updateDisplay();

  lastState = sensorState;
}

void updateDisplay() {
  display.clearDisplay();

  // Display the title
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(" Pinball");

  // Display the score, jackpot message, or countdown
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 30);

  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - countdownStartTime;

  if (gameOver) {
    if (elapsedTime < countdownDuration) {
      display.print(" Game Over");
      display.print("     ");
      display.print((countdownDuration - elapsedTime) / 1000);
    } else {
      gameOver = false; // Reset gameOver state after countdown
      gameStarted = false; // Reset game state
      score = 0; // Reset score
      countdownStartTime = millis();
      countdownDuration = 5000; // 5 seconds countdown to start a new game
    }
  } else if (gameResume) {
    display.print("  Jackpot    2x  ");
    display.print((countdownDuration - elapsedTime) / 1000);
  } else if (!gameStarted) {
    display.print("Start in ");
    display.print((countdownDuration - elapsedTime) / 1000);
  } else {
    display.print("Score:");
    display.print(score);
  }

  display.display();
}
