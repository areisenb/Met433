/* this is a very poor mock of the wiringPi library, it just
    implements what is needed for this project here, and even
    this is far away from complete */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <ctype.h>
#include <string.h>
#include "wiringPi.h"
#include <unistd.h>

#define MOCKFILE "mock.txt"

FILE *inFile = 0;
long actTimeUS = -1;
/* we only support one single delay thread */
long finTimeUS = -1;
pthread_mutex_t delayMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t delayCond = PTHREAD_COND_INITIALIZER;

pthread_t worker;
handler_t intHandler = NULL;

static void* thread_worker(void *name);
int scheduleDelay (void);

int wiringPiSetup (void) {
    long waited = 0;
    /* we just need the mutex to support our condition variable */
    pthread_mutex_lock (&delayMutex);

    /* create our worker thread */
    if( pthread_create( &worker, NULL, &thread_worker, NULL)!=0) {
        fprintf (stderr, "Could not create worker thread\n");
        exit (EXIT_FAILURE);
    }
    return 0;
}

int readNextLong (long* plRet, FILE* inFile) {
    bool inComment = false;
    static int nLine = 1;
    static int nCol = 0;
    char szDigits [20] = ""; // 19 digits should be enough
    char* cp = szDigits;
    char c;
    while ((c=fgetc (inFile)) != EOF) {
        nCol++;
        if (c == '\n') {
            nLine++;
            nCol = 0;
            inComment = false;
        } else if (inComment)
            continue;
        else if (c=='#') {
            inComment = true;
        } else if (!(isspace(c) || isdigit(c) || (c==','))) {
            fprintf (stderr,
                "illegal character %c at line %d col %d\n",
                c, nLine, nCol);
            return (EOF);
        }

        if (isdigit (c) ) {
            if (strlen (szDigits) < (sizeof (szDigits) -1)) {
                *cp++ = c;
                *cp = 0;
            } else {
                fprintf (stderr,
                    "too big number %s at line %d col %d\n",
                    szDigits, nLine, nCol);
                return (EOF);
            }
        } else if (*szDigits)
                return sscanf (szDigits, "%ld", plRet);
    }
    return (EOF);
}

static void* thread_worker(void *name) {
    long lDuration;
    fprintf (stderr, "opening file %s\n", MOCKFILE);
    actTimeUS = 0; /* this is the signal we have claimed our mutex */
    inFile = fopen (MOCKFILE, "rt");
    if (inFile == NULL) {
        fprintf (stderr, "Could not open file %s\n", MOCKFILE);
        exit (EXIT_FAILURE);
    }

    while (readNextLong (&lDuration, inFile) != EOF) {
        actTimeUS += lDuration;
        if (intHandler) {
            intHandler ();
            scheduleDelay ();
        }
    }

    /* now we force the delay to be scheduled finally */
    finTimeUS = 1;

    /* we give our main loop another chance to do some cleanup */
    sleep (2);
    /* independent of the timing we let the 'delay' loop run on last time */
    scheduleDelay ();
    /* give it a second to do some output or so */
    sleep (1);
    /* but now it is all over */
    fprintf (stderr, "exit program  - act %ld, fin %ld\n", actTimeUS, finTimeUS);

    exit (EXIT_SUCCESS);
    /* Thread-end */
    // which never attacks as we close the program before
    pthread_exit((void *)pthread_self());
}

int scheduleDelay (void) {
    /* see if there is our main thread waits for some delay */
    if ((finTimeUS > 0) && (actTimeUS >= finTimeUS)) {
        pthread_cond_signal (&delayCond);
        finTimeUS = -1;
    }
    return 0;
}

int wiringPiISR (int datapin,int edge, handler_t handler) {
    intHandler = handler;
    return 0;
}

long micros (void) {
    return actTimeUS;
}

void delay (int nMS) {
    long slept = 0;
    finTimeUS = actTimeUS + nMS*1000;
    pthread_cond_wait(&delayCond, &delayMutex);
}

