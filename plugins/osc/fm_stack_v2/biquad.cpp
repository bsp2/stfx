// ----
// ---- file   : biquad.cpp
// ---- author : adapted for voice plugin example by bsp
// ---- info   : biquad filter
// ----
// ---- created: 21May2020
// ---- changed: 25May2020
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

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#include "biquad.h"


void StBiquadCoeff::calcParams(int          _type,
                               float        _dbGain,
                               float        _freq,   // 0..1
                               float        _res     // 0..1
                               ) {
#define Q _res
#define Fc _freq

   // (note) sound dies after a short while with fmax=1.0
   _freq = Dstplugin_min(0.9999f, Dstplugin_max(_freq, 0.001f));
   _freq *= 0.5f;

   // (note) sound becomes very quiet with res < 0.5
   _res += 0.5f;
   if(_res < 0.5f)
      _res = 0.5f;
   else if(_res > 1.5f)
      _res = 1.5f;

   double norm;
   double V = pow(10, fabs(_dbGain) / 20.0);  // PEQ, LSH, HSH
   double K = tan(ST_PLUGIN_PI * Fc);
   switch(_type)
   {
      case StBiquad::LPF:
         norm = 1 / (1 + K / Q + K * K);
         a0 = K * K * norm;
         a1 = 2 * a0;
         a2 = a0;
         b1 = 2 * (K * K - 1) * norm;
         b2 = (1 - K / Q + K * K) * norm;
         break;
            
      case StBiquad::HPF:
         norm = 1 / (1 + K / Q + K * K);
         a0 = 1 * norm;
         a1 = -2 * a0;
         a2 = a0;
         b1 = 2 * (K * K - 1) * norm;
         b2 = (1 - K / Q + K * K) * norm;
         break;
            
      case StBiquad::BPF:
         norm = 1 / (1 + K / Q + K * K);
         a0 = K / Q * norm;
         a1 = 0;
         a2 = -a0;
         b1 = 2 * (K * K - 1) * norm;
         b2 = (1 - K / Q + K * K) * norm;
         break;
            
      case StBiquad::BRF:
         norm = 1 / (1 + K / Q + K * K);
         a0 = (1 + K * K) * norm;
         a1 = 2 * (K * K - 1) * norm;
         a2 = a0;
         b1 = a1;
         b2 = (1 - K / Q + K * K) * norm;
         break;
            
      case StBiquad::PEQ:
         if(_dbGain >= 0)
         {
            // boost
            norm = 1 / (1 + 1/Q * K + K * K);
            a0 = (1 + V/Q * K + K * K) * norm;
            a1 = 2 * (K * K - 1) * norm;
            a2 = (1 - V/Q * K + K * K) * norm;
            b1 = a1;
            b2 = (1 - 1/Q * K + K * K) * norm;
         }
         else
         {
            // cut
            norm = 1 / (1 + V/Q * K + K * K);
            a0 = (1 + 1/Q * K + K * K) * norm;
            a1 = 2 * (K * K - 1) * norm;
            a2 = (1 - 1/Q * K + K * K) * norm;
            b1 = a1;
            b2 = (1 - V/Q * K + K * K) * norm;
         }
         break;

      case StBiquad::LSH:
         if(_dbGain >= 0)
         {
            // boost
            norm = 1 / (1 + sqrt(2) * K + K * K);
            a0 = (1 + sqrt(2*V) * K + V * K * K) * norm;
            a1 = 2 * (V * K * K - 1) * norm;
            a2 = (1 - sqrt(2*V) * K + V * K * K) * norm;
            b1 = 2 * (K * K - 1) * norm;
            b2 = (1 - sqrt(2) * K + K * K) * norm;
         }
         else
         {
            // cut
            norm = 1 / (1 + sqrt(2*V) * K + V * K * K);
            a0 = (1 + sqrt(2) * K + K * K) * norm;
            a1 = 2 * (K * K - 1) * norm;
            a2 = (1 - sqrt(2) * K + K * K) * norm;
            b1 = 2 * (V * K * K - 1) * norm;
            b2 = (1 - sqrt(2*V) * K + V * K * K) * norm;
         }
         break;

      case StBiquad::HSH:
         if(_dbGain >= 0)
         {
            // boost
            norm = 1 / (1 + sqrt(2) * K + K * K);
            a0 = (V + sqrt(2*V) * K + K * K) * norm;
            a1 = 2 * (K * K - V) * norm;
            a2 = (V - sqrt(2*V) * K + K * K) * norm;
            b1 = 2 * (K * K - 1) * norm;
            b2 = (1 - sqrt(2) * K + K * K) * norm;
         }
         else
         {
            // cut
            norm = 1 / (V + sqrt(2*V) * K + K * K);
            a0 = (1 + sqrt(2) * K + K * K) * norm;
            a1 = 2 * (K * K - 1) * norm;
            a2 = (1 - sqrt(2) * K + K * K) * norm;
            b1 = 2 * (K * K - V) * norm;
            b2 = (V - sqrt(2*V) * K + K * K) * norm;
         }
         break;
   }
#undef Q
#undef Fc

}
