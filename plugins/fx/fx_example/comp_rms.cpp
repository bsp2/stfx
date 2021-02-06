// ----
// ---- file   : comp_rms.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a simple RMS compressor
// ----
// ---- created: 06Jun2020
// ---- changed: 07Jun2020, 08Jun2020
// ----
// ----
// ----

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "../../../plugin.h"

#include "biquad.h"

#define PARAM_DRYWET   0
#define PARAM_DRIVE1   1
#define PARAM_OUT_GAIN 2
#define PARAM_RISE     3
#define PARAM_FALL     4
#define PARAM_CEIL     5
#define PARAM_DRIVE2   6
#define PARAM_SC_HPF   7
#define NUM_PARAMS     8
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "SC Gain",
   "Out Gain",
   "Rise",
   "Fall",
   "Ceil",
   "Gain Reduction",
   "Sidechain HPF",
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,    // DRYWET
   0.63f,   // DRIVE1
   0.5f,    // OUT_GAIN
   0.14f,   // RISE
   0.06f,   // FALL
   0.53f,   // CEIL
   0.52f,   // DRIVE2
   0.5f,    // SC_HPF
};

#define MOD_DRYWET    0
#define MOD_DRIVE1    1
#define MOD_RISE      2
#define MOD_FALL      3
#define MOD_DRIVE2    4
#define MOD_OUT_GAIN  5
#define NUM_MODS      6
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Drive Pre",
   "Rise",
   "Fall",
   "Drive Post",
   "Out Gain",
};

typedef struct comp_rms_info_s {
   st_plugin_info_t base;
} comp_rms_info_t;

typedef struct comp_rms_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} comp_rms_shared_t;

struct Comp {
#define MAX_WIN_SIZE 2048
   float buf[MAX_WIN_SIZE];
   unsigned int buf_idx;
   unsigned int win_size_bits;
   unsigned int win_size;       // power of two
   unsigned int win_size_mask;  // win_size - 1

   double integrated_val;
   double smoothed_sign;
   double last_smoothed_val;
   double smoothed_val;

   StBiquad hpf;

   void setWinSizeBits(unsigned int _bits) {
      win_size_bits = _bits;
      win_size      = (1u << _bits);
      win_size_mask = (win_size - 1u);
   }

   void reset(void) {
      hpf.reset();
      setWinSizeBits(11u);
      ::memset((void*)buf, 0, win_size * sizeof(float));
      buf_idx           = 0u;
      integrated_val    = 0.0;
      smoothed_sign     = 0.0;
      last_smoothed_val = 0.0;
      smoothed_val      = 0.0;
   }

   float process(const float _inSmp, 
                 const float _rise, 
                 const float _fall
                 ) {
      // Sidechain filter
      float inVal = hpf.filter(_inSmp);

      // Calc square
      inVal *= inVal;

      // Integrate new input
      integrated_val += inVal;

      // Subtract oldest input
      integrated_val -= buf[(buf_idx - win_size + 1u) & win_size_mask];

      buf[buf_idx] = inVal;
      buf_idx = (buf_idx + 1u) & win_size_mask;

      double outVal = integrated_val / double(win_size);
      if(outVal > 0.0)
         outVal = sqrt(outVal);

      // Smoothing
      double smoothAmt;
      if(smoothed_sign >= 0.0)
      {
         smoothAmt = _rise;
      }
      else
      {
         smoothAmt = _fall;
      }

      smoothAmt = (1.0 - smoothAmt);
      smoothAmt *= smoothAmt;
      smoothAmt *= smoothAmt;
      smoothAmt *= smoothAmt;
      smoothed_val = smoothed_val + (outVal - smoothed_val) * smoothAmt;

      smoothed_sign = (smoothed_val - last_smoothed_val);
      last_smoothed_val = smoothed_val;

      // Output
      return float(smoothed_val);
   }
};

typedef struct comp_rms_voice_s {
   st_plugin_voice_t base;
   float sample_rate;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_drive1_cur;
   float mod_drive1_inc;
   float mod_rise_cur;
   float mod_rise_inc;
   float mod_fall_cur;
   float mod_fall_inc;
   float mod_ceil_cur;
   float mod_ceil_inc;
   float mod_drive2_cur;
   float mod_drive2_inc;
   float mod_drive3_cur;
   float mod_drive3_inc;
   Comp  comp_l;
   Comp  comp_r;
} comp_rms_voice_t;


static const char *ST_PLUGIN_API loc_get_param_name(st_plugin_info_t *_info,
                                                    unsigned int      _paramIdx
                                                    ) {
   (void)_info;
   return loc_param_names[_paramIdx];
}

static float ST_PLUGIN_API loc_get_param_reset(st_plugin_info_t *_info,
                                               unsigned int      _paramIdx
                                               ) {
   (void)_info;
   return loc_param_resets[_paramIdx];
}

