int a = 10; // generate PWM signal
int val = 0;
float ct = A1; //ADC value read from current-converted-voltage on CE
float c = 0;
int n = 0;
float Potstep = 0.0078; // Defined DAC resolution
int scanRate = 100; // scan rates values (mV/s)

long intervalo;

void setup() {
  TCCR1B = TCCR1B & B11111000 | B00000001; // Set dividers to 1 to change PWM frequency
  Serial.begin(9600);
  pinMode(a, OUTPUT);
  pinMode(ct, INPUT);
  intervalo = (1000000L / (scanRate * 128L)); //Calculates and stores the delay
}

void loop() {
  n = 0;
  while(n <= 1) {
    // Start the forward scan
    Serial.print("Forward loop start: ");
    for(val = 0; val <= 255; val++) {
      analogWrite(a, val);
      delay(intervalo);
      c = analogRead(ct);
      Serial.print(val);
      Serial.print(" ");
      Serial.print(c, 2);
      Serial.print(" ");
      Serial.println("");
    }
    // Start the reverse scan
    Serial.print("Reverse loop start: ");
    for(val = 255; val >= 0; val--) {
      analogWrite(a, val);
      delay(intervalo);
      Serial.print(val);
      c = analogRead(ct);
      Serial.print(" ");
      Serial.print(c, 2);
      Serial.print(" ");
      Serial.println("");
    }
    n = n + 1;
  }
}

//Script ends
