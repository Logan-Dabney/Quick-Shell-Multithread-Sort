//==============================================================================================
// The purpose of this program is to gain a better understand of threads using 2 sorting 
// methods. This programs input allows for the following: (optional parameters are denoted [])
//
// ./project2 size threshold [seed] [multithread] [pieces] [maxthreads]
//
// Using the size it will create and randomize a array of intgers of that size. Afterwards, it 
// will quick sort the array but using the threshold it will switch to shell sort when the 
// partitions reach that size. If a seed is designated, it will randomize the array using that
// seed. If -1 is chosen, it will use clock() to randomize it. If multithreading is chosen,
// it will partition the array into 10 intial segments then run each segment on a separate
// thread, but it will only run 4 at a time. If pieces is designated, it will change the 10 
// partitions the into as many as is entered by the user. Max threads is similar but it will
// change the max amount of thread running at one time from 4 to the user input.
//
// Author: 	Logan Dabney, University of Toledo
// Date:	Feb 23, 2021
// Copyright:	Copyright 2020 by Logan Dabney. All rights reserved.
//==============================================================================================

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

// this struct is each end of a partition
typedef struct {
	int START;
	int END;
}sizeOfPart;

int* SORTARRAY;										// array to be sorted
int THRESHOLD;										// switch to shell sort size
double CREATETIME, INITIALTIME, RANDTIME, PARTTIME;	// create array time, initialize array time, array randomize time, paritioning time
int PIECES = 10;									// amount of initial partitions	
sizeOfPart* PARTITIONVALS;							// holds the initial partition made from multithreading

// checks if a number is positive and is a number
bool isNumeric(char* str)
{
	for (int i = 0; i < strlen(str); i++)
		if (str[i] < '0' || str[i] > '9') return false;
	return true;
}

// checks if multithreading will be used
bool isMultithread(char* str)
{
	if (strlen(str) == 1) // must only by one char
	{
		if (str[0] == 'y' || str[0] == 'Y') return true;		// if y or Y do multithreading
		else if (str[0] == 'n' || str[0] == 'N') return false;	// if n or N don't
	}

	printf("Parameter for MULTITHREADING is not 'n','N','y','Y'!\n");	// otherwise print error message
	exit(0);
}

// checks if the array is sorted
bool isSorted(int size)
{
	for (int i = 0; i < size - 1; i++)
	{
		if (SORTARRAY[i] > SORTARRAY[i + 1])
			return false;
	}
	return true;
}

// creates, inttializes, and randomizes the array used in the rest of the application
void createArray(int size, int seed)
{
	double start, end;	// will be used to keep time

	start = clock();
	SORTARRAY = (int*)malloc(size * sizeof(int));		// create the array
	end = clock();
	CREATETIME = (end - start) / (double)CLOCKS_PER_SEC;// set time it took

	if (SORTARRAY == NULL)
	{
		printf("Memory not allocated.\n");
		exit(0);
	}

	// intialize the array
	start = clock();
	for (int i = 0; i < size; i++)						// set values from 0 to size - 1
		SORTARRAY[i] = i;
	end = clock();
	INITIALTIME = (end - start) / (double)CLOCKS_PER_SEC;// set time it took

	// radomize the array
	start = clock();
	srand(seed);
	for (int i = 0; i < size; i++)						// randomize the array by finding a value between 0 and size - 1
	{													// take that place of the array and swap it with the current i 
		int first = (rand() % (size - 1));				// of array
		int temp = SORTARRAY[first];
		SORTARRAY[first] = SORTARRAY[i];
		SORTARRAY[i] = temp;
	}
	end = clock();
	RANDTIME = (end - start) / (double)CLOCKS_PER_SEC;	// set time it took
}

// This partition will used my quickSort to sort a pivot value and split the array into 2 segments
// called partitions. While sorting the piviot it will put value on the correct sides of each segment
int partition(int start, int end)
{
	int i = start;					// create varible
	int j = end + 1;
	int pivot = SORTARRAY[start];	// use first value as pivot value
	do
	{
		do i++; while (SORTARRAY[i] < pivot);	// add one to i while the value at SORTARRAY[i] is less than the pivot value
		do j--; while (SORTARRAY[j] > pivot);	// subtract one to j while the value at SORTARRAY[j] is greater than pivot value
		if (i < j)	// if i and j didn't pass each other
		{
			int temp = SORTARRAY[i];	// swap i and j's values
			SORTARRAY[i] = SORTARRAY[j];
			SORTARRAY[j] = temp;
		}
		else // otherwise break from loop
			break;
	} while (1);

	SORTARRAY[start] = SORTARRAY[j]; // swap the pivot value with j's
	SORTARRAY[j] = pivot;
	return j;
}

