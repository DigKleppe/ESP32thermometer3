/*
 * stdev.cpp
 *
 *  Created on: Dec 23, 2024
 *      Author: dig
 */


#include "math.h"

float calculateStandardDeviation(int N, float data[])
{
    // variable to store sum of the given data
    float sum = 0;
    for (int i = 0; i < N; i++) {
        sum += data[i];
    }
 
    // calculating mean
    float mean = sum / N;
 
    // temporary variable to store the summation of square
    // of difference between individual data items and mean
    float values = 0;
 
    for (int i = 0; i < N; i++) {
        values += pow(data[i] - mean, 2);
    }
 
    // variance is the square of standard deviation
    float variance = values / N;
 
    // calculating standard deviation by finding square root
    // of variance
    float standardDeviation = sqrt(variance);
 	return standardDeviation;
    // printing standard deviation
    
}