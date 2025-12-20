//    Copyright (C) Mike Rieker, Beverly, MA USA
//    www.outerworldapps.com
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; version 2 of the License.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    EXPECT it to FAIL when someone's HeALTh or PROpeRTy is at RISk.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    http://www.gnu.org/licenses/gpl-2.0.html

// Runs on Arduino Nano Every to act as frequency counter
// It is connected to 5 nixie displays able to measure frequencies from 0 to 65535 cps

#define INPUT1_PIN 6  // signal to measure input
#define INPUT2_PIN 7  // second signal to measure

#define DSIN   2      // data input to nixies
#define DSOUT 12      // data output from nixies
#define STCP  11      // load nixie output register
#define SHCP   4      // shift nixie input register
#define _OUTEN 5      // nixie output register enable

#define HVON 020000
#define LOFF 016000
#define DIG0 000001

#define DUTYUS (1000000/60)         // duty cycle
#define DUTYONUS (DUTYUS * 7 / 10)  // nixies on for this fraction

#define SECPERDISP 2    // number of seconds to display each value
#define MULTIPLIER 1    // display this multiple of frequency
#define UPDPERSEC  7    // update frequency reading this many times per second (exact)
#define NUMDIGITS  5    // total number of digits
#define count_t uint16_t

#define EVENSPLIT ((1000000 % UPDPERSEC) == 0)
#if EVENSPLIT
#define MICROSPERUPD (1000000 / UPDPERSEC)
#else
#define MICROSPERUPD updatemicros[updateindex]
#endif

struct PinData {
  count_t volatile interruptcounter;
  count_t maincounters[UPDPERSEC*MULTIPLIER];
  count_t totalcount;
  count_t samplecounter;

  PinData ();
  void grabsample ();
  void updatevalues ();
  void shiftitout ();
};

int last_outen;
int pindisplayindex;
int updateindex;
PinData pin1;
PinData pin2;
uint32_t lastdutymicros;
uint32_t lastinmicros;
uint32_t lastsamplemicros;
#if ! EVENSPLIT
uint32_t updatemicros[UPDPERSEC*MULTIPLIER];
#endif

PinData *const pinarray[2] = { &pin1, &pin2 };

void outputdigit (uint16_t digbits);
void freqPin1Interrupt ();
void freqPin2Interrupt ();

void setup() {
  pinMode (LED_BUILTIN, OUTPUT);

  pinMode (DSIN, OUTPUT);
  pinMode (DSOUT, INPUT);
  pinMode (STCP, OUTPUT);
  pinMode (SHCP, OUTPUT);
  pinMode (_OUTEN, OUTPUT);

  last_outen = HIGH;
  digitalWrite (_OUTEN, HIGH);

  // microseconds between frequency reading updates
#if ! EVENSPLIT
  for (int j = 0; j < MULTIPLIER; j ++) {
    uint32_t accummicros = 0;
    for (int i = 0; i < UPDPERSEC; i ++) {
      uint32_t totalmicros = 1000000 * (i + 1) / UPDPERSEC;
      updatemicros[i+UPDPERSEC*j] = totalmicros - accummicros;
      accummicros = totalmicros;
    }
  }
#endif

  pinMode (INPUT1_PIN, INPUT);
  pinMode (INPUT2_PIN, INPUT);
  attachInterrupt (digitalPinToInterrupt (INPUT1_PIN), freqPin1Interrupt, RISING);
  attachInterrupt (digitalPinToInterrupt (INPUT2_PIN), freqPin2Interrupt, RISING);

  Serial.begin (9600);

  lastinmicros = lastdutymicros = lastsamplemicros = micros ();
  pindisplayindex = 0;
}

PinData::PinData ()
{
  memset (maincounters, 0, sizeof maincounters);
  totalcount = 0;
}

void loop() {
  uint32_t thismicros = micros ();

  // modulate digit brightness
  // _OUTEN = LOW for first DUTYONUS of DUTYUS microsecs
  //         HIGH for the rest of it
  uint32_t dutymicros;
  while ((dutymicros = thismicros - lastdutymicros) >= DUTYUS) {
    lastdutymicros += DUTYUS;
  }
  int this_outen = (dutymicros < DUTYONUS) ? LOW : HIGH;
  if (last_outen != this_outen) {
    last_outen = this_outen;
    digitalWrite (_OUTEN, last_outen);
  }

  // sample interrupt count exactly UPDPERSEC times per second
  if (thismicros - lastsamplemicros >= MICROSPERUPD) {

    // set up for next sample
    lastsamplemicros += MICROSPERUPD;

    // grab interrupt count and reset it
    noInterrupts ();
    pin1.grabsample ();
    pin2.grabsample ();
    interrupts ();

    // update total counts for past second
    pin1.updatevalues ();
    pin2.updatevalues ();

    // maybe at end of full second
    // change which pin we are displaying
    if (++ updateindex == UPDPERSEC*MULTIPLIER) {
      updateindex = 0;
      // comment out next line for cycle frequency only
      // uncomment for alternating cycle/instruction frequency
      ////pindisplayindex = (pindisplayindex + 1) % 2;
    }

    // start displaying new digits
    PinData *pd = pinarray[pindisplayindex];
    pd->shiftitout ();
  }
}

// sample interrupt count and clear interrupt count
// called with interrupts disabled
void PinData::grabsample ()
{
  samplecounter = interruptcounter;
  interruptcounter = 0;
}

// update total count from interrupt sampled count
void PinData::updatevalues ()
{
  // update total count for the past MULTIPLIER seconds
  totalcount += samplecounter - maincounters[updateindex];
  maincounters[updateindex] = samplecounter;
  Serial.println (totalcount);
}

// shift out the totalcount to the nixie tubes
void PinData::shiftitout ()
{
  // fill in at least 1 and up to 5 digits
  count_t count = totalcount;
  bool topdig9 = false;
  int i = 0;
  do {
    uint16_t dig = count % 10;
    topdig9 = (dig == 9);
    outputdigit (HVON | LOFF | (DIG0 << dig));
    count /= 10;
  } while ((++ i < NUMDIGITS) && (count > 0));

  // blank fill upper digits
  // - if we're displaying ___9x, leave 100s digit HV ON
  //   turn HV OFF for all other digits to save a little power
  while (++ i <= NUMDIGITS) {
    outputdigit ((topdig9 ? HVON : 0) | LOFF);
    topdig9 = false;
  }

  // update digits being displayed
  digitalWrite (STCP, HIGH);
  digitalWrite (STCP, LOW);
}

// output digit to nixie shift register chain
// output least significant digit first
// digbits : 16-bit mask
//   nc dp hv _red  _grn _blu 9 8  7 6 5 4  3 2 1 0
//  nc = not connected
//  dp = decimal point (active high)
//  hv = high-voltage on (active high)
//  _{red,grn,blu} = led enables (active low)
//  9..0 = digit enables (active high)
void outputdigit (uint16_t digbits)
{
  for (uint16_t j = 16; j > 0; -- j) {
    digitalWrite (DSIN, (digbits & 0100000) ? HIGH : LOW);
    digitalWrite (SHCP, HIGH);
    digitalWrite (SHCP, LOW);
    digbits <<= 1;
  }
}

// interrupt routine just counts rising edges
void freqPin1Interrupt ()
{
  pin1.interruptcounter ++;
}
void freqPin2Interrupt ()
{
  pin2.interruptcounter ++;
}