// This is the sort used one a partition reachs the threshold value
// it is a variation of insertion sort
void shellSort(int start, int end, int size)
{
	int interval = 1;				// This is used to create the hibbard sequence. (1,3,7,15,...)

	while (interval <= size)		// The largest interval will be the largest power of 2 - 1 that fits in the size
		interval *= 2;
	interval = (interval / 2) - 1;

	do
	{
		for (int i = start; i < (end + 1 - interval); i++)		// + 1 on end because it would stop one before the end value other wise and wont sort it
		{
			for (int j = i; j >= start; j -= interval)			// tooth-to-tooth is interval
			{
				if (SORTARRAY[j] <= SORTARRAY[j + interval])	// if the next interval up is greater than j in SORTARRAY just break
					break;
				else                                            // otherwise swap them
				{
					int temp = SORTARRAY[j];
					SORTARRAY[j] = SORTARRAY[j + interval];
					SORTARRAY[j + interval] = temp;
				}
			}
		}
		interval = interval >> 1;								// decriment interval 
	} while (interval > 0);
}

// used to make the designated amount of initial partitions when selected to use multithreading
void multithreadPartition(int start, int end)
{
	double clockStart, clockEnd;
	clockStart = clock();
	int index = 0;							// for tracking the next largest interval to partion

	for (int i = 0; i < PIECES - 1; i++)	// parition one less time than pieces
	{
		int q = partition(start, end);

		PARTITIONVALS[index].START = start;			// set the index (which used to be the largest) to left half 
		PARTITIONVALS[index].END = q - 1;
		PARTITIONVALS[i + 1].START = q + 1;			// set i + 1 to the right half
		PARTITIONVALS[i + 1].END = end;

		start = PARTITIONVALS[0].START;			// find new largest section of array from given partition values
		end = PARTITIONVALS[0].END;
		index = 0;
		for (int j = 1; j <= i + 1; j++)		// j will start at one because the first set of start and end is 0
		{
			if ((end - start) < (PARTITIONVALS[j].END - PARTITIONVALS[j].START))	// if old invterval is smaller than new, replace
			{
				start = PARTITIONVALS[j].START;
				end = PARTITIONVALS[j].END;
				index = j;
			}
		}
	}

	int j;
	for (int i = 1; i < PIECES; i++)		// sort greatest to least (insertion sort)
	{
		sizeOfPart temp = PARTITIONVALS[i];
		j = i - 1;

		while (j >= 0 && (PARTITIONVALS[j].END - PARTITIONVALS[j].START) < (temp.END - temp.START)) // find position where temp goes
		{
			PARTITIONVALS[j + 1] = PARTITIONVALS[j];
			j--;
		}
		PARTITIONVALS[j + 1] = temp;	// put temp in it's position
	}

	clockEnd = clock();
	PARTTIME = (clockEnd - clockStart) / (double)CLOCKS_PER_SEC;	// setting he time it took to partition
}

// this is used to do the recursive part of the quick sort algorithm.
void quickSort(int start, int end)
{
	if (start < end)					// if array does not contain 1 element
	{
		int size = (end - start) + 1;

		if (size > THRESHOLD)				// as long as size of parition is greater than threshold quickSort
		{
			int q = partition(start, end);		// partition/ sort array and return split value
			quickSort(start, q - 1);			// quick sort the first half of sorted array
			quickSort(q + 1, end);				// quick sort the second half of the array
		}
		else
			shellSort(start, end, size);	// otherwise do shellsort
	}
}

// this function is used to connect the pthreads to quicksort by converting the
// parameters passed to runner() to a sizeOfPart to be used in quickSort
void* runner(void* param)
{
	sizeOfPart* partitionVal = (sizeOfPart*)param;		// convert the partition value passed to runner to a sizeOfPart
	quickSort(partitionVal->START, partitionVal->END);	// quick sort the partition
}

