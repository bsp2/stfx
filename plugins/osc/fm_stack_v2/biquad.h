// ----
// ---- file   : biquad.h
// ---- author : adapted for voice plugin example by bsp
// ---- info   : biquad filter
// ----
// ---- created: 21May2020
// ---- changed: 25May2020, 16Sep2023
// ----
// ----
// ----
// 
// based on http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
//
//  Created by Nigel Redmon on 11/24/12
//  EarLevel Engineering: earlevel.com
//  Copyright 2012 Nigel Redmon
//
//  For a complete explanation of the Biquad code:
//  http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
//
//  License:
//
//  This source code is provided as is, without warranty.
//  You may copy and distribute verbatim copies of this document.
//  You may modify and use this source code to create binary code
//  for your own purposes, free or commercial.
//
//

#ifndef __ST_BIQUAD_H__
#define __ST_BIQUAD_H__


struct StBiquadCoeff {

   double a0, a1, a2, b1, b2;

   void reset(void) {
      a0 = 1.0;
      a1 = a2 = b1 = b2 = 0.0;
   }

   void calcParams (int   _type,
                    float _dbGain,
                    float _freq,   // 0..1
                    float _res     // 0..1
                    );

   void lerp (const StBiquadCoeff &a, const StBiquadCoeff &b, const float t) {
      a0 = a.a0 + (b.a0 - a.a0) * t;
      a1 = a.a1 + (b.a1 - a.a1) * t;
      a2 = a.a2 + (b.a2 - a.a2) * t;
      b1 = a.b1 + (b.b1 - a.b1) * t;
      b2 = a.b2 + (b.b2 - a.b2) * t;
   }
};

struct StBiquad {

   // Filter type
   enum {
      LPF  = 1,  // Low Pass
      HPF  = 2,  // High Pass
      BPF  = 3,  // Band Pass
      BRF  = 4,  // Band Reject / Notch
      PEQ  = 5,  // Parametric EQ
      LSH  = 6,  // Low Shelf
      HSH  = 7   // High Shelf
   };

   double x1, x2;  // input delayed by 1/2 samples
   double y1, y2;  // output delayed by 1/2 samples
   double z1, z2;

   StBiquadCoeff cur;
   StBiquadCoeff next;
   StBiquadCoeff step;

   inline void reset(void) {
      x1 = x2 = 0.0f;
      y1 = y2 = 0.0f;
      z1 = z2 = 0.0;

      cur .reset();
      next.reset();
      step.reset();
   }

   inline void shuffleCoeff (void) {
      cur = next;
   }

   inline void calcStep (unsigned int _blkSz) {
      step.a0 = (next.a0 - cur.a0) / _blkSz;
      step.a1 = (next.a1 - cur.a1) / _blkSz;
      step.a2 = (next.a2 - cur.a2) / _blkSz;
      step.b1 = (next.b1 - cur.b1) / _blkSz;
      step.b2 = (next.b2 - cur.b2) / _blkSz;
   }

   inline void calcParams (unsigned int _blkSz,
                           int          _type,
                           float        _dbGain,
                           float        _freq,   // 0..1
                           float        _res     // 0..1
                           ) {
      shuffleCoeff();

      next.calcParams(_type,
                      _dbGain,
                      _freq,
                      _res
                      );

      calcStep(_blkSz);
   }

   inline void stepCoeff (void) {
      cur.a0 += step.a0;
      cur.a1 += step.a1;
      cur.a2 += step.a2;
      cur.b1 += step.b1;
      cur.b2 += step.b2;
   }

   inline float filter (const float _inSmp) {
      double out = _inSmp * cur.a0 + z1;
      z1 = _inSmp * cur.a1 + z2 - cur.b1 * out;
      z2 = _inSmp * cur.a2 - cur.b2 * out;

      stepCoeff();

      float out32 = float(out);
      return Dstplugin_fix_denorm_32(out32);
   }

   inline void calcParamsNoStep (int   _type,
                                 float _dbGain,
                                 float _freq,   // 0..1
                                 float _res     // 0..1
                                 ) {
      cur.calcParams(_type,
                     _dbGain,
                     _freq,
                     _res
                     );
   }

   inline float filterNoStep (const float _inSmp) {
      double out = _inSmp * cur.a0 + z1;
      z1 = _inSmp * cur.a1 + z2 - cur.b1 * out;
      z2 = _inSmp * cur.a2 - cur.b2 * out;

      float out32 = float(out);
      return Dstplugin_fix_denorm_32(out32);
   }

};


#endif // __ST_BIQUAD_H__
