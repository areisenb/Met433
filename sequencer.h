#define RX_BUFFERS 8
#define RX_BUFFER_SIZE 16
#define RX_DBG_BUFFER_SIZE (RX_BUFFER_SIZE*8*2 + 64)

class CSequencer {
    /* factory variables */
    static CSequencer aSequencer [RX_BUFFERS];
    static int writeIdx;
    static int readIdx;
    static int rcvdFrames;

    /* instance variables */
    /* it is a very simple state machine - we implement:
        I...Idle
        S...Synced
        R...Received until next sync
        N...Received until max no of chars reached */
    char cRcvState;
    unsigned char rxBuffer [RX_BUFFER_SIZE + 1];
    char rxDbgBuffer [RX_DBG_BUFFER_SIZE + 1];
    int rxBufIdx;
    int rxDbgBufIdx;
    bool bRXError;

    void clear ();
    unsigned char push1Bit (int bufIdx, unsigned char ucByte);

  public:
    /* factory methods */
    static CSequencer* attach (char cBit);
    static CSequencer* getNext ();
    static char* showStati ();

    /* instance methods */
    CSequencer ();
    inline bool isAvailable () { return (cRcvState == 'I'); };
    inline bool hasReceived () { return (strchr ("RN", cRcvState) != NULL); };
    void init (char cBit);
    int addBit (char cBit);
    CSequencer* finish ();
    bool readData (char szOut [], int* pBitLen);

    char* showStatus ();
    char* getHexBuffer();
    char* getDbgBuffer();

};

