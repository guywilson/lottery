#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define NUM_RANDOM_BYTES               256
#define NUM_WARMUP_CYCLES                3

struct _ball_t {
    struct _ball_t * previous;
    struct _ball_t * next;

    uint8_t value;
};

typedef struct _ball_t ball_t;

static uint8_t randomBytes[NUM_RANDOM_BYTES];
static int randomBytePointer = 0;

static void printUsage(char * pszProgName) {
	printf("Using lottery:\n");
    printf("    lottery --help (show this help)\n");
    printf("    lottery [options]\n");
    printf("    options: --game=[game] where game in ('eur', 'lot', 'sfl')\n");
    printf("             --draws=[num draws]\n\n");
}

void printSet(ball_t * set) {
    uint8_t start = set->value;
    int c = 0;

    while (1) {
        printf("%02d ", set->value);
        set = set->next;

        c++;
        
        if (c == 10) {
            printf("\n");
            c = 0;
        }

        if (set->value == start) {
            break;
        }
    }

    printf("\n");
}

static void generateRandomBytes(void) {
    FILE * fp = fopen("/dev/urandom", "rb");
    
    if (fp == NULL) {
        perror("Failed to open /dev/urandom");
        exit(EXIT_FAILURE);
    }

    size_t bytesRead = fread(randomBytes, 1, NUM_RANDOM_BYTES, fp);
    
    if (bytesRead != NUM_RANDOM_BYTES) {
        perror("Failed to read random bytes");
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    
    fclose(fp);
}

static uint8_t getRandomByte(void) {
    if (randomBytePointer >= NUM_RANDOM_BYTES) {
        randomBytePointer = 0;
    }
    
    return randomBytes[randomBytePointer++];
}

static uint16_t getRandomUint16(void) {
    uint16_t randomValue = 0;
    
    for (int i = 0; i < sizeof(uint16_t); i++) {
        randomValue |= (getRandomByte() << (i * 8));
    }
    
    return randomValue;
}

static inline void initialiseRNG(void) {
    static uint16_t seedArray[3];

    seedArray[0] = getRandomUint16();
    seedArray[1] = getRandomUint16();
    seedArray[2] = getRandomUint16();

    seed48(seedArray);

    srandom(time(NULL));
}

static inline ball_t * getBallAt(ball_t * set, int index) {
    ball_t * ball = set;

    for (int i = 0;i < index;i++) {
        ball = ball->next;

        if (ball == NULL) {
            break;
        }
    }

    return ball;
}

static inline int getRandomIndex(int max) {
    return ((int)(drand48() * (double)max));
}

static inline ball_t * getRandomBall(ball_t * set, int max) {
    return getBallAt(set, getRandomIndex(max));
}

static ball_t * removeBallAt(ball_t * set, int index) {
    ball_t * ball = getBallAt(set, index);

    ball->previous->next = ball->next;
    ball->next->previous = ball->previous;

    return ball;
}

static void sortBalls(uint8_t * balls, int numBalls) {
    for (int i = 0; i < numBalls - 1; i++) {
        for (int j = i + 1; j < numBalls; j++) {
            if (balls[i] > balls[j]) {
                uint8_t temp = balls[i];
                balls[i] = balls[j];
                balls[j] = temp;
            }
        }
    }
}

static void initialiseSet(ball_t * set, int maxBallNumber) {
    for (int i = 0; i < maxBallNumber; i++) {
        ball_t * ball = &set[i];

        ball->value = i + 1;

        if (i == 0) {
            ball->previous = NULL;
            ball->next = &set[i + 1];
        }
        else if (i == maxBallNumber - 1) {
            ball->previous = &set[i - 1];
            ball->next = NULL;
        }
        else {
            ball->previous = &set[i - 1];
            ball->next = &set[i + 1];
        }
    }

    set[0].previous = &set[maxBallNumber - 1];
    set[maxBallNumber - 1].next = &set[0];
}

static void shuffleSetRounds(ball_t * set, int maxBallNumber, int numRounds) {
    for (int c = 0; c < numRounds; c++) {
        // Shuffle the set using Fisher-Yates algorithm
        for (int i = maxBallNumber - 1; i > 0; i--) {
            ball_t * rball = getRandomBall(set, maxBallNumber);
            ball_t * tball = getBallAt(set, i);

            uint8_t temp = tball->value;
            tball->value = rball->value;
            rball->value = temp;
        }
    }
}

static void shuffleSet(ball_t * set, int maxBallNumber) {
    int numRounds = 256 + (getRandomUint16() / 2);
    shuffleSetRounds(set, maxBallNumber, numRounds);
}

static uint8_t * getBalls(ball_t * set, int maxBallNumber, int numBalls) {
    uint8_t * balls = (uint8_t *)malloc(numBalls * sizeof(uint8_t));
    
    if (balls == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    int ballPoint = maxBallNumber / 3;

    for (int i = 0;i < numBalls;i++) {
        ball_t * chosenBall = removeBallAt(set, ballPoint);

        balls[i] = chosenBall->value;

        shuffleSetRounds(set, maxBallNumber, (int)getRandomByte() + 13);
    }

    sortBalls(balls, numBalls);

    return balls;
}

static uint8_t * drawBalls(int setLength, int numBalls) {
    ball_t * set = (ball_t *)malloc(setLength * sizeof(ball_t));

    if (set == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    
    initialiseSet(set, setLength);
    shuffleSet(set, setLength);

    uint8_t * balls = getBalls(set, setLength, numBalls);

    if (balls == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
    }

    free(set);

    return balls;
}

static void printBalls(uint8_t * balls, int setLength, int numBalls) {
    printf("Draw %d from 1-%d: *** ", numBalls, setLength);

    for (int i = 0; i < numBalls; i++) {
        printf("%d ", balls[i]);
    }
    printf("***\n");
}

int main(int argc, char *argv[]) {
    int primarySetLength = 0;
    int numPrimaryBalls = 0;
    int secondarySetLength = 0;
    int numSecondaryBalls = 0;
    int numDraws = 1;

    if (argc > 1) {
        for (int i = 1;i < argc;i++) {
            char * arg = argv[i];

            if (arg[0] == '-') {
                if (strncmp(arg, "--help", 6) == 0) {
                    printUsage(argv[0]);
                    return 0;
                }
                else if (strncmp(arg, "--game=", 7) == 0) {
                    char * pszGame = strdup(&arg[7]);

                    if (strncmp(pszGame, "eur", 3) == 0) {
                        primarySetLength = 50;
                        numPrimaryBalls = 5;

                        secondarySetLength = 12;
                        numSecondaryBalls = 2;
                    }
                    else if (strncmp(pszGame, "lot", 3) == 0) {
                        primarySetLength = 59;
                        numPrimaryBalls = 6;
                    }
                    else if (strncmp(pszGame, "sfl", 3) == 0) {
                        primarySetLength = 47;
                        numPrimaryBalls = 5;

                        secondarySetLength = 10;
                        numSecondaryBalls = 1;
                    }
                    else {
                        fprintf(stderr, "Invalid game type '%s'\n", pszGame);
                        return -1;
                    }
                }
                else if (strncmp(arg, "--draws=", 8) == 0) {
                    numDraws = atoi(&arg[8]);
                }
                else {
                    fprintf(stderr, "Invalid option %s - %s --help for help", arg, argv[0]);
                    return -1;
                }
            }
        }
    }
    else {
        printUsage(argv[0]);
        return 0;
    }

    generateRandomBytes();
    initialiseRNG();

    /*
    ** Warm up the machine...
    */
    uint8_t * balls;
    for (int i = 0;i < numDraws + NUM_WARMUP_CYCLES;i++) {
        balls = drawBalls(primarySetLength, numPrimaryBalls);

        if (balls == NULL) {
            return -1;
        }

        if (i >= NUM_WARMUP_CYCLES) {
            printBalls(balls, primarySetLength, numPrimaryBalls);
        }

        free(balls);

        if (secondarySetLength > 0) {
            balls = drawBalls(secondarySetLength, numSecondaryBalls);

            if (balls == NULL) {
                return -1;
            }

            if (i >= NUM_WARMUP_CYCLES) {
                printBalls(balls, secondarySetLength, numSecondaryBalls);
            }

            free(balls);
        }

        if (i >= NUM_WARMUP_CYCLES) {
            printf("\n");
        }
    }

    return 0;
}
