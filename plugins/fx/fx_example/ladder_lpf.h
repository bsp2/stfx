// ----
// ---- file   : ladder_lpf.h
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a Ladder-like low pass filter that supports per-sample-frame parameter interpolation
// ----           see <http://musicdsp.org/showArchiveComment.php?ArchiveID=24>
// ----
// ---- created: 05Jun2020
// ---- changed: 
// ----
// ----
// ----

#ifndef ST_LADDER_LPF_H__
#define ST_LADDER_LPF_H__


class StLadderLPF {
  public:
   float x, y1, y2, y3, y4; // temporary
   float oldx, oldy1, oldy2, oldy3;
   float newy4;

   float param_p_cur;
   float param_p_inc;
   float param_k_cur;
   float param_k_inc;
   float param_r_cur;
   float param_r_inc;
   float gain_cur;
   float gain_inc;
   float gain2_cur;
   float gain2_inc;

   inline void reset(void) {
      x = y1 = y2 = y3 = y4 = 0.0f;
      oldx = oldy1 = oldy2 = oldy3 = 0.0f;
      newy4 = 0.0f;
   }

   inline void shuffle(void) {
      oldx  = x;
      oldy1 = y1;
      oldy2 = y2;
      oldy3 = y3;
      y4    = newy4;
   }

   void calcCoeff(unsigned int _numFrames, float freq, float q, float drive3/*1.13f*/, float drive4/*1.0f*/) {
      float nextP = freq * (1.8f - 0.8f * freq);
      float nextK = nextP + nextP - 1.0f;
      
      float t  = (1.0f - nextP) * 1.386249f;
      float t2 = 12.0f + t * t;
      float nextR = q * (t2 + 6.0f * t) / (t2 - 6.0f * t);

#define FREQ_THRESHOLD 0.4f
      float nextGain;
      if(freq <= FREQ_THRESHOLD)
      {
         float f = (freq / FREQ_THRESHOLD);
         nextGain = q * (1.0f-f);
         nextGain = nextGain*nextGain * (3.02f - 2.0f*nextGain);
         /* nextGain = (/\*1.0f + *\/nextGain * drive3/\**1.13f*\/); */
         nextGain = ((1.0f + nextGain) * drive3/**1.13f*/);
      }
      else
         nextGain = drive3;

      if(0u == _numFrames)
      {
         param_p_cur = nextP;
         param_p_inc = 0.0f;
         param_k_cur = nextK;
         param_k_inc = 0.0f;
         param_r_cur = nextR;
         param_r_inc = 0.0f;
         gain_cur = nextGain;
         gain_inc = 0.0f;
         gain2_cur = drive4;
         gain2_inc = 0.0f;
      }
      else
      {
         float recBlockSize = (1.0f / _numFrames);
         param_p_inc = (nextP    - param_p_cur) * recBlockSize;
         param_k_inc = (nextK    - param_k_cur) * recBlockSize;
         param_r_inc = (nextR    - param_r_cur) * recBlockSize;
         gain_inc    = (nextGain - gain_cur)    * recBlockSize;
         gain2_inc   = (drive4   - gain2_cur)   * recBlockSize;
      }
   }

   inline float filter(float _smp) {
      // Inverted feed back for corner peaking
      x = _smp - param_r_cur * y4;
      x = Dstplugin_fix_denorm_32(x);

      //Four cascaded onepole filters (bilinear transform)
      y1    = x *param_p_cur + oldx *param_p_cur - y1*param_k_cur;
      y2    = y1*param_p_cur + oldy1*param_p_cur - y2*param_k_cur;
      y3    = y2*param_p_cur + oldy2*param_p_cur - y3*param_k_cur;
      newy4 = y3*param_p_cur + oldy3*param_p_cur - y4*param_k_cur;   

#if 1
      y1    = Dstplugin_fix_denorm_32(y1);
      y2    = Dstplugin_fix_denorm_32(y2);
      y3    = Dstplugin_fix_denorm_32(y3);
      newy4 = Dstplugin_fix_denorm_32(newy4);
#endif

      //Clipper band limited sigmoid
      newy4 = newy4 - ((newy4*newy4*newy4)/6.0f);

      /* if(newy4 > 2.0f) */
      /*    newy4 = 2.0f; */
      /* else if(newy4 < -2.0f) */
      /*    newy4 = -2.0f; */
      newy4 = tanhf(gain_cur * newy4);
      /* newy4 = gain_cur * newy4; */

      float out = tanhf(gain2_cur * newy4);
      /* float out = gain2_cur * newy4; */

      param_p_cur += param_p_inc;
      param_k_cur += param_k_inc;
      param_r_cur += param_r_inc;
      gain_cur    += gain_inc;
      gain2_cur   += gain2_inc;

      shuffle();

      return out;
   }
};


#endif // ST_LADDER_LPF_H__
