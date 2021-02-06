#pragma once

#include "DSPEffect.h"
#include "DSPMath.h"


namespace dsp {


struct HQTanh : DSPEffect {

    /* oversampling channel */
    static const int STD_CHANNEL = 0;

    int factor;
    double in, out, xn1, fn1;
    Resampler<1> *rs;


    HQTanh(float sr, int factor, int quality = 4) : DSPEffect(sr) {
        HQTanh::factor = factor;

        rs = new Resampler<1>(factor, factor * quality);
    }


    /**
     * @brief Returns the actual sample-rate which is used by oversampled computation
     * @return
     */
    float getOversampledRate() {
        return sr * rs->getFactor();
    }


    void init() override {
        DSPEffect::init();
    }


    void invalidate() override {
        DSPEffect::invalidate();
    }


    /**
     * @brief Compute next value
     * @param x
     * @return
     */
    double next(double x) {
        in = x;
        process();

        return out;
    }


    /**
     * @brief Generate an anti-aliased tanh
     * @param x
     * @return
     */
    inline double computeAA(double x) {
        double fn = log(cosh(x));
        double xn, out;

        if (abs(x - xn1) < 10e-10) {
            xn = (x + xn1) / 2.;
            out = tanh(xn);
        } else {
            out = (fn - fn1) / (x - xn1);
        }

        fn1 = fn;
        xn1 = x;

        return out;
    }


    /**
     * @brief Compute tanh
     */
    inline void process() override {
        rs->doUpsample(STD_CHANNEL, in);

        for (int i = 0; i < rs->getFactor(); i++) {
            double x = rs->getUpsampled(STD_CHANNEL)[i];
            rs->data[STD_CHANNEL][i] = computeAA(x);
        }

        out = rs->getDownsampled(STD_CHANNEL);
    }

};


struct HQClip : DSPEffect {

    /* oversampling channel */
    static const int STD_CHANNEL = 0;

    int factor;
    double in, out, xn1, fn1;
    Resampler<1> *rs;


    HQClip(float sr, int factor, int quality = 4) : DSPEffect(sr) {
        HQClip::factor = factor;

        rs = new Resampler<1>(factor, factor * quality);
    }


    /**
     * @brief Returns the actual sample-rate which is used by oversampled computation
     * @return
     */
    float getOversampledRate() {
        return sr * rs->getFactor();
    }


    void init() override {
        DSPEffect::init();
    }


    void invalidate() override {
        DSPEffect::invalidate();
    }


    /**
     * @brief Compute next value
     * @param x
     * @return
     */
    double next(double x) {
        in = x;
        process();

        return out;
    }


    /**
     * @brief Generate an anti-aliased clipping
     * @param x
     * @return
     */
    inline double computeAA(double x) {
        double xa = x + 1;
        double xb = x - 1;
        double fn = ((xa * xa) * sign(xa) - (xb * xb) * sign(xb) - 2) / 4.;
        double xn, out;

        if (abs(x - xn1) < 10e-10) {
            xn = (x + xn1) / 2.;
            out = (abs(xn + 1) - abs(xn - 1)) / 2.;
        } else {
            out = (fn - fn1) / (x - xn1);
        }

        fn1 = fn;
        xn1 = x;

        return out;
    }


    /**
     * @brief Compute tanh
     */
    inline void process() override {
        rs->doUpsample(STD_CHANNEL, in);

        for (int i = 0; i < rs->getFactor(); i++) {
            double x = rs->getUpsampled(STD_CHANNEL)[i];
            rs->data[STD_CHANNEL][i] = computeAA(x);
        }

        out = rs->getDownsampled(STD_CHANNEL);
    }

};


}
