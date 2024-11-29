/*
Copyright (c) 2018-2019 bsp

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <math.h>

class Sway {

  public:
   sF32 min_t;
   sF32 max_t;
   sF32 min_a;
   sF32 max_a;
   sF32 scale_a;
   sF32 offset_a;

   static const sSI SCALE_T_SEC = 60;
   static const sSI SCALE_MAX   = 1;

	float sample_rate;

   float cur_rand_val_step;
   float cur_rand_val;
   int   cur_rand_val_countdown;

   float last_min_t;
   float last_max_t;

   sF32 cur_min_t;
   sF32 cur_max_t;

   sF32 cur_scale_a;


   static float randf(float _max) {
      return ((rand()*(0.999999999999f / float(RAND_MAX))) * _max);
   }

   void setSampleRate(sF32 _rate) {
      sample_rate = _rate;
   }

   void setMinMaxT(sF32 _minT, sF32 _maxT) {
      min_t = _minT;
      max_t = _maxT;

      // Sort min / max
      if(min_t > max_t)
      {
         float t = min_t;
         min_t = max_t;
         max_t = t;
      }

      // Bias towards slow modulation
      cur_min_t = min_t * min_t;
      cur_min_t = cur_min_t * cur_min_t;

      cur_max_t = max_t * max_t;
      cur_max_t = cur_max_t * cur_max_t;

      cur_rand_val_countdown = -1;
   }

   sF32 getMinT(void) const {
      return min_t;
   }

   sF32 getMaxT(void) const {
      return max_t;
   }

   void setMinMaxA(sF32 _minA, sF32 _maxA) {
      min_a = _minA;
      max_a = _maxA;

      // Sort min / max
      if(min_a > max_a)
      {
         float t = min_a;
         min_a = max_a;
         max_a = t;
      }

      cur_rand_val_countdown = -1;
   }

   sF32 getMinA(void) const {
      return min_a;
   }

   sF32 getMaxA(void) const {
      return max_a;
   }

   void setScaleA(sF32 _scaleA) {
      scale_a = _scaleA;

      // Bias towards subtle modulation
      cur_scale_a = scale_a * scale_a;
      cur_scale_a = cur_scale_a * cur_scale_a;
      cur_scale_a *= float(SCALE_MAX);
   }

   sF32 getScaleA(void) const {
      return scale_a;
   }

   void setOffsetA(sF32 _off) {
      offset_a = _off;
   }

   sF32 getOffsetA(void) const {
      return offset_a;
   }

	Sway(void) {
      sample_rate = 44100.0f;
      cur_rand_val_countdown = -1;
      last_min_t = -9999.0f;
      last_max_t = -9999.0f;

      setMinMaxT(0.3f, 0.5f);
      setMinMaxA(-1.0f, 1.0f);
      setScaleA(0.0f);
      setOffsetA(0.0f);
   }

   sF32 process(void) {
      if(cur_rand_val_countdown < 0)
      {
         // First sample after reset / init
         cur_rand_val = randf(max_a - min_a) + min_a;
      }

      if(--cur_rand_val_countdown <= 0)
      {
         // Next target val
         cur_rand_val_countdown = int((randf(cur_max_t - cur_min_t) + cur_min_t) * (sample_rate * float(SCALE_T_SEC)));
         if(cur_rand_val_countdown < 1)
            cur_rand_val_countdown = 1;

         sF32 nextVal = randf(max_a - min_a) + min_a;
         cur_rand_val_step = (nextVal - cur_rand_val) / cur_rand_val_countdown;
      }

      cur_rand_val += cur_rand_val_step;

      // Set output
      sF32 r = cur_rand_val * cur_scale_a + offset_a;
      return r;
   }

};
