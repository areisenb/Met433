#include <stdio.h>
#include <string.h>
#include "sequencer.h"

/* the factory's static variables */
CSequencer CSequencer::aSequencer [RX_BUFFERS];
int CSequencer::writeIdx = 0;
int CSequencer::readIdx = 0;
int CSequencer::rcvdFrames = 0;

/* the factory methods of the sequencer */
CSequencer* CSequencer::attach (char cBit) {
  CSequencer* pSeq;
  writeIdx = (++writeIdx) % RX_BUFFERS;
  pSeq = aSequencer + writeIdx;
  if (pSeq->isAvailable ()) {
    pSeq->init (cBit);
  } else pSeq = NULL;
  return pSeq;
}

CSequencer* CSequencer::getNext () {
  readIdx = (++readIdx) % RX_BUFFERS;
  return aSequencer + readIdx;
}

char* CSequencer::showStati () {
  static char szOut [1024] = ""; // not the cleanest way
  char* cpOut = szOut;

  cpOut += sprintf (cpOut, "%d frame(s) received\n", rcvdFrames);
  cpOut += sprintf (cpOut, "idxWrite: %2d, idxRead: %2d\n", writeIdx, readIdx);
  for (int i=0; i<RX_BUFFERS; i++)
    cpOut += sprintf (cpOut, "RX %02d %s \n", i, aSequencer[i].showStatus ());
  cpOut += sprintf (cpOut, "\n");
  return (szOut);
}

/* the sequencer's instance methods */
CSequencer::CSequencer () {
  clear ();
}

void CSequencer::clear () {
  memset (rxBuffer, 0, sizeof (rxBuffer));
  memset (rxDbgBuffer, 0, sizeof (rxDbgBuffer));
  rxBufIdx = 0;
  rxDbgBufIdx = 0;
  nFirstRXError = -1;
  cRcvState = 'I'; // we start in idle state
}

void CSequencer::init (char cBit) {
  clear ();
  cRcvState = 'S';
  rxDbgBuffer [rxDbgBufIdx++] = cBit;
}

unsigned char CSequencer::push1Bit (int bufIdx, unsigned char ucByte) {
    return (ucByte | (1 << (7- (bufIdx & 7))));
}

int CSequencer::addBit (char cBit) {
  if (cRcvState != 'S') return -1;
  switch (cBit) {
    case 'T':
      rxBuffer [rxBufIdx/8] = push1Bit (rxBufIdx, rxBuffer [rxBufIdx/8]);
      rxBufIdx ++;
      break;
    case 'F':
      rxBufIdx ++;
      break;
    case 'S':
      /* we have finished our reception */
      cRcvState = 'R';
      break;
    case 'M':
      /* perfekt - theoretically we should check is Bit is followed by a MARK */
      break;
    default:
      nFirstRXError = rxBufIdx;
      break;
  }
  rxDbgBuffer [rxDbgBufIdx++] = cBit;
  if (rxDbgBufIdx > RX_DBG_BUFFER_SIZE) {
    rxDbgBufIdx =  RX_DBG_BUFFER_SIZE;
    rxDbgBuffer [rxDbgBufIdx] = '.';
    /* more than 40 Byte of scrap finishes reception as well */
    cRcvState = 'N';
  }
  if (rxBufIdx/8 > RX_BUFFER_SIZE)
    cRcvState = 'N';
  if (hasReceived ()) {
    rcvdFrames++;
    return -1;
  }
  return 0;
}

CSequencer* CSequencer::finish () {
  clear ();
  return getNext ();
}

char* CSequencer::showStatus () {
  static char szOut [1024] = ""; // not the cleanest way
  char* cpOut = szOut;
  cpOut += sprintf (cpOut, "State: %c, rxBufIdx: %d, rxDbgBufIdx: %d", 
    cRcvState, rxBufIdx, rxDbgBufIdx);
  return szOut;
}

void CSequencer::readData (char szOut [], int* pBitLen, int* pFirstErrIdx) {
  if (!hasReceived ()) {
    szOut[0] = 0;
    *pFirstErrIdx = 0;
  }

  *pBitLen = (*pBitLen < (rxBufIdx+1)) ? *pBitLen : rxBufIdx+1;
  memcpy (szOut, rxBuffer, (*pBitLen-1)/8+1);
  *pFirstErrIdx = nFirstRXError;
}

char* CSequencer::getHexBuffer() {
  static char szOut [1024] = ""; // not the cleanest way
  char* cpOut = szOut;
  static char szTemp [80] = "";

  if (!hasReceived ()) {
    sprintf (cpOut, "nothing received yet\n");
    return szOut;
  }

  if (nFirstRXError < 0)
    strcpy (szTemp, "without any errors");
  else
    sprintf (szTemp, "with errors from Bit %d on", nFirstRXError);
  cpOut += sprintf(cpOut, "RX Buffer %d Bits received %s until %s\n",
    rxBufIdx,
    szTemp,
    cRcvState == 'R' ? "next sync" :
    cRcvState == 'N' ? "buffer full" : "???");
  for (int i = 0; i < (sizeof (rxBuffer) / 16); i++) {
    char szHex[256];
    char szASCII[256];
    char *cpHex = szHex;
    char *cpASCII = szASCII;
    for (int j = 0; j < 16; j++) {
      if ((16*i + j) < sizeof (rxBuffer)) {
        unsigned char uc = rxBuffer[i * 16 + j];
        cpHex += sprintf(cpHex, "%02X ", uc);
        cpASCII += sprintf(cpASCII, "%c",
                          ((uc >= ' ') && (uc <= 'z')) ? uc : '.');
      } else {
        cpHex += sprintf(cpHex, "   ");
        cpASCII += sprintf (cpASCII, " ");
      }
    }
    cpOut += sprintf(cpOut, "%3d: %s %s\n", i * 16, szHex, szASCII);
  }
  cpOut += printf(cpOut, "\n");
  return szOut;
}

char* CSequencer::getDbgBuffer () {
  static char szOut [1024] = ""; // not the cleanest way
  char* cpOut = szOut;
  char *cp = rxDbgBuffer;
  int i=0;
  int len = sizeof (rxDbgBuffer);
  cpOut += sprintf (cpOut, "RXDBGBUFFER received %d symbols\n", rxDbgBufIdx);
  while (strlen (cp) && (len>0)) {
    strncpy (cpOut, cp, 4);
    cpOut [4] = ' ';
    cpOut += 5;
    if ((i++)%2)
      *cpOut++ = ' ';
    if (i%16 == 0)
      *cpOut++ = '\n';
    *cpOut = 0;
    cp += 4;
    len -= 4;
  }
  cpOut += sprintf (cpOut, "\n");
  return szOut;
}