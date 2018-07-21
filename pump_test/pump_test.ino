const byte gate1 = 4;
const byte gate2 = 5;
const byte gate3 = 6;
const byte gate4 = 7;
const byte gate5 = 8;

void setup() {
  // put your setup code here, to run once:
  pinMode(gate1, OUTPUT);
  pinMode(gate2, OUTPUT);
  pinMode(gate3, OUTPUT);
  pinMode(gate4, OUTPUT);
  pinMode(gate5, OUTPUT);
  digitalWrite(gate5, LOW);
  digitalWrite(gate1, LOW);
  digitalWrite(gate2, LOW);
  digitalWrite(gate3, LOW);
  digitalWrite(gate4, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
  digitalWrite(gate1, HIGH);
  // 16ml / 1s
  //accurate down to 500ms
  delay(1000);
  digitalWrite(gate1, LOW);

  delay(1000);
  digitalWrite(gate2, HIGH);
  // 16ml / 1s
  //accurate down to 500ms
  delay(1000);
  digitalWrite(gate2, LOW);

    delay(1000);
  digitalWrite(gate3, HIGH);
  // 16ml / 1s
  //accurate down to 500ms
  delay(1000);
  digitalWrite(gate3, LOW);

    delay(1000);
  digitalWrite(gate4, HIGH);
  // 16ml / 1s
  //accurate down to 500ms
  delay(1000);
  digitalWrite(gate4, LOW);

      delay(1000);
  digitalWrite(gate5, HIGH);
  // 16ml / 1s
  //accurate down to 500ms
  delay(1000);
  digitalWrite(gate5, LOW);
}
