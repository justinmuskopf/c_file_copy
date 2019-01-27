
#ifndef FILE_GENERATOR_H
#define FILE_GENERATOR_H

#define FILENAME_FMT "%04dMB.txt"
#define CLOCK_ID CLOCK_REALTIME

struct timespec;

int getNumberOfFilesToGenerate();
int getNumBytesInMB(int mb);
void generateRandomFile(int sizeInMB, int maxBufferSize, char *filename);
double getTimeElapsed(struct timespec start);

#endif