static float ST_PLUGIN_API loc_get_param_value(st_plugin_shared_t *_shared,
                                               unsigned int        _paramIdx
                                               ) {
   ST_PLUGIN_SHARED_CAST(comp_rms_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(comp_rms_shared_t);
   shared->params[_paramIdx] = _value;
}

static const char *ST_PLUGIN_API loc_get_mod_name(st_plugin_info_t *_info,
                                                  unsigned int      _modIdx
                                                  ) {
   (void)_info;
   return loc_mod_names[_modIdx];
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(comp_rms_voice_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
      voice->comp_l.reset();
      voice->comp_r.reset();
#if 0
      printf("------- comp note on -----\n");
      fflush(stdout);
#endif
   }
}

static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(comp_rms_voice_t);
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(comp_rms_voice_t);
   (void)_frameOffset;
   voice->mods[_modIdx] = _value;
}

static void ST_PLUGIN_API loc_prepare_block(st_plugin_voice_t *_voice,
                                            unsigned int       _numFrames,
                                            float              _freqHz,
                                            float              _note,
                                            float              _vol,
                                            float              _pan
                                            ) {
   ST_PLUGIN_VOICE_CAST(comp_rms_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(comp_rms_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

#if 0
   unsigned int winSizeBits = (unsigned int)(shared->params[PARAM_WINSIZE] * 11.0f + 0.5f);
   if(winSizeBits != voice->comp_l.win_size_bits)
   {
      voice->comp_l.reset();
      voice->comp_r.reset();
      voice->comp_l.setWinSizeBits(winSizeBits);
      voice->comp_r.setWinSizeBits(winSizeBits);
   }
#endif

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modDrive1 = (shared->params[PARAM_DRIVE1]-0.5f)*2.0f + voice->mods[MOD_DRIVE1];
   modDrive1 = powf(10.0f, modDrive1 * 2.0f);

   float modRise = shared->params[PARAM_RISE] * powf(10.0f, voice->mods[MOD_RISE]);
   modRise = Dstplugin_clamp(modRise, 0.0f, 1.0f);

   float modFall = shared->params[PARAM_FALL] * powf(10.0f, voice->mods[MOD_FALL]);
   modFall = Dstplugin_clamp(modFall, 0.0f, 1.0f);

   float modCeil = (shared->params[PARAM_CEIL]-0.5f)*2.0f;
   modCeil = powf(10.0f, modCeil * 2.0f);

   // float modDrive2 = (shared->params[PARAM_DRIVE2]-0.5f)*2.0f + voice->mods[MOD_DRIVE2];
   float modDrive2 = shared->params[PARAM_DRIVE2] + voice->mods[MOD_DRIVE2];
   modDrive2 = powf(10.0f, -modDrive2 * 2.0f);

   float modDrive3 = (shared->params[PARAM_OUT_GAIN]-0.5f)*2.0f + voice->mods[MOD_OUT_GAIN];
   modDrive3 = powf(10.0f, modDrive3 * 2.0f);

   float modFreq = _freqHz * powf(10.0f, (shared->params[PARAM_SC_HPF]-0.5f)*2.0f*2.0f);
   if(modFreq >= (voice->sample_rate*0.5f))
      modFreq = 1.0f;
   else
      modFreq /= (voice->sample_rate*0.5f);
#if 0
   {
      static int xxx = 0;
      if(0 == (++xxx & 2047))
      {
         printf("xxx modFreq=%f\n", modFreq);
         fflush(stdout);
      }
   }
#endif

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_drive1_inc = (modDrive1 - voice->mod_drive1_cur) * recBlockSize;
      voice->mod_rise_inc   = (modRise   - voice->mod_rise_cur)   * recBlockSize;
      voice->mod_fall_inc   = (modFall   - voice->mod_fall_cur)   * recBlockSize;
      voice->mod_ceil_inc   = (modCeil   - voice->mod_ceil_cur)   * recBlockSize;
      voice->mod_drive2_inc = (modDrive2 - voice->mod_drive2_cur) * recBlockSize;
      voice->mod_drive3_inc = (modDrive3 - voice->mod_drive3_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      voice->mod_drive1_cur = modDrive1;
      voice->mod_drive1_inc = 0.0f;
      voice->mod_rise_cur   = modRise;
      voice->mod_rise_inc   = 0.0f;
      voice->mod_fall_cur   = modFall;
      voice->mod_fall_inc   = 0.0f;
      voice->mod_ceil_cur   = modCeil;
      voice->mod_ceil_inc   = 0.0f;
      voice->mod_drive2_cur = modDrive2;
      voice->mod_drive2_inc = 0.0f;
      voice->mod_drive3_cur = modDrive3;
      voice->mod_drive3_inc = 0.0f;

      _numFrames = 1u;
   }

   voice->comp_l.hpf.calcParams(_numFrames,
                                StBiquad::HPF,
                                0.0f/*gainDB*/,
                                modFreq,
                                0.0/*modQ*/
                                );

   voice->comp_r.hpf.calcParams(_numFrames,
                                StBiquad::HPF,
                                0.0f/*gainDB*/,
                                modFreq,
                                0.0/*modQ*/
                                );
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(comp_rms_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(comp_rms_shared_t);

   unsigned int k = 0u;

   // if(_bMonoIn)
   // {
   // }
   // else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float r = _samplesIn[k + 1u];

         float outL = l * voice->mod_drive1_cur;
         float rmsL = voice->comp_l.process(outL, voice->mod_rise_cur, voice->mod_fall_cur);
#if 0
         {
            static int xxx = 0;
            if(0 == (++xxx & 2047))
            {
               printf("xxx rmsL=%f\n", rmsL);
               fflush(stdout);
            }
         }
#endif
         if(rmsL > voice->mod_ceil_cur)
            rmsL = voice->mod_ceil_cur;
         rmsL /= voice->mod_ceil_cur;
#if 0
         rmsL = rmsL * rmsL * (3.0f - 2.0f * rmsL);
#endif
         rmsL = 1.0f + (voice->mod_drive2_cur - 1.0f) * rmsL;
         // outR = outR * (1.0f + voice->mod_drive2_cur * (1.0f - rmsR)) * voice->mod_drive3_cur;
         outL = outL * rmsL;
         // outL = outL * (1.0f + voice->mod_drive2_cur * (1.0f - rmsL)) * voice->mod_drive3_cur;
         // outL = outL * (voice->mod_drive2_cur * (voice->mod_ceil_cur - rmsL));
         outL = tanhf(outL) * voice->mod_drive3_cur;

         float outR = r * voice->mod_drive1_cur;
         float rmsR = voice->comp_r.process(outR, voice->mod_rise_cur, voice->mod_fall_cur);
         if(rmsR > voice->mod_ceil_cur)
            rmsR = voice->mod_ceil_cur;
         rmsR /= voice->mod_ceil_cur;
#if 0
         rmsR = rmsR * rmsR * (3.0f - 2.0f * rmsR);
#endif
         rmsR = 1.0f + (voice->mod_drive2_cur - 1.0f) * rmsR;
         // outR = outR * (1.0f + voice->mod_drive2_cur * (1.0f - rmsR)) * voice->mod_drive3_cur;
         outR = outR * rmsR;
         outR = tanhf(outR) * voice->mod_drive3_cur;

         outL = l + (outL - l) * voice->mod_drywet_cur;
         outR = r + (outR - r) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);
         outR = Dstplugin_fix_denorm_32(outR);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_drive1_cur += voice->mod_drive1_inc;
         voice->mod_rise_cur   += voice->mod_rise_inc;
         voice->mod_fall_cur   += voice->mod_fall_inc;
         voice->mod_ceil_cur   += voice->mod_ceil_inc;
         voice->mod_drive2_cur += voice->mod_drive2_inc;
         voice->mod_drive3_cur += voice->mod_drive3_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   comp_rms_shared_t *ret = (comp_rms_shared_t *)malloc(sizeof(comp_rms_shared_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info  = _info;
      memcpy((void*)ret->params, (void*)loc_param_resets, NUM_PARAMS * sizeof(float));
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_shared_delete(st_plugin_shared_t *_shared) {
   free((void*)_shared);
}

static st_plugin_voice_t *ST_PLUGIN_API loc_voice_new(st_plugin_info_t *_info) {
   comp_rms_voice_t *ret = (comp_rms_voice_t *)malloc(sizeof(comp_rms_voice_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info   = _info;
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   free((void*)_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {
   free((void*)_info);
}

extern "C" {
st_plugin_info_t *comp_rms_init(void) {
   comp_rms_info_t *ret = (comp_rms_info_t *)malloc(sizeof(comp_rms_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp compressor rms";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "compressor rms";
      ret->base.short_name  = "comp rms";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
      ret->base.category    = ST_PLUGIN_CAT_COMPRESSOR;
      ret->base.num_params  = NUM_PARAMS;
      ret->base.num_mods    = NUM_MODS;

      ret->base.shared_new       = &loc_shared_new;
      ret->base.shared_delete    = &loc_shared_delete;
      ret->base.voice_new        = &loc_voice_new;
      ret->base.voice_delete     = &loc_voice_delete;
      ret->base.get_param_name   = &loc_get_param_name;
      ret->base.get_param_reset  = &loc_get_param_reset;
      ret->base.get_param_value  = &loc_get_param_value;
      ret->base.set_param_value  = &loc_set_param_value;
      ret->base.get_mod_name     = &loc_get_mod_name;
      ret->base.set_sample_rate  = &loc_set_sample_rate;
      ret->base.note_on          = &loc_note_on;
      ret->base.set_mod_value    = &loc_set_mod_value;
      ret->base.prepare_block    = &loc_prepare_block;
      ret->base.process_replace  = &loc_process_replace;
      ret->base.plugin_exit      = &loc_plugin_exit;
   }

   return &ret->base;
}
}
