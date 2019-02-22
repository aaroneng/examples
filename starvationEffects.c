#include <malloc.h>
#include <sys/time.h>
#include <stdio.h>
#include <time.h>

void handleEvent(long l){
	while(l--){}
}

long getNumPendingEvents(long eventRatePerSecond, unsigned long elapsedTimeus, long processedEvents) {
	return elapsedTimeus * eventRatePerSecond / 1000000L - processedEvents + 1;
}

long getTimeOfEventInNanoSeconds(long eventNum, long eventRatePerSecond) {
	return 1000000000L * eventNum / eventRatePerSecond;	
}

int main(int argc, char ** argv)
{
	long eventArrivalRatePerSecond;
	long eventCost;

	if (
		3 != argc ||
		1 != sscanf(argv[1], "%lu", &eventArrivalRatePerSecond) ||
		0 >= eventArrivalRatePerSecond ||
		1 != sscanf(argv[2], "%lu", &eventCost) ||
		0 >= eventCost
	) {
		fprintf(stderr, "Usage: %s <eventArrivalRatePerSecond> <eventCost>\nargs must be > 0", argv[0]);
		return 1;
	}

	long processedEvents=0, processedEventsAtLastStatus = 0;
	long  nanoSecondsPerEventArrival = 1000000000L / eventArrivalRatePerSecond;
	struct timeval now;
	struct timespec sleepTime;
	sleepTime.tv_sec=0;
	gettimeofday(&now, NULL);
	unsigned long sleepStartTime, nowus, usSinceLastStatus, loopStartTimeus;
	unsigned long usSleptSinceLastStatus=0;
	unsigned long programStartTimeus = now.tv_sec * 1000000L + now.tv_usec;
	unsigned long lastStatusTimeus = programStartTimeus;

	while(1) {
		gettimeofday(&now, NULL);
		loopStartTimeus = now.tv_sec * 1000000L + now.tv_usec - programStartTimeus;
		long numPendingEvents = getNumPendingEvents(eventArrivalRatePerSecond, loopStartTimeus, processedEvents);
		if(numPendingEvents==0) {
			sleepTime.tv_nsec = getTimeOfEventInNanoSeconds(processedEvents, eventArrivalRatePerSecond) - loopStartTimeus* 1000L;
			if(sleepTime.tv_nsec > 999999999L)
				sleepTime.tv_nsec = 999999999L;
			gettimeofday(&now, NULL);
			sleepStartTime = now.tv_sec * 1000000L + now.tv_usec;
			nanosleep(&sleepTime, NULL);
			gettimeofday(&now, NULL);
			usSleptSinceLastStatus += now.tv_sec * 1000000L + now.tv_usec - sleepStartTime;
			continue;
		}
		while(numPendingEvents-->0) {
			handleEvent(eventCost);
			processedEvents++;
			gettimeofday(&now, NULL);
			nowus = now.tv_sec * 1000000L + now.tv_usec;
			usSinceLastStatus = nowus - lastStatusTimeus;
			if(usSinceLastStatus>= 1000000L) {
				printf("%ld events processed in %.2f seconds, backlog: %ld events, time runnable: %.2f%%\n", 
					processedEvents - processedEventsAtLastStatus, 
					(float)usSinceLastStatus / 1000000.F, 
					numPendingEvents,
					100.F * (1.F - (float)usSleptSinceLastStatus / (float)usSinceLastStatus)
				);
				usSleptSinceLastStatus = 0;
				processedEventsAtLastStatus = processedEvents;
				lastStatusTimeus = nowus;
				break;
			}
		}
	}

	return 0;
}