int main(int argc, char* argv[])
{
	if (argc >= 3 && isNumeric(argv[1]) && isNumeric(argv[2]))
	{
		double totalCpuStart, totalCpuEnd, sortCpuStart, sortCpuEnd;				// used to keep track of clock() time
		struct timeval totalWallStart, totalWallEnd, sortWallStart, sortWallEnd;	// used to keep track of gettimeofday() time
		int size = atoi(argv[1]);													// size of array
		int seed = 1;																// srand() is seeded with 1 if nothing is entered
		int maxThreads = 4;															// max threads intially 4
		bool multithread = false;													// starts of false
		THRESHOLD = atoi(argv[2]);													// threshold is the second entered argument

		if (argc >= 4)															// if seed was specified
		{
			seed = (atoi(argv[3]) == -1) ? clock() : atoi(argv[3]);				// if -1 seed with clock other wise put what was entered
			if (argc >= 5)														// if multithreading was specified (only if seed was)
			{
				multithread = isMultithread(argv[4]);							// checks if the user wants to multithread

				if (argc >= 6 && isNumeric(argv[5])) {							// if pieces was specified (only if multithread was)
					PIECES = atoi(argv[5]);

					if (argc == 7 && isNumeric(argv[6])) {						// if maxthreads was specified (only if pieces was)
						maxThreads = atoi(argv[6]);

						if (PIECES < maxThreads)								// maxthreads can't be larger than pieces
						{
							printf("PIECES can not be smaller than MAX THREADS!\n");
							exit(0);
						}
					}
				}
			}
		}

		totalCpuStart = clock();				// start the total cpu time
		gettimeofday(&totalWallStart, NULL);	// and wall time
		createArray(size, seed);

		if (multithread)
		{
			PARTITIONVALS = (sizeOfPart*)malloc(PIECES * sizeof(sizeOfPart));		// for storing partition values
			pthread_t* threads = (pthread_t*)malloc(PIECES * sizeof(pthread_t));	// for all threads

			multithreadPartition(0, size - 1);	// get initial partitions

			sortCpuStart = clock();					// start sorting cpu time
			gettimeofday(&sortWallStart, NULL);		// and sorting wall time

			for (int i = 0; i < maxThreads; i++) {	// run max threads allowed
				pthread_create(&threads[i], NULL, runner, (void*)&PARTITIONVALS[i]);
				printf("(%9d, %9d, %9d)\n", PARTITIONVALS[i].START, PARTITIONVALS[i].END, (PARTITIONVALS[i].END - PARTITIONVALS[i].START) + 1);
			}

			int threadCount = maxThreads;	// counting all threads that have been created, right now its maxthreads
			while (threadCount < PIECES)	// while the threadCount is less than the amount of paritions
			{
				int error;	// checking the error code
				for (int i = maxThreads - 1; i >= 0; i--)	// check all the threads currently running
				{
					error = pthread_tryjoin_np(threads[i], NULL);	// try to join a thread back to the parent
					if (error == 0 && threadCount < PIECES)			// if it works and thread count is less than amount of partitions
					{
						pthread_create(&threads[i], NULL, runner, (void*)&PARTITIONVALS[threadCount]);	// create a new thread
						printf("(%9d, %9d, %9d)\n", PARTITIONVALS[threadCount].START, PARTITIONVALS[threadCount].END, (PARTITIONVALS[threadCount].END - PARTITIONVALS[threadCount].START) + 1);
						threadCount++;																	// increment the thredcount
					}
				}
			}

			for (int i = 0; i < PIECES; i++) pthread_join(threads[i], NULL); // join all threads together

			sortCpuEnd = clock();				// end sorting cpu time
			gettimeofday(&sortWallEnd, NULL);	// and sort wall time

			free(threads);						// free memory for threads
		}
		else {	// no multithreading
			sortCpuStart = clock();				// start sort cpu time		
			gettimeofday(&sortWallStart, NULL);	// and sort wall time

			quickSort(0, size - 1);				// sort array

			sortCpuEnd = clock();				// end sort cpu time
			gettimeofday(&sortWallEnd, NULL);	// and sort wall time
		}
		totalCpuEnd = clock();					// end total cpu time
		gettimeofday(&totalWallEnd, NULL);		// and total wall time

		if (!isSorted(size))					// check if array is sorted
		{
			printf("NOT SORTED\n");
			exit(0);
		}

		printf("     SIZE    THRESHOLD SD PC T Create   INIT  Shuffle   Part  SrtWall Srt CPU ALLWall ALL CPU\n");	// print out values calculated values
		printf("   --------- --------- -- -- - ------ ------- ------- ------- ------- ------- ------- -------\n");
		printf("F: %9d %9d %2d %2d %d ", size, THRESHOLD, seed, PIECES, maxThreads);
		printf("%6.3f %7.3f %7.3f %7.3f ", CREATETIME, INITIALTIME, RANDTIME, PARTTIME);
		printf("%7.3f %7.3f %7.3f %7.3f\n",
			((double)(sortWallEnd.tv_sec - sortWallStart.tv_sec) + ((sortWallEnd.tv_usec - sortWallStart.tv_usec) / 1000000.0)),	// sorting wall time
			(sortCpuEnd - sortCpuStart) / (double)CLOCKS_PER_SEC,																	// sorting cpu time
			((double)(totalWallEnd.tv_sec - totalWallStart.tv_sec) + ((totalWallEnd.tv_usec - totalWallStart.tv_usec) / 1000000.0)),// total wall time
			(totalCpuEnd - totalCpuStart) / (double)CLOCKS_PER_SEC);																// total cpu time

		free(PARTITIONVALS);
		free(SORTARRAY);
	}
	else
		printf("Incorrect parameters");
}
