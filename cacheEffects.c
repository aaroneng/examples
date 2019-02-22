#include <malloc.h>
#include <sys/time.h>
#include <stdio.h>

int main(int argc, char ** argv)
{
	int arrSize = 256 * 1024 * 1024;
	int *arr = memalign(64, sizeof(int) * arrSize);
	
	struct timeval startTime, endTime;
	int x, y, z;
	
	gettimeofday(&startTime, NULL);
	for(x=0; x<10; x++)
		for(y=0; y<arrSize; y++)
			arr[y]++;
	gettimeofday(&endTime, NULL);
        unsigned long sequentialElapsedTime = (endTime.tv_sec - startTime.tv_sec) * 1000000l - startTime.tv_usec + endTime.tv_usec;

	gettimeofday(&startTime, NULL);
	for(x=0; x<10; x++)
		for(y=0; y<16; y++)
			for(z=y; z<arrSize; z+=16)
				arr[z]++;
	gettimeofday(&endTime, NULL);
	unsigned long lineSkippingTime = (endTime.tv_sec - startTime.tv_sec) * 1000000l - startTime.tv_usec + endTime.tv_usec;

	printf("Sequential: %lu us, Line skipping: %lu us\n", sequentialElapsedTime, lineSkippingTime);

	return 0;
}
