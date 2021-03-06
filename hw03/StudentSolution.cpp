// -*- C++ -*-

/*
 * StudentSolution.cpp
 *
 * HW 03
 *
 * THIS is the only file we will look at when we grade your homework.
 * All other changes you made to any other files will be discarded.
 */

#include <cstdlib>
#include <cilk/cilk.h>
#include <iostream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
using namespace std;

#define MIN(x, y) ((x < y) ? x : y)
#define MAX(x, y) ((x > y) ? x : y)

// YOUR sequential partition algorithm
int partitionSequential(vector<int> &sequence, int beginIndex, int endIndex) {
	int pivdex = beginIndex + rand() % (endIndex - beginIndex);
	int pivot = sequence[pivdex];
	sequence[pivdex] = sequence[endIndex]; //swap the pivot with the last element
	sequence[endIndex] = pivot;
	pivdex = beginIndex; //set the pivot index at the start
	for (int i = beginIndex; i < endIndex; ++i) {
		int pivnum = sequence[pivdex];
		if (sequence[i] < pivot) {
			sequence[pivdex] = sequence[i]; //swap the element with the one at the pivdex
			sequence[i] = pivnum;
			++pivdex; //increment the pivdex
		}
	}
	if (sequence[pivdex] > pivot) {
		sequence[endIndex] = sequence[pivdex];
		sequence[pivdex] = pivot;
	}
	return pivdex;
}

// YOUR sequential quicksort algorithm
void quicksortSequential(vector<int> &sequence, int beginIndex, int endIndex) {
	if (endIndex - beginIndex == 1) {
		int first = sequence[beginIndex];
		if (first > sequence[endIndex]) {
			sequence[beginIndex] = sequence[endIndex];
			sequence[endIndex] = first;
		}
	}
	else if (endIndex - beginIndex > 1)
	{
		int pivot = partitionSequential(sequence,beginIndex,endIndex);
		quicksortSequential(sequence, beginIndex, pivot);
		quicksortSequential(sequence, pivot + 1, endIndex);
	}
}

// YOUR parallel prefix algorithm
/* Depending on how you implement this, you may or may not use the "copy" vector
 * If you do not use the copy, just don't use it -- don't change the signature!
 */
void parallelPrefix(vector<int> &sequence, vector<int> &copy) {
	int size = sequence.size();
	if (size == 3) {
		sequence[2] = sequence[1] + sequence[2];
	}
	else if (size > 3) {
		vector<int> smaller(size/2 + 1);
		smaller[0] = 0;
		for (int i = 1; i < size; i+=2) {
			int j = (i + 1)/2;
			if (i == size - 1) {
				smaller[j] = sequence[i];
			}
			else {
				smaller[j] = sequence[i] + sequence[i+1];
			}
		}
		parallelPrefix(smaller,smaller);
		for (int k = 0; k < size; ++k) {
			if (k%2 == 0) {
				sequence[k] = smaller[k/2];
			}
			else {
				sequence[k] = sequence[k] + smaller[(k-1)/2];
			}
		}
	}
}

//Divide and conquer search algorithm that returns the max index at which an int exists in an array
int binSearch(vector<int> &sequence, int pivot, int beginIndex, int endIndex) {
	//printf("got in here with %d to %d\n",beginIndex,endIndex);
	int size = (endIndex - beginIndex);
	if (endIndex < beginIndex) {
		return -1;
	}
	if (endIndex == beginIndex) {
		if(sequence[beginIndex] == pivot) {
			return beginIndex;
		}
		else return -1;
	}
	else {
		int left = cilk_spawn(binSearch(sequence,pivot, beginIndex, beginIndex + size/2));
		int right = binSearch(sequence,pivot,beginIndex + size/2 + 1,endIndex);
		cilk_sync;
		return MAX(left,right);
	}
}

// YOUR parallel partition algorithm
int partitionParallel(vector<int> &sequence, int beginIndex, int endIndex, vector<int> &copy) {
	int size = 1 + endIndex - beginIndex;
	vector<int> left1(size + 1); left1[0] = 0;
	vector<int> right1(size + 1); right1[0] = 0;
	int pivdex = beginIndex + rand() % (size);
	int pivot = sequence[pivdex];
	cilk_for (int i = beginIndex; i <= endIndex; ++i) {
		if (sequence[i] <= pivot) {
			left1[i+1-beginIndex] = 1;
			right1[i+1-beginIndex] = 0;
		}
		else {
			left1[i+1-beginIndex] = 0;
			right1[i+1-beginIndex] = 1;
		}
	}
	parallelPrefix(left1,left1);
	parallelPrefix(right1,right1);
	cilk_for (int j = beginIndex; j <= endIndex; ++j) {
		if (left1[j-beginIndex] != left1[j+1-beginIndex]) {
			sequence[beginIndex + left1[j-beginIndex]] = copy[j];//ok to use left1[j]??
		}
		else {
			sequence[left1[left1.size()-1] + beginIndex + right1[j-beginIndex]] = copy[j];
		}
	}
	int numdex = binSearch(sequence,pivot,beginIndex,endIndex); //find pivot in array
	pivdex = left1[left1.size()-1] + beginIndex-1;
	sequence[numdex] = sequence[pivdex]; //swap pivot to pivot index
	sequence[pivdex] = pivot;
	return pivdex;
}

// YOUR parallel quicksort algorithm
void quicksortParallel(vector<int> &sequence, int beginIndex, int endIndex, vector<int> &copy) {
	cilk_for(int i = beginIndex; i <= endIndex; ++i) {
		copy[i] = sequence[i];
	}
	if (endIndex - beginIndex == 1) {
		int first = sequence[beginIndex];
		if (first > sequence[endIndex]) {
			sequence[beginIndex] = copy[endIndex];
			sequence[endIndex] = first;
		}
	}
	else if (endIndex - beginIndex == 2) {
		quicksortSequential(sequence,beginIndex,endIndex);
	}
	else if (endIndex - beginIndex > 2)
	{
		int pivdex = partitionParallel(sequence,beginIndex,endIndex,copy);
		cilk_spawn(quicksortParallel(sequence, beginIndex, pivdex - 1,copy));
		quicksortParallel(sequence, pivdex + 1, endIndex,copy);
		cilk_sync;
	}
}



