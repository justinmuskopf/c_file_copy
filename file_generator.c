#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>
#include "file_generator.h"

#define BYTES_IN_MB 1048576

#define SEC_TO_NS 1000000000

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)<(b))?(b):(a))

typedef struct timespec ts;

// Returns the current time - start
// To represent a float time in seconds
double getTimeElapsed(struct timespec start)
{
    struct timespec now;
    clock_gettime(CLOCK_ID, &now);

    int s = now.tv_sec - start.tv_sec;
    long ns = now.tv_nsec - start.tv_nsec;
    if (s > 0 && ns < 0)
    {
        s--;
        ns += SEC_TO_NS;
    }

    double timeInSeconds = (double)s + (ns / (double)SEC_TO_NS);

    return timeInSeconds;
}

// Place into filename the formatted filename of this size
static void getFilenameFromSize(int sizeInMB, char *filename)
{
    sprintf(filename, FILENAME_FMT, sizeInMB);
}


// Returns the number of bytes in mb MB
int getNumBytesInMB(int mb)
{
    return mb * BYTES_IN_MB;
}

// Returns the number of files to generate based on
// doubling the size until it reaches max size
int getNumberOfFilesToGenerate(int minSize, int maxSize)
{
    int mb = minSize;

    int numFiles = 0;

    while (mb <= maxSize)
    {
        numFiles++;
        
        mb *= 2;
    }

    return numFiles;
}

// Generates a random file of size sizeInMB
void generateRandomFile(int sizeInMB, int maxBufferSize, char *filename)
{
    ts start;
    clock_gettime(CLOCK_ID, &start);

    getFilenameFromSize(sizeInMB, filename);

    printf("Generating file %s of size %04d MB... ", filename, sizeInMB);

    int bytesNeeded = getNumBytesInMB(sizeInMB);
    
    FILE *generatedFile = fopen(filename, "w+");
    if (generatedFile == NULL)
    {
        printf("Error! could not open %s for writing! Exiting generator.\n", filename);
        return;
    }
	
    int bytesRead;
    int bytesWritten = 0;
    int totalBytesWritten = 0;

    // Create the buffer size, limiting at maxBufferSize
    int bufferSize = ((bytesNeeded >= maxBufferSize) ? maxBufferSize : bytesNeeded);
 
    uint8_t *buffer = (uint8_t *)malloc(sizeof(uint8_t) * bufferSize);
    if (buffer == NULL)
    {
        printf("Failed to allocate %d bytes to buffer!\n", bufferSize);
    }

    FILE *dev_urandom = fopen("/dev/urandom", "rb");
    if (dev_urandom == NULL)
    {
        printf("Error! Could not open urandom for reading! Exiting generator.\n");
        free(buffer);
        return;
    }
	
    // Until the totalBytes is what is needed
    while (totalBytesWritten < bytesNeeded)
    {
        int bytesLeft = MAX(0, totalBytesWritten - bytesNeeded);
        int bytesToRetrieve = (bytesLeft > 0 && bytesLeft < bufferSize) ? bytesLeft : bufferSize;

        bytesRead = fread(buffer, sizeof(uint8_t), bytesToRetrieve, dev_urandom);
        if (bytesRead == -1)
        {
            printf("Failed to generate file %s! Exiting generator.\n", filename);
			free(buffer);
            return;
        }

        bytesWritten = fwrite(buffer, sizeof(uint8_t), bytesRead, generatedFile);
        if (bytesWritten == -1)
        {
            printf("Failed to generate file %s! Exiting generator.\n", filename);
			free(buffer);
            return;
        }

        totalBytesWritten += bytesWritten;
    }
    
    printf("Took %f seconds.\n", getTimeElapsed(start));

    free(buffer);

    fclose(dev_urandom);
    fclose(generatedFile);
}

