#pragma once

#include "WaveShaper.h"

#define RSHAPER_GAIN 0.1

namespace dsp {

struct ReShaper : WaveShaper {

    Noise *noise;


public:

    explicit ReShaper(float sr);

    void init() override;
    void invalidate() override;
    void process() override;
    double compute(double x) override;

};

}
