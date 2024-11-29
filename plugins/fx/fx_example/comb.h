/* ----
 * ---- file   : comb.h
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
 * ---- info   : Tuned Delay Line / Comb Filter
 * ----
 * ---- changed: 27Jun2019, 28Jun2019
 * ----
 * ----
 * ----
 */

#ifndef COMB_H__
#define COMB_H__


class Comb {
  public:
	sF32  sample_rate;  // e.g. 44100.0
   sF32  note;         // 1.0 .. 127.0
   sF32  freq_scl;     // 0.0 .. 1.0
   sF32  fb_amt;       // feedback amount
   sBool b_post;       // comb +/-
   sF32  wet_amt;      // 0..1

#define BUF_SIZE (256u*1024u)
#define BUF_SIZE_MASK (BUF_SIZE - 1u)
   sF32  delay_buf[BUF_SIZE];  // ring buffer
   sUI   delay_buf_idx;
   sF32  last_dly_val;
   sUI   dly_smp_off;

  public:
   Comb(void) {
      note        = 48.0f;
      freq_scl    = 1.0f;
      sample_rate = 44100.0f;
      fb_amt      = 0.7f;
      b_post      = 1;
      wet_amt     = 0.5f;

      delay_buf_idx = 0u;
      last_dly_val  = 0.0f;
      dly_smp_off   = 0u;

      ::memset((void*)delay_buf, 0, sizeof(delay_buf));

      handleParamsUpdated();
   }

   ~Comb() {
   }

   void setSampleRate(sF32 _rate) {
      // e.g. 44100.0f
      sample_rate = _rate;
   }

   void setNote(sF32 _note) {
      // linear note 1..127
      note = (_note < 0.00001f) ? 0.00001f : (_note > 127.0f) ? 127.0f : _note;
   }

   void setFreqScl(sF32 _scl) {
      freq_scl = (_scl < 0.0f) ? 0.0f : (_scl > 1.0f) ? 1.0f : _scl;
   }

   void setFbAmt(sF32 _fb) {
      fb_amt = (_fb < 0.0f) ? 0.0f : (_fb > 0.999f) ? 0.999f : _fb;
   }

   void setEnablePost(sBool _bEnable) {
      b_post = _bEnable;
   }

   void setWetAmt(sF32 _wetAmt) {
      wet_amt = (_wetAmt < 0.0f) ? 0.0f : (_wetAmt > 1.0f) ? 1.0f : _wetAmt;
   }

   void handleParamsUpdated(void) {
      sF32 freq = 261.626f/*c-4*/ * powf(2.0f, (note-(12*4)) / 12.0f);
      freq *= freq_scl;
      if(freq < 1.0f)
         freq = 1.0f;
      else if(freq >= sample_rate)
         freq = sample_rate;
      dly_smp_off = sUI((1.0f * sample_rate) / freq);
      /* printf("xxx note=%f freq=%f dly_smp_off=%u\n", note, freq, dly_smp_off); */
   }

   sF32 process(sF32 _inSmp) {

      sUI dlySmpOffI = sUI(delay_buf_idx - dly_smp_off) & BUF_SIZE_MASK;
      sF32 dlyVal = delay_buf[dlySmpOffI];

      if(b_post)
      {
         dlyVal += _inSmp;
      }

      // feedback filter
      sF32 fbVal = (last_dly_val + dlyVal) * 0.5f;
      last_dly_val = dlyVal;

      fbVal *= fb_amt;

      if(!b_post)
      {
         fbVal += _inSmp;
      }

      // Write new delay sample to ring buffer
      delay_buf[delay_buf_idx] = fbVal;
      delay_buf_idx = (delay_buf_idx + 1u) & BUF_SIZE_MASK;

      // Final output
      sF32 outVal;

      if(b_post)
      {
         outVal = _inSmp + (fbVal - _inSmp) * wet_amt;
      }
      else
      {
         outVal = _inSmp + (dlyVal - _inSmp) * wet_amt;
      }

      return outVal;
   }
   
};


#endif // COMB_H__
