
// (todo) author credit (but who wrote this ? Ewan Hemingway ?)

#pragma once

#include <math.h>
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795029
#endif // M_PI

enum FilterMode {
	LOWPASS,
	HIGHPASS,
	BANDPASS,
	NUM_TYPES
};

class StateVariableFilter2ndOrder {
public:

	StateVariableFilter2ndOrder() {

	}

	void setParameters(float fc, float q) {
		// avoid repeated evaluations of tanh if not needed
		if (fc != fcCached) {
			g = (float)tan(M_PI * fc);
			fcCached = fc;
		}
		R = 1.0f / q;
	}

	void process(float input) {
		hp = (input - (R + g) * mem1 - mem2) / (1.0f + g * (R + g));
		bp = g * hp + mem1;
		lp = g * bp + mem2;
		mem1 = g * hp + bp;
		mem2 = g * bp + lp;
	}

	float output(FilterMode mode) {
		switch (mode) {
			case LOWPASS: return lp;
			case HIGHPASS: return hp;
			case BANDPASS: return bp * 2.f * R;
			default: return 0.0;
		}
	}

   void reset(void) {
      g = 0.f;
      R = 0.5f;
      fcCached = -1.f;
      hp = 0.0f;
      bp = 0.0f;
      lp = 0.0f;
      mem1 = 0.0f;
      mem2 = 0.0f;
   }

private:
	float g = 0.f;
	float R = 0.5f;

	float fcCached = -1.f;

	float hp = 0.0f, bp = 0.0f, lp = 0.0f, mem1 = 0.0f, mem2 = 0.0f;
};

class StateVariableFilter4thOrder {

public:
	StateVariableFilter4thOrder() {

	}

	void setParameters(float fc, float q) {
		stage1.setParameters(fc, 1.0);
		stage2.setParameters(fc, q);
	}

	float process(float input, FilterMode mode) {
		stage1.process(input);
		float s1 = stage1.output(mode);

		stage2.process(s1);
		float s2 = stage2.output(mode);

		return s2;
	}

private:
	StateVariableFilter2ndOrder stage1, stage2;
}; 
