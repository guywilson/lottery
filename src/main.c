#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define NUM_BALLS_IN_PRIMARY_SET         5
#define NUM_BALLS_IN_SECONDARY_SET       2

#define NUM_RANDOM_BYTES               256

#define PRIMARY_SET_MAX                 50
#define SECONDAY_SET_MAX                11

static uint8_t randomBytes[NUM_RANDOM_BYTES];
static int randomBytePointer = 0;

static int _getProgNameStartPos(char * pszProgName) {
	int i = strlen(pszProgName);

	while (pszProgName[i] != '/' && i > 0) {
		i--;	
	}

	if (pszProgName[i] == '/') {
		i++;
	}

	return i;
}

static void printUsage(char * pszProgName) {
	printf("Using %s:\n", &pszProgName[_getProgNameStartPos(pszProgName)]);
    printf("    %s --help (show this help)\n", &pszProgName[_getProgNameStartPos(pszProgName)]);
    printf("    %s [options]\n", &pszProgName[_getProgNameStartPos(pszProgName)]);
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

static uint32_t getRandomUint32(void) {
    uint32_t randomValue = 0;
    
    for (int i = 0; i < sizeof(uint32_t); i++) {
        randomValue |= (getRandomByte() << (i * 8));
    }
    
    return randomValue;
}

static uint16_t getRandomUint16(void) {
    uint16_t randomValue = 0;
    
    for (int i = 0; i < sizeof(uint16_t); i++) {
        randomValue |= (getRandomByte() << (i * 8));
    }
    
    return randomValue;
}

static int inline constrainedRand(int max) {
    return (rand() / ((RAND_MAX + 1u) / max));
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

static void shuffleSetRounds(uint8_t * set, int maxBallNumber, int numRounds) {
    srand(getRandomUint32());

    for (int c = 0; c < numRounds; c++) {
        // Shuffle the set using Fisher-Yates algorithm
        for (int i = maxBallNumber - 1; i > 0; i--) {
            int j = constrainedRand(maxBallNumber);
            uint8_t temp = set[i];
            set[i] = set[j];
            set[j] = temp;
        }
    }
}

static void shuffleSet(uint8_t * set, int maxBallNumber) {
    int numRounds = 256 + (getRandomUint16() / 8);
    shuffleSetRounds(set, maxBallNumber, numRounds);
}

static uint8_t * getBalls(uint8_t * set, int maxBallNumber, int numBalls) {
    uint8_t * balls = (uint8_t *)malloc(numBalls * sizeof(uint8_t));
    
    if (balls == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    // Choose a point to take balls from...
    int ballPoint = maxBallNumber / 3;

    for (int i = 0; i < numBalls; i++) {
        balls[i] = set[ballPoint++];

        shuffleSetRounds(set, maxBallNumber, 256);

        if (ballPoint >= maxBallNumber) {
            ballPoint = 0;
        } 
    }

    sortBalls(balls, numBalls);

    return balls;
}

static void initialiseSet(uint8_t * set, int maxBallNumber) {
    for (int i = 0; i < maxBallNumber; i++) {
        set[i] = i + 1;
    }
}

int drawBalls(int setLength, int numBalls) {
    uint8_t * set = (uint8_t *)malloc(setLength * sizeof(uint8_t));

    if (set == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }
    
    initialiseSet(set, setLength);
    shuffleSet(set, setLength);

    // printf("\nSet: ");
    // for (int i = 0; i < setLength; i++) {
    //     printf("%d ", set[i]);
    // }
    // printf("\n\n");

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
