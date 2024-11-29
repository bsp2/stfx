/* ----
 * ---- file   : allpass.h
 * ---- author : bsp
 * ---- legal  : Distributed under terms of the MIT LICENSE (MIT).
 * ----
 * ---- Permission is hereby granted, free of charge, to any person obtaining a copy
 * ---- of this software and associated documentation files (the "Software"), to deal
 * ---- in the Software without restriction, including without limitation the rights
 * ---- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * ---- copies of the Software, and to permit persons to whom the Software is
 * ---- furnished to do so, subject to the following conditions:
 * ----
 * ---- The above copyright notice and this permission notice shall be included in
 * ---- all copies or substantial portions of the Software.
 * ----
 * ---- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * ---- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * ---- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * ---- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * ---- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * ---- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * ---- THE SOFTWARE.
 * ----
 * ---- info   : All-Pass Filter / feedforward / delay
 * ----           <https://dsp.stackexchange.com/questions/19998/allpass-filter-feedforward-feedback-design-and-code>
 * ----           <https://valhalladsp.com/2011/01/21/reverbs-diffusion-allpass-delays-and-metallic-artifacts/>
 * ----
 * ---- changed: 27Jun2019, 08Jul2019, 10Jul2019, 20Apr2023
 * ----
 * ----
 * ----
 */

#ifndef ALLPASS_FF_DLY_H__
#define ALLPASS_FF_DLY_H__


class AllPassFFDly {
  public:
   sF32  sample_rate;
   sF32  note;         // 1.0 .. 127.0
   sF32  freq_scl;     // 0.0 .. 1.0

#define BUF_SIZE_NORM (512u*1024u)  // for parameters
#define BUF_SIZE      (64u*1024u)   // actual max buffer size
#define BUF_SIZE_MASK (BUF_SIZE - 1u)
   sF32  delay_buf[BUF_SIZE];  // ring buffer
   sUI   delay_buf_idx;
   sUI   delay_buf_sz;
   sF32  delay_buf_sz_norm;
   sF64  dly_smp_off;
   sF64  new_dly_smp_off;
   sF64  dly_lerp_spd;
   sF32  freq_mod_norm;
   sBool b_linear;

   sF32 state;
   sF32 b;  // -1..1  (usually +<f>)
   sF32 c;  // -1..1  (usually -<f>)

   Sway sway_freq;

  public:
   AllPassFFDly(void) {
      sample_rate = 48000.0f;
      note        = 48.0f;
      freq_scl    = 0.3f;

      delay_buf_idx = 0u;
      dly_smp_off   = 0.0;
      new_dly_smp_off  = 0.0;
      dly_lerp_spd = 0.00001;
      delay_buf_sz = BUF_SIZE;
      delay_buf_sz_norm = 1.0f;

      freq_mod_norm = 0.0f;

      b_linear = 1;

      ::memset((void*)delay_buf, 0, sizeof(delay_buf));

      b = 0.5f;
      c = -0.5f;

      handleParamsUpdated();
      dly_smp_off = new_dly_smp_off;

      sway_freq.setScaleA(0.0f);
      sway_freq.setMinMaxA(0.0f, 1.0f);
   }

   ~AllPassFFDly() {
   }

   void setSampleRate(sF32 _rate) {
      sway_freq.setSampleRate(_rate);
   }

   void setNote(sF32 _note) {
      // linear note 1..127
      note = (_note < 0.00001f) ? 0.00001f : (_note > 127.0f) ? 127.0f : _note;
   }

   void setFreqScl(sF32 _scl) {
      freq_scl = (_scl < 0.0f) ? 0.0f : (_scl > 1.0f) ? 1.0f : _scl;
   }

   void setFreqModNorm(sF32 _mod) {
      freq_mod_norm = (_mod - 0.5f) * 2.0f;  // 0..1 => -1..1
   }

   sF32 getFreqModNorm(void) const {
      return freq_mod_norm * 0.5f + 0.5f;  // -1..1 => 0..1
   }

   void setDlyLerpSpdNorm(sF32 _spdNorm) {
      dly_lerp_spd = 0.000001 + (0.01 - 0.000001) * (sF64(_spdNorm) * sF64(_spdNorm));
   }

   void setDlyBufSizeNorm(sF32 _szNorm) {
      delay_buf_sz_norm = _szNorm;
      delay_buf_sz = sUI((BUF_SIZE_NORM * _szNorm) * _szNorm * _szNorm);
      if(delay_buf_sz < 1u)
         delay_buf_sz = 1u;
      else if(delay_buf_sz > BUF_SIZE)
         delay_buf_sz = BUF_SIZE;
   }

   sF32 getDlyBufSizeNorm(void) const {
      return delay_buf_sz_norm;
   }

   void handleParamsUpdated(void) {
      sF64 freq = 261.626f/*c-4*/ * powf(2.0f, (note-(12*4)) / 12.0f);
      freq *= freq_scl;
      freq *= pow(2.0, freq_mod_norm * 8.0);

      if(freq < 0.25)
         freq = 0.25;
      else if(freq >= sample_rate)
         freq = sample_rate;
      new_dly_smp_off = (1.0 * sample_rate) / freq;
      /* printf("xxx note=%f freq=%f dly_smp_off=%u\n", note, freq, sUI(new_dly_smp_off)); */
   }

   void setB(sF32 _b) {
      b = (_b < -1.0f) ? -1.0f : (_b > 1.0f) ? 1.0f : _b;
   }

   void setC(sF32 _c) {
      c = (_c < -1.0f) ? -1.0f : (_c > 1.0f) ? 1.0f : _c;
   }

   /* static float randf(float _max) { */
   /*    return ((rand()*(0.999999999999f / float(RAND_MAX))) * _max); */
   /* } */

   sF32 process(sF32 _f) {
      dly_smp_off = dly_smp_off + (new_dly_smp_off - dly_smp_off) * dly_lerp_spd/*0.0001*/;

      sF32 dlySmpOff = sF32(dly_smp_off);
      dlySmpOff *= 1.0f + sway_freq.process();

      sF32 dlyVal;
      sUI dlyRelOff = sUI(dlySmpOff);
      sUI dlySmpOffI = sUI(delay_buf_idx - dlyRelOff) % delay_buf_sz;

      if(b_linear)
      {
         sF32 dlySmpOffFrac = sF32(dlySmpOff) - sSI(dlySmpOff);
         sF32 dlyValC = delay_buf[dlySmpOffI];
         sF32 dlyValP = delay_buf[sUI(dlySmpOffI - 1u) % delay_buf_sz];
         dlyVal = dlyValC + (dlyValP - dlyValC) * dlySmpOffFrac;
      }
      else
      {
         dlyVal = delay_buf[dlySmpOffI];
      }

      sF32 v = _f - c * dlyVal;
      sF32 out = v * b + dlyVal;

      delay_buf[delay_buf_idx] = v;
      delay_buf_idx = (delay_buf_idx + 1u) % delay_buf_sz;

      /* /\* sF32 v = _f - c * state; *\/ */
      /* /\* sF32 out = v * b + state; *\/ */
      /* /\* state = v; *\/ */

      return out;
   }
   
};


#endif // ALLPASS_FF_DLY_H__
