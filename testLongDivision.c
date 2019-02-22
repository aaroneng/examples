#include <stdlib.h>
#include <malloc.h>
#include <sys/time.h>
#include <stdio.h>

static void longDivisionBase(long iterations, long* buffer, int bufSize)
{
	long v, v1, v2;
	int x=0;
	while(0 < iterations--)
	{
// Despite the C code and resultant assembly being shorter and simpler,
// the commented out code takes longer to run on my test system.
/*
		v1=buffer[x++];
		if(x==bufSize) 
			x=0;
		v2=buffer[x];
*/
		v1=buffer[x++];
		if(x==bufSize)
			x=0;
		v2=buffer[x++];
		if(x==bufSize)
			x=0;
		v = v1 / v2;
	}
}
static void longDivisionV1(long iterations, long* buffer, int bufSize)
{
        long v, v1, v2;
        int x=0;
        while(0 < iterations--)
        {
                v1=buffer[x++];
                if(x==bufSize)
                        x=0;
                v2=buffer[x++];
                if(x==bufSize)
                        x=0;
                v = v1 / v2 / v2;
        }
}
static void longDivisionV2(long iterations, long* buffer, int bufSize)
{
        long v, v1, v2;
        int x=0;
        while(0 < iterations--)
        {
                v1=buffer[x++];
                if(x==bufSize)
                        x=0;
                v2=buffer[x++];
                if(x==bufSize)
                        x=0;
                v = v1 / v2;
		v = v1 / v2;
        }
}
long* getRandomLongs(int numLongs)
{
	int sizeInBytes = numLongs * sizeof(long);
	char *ret = malloc(sizeInBytes);
	if(!ret){
		fprintf(stderr, "malloc failed for %d bytes\n", sizeInBytes);
	} else {
		while(sizeInBytes)
			ret[--sizeInBytes] = (char)rand();
	}
	return (long*)ret;
}
int setupRandomize()
{
        //This program requires rand() to return at least 8 consecutive, least significant, random bits
	int i = RAND_MAX;
       	int maxRandBits = 0;
	while(1){
                if(0 == (int)(i & 1))
                        break;
                maxRandBits++;
                i = i >> 1;
        }
        if(maxRandBits<8){
		fprintf(stderr, "Insufficient bits in random library\n");
                return 1;
	}
	
	//Seed random based on current time
	struct timeval now;
        gettimeofday(&now, NULL);
        srand(now.tv_sec * 1000000 + now.tv_usec);

        return 0;
}

static long timeFunction(void (*function)(long, long*, int), long iterations, void *buffer, int setSize){
	struct timeval startTime, endTime;
	gettimeofday(&startTime, NULL);
	(*function)(iterations, buffer, setSize);
	gettimeofday(&endTime, NULL);
	return (endTime.tv_sec - startTime.tv_sec) * 1000000l - startTime.tv_usec + endTime.tv_usec;
}

int main(int argc, char ** argv)
{
	static int RANDOM_SET_SIZE = 10000;
	static long NUM_OPS = 1 * 1000 * 1000 * 1000;
	//Get random values for testing
	if(0 != setupRandomize())
	{
		fprintf(stderr, "Failed to setup randomization\n");
		return 1;
	}
	
	//Main test loop
	while(1)
	{
		long *buffer = getRandomLongs(RANDOM_SET_SIZE);
		long baseTime = timeFunction(&longDivisionBase, NUM_OPS, buffer, RANDOM_SET_SIZE);
		long v1Time = timeFunction(&longDivisionV1, NUM_OPS, buffer, RANDOM_SET_SIZE);
		long v2Time = timeFunction(&longDivisionV2, NUM_OPS, buffer, RANDOM_SET_SIZE);
		free(buffer);
		double averageV1opTimeNanoSeconds = (double)(v1Time-baseTime) / (double)NUM_OPS * 1000.d;
		double averageV2opTimeNanoSeconds = (double)(v2Time-baseTime) / (double)NUM_OPS * 1000.d;
		printf(
			"avg v1opTime: %f ns, avg v2opTime: %f ns, base time: %ld us, v1 time: %ld us, v2 time: %ld us, num ops: %ld\n", 
			averageV1opTimeNanoSeconds, 
			averageV2opTimeNanoSeconds, 
			baseTime, 
			v1Time, 
			v2Time, 
			NUM_OPS
			);
	}

	return 0;
}
