//***********************************This program for board version 1.7  *********************
//Includes 2 inputs to select 25, 26, 27 & 28MHz (all x8) - default to 25 if no solder bridges present
//
//
#include <SPI.h>

#define Latch1 P2_1 // switch frequencies for receive or transmit
#define meas12v P1_1 // check 12 volt input
#define meas3v P1_2 // Check 3 volt supply
#define meas5v P1_3 // Check 5 volt supply
#define Latch2 P3_1 // switch frequencies for receive or transmit
#define tx_off P2_0 // NEG turn off upconverter
#define rxtx P2_7 // switch frequencies for receive or transmit
#define Vctl P3_2 // switch for receive or transmit
#define txout P3_0 // toggling txout signal or Tx out
#define supplyFault P2_3 // 
#define Local 10000000 // nominal oscillator frequency
#define ENABLE_FRAM_WRITE

uint16_t supply12, supply3, supply5;
uint8_t d;
uint8_t e, g;
uint16_t hyster = 0;
uint8_t OnceOnly = 0, rxFlag =  0, UneFois = 0;
uint8_t Level309 = 0;
uint8_t Level1970 = 0;
uint32_t R0;

// Add setup code
void setup() {
  SYSCFG0 = FRWPPW | DFWP;            // Program FRAM write enable
  pinMode(rxtx, INPUT);           // rxtx, TX input
  pinMode(tx_off, OUTPUT);        // enable up converter
  pinMode(P1_4, INPUT_PULLUP);    // trim+
  pinMode(P1_5, INPUT_PULLUP);    // trim-
  pinMode(P1_6, INPUT_PULLUP);    // freqsel 1
  pinMode(P1_7, INPUT_PULLUP);    // freqsel 2
  pinMode(Latch1, OUTPUT);        // L1
  pinMode(Latch2, OUTPUT);        // L2
  pinMode(Vctl, OUTPUT);          // Vctl on rf IC
  pinMode(txout, OUTPUT);          // txout signal
  pinMode(supplyFault, OUTPUT);          // txout signal
  // initial conditions...
  digitalWrite(Latch1, LOW);
  digitalWrite(Latch2, LOW);
  digitalWrite(Vctl, LOW);        // enables RX on rf IC
  digitalWrite(tx_off, LOW);      // disable transmit on up=converter
  digitalWrite(txout, LOW);   // repeat rxtx for external devices
  digitalWrite(supplyFault, LOW);
  hyster = 0;                     // just to make sure...
  SPI.begin();  // initialise SPI bus
  SPI.setDataMode(0);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(4);

  d = (P1IN & 0xc0); // check bits 6 & 7 to see which freq reqd
  switch (d)
  {
    case (0xC0): R0 = 0x00140000; // set Main output of PLL2 to 200MHz
      break;
    case (0x80): R0 = 0x00148018; // set Main output of PLL2 to 208MHz
      break;
    case (0x40): R0 = 0x00158008; // set Main output of PLL2 to 216MHz
      break;
    case (0x00): R0 = 0x00160020; // set Main output of PLL2 to 224MHz
      break;
    default:
      R0 = 0x00140000;
      break;
  }


  // Set up reference oscillator for LNB
  write4Bytes2(0x00580005); //R5
  write4Bytes2(0x0042803c); // bits 3 & 4 power out
  write4Bytes2(0x000004b3);
  write4Bytes2(0x01004fc2); //0x010041c2 ???
  write4Bytes2(0x00008029);  
  write4Bytes2(R0);
}

// Main loop code takes about 15uS

void loop() {  // check RxTx pin level H=Tx
  e = digitalRead(rxtx);
  if (e == 1) {  // Change to the Tx frequency AND the output on 4350
    //      digitalWrite(EN, LOW);
    hyster = 48000;    // set hysteresis value - experimentally determined  ~0.7s
//    OnceOnly = 0; //

    if (UneFois == 0) { // first time here?
      UneFois = 1;  // do it once only
      OnceOnly = 0; // allow rx routine to repeat

      write4Bytes1(0x00580005);
      write4Bytes1(0x009505A4);  // | (Level1970 << 6)); //5 rf-en, 8 aux-en 2dBm
      write4Bytes1(0x000004B3);
      write4Bytes1(0x00004F42);
      write4Bytes1(0x08008051);  // 08008011
//      write4Bytes1(0x00C50000); // set Aux output to 1970MHz OR MORE ACCURATELY 1969.5
      write4Bytes1(0x00c48048);
    digitalWrite(Vctl, HIGH); // let mixer get incoming signal from transceiver
    digitalWrite(tx_off, HIGH);  // enable transmit up converter
    digitalWrite(txout, HIGH);   // repeat rxtx for control of external devices
    }
  }

  else  { // just changed to Rx...
    if (hyster != 0) hyster--;
    if ((hyster == 0) && (OnceOnly == 0)) {
      OnceOnly = 1;// prevents doing this repeatedly in the main loop..
      UneFois = 0;
      
      write4Bytes1(0x00580005); //R5
      write4Bytes1(0x00b502e4); // | (Level309 << 3) | (Level1970 << 6)); //5 rf-en, 8 aux-en
      write4Bytes1(0x000004B3);
      write4Bytes1(0x00005f42);
      write4Bytes1(0x00008029);
      write4Bytes1(0x007b8008); // set Main output to 309MHz

      digitalWrite(Vctl, LOW); // let mixer get incoming signal from transceiver
      digitalWrite(tx_off, LOW);  // disable transmit up converter
      digitalWrite(txout, LOW);   // repeat rxtx for external devices
    } //allow once, after hysteresis time
  }  // else
/*
// now check voltages?
  supply12 = analogRead(meas12v);  
  supply3 = analogRead(meas3v);  
  supply5 = analogRead(meas5v);  
  if ((supply12 < 10000) || (supply5 < 10000) || (supply3 < 10000))   digitalWrite(supplyFault, HIGH);
*/
} // main loop



void write4Bytes1(uint32_t d)
{
//  digitalWrite(Latch1, LOW);
  for (int i = 3; i >= 0; i--)          // 4 x bytes
    SPI.transfer((d >> 8 * i) & 0xFF); // MSByte first
  digitalWrite(Latch1, HIGH);
  digitalWrite(Latch1, LOW);
}

void write4Bytes2(uint32_t d)
{
//  digitalWrite(Latch2, LOW);
  for (int i = 3; i >= 0; i--)          // 4 x bytes
    SPI.transfer((d >> 8 * i) & 0xFF); // MSByte first
  delay(1);
  digitalWrite(Latch2, HIGH);
  digitalWrite(Latch2, LOW);
}
