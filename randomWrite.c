#define _GNU_SOURCE
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include <malloc.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fs.h>

int LONG_BITS = sizeof(long) * 8;
int RAND_LONG_ITERATIONS;
int RAND_MAX_BITS;

//Determine consecutive least significant bits in the int RAND_MAX that can be used to construct longs
int setRandMaxBits()
{
        int i = RAND_MAX;
        int numBits=0;
        while(1){
                if(0 == (int)(i & 1))
                        break;
                numBits++;
                i = i >> 1;
        }
        if(numBits==0)
                return -1;
        RAND_MAX_BITS=numBits;
        RAND_LONG_ITERATIONS=LONG_BITS/numBits;
        if(0 < LONG_BITS % numBits)
                RAND_LONG_ITERATIONS++;
	return 0;
}

long getRandomLongWithMaxBits(int maxBits)
{
	long ret = 0;
	int x, iterations = maxBits / RAND_MAX_BITS;
	for(x=0; x<iterations; x++) 
		ret = (ret << RAND_MAX_BITS) + rand();
	int remainingBits = maxBits % RAND_MAX_BITS;
	if(remainingBits==0)
		return ret;
	long divisor = 1;
	for(x=0; x<remainingBits; x++)
		divisor *= 2;
	return (ret << remainingBits) + (rand() % divisor);
}

//Get the number of bits (x) required to represent value, such that (2^x)-1 will be >=value
int getNumBitsToRepresent(long value)
{
	if(value<0)
		return -1;
	if(value==0)
		return 0;
	long valueMinus1=value-1, i=1;
	int numBits=1;
	while(i<valueMinus1) 
	{
		numBits++;
		i *= 2;
	}
	return numBits;
}

//Get an unbiased random long such that 0 <= return value <= maxValue
long getUnbiasedRandomLong(long maxValue)
{
        if(maxValue<0)
                return -1;
        if(maxValue==0)
                return 0;
        int numBitsToRepresent = getNumBitsToRepresent(maxValue);
        long ret = getRandomLongWithMaxBits(numBitsToRepresent);
        while(ret>maxValue)
        	ret = getRandomLongWithMaxBits(numBitsToRepresent);
        return ret;
}

//Seed the random number generator with the current timestamp
void seedRand()
{
        struct timeval now;
        gettimeofday(&now, NULL);
        srand(now.tv_sec * 1000000 + now.tv_usec);
}

//Open the device and use ioctl to retrieve the logical sector size and number of sectors
int setDeviceSizeInfo(char *devicePath, long *numBlocks, long *sectorSizeBytes) 
{
	int fd = open(devicePath, O_RDONLY);
	if(fd == -1){
		printf("Failed to open %s, err: %d\n", devicePath, errno);
		return fd;
	}
       	ioctl(fd, BLKGETSIZE, numBlocks);
       	ioctl(fd, BLKSSZGET, sectorSizeBytes);
	close(fd);
	return 0;
}

//Returns the number of sectors required to hold numBytes
int getNumSectors(int numBytes, int sectorSizeBytes)
{
	if(0 == numBytes % sectorSizeBytes)
		return numBytes / sectorSizeBytes;
	return numBytes / sectorSizeBytes + 1;
}

int main(int argc, char **argv)
{
	if(argc!=4){
		printf("Requires 3 arguments: <devicePath> <minWriteSize> <maxWriteSize>\n");
		return 1;
	}

	//Parse args
	int minWriteSizeBytes, maxWriteSizeBytes;
	char *devicePath = argv[1];
	sscanf(argv[2], "%d", &(minWriteSizeBytes));
        sscanf(argv[3], "%d", &(maxWriteSizeBytes));
	
	//Open the device and retrieve logical sector size and number of sectors
	long sectorSizeBytes, numDeviceSectors;
	if(0 != setDeviceSizeInfo(devicePath, &numDeviceSectors, &sectorSizeBytes))
		return 1;
	//printf("Device %s has %ld sectors of logical size %ld bytes\n", devicePath, numDeviceSectors, sectorSizeBytes);

	//Retrieve the systems memory page size in order to better align the IO buffer for efficient direct IO
	long pageSize = sysconf(_SC_PAGESIZE);
        if(pageSize % sectorSizeBytes !=0){
                printf("Page size of %ld is not a multiple of sector size of %ld, memory buffer will be aligned to sector size, which may result in sub-optimal IO sizes\n", pageSize, sectorSizeBytes);
                pageSize = sectorSizeBytes;
        }

	//Calculate the minimum and maximum number of sectors to write per request
	int minWriteSizeSectors = getNumSectors(minWriteSizeBytes, sectorSizeBytes);
       	int maxWriteSizeSectors = getNumSectors(maxWriteSizeBytes, sectorSizeBytes);
	int maxWriteSizeDiffSectors = maxWriteSizeSectors - minWriteSizeSectors;

	//Initialize random values/functions
	if(0 != setRandMaxBits()){
		printf("Failed to initialize random values/functions\n");
		return 1;
	}
        seedRand();

	//Open the device for writing and DMA
	int fd = open(devicePath, O_WRONLY | O_DIRECT);
       	if(fd == -1){
		printf("Failed to open %s, err: %d\n", devicePath, errno);
		return 1;
	}

	//Create a memory aligned buffer to hold the op payload
	char *writeBuffer = memalign(pageSize, maxWriteSizeSectors * sectorSizeBytes);

	//Do the ops
	while(1==1){
		long numSectorsToWrite = getUnbiasedRandomLong(maxWriteSizeDiffSectors) + minWriteSizeSectors;
		long bytesToWrite = numSectorsToWrite * sectorSizeBytes;
		long sectorToSeekTo = getUnbiasedRandomLong(numDeviceSectors - numSectorsToWrite);
		long byteToSeekTo = sectorToSeekTo * sectorSizeBytes;
		
		//Seek to the random sector-aligned offset
		if(byteToSeekTo != lseek(fd, byteToSeekTo, SEEK_SET)){
			printf("Failed to seek to offset %ld, errno: %d\n", byteToSeekTo, errno);
			return 1;
		}

		//Write from the device
		if(bytesToWrite != write(fd, writeBuffer, bytesToWrite)){
			printf("Failed to write %ld bytes, errno: %d\n", bytesToWrite, errno);
			return 1;
		}
		//printf("Write %ld bytes from offset %ld\n", bytesToWrite, byteToSeekTo);
	}
	return 0;
}
