
// Runs on Arduino Nano Every to act as frequency counter
// It is connected to 5 7-segment displays able to measure frequencies from 0 to 65535 cps

//               SCK  PE2    D13   L-1   R-1   D12    PE1  MISO (SC1)   DIG_4_PIN
//                         +3.3V   L-2   R-2   D11    PE0  MOSI (SC1)   SEG_A_PIN
//                    PD7   AREF   L-3   R-3   D10    PB1               DIG_3_PIN
//             AIN[3] PD3 A0/D14   L-4   R-4   D9     PB0               SEG_B_PIN
//             AIN[2] PD2 A1/D15   L-5   R-5   D8     PE3               SEG_F_PIN
//             AIN[1] PD1 A2/D16   L-6   R-6   D7     PA1               DIG_2_PIN
//             AIN[0] PD0 A3/D17   L-7   R-7   D6     PF4               SEG_G_PIN
//               SDA  PA2 A4/D18   L-8   R-8   D5     PB2               SEG_C_PIN
//               SCL  PA3 A5/D19   L-9   R-9   D4     PC6               DIG_1_PIN
//  DIG_0_PIN  AIN[4] PD4 A6/D20  L-10   R-10  D3     PF5               SEG_D_PIN
//  INPUT_PIN  AIN[5] PD5 A7/D21  L-11   R-11  D2     PA0               SEG_E_PIN
//                           +5V  L-12   R-12  GND
//                        _RESET  L-13   R-13  _RESET
//                           GND  L-14   R-14  RX     PC5
//                           VIN  L-15   R-15  TX     PC4

#define INPUT_PIN 21    // L-11
#define SEG_A_PIN 11    // R-2    R1     top
#define SEG_B_PIN 9     // R-4    R2     upper right
#define SEG_C_PIN 5     // R-8    R3     lower right
#define SEG_D_PIN 3     // R-10   R4     bottom
#define SEG_E_PIN 2     // R-11   R5     lower left
#define SEG_F_PIN 8     // R-5    R6     upper left
#define SEG_G_PIN 6     // R-7    R7     center
#define DIG_0_PIN 20    // L-10   Q1/D1  10**0
#define DIG_1_PIN 4     // R-9    Q2/D2  10**1
#define DIG_2_PIN 7     // R-6    Q3/D3  10**2
#define DIG_3_PIN 10    // R-3    Q4/D4  10**3
#define DIG_4_PIN 12    // R-1    Q5/D5  10**4

#define UPDPERSEC 5     // update frequency reading this many times per second (exact)
#define DISPLAYHZ 60    // refresh each digit this many times per second (approx)
#define NUMDIGITS 5     // total number of digits

#define DOWNSIDEUP 1

#define EVENSPLIT ((1000000 % UPDPERSEC) == 0)
#if EVENSPLIT
#define MICROSPERUPD (1000000 / UPDPERSEC)
#else
#define MICROSPERUPD updatemicros[updateindex]
#endif

bool ledison;
uint8_t lastsegments;
uint16_t volatile interruptcounter;
uint16_t updateindex;
uint16_t maincounters[UPDPERSEC];
uint16_t totalcount;
uint32_t lastdisplaymicros;
uint32_t lastsamplemicros;
#if ! EVENSPLIT
uint32_t updatemicros[UPDPERSEC];
#endif

uint8_t const digpins[NUMDIGITS] = {
#if DOWNSIDEUP
  DIG_4_PIN, DIG_3_PIN, DIG_2_PIN, DIG_1_PIN, DIG_0_PIN
#else
  DIG_0_PIN, DIG_1_PIN, DIG_2_PIN, DIG_3_PIN, DIG_4_PIN
#endif
};
uint8_t const segpins[7] = {
#if DOWNSIDEUP
  SEG_D_PIN, SEG_E_PIN, SEG_F_PIN, SEG_A_PIN, SEG_B_PIN, SEG_C_PIN, SEG_G_PIN
#else
  SEG_A_PIN, SEG_B_PIN, SEG_C_PIN, SEG_D_PIN, SEG_E_PIN, SEG_F_PIN, SEG_G_PIN
#endif
};
uint8_t const digtosegs[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x67 };
uint8_t digsegs[NUMDIGITS];
uint8_t scandigit;

