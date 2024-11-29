/* ----
 * ---- file   : delay.h
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
 * ---- info   : Feedback Delay Line
 * ----
 * ---- changed: 27Jun2019, 28Jun2019, 07Jul2019, 08Jul2019, 20Apr2023
 * ----
 * ----
 * ----
 */

#ifndef DELAY_H__
#define DELAY_H__


class Delay {
  public:
	sF32  sample_rate;  // e.g. 48000.0
   sF32  note;         // 1.0 .. 127.0
   sF32  freq_scl;     // 0.0 .. 1.0
   sF32  fb_amt;       // feedback amount
   sF32  fb_dcy;       // feedback decay (-1..1, def=0.5)

   sF32  fb_mod_norm;  // 0..1  0.5=>1.0
   sF32  fb_mod;       // 0..1 (via powf)

   sBool b_linear;

   Sway sway_freq;
   Sway sway_fb;

#define BUF_SIZE_NORM (512u*1024u)  // for parameters
#define BUF_SIZE      (64u*1024u)   // actual max buffer size
#define BUF_SIZE_MASK (BUF_SIZE - 1u)
   sF32  delay_buf[BUF_SIZE];  // ring buffer
   sUI   delay_buf_idx;
   sUI   delay_buf_sz;
   sF32  delay_buf_sz_norm;
   sF32  last_dly_val;
   sF64  dly_smp_off;
   sF64  new_dly_smp_off;
   sF64  dly_lerp_spd;
   sF32  freq_mod_norm;

   const sUI *prime_tbl;  // NULL=no prime lock

  public:

   void init(void) {
      sample_rate = 48000.0f;
      note        = 48.0f;
      freq_scl    = 0.3f;
      fb_amt      = 0.7f;
      fb_dcy      = 0.5f;

      delay_buf_idx = 0u;
      last_dly_val  = 0.0f;
      dly_smp_off   = 0.0;
      new_dly_smp_off  = 0.0;
      dly_lerp_spd = 0.00001;
      delay_buf_sz = BUF_SIZE;
      delay_buf_sz_norm = 1.0f;

      freq_mod_norm = 0.0f;
      fb_mod_norm = 0.5f;
      fb_mod = 1.0f;

      b_linear = 1;

      ::memset((void*)delay_buf, 0, sizeof(delay_buf));

      handleParamsUpdated();
      dly_smp_off = new_dly_smp_off;

      sway_freq.setScaleA(0.0f);
      sway_freq.setMinMaxA(0.0f, 1.0f);
      sway_fb.setScaleA(0.0f);
      sway_fb.setMinMaxA(-1.0f, 1.0f);
      
      prime_tbl = NULL;
   }

   Delay(void) {
      init();
   }

   ~Delay() {
   }

   void setSampleRate(sF32 _rate) {
      // e.g. 48000.0f
      sample_rate = _rate;
      sway_freq.setSampleRate(_rate);
      sway_fb.setSampleRate(_rate);
   }

   void setPrimeTbl(const sUI *_tbl) {
      prime_tbl = _tbl;
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

   void setFbAmt(sF32 _fb) {
      fb_amt = (_fb < 0.0f) ? 0.0f : (_fb > 0.999f) ? 0.999f : _fb;
   }

   void setFbModNorm(sF32 _mod) {
      fb_mod_norm = _mod;
      fb_mod = (fb_mod_norm - 0.5f) * 2.0f; // => -1..1
      if(fb_mod >= 0)
         fb_mod = fb_mod * fb_mod * fb_mod;
      else
         fb_mod = -fb_mod * fb_mod * -fb_mod;
      fb_mod = fb_mod * 2.0f;  // => -2..2
      fb_mod = powf(2.0f, fb_mod);
   }

   sF32 getFbModNorm(void) const {
      return fb_mod_norm;
   }

   void setFbDcy(sF32 _dcy) {
      fb_dcy = (_dcy < -1.0f) ? -1.0f : (_dcy > 1.0f) ? 1.0f : _dcy;
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

   sF32 process(sF32 _inSmp) {

      dly_smp_off = dly_smp_off + (new_dly_smp_off - dly_smp_off) * dly_lerp_spd/*0.0001*/;

      sF32 dlySmpOff = sF32(dly_smp_off);
      dlySmpOff *= 1.0f + sway_freq.process();

      sF32 dlyVal;
      /* sUI dlySmpOffI = sUI(delay_buf_idx - sUI(dlySmpOff)) & BUF_SIZE_MASK; */
      sUI dlyRelOff = sUI(dlySmpOff);
      if(NULL != prime_tbl)
         dlyRelOff = prime_tbl[dlyRelOff];
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

      // feedback filter
      sF32 fbVal = (last_dly_val + dlyVal) * fb_dcy;
      last_dly_val = dlyVal;

      // Write new delay sample to ring buffer
      sF32 fbAmt = fb_amt * fb_mod;
      fbAmt *= 1.0f + sway_fb.process();
      if(fbAmt > 1.0f) fbAmt = 1.0f;

      delay_buf[delay_buf_idx] = fbVal * fbAmt + _inSmp;
      /* delay_buf_idx = (delay_buf_idx + 1u) & BUF_SIZE_MASK; */
      delay_buf_idx = (delay_buf_idx + 1u) % delay_buf_sz;

      // Final output
      return fbVal;
   }
   
};


#endif // DELAY_H__
