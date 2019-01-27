#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>
#include "file_generator.h"

#define MIN_FILE_SIZE_MB 1                // The minimum file size to generate
#define MAX_FILE_SIZE_MB 128              // The maximum file size to generate

#define LINE_LEN 128                      // The default line length in a file

#define FILENAME_MAX_LEN 32               // Maximum filename length

#define COPY_FILENAME "temp_copy.txt"     // The filename that will be copied to
#define OUTPUT_FILENAME "trial_stats.txt" // The statistics output filename

#define UINT_SIZE sizeof(uint8_t)
#define CHAR_SIZE sizeof(char)

#define MAX_RAM_BYTES 1024                // Maximum bytes to be given to buffers

#define MIN(a,b) (((a)<(b))?(a):(b))

#define BINARY_PRECISION 4                // The precision at which to pad numbers

static void die(char *errString, int code)
{
    printf("%s\n", errString);
    exit(code);
}

// Generates each file to be copied
// By default, Files of sizes 1, 2, 4, 8, 16, 32, 64, and 128 MB are generated.
static void generateAllFiles(char **filenames, int *fileSizesInMB, int max_ram)
{
    int mb;
    int idx = 0;
    for (mb = MIN_FILE_SIZE_MB; mb <= MAX_FILE_SIZE_MB; mb *= 2)
    {
        // Generate the random file, storing filename and size in their arrays
        generateRandomFile(mb, max_ram, filenames[idx]);
        fileSizesInMB[idx] = mb;

        idx += 1;
    }
}

// Copies a file by reading/writing at the amount specified by nBytes.
// sizeInMB represents the file's size in MB
static float copyFileByNBytes(char *filename, int nBytes, int sizeInMB)
{
    // Open the file to be copied
    FILE *in = fopen(filename, "rb");
    if (in == NULL)
    {
        printf("Failed to open %s! Returning.", filename);
        return -1;
    }

    // Open the file to be written to
    FILE *out = fopen(COPY_FILENAME, "wb");
    if (out == NULL)
    {
        printf("Failed to open %s! Returning.", COPY_FILENAME);
        return -1;
    }

    int numBytesToCopy = getNumBytesInMB(sizeInMB);

    // The buffer to use for copying
    uint8_t *byteBuffer = (uint8_t *)malloc((UINT_SIZE * numBytesToCopy));
    if (byteBuffer == NULL)
    {
        printf("Failed to allocate %d bytes!\n", UINT_SIZE * numBytesToCopy);
        return -1;
    }

    int bytesCopied = 0;

    int ret;

    struct timespec startTime;
    clock_gettime(CLOCK_ID, &startTime);

    // While there are more bytes to copy...
    while (bytesCopied < numBytesToCopy)
    {
        // Read up to nBytes from file, store number read in ret
        ret = fread(byteBuffer, UINT_SIZE, nBytes, in);
        if (ret == -1)
        {
            printf("Failed to read from %s! Aborting copy.\n", filename);
            return -1;
        }

        // Write the buffer to the output file
        ret = fwrite(byteBuffer, UINT_SIZE, ret, out);
        if (ret == -1)
        {
            printf("Failed to write to %s! Aborting copy.\n", filename);
            return -1;
        }

        bytesCopied += ret;
    }

    double elapsed = getTimeElapsed(startTime);

    free(byteBuffer);

    fclose(in);
    fclose(out);

    // Remove the output file
    remove(COPY_FILENAME);

    return elapsed;
}

// Outputs a trial's statistics to a text file
static void outputTrialToFile(int fileSize, int nBytes, double elapsed)
{
    // Open the output file
    FILE *out = fopen(OUTPUT_FILENAME, "a+");
    if (out == NULL)
    {
        printf("Error! Failed to open %s!\n", OUTPUT_FILENAME);
        return;
    }

    char *line = (char *)malloc(sizeof(char) * LINE_LEN);
    if (line == NULL)
    {
        printf("Failed to allocate %d bytes!\n", sizeof(char) * LINE_LEN);
        return;
    }

    // Format the line and write to the file
    snprintf(line, LINE_LEN, "%d %d %f\n", fileSize, nBytes, elapsed);
    fwrite(line, CHAR_SIZE, strlen(line), out);

    fclose(out);
    free(line);
}

// Copy a file for each number of bytes
static void copyFile(char *filename, int fileSizeInMB, int maxBufferSize)
{
    // Start number of bytes at 1
    int nBytes = 1;

    double elapsed;

    printf("Copying %s to %s...\n", filename, COPY_FILENAME);

    // Copy file by nBytes up to maxBufferSize
    while (nBytes <= maxBufferSize)
    {
        printf("... %05d byte(s) at a time... ", nBytes);

        elapsed = copyFileByNBytes(filename, nBytes, fileSizeInMB);
        if (elapsed == -1)
        {
            return;
        }   

        printf("Took %f seconds!\n", elapsed);

        outputTrialToFile(fileSizeInMB, nBytes, elapsed);

        // Double nBytes
        nBytes *= 2;
    }

    printf("\n");
}


int main(int argc, char *argv[])
{
    // Flush printf's immediately
    setbuf(stdout, NULL);

    int i;

    int max_ram = MAX_RAM_BYTES;
    if (argc > 1)
    {
        int providedRam = atoi(argv[1]);
        if (providedRam == 0)
        {
            printf("Error! Invalid max RAM argument provided: %s. Defaulting to %d.\n", argv[1], MAX_RAM_BYTES);
        }
        else
        {
            max_ram = atoi(argv[1]);
        }
    }

    printf("Using Max RAM size of %dB for file generation/copying...\n", max_ram);

    remove(OUTPUT_FILENAME);

    // The number of files to generate based on min/max file sizes
    int numFiles = getNumberOfFilesToGenerate(MIN_FILE_SIZE_MB, MAX_FILE_SIZE_MB);

    // Allocate the array of file sizes
    int *fileSizesInMB = (int *)malloc(sizeof(int) * numFiles);
    if (fileSizesInMB == NULL)
    {
        die("Failed to allocate filesize array! Exiting.", 1);
    }

    // Allocate the array of filenames
    char **filenames = (char **)malloc(sizeof(char *) * numFiles);
    if (filenames == NULL)
    {
        die("Failed to allocate filenames array! Exiting.", 1);
    }

    for (i = 0; i < numFiles; i++)
    {
        filenames[i] = (char *)malloc(sizeof(char) * FILENAME_MAX_LEN);
        if (filenames[i] == NULL)
        {
            die("Failed to allocate filename!", 1);
        }
    }

    generateAllFiles(filenames, fileSizesInMB, max_ram);

    for (i = 0; i < numFiles; i++)
    {
        copyFile(filenames[i], fileSizesInMB[i], max_ram);
    }

    // Remove generated files and free their filenames
    for (i = 0; i < numFiles; i++)
    {
        remove(filenames[i]);
        free(filenames[i]);
    }
    free(filenames);

    free(fileSizesInMB);

    return 0;
}
