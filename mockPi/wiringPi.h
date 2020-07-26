#define INT_EDGE_BOTH 1

typedef void (*handler_t)();

int wiringPiSetup (void);
int wiringPiISR (int datapin,int edge, handler_t handler);
long micros (void);
void delay (int nMS);
int digitalRead (int pin) ;
