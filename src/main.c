#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define NUM_RANDOM_BYTES               256

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

// static uint32_t getRandomUint32(void) {
//     uint32_t randomValue = 0;
    
//     for (int i = 0; i < sizeof(uint32_t); i++) {
//         randomValue |= (getRandomByte() << (i * 8));
//     }
    
//     return randomValue;
// }

static uint16_t getRandomUint16(void) {
    uint16_t randomValue = 0;
    
    for (int i = 0; i < sizeof(uint16_t); i++) {
        randomValue |= (getRandomByte() << (i * 8));
    }
    
    return randomValue;
}

static inline void initialiseRNG(void) {
    time_t currentTime = time(NULL);
    
    static uint16_t seedArray[3];

    seedArray[0] = getRandomUint16() ^ (currentTime & 0xFFFF);
    seedArray[1] = getRandomUint16() ^ ((currentTime >> 16) & 0xFFFF);
    seedArray[2] = getRandomUint16();

    seed48(seedArray);
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

static inline ball_t * getRandomBall(ball_t * set, int max) {
    return getBallAt(set, (int)(drand48() * (double)max));
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

    // Choose a point to take balls from...
    int ballPoint = maxBallNumber / 3;

    for (int i = 0;i < numBalls;i++) {
        ball_t * chosenBall = removeBallAt(set, ballPoint);

        balls[i] = chosenBall->value;

        shuffleSetRounds(set, maxBallNumber, 256);
    }

    sortBalls(balls, numBalls);

    return balls;
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

int drawBalls(int setLength, int numBalls) {
    ball_t * set = (ball_t *)malloc(setLength * sizeof(ball_t));

    if (set == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }
    
    initialiseSet(set, setLength);
    shuffleSet(set, setLength);

    uint8_t * balls = getBalls(set, setLength, numBalls);

    if (balls == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        free(set);
        return -1;
    }

    printf("Draw %d from 1-%d: ", numBalls, setLength);

    for (int i = 0; i < numBalls; i++) {
        printf("%d ", balls[i]);
    }

    printf("\n");

    free(balls);
    free(set);

    return 0;
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

    for (int i = 0;i < numDraws;i++) {
        if (drawBalls(primarySetLength, numPrimaryBalls)) {
            return -1;
        }

        if (secondarySetLength > 0) {
            if (drawBalls(secondarySetLength, numSecondaryBalls)) {
                return -1;
            }
        }
    }

    return 0;
}
