// =========================
// 7-SEGMENT PINS
// =========================
const int a = 2;
const int b = 3;
const int c = 4;
const int d = 5;
const int e = 6;
const int f = 7;
const int g = 8;

const int dp = 9;

// =========================
// PUSH BUTTONS
// =========================
const int increaseButton = 10;
const int resetButton = 11;

int count = 0;


// =========================
// NUMBER PATTERNS
// LOW  = ON
// HIGH = OFF
// =========================

//       a b c d e f g
const byte numbers[10][7] = {
  {0,0,0,0,0,0,1}, // 0
  {1,0,0,1,1,1,1}, // 1
  {0,0,1,0,0,1,0}, // 2
  {0,0,0,0,1,1,0}, // 3
  {1,0,0,1,1,0,0}, // 4
  {0,1,0,0,1,0,0}, // 5
  {0,1,0,0,0,0,0}, // 6
  {0,0,0,1,1,1,1}, // 7
  {0,0,0,0,0,0,0}, // 8
  {0,0,0,0,1,0,0}  // 9
};


// =========================
// DISPLAY NUMBER
// =========================
void displayNumber(int number) {

  digitalWrite(a, numbers[number][0]);
  digitalWrite(b, numbers[number][1]);
  digitalWrite(c, numbers[number][2]);
  digitalWrite(d, numbers[number][3]);
  digitalWrite(e, numbers[number][4]);
  digitalWrite(f, numbers[number][5]);
  digitalWrite(g, numbers[number][6]);

  // DP OFF
  digitalWrite(dp, HIGH);
}


// =========================
// SETUP
// =========================
void setup() {

  pinMode(a, OUTPUT);
  pinMode(b, OUTPUT);
  pinMode(c, OUTPUT);
  pinMode(d, OUTPUT);
  pinMode(e, OUTPUT);
  pinMode(f, OUTPUT);
  pinMode(g, OUTPUT);
  pinMode(dp, OUTPUT);

  // Buttons use external 10kΩ pull-up resistors
  pinMode(increaseButton, INPUT);
  pinMode(resetButton, INPUT);

  // Start at 0
  displayNumber(count);

  Serial.begin(9600);
}


// =========================
// LOOP
// =========================
void loop() {

  // =========================
  // INCREASE BUTTON
  // =========================
  if (digitalRead(increaseButton) == LOW) {

    count++;

    // After 9, go back to 0
    if (count > 9) {
      count = 0;
    }

    displayNumber(count);

    Serial.print("Count: ");
    Serial.println(count);

    // Wait until button is released
    while (digitalRead(increaseButton) == LOW) {
      delay(10);
    }

    delay(50);
  }


  // =========================
  // RESET BUTTON
  // =========================
  if (digitalRead(resetButton) == LOW) {

    count = 0;

    displayNumber(count);

    Serial.println("Reset: 0");

    // Wait until reset button is released
    while (digitalRead(resetButton) == LOW) {
      delay(10);
    }

    delay(50);
  }
}