#pragma once

/* #include "util/math.hpp" */


/** The normalized sinc function. https://en.wikipedia.org/wiki/Sinc_function */
inline float sinc(float x) {
	if (x == 0.f)
		return 1.f;
	x *= float(M_PI);
	return sinf(x) / x;
}

inline float quadraticBipolar(float x) {
	float x2 = x*x;
	return (x >= 0.f) ? x2 : -x2;
}

inline float cubic(float x) {
	return x*x*x;
}

inline float quarticBipolar(float x) {
	float y = x*x*x*x;
	return (x >= 0.f) ? y : -y;
}

inline float quintic(float x) {
	// optimal with --fast-math
	return x*x*x*x*x;
}

inline float sqrtBipolar(float x) {
	return (x >= 0.f) ? sqrtf(x) : -sqrtf(-x);
}

/** This is pretty much a scaled sinh */
inline float exponentialBipolar(float b, float x) {
	const float a = b - 1.f / b;
	return (powf(b, x) - powf(b, -x)) / a;
}

inline float gainToDb(float gain) {
	return log10f(gain) * 20.f;
}

inline float dbToGain(float db) {
	return powf(10.f, db / 20.f);
}



/** Returns the minimum of `a` and `b` */
inline int min(int a, int b) {
	return (a < b) ? a : b;
}

/** Returns the maximum of `a` and `b` */
inline int max(int a, int b) {
	return (a > b) ? a : b;
}

/** Limits `x` between `a` and `b`
Assumes a <= b
*/
inline int clamp(int x, int a, int b) {
	return min(max(x, a), b);
}

/** Limits `x` between `a` and `b`
If a > b, switches the two values
*/
inline int clamp2(int x, int a, int b) {
	return clamp(x, min(a, b), max(a, b));
}

/** Returns the minimum of `a` and `b` */
inline float min(float a, float b) {
	return (a < b) ? a : b;
}

/** Returns the maximum of `a` and `b` */
inline float max(float a, float b) {
	return (a > b) ? a : b;
}

/** Limits `x` between `a` and `b`
Assumes a <= b
*/
inline float clamp(float x, float a, float b) {
	return min(max(x, a), b);
}

/** Limits `x` between `a` and `b`
If a > b, switches the two values
*/
inline float clamp2(float x, float a, float b) {
	return clamp(x, min(a, b), max(a, b));
}
