#include <Wire.h>
const uint8_t LMP_ADDR = 0x48;          // 0x48 <<1 = 0x90 in 8‑bit form
const int VRE = A0;              // Pin A0 connects to RE of LMP91000EVM
const int VWE = A1;              // Pin A1 connects to WE of LMP91000EVM
const int VOUT = A2;             // Pin A2 connects to VOUT of LMP91000EVM
const int VREF = A3;             // Pin A3 connects to VREF of LMP91000EVM

const unsigned long stepDelay = 500; // milliseconds per step (adjust for scan rate: 100mV/s for 500ms, 50mV/s for 1000ms)

#define REG_LOCK   0x01 // Address of unlock register
#define REG_TIA    0x10 // Address of TIA control register, it allows the configuration of the transimpedance gain (RTIA) and the load resistance (RLoad)
#define REG_REF    0x11 // Address of reference control register, allows the configuration of the Internal zero, Bias and Reference source
#define REG_MODECN 0x12 // Address of mode control register, allows the configuration of the Operation Mode of the LMP91000

int biasPercentCodes[] = {13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,   // -24% to 0% of VREF (2.5V), about 50mV per step
                          1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};    // 0% to +24% of VREF (2.5V)

int numSteps = sizeof(biasPercentCodes)/sizeof(biasPercentCodes[0]);


void i2cWrite(byte reg, byte data) {
  Wire.beginTransmission(LMP_ADDR);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}


byte i2cRead(byte reg) {
    // Tell the Wire library which device we want to talk to:
    Wire.beginTransmission(LMP_ADDR);

    // Send the register‑address we want to read from:
    Wire.write(reg);

    // End the “write” phase—but with a *repeated‑start* (no STOP bit)
    // so that the bus stays claimed:
    Wire.endTransmission(false);

    // Request exactly 1 byte back from that same device address:
    Wire.requestFrom(LMP_ADDR, (uint8_t)1);

    // If a byte arrived, return it; otherwise return 0xFF to flag “no data”:
    return Wire.available() ? Wire.read() : 0xFF;
}


void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Unlock register TIACN and REFCN and make them writable
  i2cWrite(REG_LOCK, 0x00);

  // Verify the state of TIACN and REFCN, if lockVal == 0x00 then both registers are writtable
  byte lockVal = i2cRead(REG_LOCK);
  Serial.print("LOCK register = 0x");
  Serial.println(lockVal, HEX); 

  //Transimpedance gain and load resistance settings:
  byte tiaReg = 0x14; // define 35kOhms gain with 10 Ohms load register bit[7:5] 000, bit[4:2] xxx (gain), bit[1:0] xx (load register)
  //param - value - gain resistor
  //0 - 000 - External resistor
  //1 - 001 - 2.75 kOhm
  //2 - 010 - 3.5 kOhm
  //3 - 011 - 7 kOhm
  //4 - 100 - 14 kOhm
  //5 - 101 - 35 kOhm
  //6 - 110 - 120 kOhm
  //7 - 111 - 350 kOhm

  // --------------load register---------------//
  //param - value - RLoad
  //0 - 00 - 10 Ohm
  //1 - 01 - 33 Ohm
  //2 - 10 - 50 Ohm
  //3 - 11 - 100 Ohm
  
  i2cWrite(REG_TIA, tiaReg);

  //Reference and Bias settings:
  byte refcn = 0xA0; // Ext ref, 50% zero, neg polarity (temp), 0% bias
  i2cWrite(REG_REF, refcn);

  //Mode settings:
  i2cWrite(REG_MODECN, 0x03);// Set the LMP91000 EVM from STANDBY mode to three-lead potentiostat mode

  int ref = analogRead(VREF); // verify the output voltage is about 50% to Vref (2.5V)
  float voltage = ref * (5.0 / 1024.0);
  Serial.print("");
  Serial.print("Measured reference voltage = ");
  Serial.print(voltage, 3);
  Serial.println(" V");
  Serial.println("");
  delay(1000);
}


void loop() {
  // Sweep from -0.6V to +0.6V
  for (int i = 0; i < numSteps; ++i) {
    int Vre = analogRead(VRE);                            // Read voltage on RE
    int Vwe = analogRead(VWE);                            // Read voltage on WE
    int ref = analogRead(VREF);                           // Read reference voltage
    float voltage_baseline = 0.5 * ref * (5.0 / 1024.0);  // Calculate the level of internal zero
    
    int code = biasPercentCodes[i];
    bool isNegative = (i < 14); // first half array is negative bias

    byte refcn = 0x80 | 0x20;   // ext ref + 50% zero
    if (!isNegative) refcn |= 0x10; 

    refcn |= (code & 0x0F);    // bias magnitude (0x0 to 0xD)

    // Write REFCN register with new bias
    i2cWrite(REG_REF, refcn);
    
    // Small settling delay (scan rate)
    delay(stepDelay);
    
    // Read analog output
    int adcValue = analogRead(VOUT);

    // In the loop after analogRead:
    float voltage = adcValue * 5.0 / 1024.0;            // Vout in volts
    float current_nA = (voltage - voltage_baseline) / (35e3) * 1e6; // baseline 1.25V, R_ TIA = 35kΩ

    float sweep = (Vwe - Vre) *(5.0/1024.0);
    Serial.print(sweep);
    Serial.print(" ");
    Serial.println(current_nA, 3);
    

  }
  // Sweep from +0.6V to -0.6V
  for (int i = numSteps - 2; i > 0; --i) {
    int Vre = analogRead(VRE);                            // Read voltage on RE
    int Vwe = analogRead(VWE);                            // Read voltage on WE
    int ref = analogRead(VREF);                           // Read reference voltage
    float voltage_baseline = 0.5 * ref * (5.0 / 1024.0);  // Calculate the level of internal zero
    int code = biasPercentCodes[i];
    bool isNegative = (i < 14); 


    byte refcn = 0x80 | 0x20;   // ext ref + 50% zero
    if (!isNegative) refcn |= 0x10; 

    refcn |= (code & 0x0F);    // bias magnitude (0x0 to 0xD)

    // Write REFCN register with new bias
    i2cWrite(REG_REF, refcn);
    
    // Small settling delay (could also use a precise timer)
    delay(stepDelay);
    
    // Read analog output
    int adcValue = analogRead(VOUT);

    // In the loop after analogRead:
    float voltage = adcValue * 5.0 / 1024.0;            // Vout in volts
    float current_nA = (voltage - voltage_baseline) / (35e3) * 1e6; // baseline 1.25V, R_TIA = 35kΩ

    float sweep = (Vwe - Vre) *(5.0/1024.0);
    Serial.print(sweep);
    Serial.print(" ");
    Serial.println(current_nA, 3);
    
  }

}