void freqPinInterrupt ();

void setup() {

  pinMode (LED_BUILTIN, OUTPUT);

  // turns on segment when LOW
  for (uint16_t seg = 0; seg < 7; seg ++) {
    pinMode (segpins[seg], OUTPUT);
  }

  // turns on digit when HIGH
  for (uint16_t dig = 0; dig < NUMDIGITS; dig ++) {
    pinMode (digpins[dig], OUTPUT);
    // turn on all segments for all digits when the loop() executes until first sample taken
    digsegs[dig] = 0x7F;
  }

  // microseconds between frequency reading updates
#if ! EVENSPLIT
  uint32_t accummicros = 0;
  for (uint16_t i = 0; i < UPDPERSEC; i ++) {
    uint32_t totalmicros = 1000000 * (i + 1) / UPDPERSEC;
    updatemicros[i] = totalmicros - accummicros;
    accummicros = totalmicros;
  }
#endif

  pinMode (INPUT_PIN, INPUT);
  attachInterrupt (digitalPinToInterrupt (INPUT_PIN), freqPinInterrupt, RISING);

  Serial.begin(9600);

  lastdisplaymicros = lastsamplemicros = micros ();

  memset(maincounters, 0, sizeof maincounters);
  totalcount = 0;
}

void loop() {
  uint32_t thismicros = micros ();

  // see if it's time to turn off one digit and turn on the next
  if (thismicros - lastdisplaymicros >= 1000000 / (NUMDIGITS * DISPLAYHZ)) {
    lastdisplaymicros += 1000000 / (NUMDIGITS * DISPLAYHZ);
    digitalWrite (digpins[scandigit], LOW);       // turn off previous digit
    if (++ scandigit == NUMDIGITS) scandigit = 0; // increment on to next digit
    uint8_t segments = digsegs[scandigit];        // get segments for next digit
    uint8_t updates  = segments ^ lastsegments;   // see which segments need to be updated
    lastsegments = segments;
    uint8_t seg = 0;                              // start with updating segment 'a'
    while (updates > 0) {                         // loop as long as more to update
      if (updates & 1) {                          // spend time updating if different
        digitalWrite (segpins[seg], (segments & 1) ? LOW : HIGH);
      }
      seg ++;                                     // on to next segment
      segments /= 2;
      updates  /= 2;
    }
    digitalWrite (digpins[scandigit], HIGH);      // turn digit on
  }

  // sample interrupt count exactly UPDPERSEC times per second
  if (thismicros - lastsamplemicros >= MICROSPERUPD) {
    // set up for next sample
    lastsamplemicros += MICROSPERUPD;
    // grab interrupt count and reset it
    noInterrupts();
    uint16_t thiscount = interruptcounter;
    interruptcounter = 0;
    interrupts();
    // update total count for the past second
    totalcount += thiscount - maincounters[updateindex];
    maincounters[updateindex] = thiscount;
    if (++ updateindex == UPDPERSEC) updateindex = 0;
    Serial.println(totalcount);
    // fill in at least 1 and up to 5 digits
    uint16_t count = totalcount;
    uint16_t i = 0;
    do {
      uint16_t dig = count % 10;
      digsegs[i++] = digtosegs[dig];
      count /= 10;
    } while ((i < NUMDIGITS) && (count > 0));
    // blank fill upper digits
    while (i < NUMDIGITS) digsegs[i++] = 0;
    // toggle the on-board LED
    ledison ^= true;
    digitalWrite (LED_BUILTIN, ledison ? HIGH : LOW);
  }
}

void freqPinInterrupt ()
{
  interruptcounter ++;
}
