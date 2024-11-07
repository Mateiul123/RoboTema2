#include <Arduino.h>
#include <string.h>

#define BAUD 28800

#define PIN_LED_RED 5
#define PIN_LED_GREEN 10
#define PIN_LED_BLUE 9

#define BUTTON_START 2
#define BUTTON_DIFF 3

#define BACKSPACE 8

unsigned long inputTimeout[3] = {5000, 4000, 3000}, gameDuration = 30000, wordTime = 0, debounceDelay = 300, startDelay = 3000, startErrorMargin = 50;
char currentWord[30], targetWord[30];
char dictionary[10][30] = {"ce","cuvinte","ar","trebui","sa","folosesc","aici","nu","stiu","altceva"};
int inputIndex = 0, difficulty = 0, ledState = 0, blinks = 0, score = 0;
char diffNames[3][10] = {"\nEZ\n", "\nHard\n", "\nHARDER\n"};
volatile unsigned long buttonDiffPressTime = 0, buttonStartPressTime = 0, gameStartTime = 0, lastBlinkTime = 0;
bool idle = true, active = false;

void setLEDColor(int redVal, int greenVal, int blueVal) 
{
  analogWrite(PIN_LED_RED, redVal);
  analogWrite(PIN_LED_GREEN, greenVal);
  analogWrite(PIN_LED_BLUE, blueVal);
}

void chooseNewWord(int result)
{
  if(result)
  {
    score++;
    Serial.println("\n");
  }
  else
  {
    Serial.println("\nA expirat timpul!\n");
  }
  strcpy(currentWord, " ");
  inputIndex = 0;
  strcpy(targetWord, dictionary[(random(10) + gameStartTime) % 10]);
  Serial.println(targetWord);
  wordTime = millis();
}

void handleDifficultyButton()
{
  if(idle)
  {
    if(millis() - buttonDiffPressTime > debounceDelay && digitalRead(BUTTON_DIFF) == LOW)
    {
      buttonDiffPressTime = millis();
      difficulty = (difficulty + 1) % 3;
      Serial.println(diffNames[difficulty]);
    }
  }
}

int checkWord(const char *wordToCheck)
{
  int length = strlen(wordToCheck);
  for(int i = 0; i < length; i++)
  {
    if(wordToCheck[i] != targetWord[i])
      return -1;
  }
  if(length == int(strlen(targetWord)))
    return 1;
  return 0;
}

void handleStartButton()
{
  if(millis() - buttonStartPressTime > debounceDelay && digitalRead(BUTTON_START) == LOW)
  {
    buttonStartPressTime = millis();
    idle = !idle;
    active = !active;
    if(active)
    {
      gameStartTime = millis();
      score = 0;
      lastBlinkTime = millis();
      blinks = 0;
    }
    else
    {
      Serial.println("\nTerminat!\nScor:");
      Serial.println(score);
      Serial.println("\n");
      setLEDColor(100, 100, 100);
    }
  }
}

void blinkLED()
{
  if(millis() - lastBlinkTime > startDelay / 6)
  {
    lastBlinkTime = millis();
    setLEDColor(100 * ledState, 100 * ledState, 100 * ledState);
    ledState = (ledState + 1) % 2;
    if(!(blinks % 2))
    {
      Serial.println(char(3 - blinks / 2 + 48));
    }
    blinks++;
  }

  if(blinks == 6)
  {
    inputIndex = 0;
    strcpy(currentWord, "");
    strcpy(targetWord, dictionary[(random(10) + gameStartTime) % 10]);
    Serial.println(targetWord);
    wordTime = millis();
    setLEDColor(0, 100, 0);
    blinks++;
  }
}

void readCharacter()
{
  char character = Serial.read();
  if(int(character) == BACKSPACE)
  {
    if(inputIndex > 0)
    {
      currentWord[inputIndex - 1] = NULL;
      inputIndex--;
    }
  }
  else
  {
    currentWord[inputIndex] = character;
    inputIndex++;
    currentWord[inputIndex] = NULL;
  }
}

void checkGameStatus()
{
  if(active && millis() - gameStartTime > gameDuration)
  {
    idle = true;
    active = false;
    Serial.println("\nS-a terminat!\nScor:");
    Serial.println(score);
    Serial.println("\n");
    setLEDColor(100, 100, 100);
  }
}

void setup()
{
    Serial.begin(BAUD);
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_BLUE, OUTPUT);

    setLEDColor(100, 100, 100);

    pinMode(BUTTON_START, INPUT_PULLUP);
    pinMode(BUTTON_DIFF, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(BUTTON_START), handleStartButton, FALLING);
    attachInterrupt(digitalPinToInterrupt(BUTTON_DIFF), handleDifficultyButton, FALLING);

    Serial.println("\nJocul poate incepe\n");
}

void loop()
{
  checkGameStatus();

  if(active)
  {
    if(millis() - gameStartTime <= startDelay + startErrorMargin)
    {
      blinkLED();
    }
    else
    {
      if(Serial.available()) 
      {
          readCharacter();
          if(checkWord(currentWord) == 1)
          {
            chooseNewWord(1);
          }
          else if(checkWord(currentWord) == -1)
          {
            setLEDColor(100, 0, 0);
          }
          else
          {
            setLEDColor(0, 100, 0);
          }
      }
      if(millis() - wordTime > inputTimeout[difficulty])
      {
        chooseNewWord(0);
      }
    }
  }
}
