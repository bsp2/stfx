#pragma once

#include <stdint.h>

////////////////////
// Random number generator
// random.cpp
////////////////////

/** Seeds the RNG with the current time */
void randomInit();
/** Returns a uniform random uint32_t from 0 to UINT32_MAX */
uint32_t randomu32();
uint64_t randomu64();
/** Returns a uniform random float in the interval [0.0, 1.0) */
float randomUniform();
/** Returns a normal random number with mean 0 and standard deviation 1 */
float randomNormal();
