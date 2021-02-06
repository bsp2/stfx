#pragma once

#include "LookupTable.h"
#include "SinOscillator.h"
#include "BiquadFilter.h"
#include "BiquadParams.h"
#include "BiquadState.h"
#include "HilbertFilterDesigner.h"

/**
 * Complete Frequency Shifter composite
 *
 */
class FrequencyShifter
{
public:
    FrequencyShifter(void)
    {
    }
    void setSampleRate(float rate)
    {
        reciprocalSampleRate = 1 / rate;
        HilbertFilterDesigner<T>::design(rate, hilbertFilterParamsSin, hilbertFilterParamsCos);
    }

    // must be called after setSampleRate
    void init()
    {
        SinOscillator<T, true>::setFrequency(oscParams, T(.01));
        exponential2 = ObjectCache<T>::getExp2();   // Get a shared copy of the 2**x lookup.
                                                    // This will enable exp mode to track at
                                                    // 1V/ octave.
    }

    /**
     * Main processing entry point. Called every sample
     */
    float process (const float _inSmp, const float _pitchCV);

    typedef float T;        // use floats for all signals
    T freqRange = 5;        // the freq range switch
private:
    SinOscillatorParams<T> oscParams;
    SinOscillatorState<T> oscState;
    BiquadParams<T, 3> hilbertFilterParamsSin;
    BiquadParams<T, 3> hilbertFilterParamsCos;
    BiquadState<T, 3> hilbertFilterStateSin;
    BiquadState<T, 3> hilbertFilterStateCos;

    std::shared_ptr<LookupTableParams<T>> exponential2;

    float reciprocalSampleRate;
};

inline float FrequencyShifter::process(const float _inSmp, const float _pitchCV)
{
    assert(exponential2->isValid());

    // Add the knob and the CV value.
    T freqHz;
    T cvTotal = _pitchCV;
    int outMode = 0; // 0=sin/up, 1=cos/down

    if(freqRange < 0.0001f) // exp?
    {
       // (note) not really suitable for modulation b/c of sin/cos phase shift at cv=0
       if(cvTotal < 0.0f)
       {
          cvTotal = -cvTotal;
          cvTotal = (cvTotal*2.0f - 5.0f);
          outMode = 1;
       }
       else
       {
          cvTotal = (cvTotal*2.0f - 5.0f);
       }
    }

    if (cvTotal > 5) {
        cvTotal = 5;
    }
    if (cvTotal < -5) {
        cvTotal = -5;
    }
    if (freqRange > .2) {
        cvTotal *= freqRange;
        cvTotal *= T(1. / 5.);
        freqHz = cvTotal;
    } else {
        cvTotal += 7;           // shift up to GE 2 (min value for out 1v/oct lookup)
        freqHz = LookupTable<T>::lookup(*exponential2, cvTotal);
        freqHz /= 2;            // down to 2..2k range that we want.
    }

    SinOscillator<float, true>::setFrequency(oscParams, freqHz * reciprocalSampleRate);

    // Generate the quadrature sin oscillators.
    T x, y;
    SinOscillator<T, true>::runQuadrature(x, y, oscState, oscParams);

    // Filter the input through th quadrature filter
    const T input = _inSmp;
    const T hilbertSin = BiquadFilter<T>::run(input, hilbertFilterStateSin, hilbertFilterParamsSin);
    const T hilbertCos = BiquadFilter<T>::run(input, hilbertFilterStateCos, hilbertFilterParamsCos);

    // Cross modulate the two sections.
    x *= hilbertSin;
    y *= hilbertCos;

    // And combine for final SSB output.
    if(0 == outMode)
    {
       // SIN_OUTPUT
       return x + y;
    }
    else
    {
       // COS_OUTPUT
       return x - y;
    }
}
