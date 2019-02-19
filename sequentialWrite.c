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
	if(argc!=3){
		printf("Requires 2 arguments: <devicePath> <writeSizeBytes>\n");
		return 1;
	}

	//Parse args
	int writeSizeBytes;
	char *devicePath = argv[1];
	sscanf(argv[2], "%d", &(writeSizeBytes));
	
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
	int writeSizeSectors = getNumSectors(writeSizeBytes, sectorSizeBytes);
	if(writeSizeSectors > numDeviceSectors) {
		printf("Can not issue writes for %lu sectors because the device only has %lu sectors\n", writeSizeSectors, numDeviceSectors);
		return 1;
	}

	//Open the device for writing and DMA
	int fd = open(devicePath, O_WRONLY | O_DIRECT);
       	if(fd == -1){
		printf("Failed to open %s, err: %d\n", devicePath, errno);
		return 1;
	}

	//Create a memory aligned buffer to hold the op payload
	char *writeBuffer = memalign(pageSize, writeSizeSectors * sectorSizeBytes);

	long numSectorsWrite=0;

	//Do the ops
	while(1==1){
		
		//
		if(numSectorsWrite + writeSizeSectors > numDeviceSectors) {
			if(0 != lseek(fd, 0, SEEK_SET)) {
				printf("Failed to seek to offset 0, errno: %d\n", errno);
				return 1;
			}
			numSectorsWrite=0;
		}

		//Write from the device
		if(writeSizeBytes != write(fd, writeBuffer, writeSizeBytes)){
			printf("Failed to write %ld bytes, errno: %d\n", writeSizeBytes, errno);
			return 1;
		}
		numSectorsWrite += writeSizeSectors;
		//printf("Write %ld bytes from offset %ld\n", writeSizeBytes, byteToSeekTo);
	}
	return 0;
}
