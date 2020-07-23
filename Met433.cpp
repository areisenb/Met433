/* Convert RF signal into bits (temperature sensor version) 
 * Written by : Ray Wang (Rayshobby LLC)
 * http://rayshobby.net/?p=8827
 * Update: adapted to RPi using WiringPi 
 */

// ring buffer size has to be large enough to fit
// data between two successive sync signals


#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "sequencer.h"

#define RING_BUFFER_SIZE 256

#define SYNC_LENGTH_MAX 9300
#define SYNC_LENGTH_MIN 8900
#define SYNC_LENGTH 9100
#define SEP_LENGTH_MAX 630
#define SEP_LENGTH_MIN 450
#define BIT1_LENGTH_MAX 4300
#define BIT1_LENGTH_MIN 3900
#define BIT0_LENGTH_MAX 2300
#define BIT0_LENGTH_MIN 1900
#define SPIKE_LENGTH_MAX 200

#define DATA_PIN 2  // wiringPi GPIO 2 (P1.13)


unsigned long timings[RING_BUFFER_SIZE];
char detections [RING_BUFFER_SIZE];

unsigned long ulFrame = 0;

char* getISO8601Time () {
  static char szTime [256];
  time_t now = time(&now);
  if (now == -1) {
    strcpy (szTime, "invalid time");
    return szTime;
  }
  tm *ptm = gmtime(&now);
  strftime (szTime, sizeof (szTime), "%FT%TZ", ptm);
  return szTime;
}

char* decodeMessage (CSequencer* pSeq) {
  /* coding looks like:
    25.7° 20% 91 06 10 11 48
              AA BC DD DE EF             
    AA  (91) ...sensor ID or so - no idea
    B   (0)  ...no idea
    C   (6)  ...no idea
    C&3 (2)  ...channel id (0->ch 1, 1->ch 2, 2->ch3
    DDD (101)...temp (257 -> 25.7°)
    EE  (14) ...humidity (20 -> 20%)
    F   (8)  ...no idea  - CRC?
  */
  static char szOut [1024];
  char* cp = szOut;
  char szMsg [10];
  int nDeziTemp; // to avoid any floating point operations
  int nHumidity;
  int nMsgBitLen = sizeof (szMsg)*8 - 1;
  bool bOK = pSeq->readData (szMsg, &nMsgBitLen);

  *cp = 0;
  if (*szMsg == 0)
    /* a total empty string does not help a lot */
    return szOut;
  if (nMsgBitLen < 28)
    /* if the message is too short, we cannot even read the temperature */
    return szOut;
  cp += sprintf (cp, "%s ", getISO8601Time ());
  cp += sprintf (cp, "channel: %d ", 1 + (szMsg[1] & 3));
  nDeziTemp = szMsg[2] * 16 + szMsg[3] / 16;
  cp += sprintf (cp, "temp: %d.%d ", nDeziTemp/10, nDeziTemp%10);
  if (nMsgBitLen >= 36) {
    /* OK we got humidity as well */
    nHumidity = (szMsg[3] & 0x0F) * 16 + szMsg[4] / 16;
    cp += sprintf (cp, "humidity: %d%% ", nHumidity);
  }
  cp += sprintf (cp, "Byte 0: %02X ", szMsg[0]);
  cp += sprintf (cp, "Byte 1: %02X ", szMsg[1]);
  if (!bOK)
    cp += sprintf (cp, "ERROR in detection");
  cp += sprintf (cp, "\n");
  return szOut;
}

char bitDetector (long t2, long t1, long t0) {
    unsigned long delta1 = t1 - t2;
    unsigned long delta0 = t0 - t1;
    char bitRet = 'I';
    bool bWasValidSep = (delta1 > SEP_LENGTH_MIN) && (delta1 < SEP_LENGTH_MAX);

    if ((delta0 > SYNC_LENGTH_MIN) && (delta0 < SYNC_LENGTH_MAX))
        bitRet = 'S'; //SYNC
    else if ((delta0 > BIT1_LENGTH_MIN) && (delta0 < BIT1_LENGTH_MAX))
        bitRet = 'T'; //TRUE
    else if ((delta0 > BIT0_LENGTH_MIN) && (delta0 < BIT0_LENGTH_MAX))
        bitRet = 'F'; //FALSE
    else if ((delta0 > SEP_LENGTH_MIN) && (delta0 < SEP_LENGTH_MAX))
        bitRet = 'M'; //MARK
    else if (delta0 < SPIKE_LENGTH_MAX)
        bitRet = 'P'; //SPIKE PULS
    /* we want a valid seperator before any bit and the sync pulse */
    if (strchr ("STF", bitRet) && !bWasValidSep)
        bitRet = tolower (bitRet);
    return bitRet;
}

void handler(){
  static CSequencer* pSequencer = NULL;
  static long t2 = 0;
  static long t1 = 0;
  static int bufIdx = 0;
  long t0 = micros();
  char rcvd = bitDetector (t2, t1, t0);

  /* we only buffer, if it is more than a spike */
  if (tolower (rcvd) != 'p') {
    timings[bufIdx] = t0 - t1;
    detections[bufIdx] = rcvd;
    bufIdx = (++bufIdx) % RING_BUFFER_SIZE;
    if (pSequencer)
      if (pSequencer->addBit (rcvd) < 0)
        pSequencer = NULL;
    if ((!pSequencer) && (rcvd == 'S'))
      pSequencer = CSequencer::attach (rcvd);
  }
  t2 = t1;
  t1 = t0;
  ulFrame ++;
}

void bufOut (char acDetects [], unsigned long aulTiming [], int len) {
    printf ("Frame: %d\n", ulFrame);
    for (int i=0; i<(len/8); i++) {
        char szDets [256];
        char szTimings [256];
        char* cpDets = szDets;
        char* cpTimings = szTimings;
        for (int j=0; j<8; j++) {
            cpDets += sprintf (cpDets, "%c ", acDetects[i*8+j]);
            cpTimings += sprintf (cpTimings, "%6d ", aulTiming[i*8+j]);
        }
        printf ("%3d %s %s\n", i*8, szDets, szTimings);
    }
    printf ("\n");
}

int main(int argc, char *argv[]){
  int nVerb = 0;
  CSequencer* pSequencer = CSequencer::getNext ();

  if (argc > 1) {
    if (strcmp (argv[1], "-v") == 0) {
      nVerb = 1;
    }
    if (strcmp (argv[1], "-vv") == 0) {
      nVerb = 2;
    }
    if (strcmp (argv[1], "-vvv") == 0) {
      nVerb = 3;
    }
  }

  if(wiringPiSetup() == -1){
    printf("no wiring pi detected\n");
    return 0;
  }
  wiringPiISR(DATA_PIN,INT_EDGE_BOTH,&handler);

  while (true) {
    delay (1000);
    if (nVerb > 2)
      bufOut (detections, timings, RING_BUFFER_SIZE);
    if (nVerb > 1)
      printf (CSequencer::showStati ());
    while (pSequencer->hasReceived ()) {
      if (nVerb > 0) {
        printf (pSequencer -> getHexBuffer ());
        printf (pSequencer -> getDbgBuffer ());
      }
      printf (decodeMessage (pSequencer));
      pSequencer = pSequencer->finish ();
    }
    fflush (stdout);
  }
}


