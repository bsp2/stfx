// ----
// ---- file   : rms.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020-2024 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : RMS envelope generator
// ----
// ---- created: 09Jun2020
// ---- changed: 21Jan2024
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
#define PARAM_DRIVE    1
#define PARAM_RISE     2
#define PARAM_FALL     3
#define PARAM_HPF      4
#define PARAM_WINSIZE  5
#define NUM_PARAMS     6
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Drive",
   "Rise",
   "Fall",
   "HPF",
   "Window"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,    // DRYWET
   0.5f,    // DRIVE
   0.5f,    // RISE
   0.5f,    // FALL
   0.0f,    // HPF
   0.5f,    // WIN
};

#define MOD_DRYWET   0
#define MOD_DRIVE    1
#define MOD_RISE     2
#define MOD_FALL     3
#define MOD_HPF      4
#define NUM_MODS     5
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Drive",
   "Rise",
   "Fall",
   "HPF",
};

typedef struct rms_info_s {
   st_plugin_info_t base;
} rms_info_t;

typedef struct rms_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} rms_shared_t;

struct RMS {
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

typedef struct rms_voice_s {
   st_plugin_voice_t base;
   float sample_rate;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_drive_cur;
   float mod_drive_inc;
   float mod_rise_cur;
   float mod_rise_inc;
   float mod_fall_cur;
   float mod_fall_inc;
   RMS   rms_l;
   RMS   rms_r;
} rms_voice_t;


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
   ST_PLUGIN_SHARED_CAST(rms_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(rms_shared_t);
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
   ST_PLUGIN_VOICE_CAST(rms_voice_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
      voice->rms_l.reset();
      voice->rms_r.reset();
   }
}

static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(rms_voice_t);
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(rms_voice_t);
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
   ST_PLUGIN_VOICE_CAST(rms_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(rms_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   unsigned int winSizeBits = (unsigned int)(shared->params[PARAM_WINSIZE] * 11.0f + 0.5f);
   if(winSizeBits != voice->rms_l.win_size_bits)
   {
      voice->rms_l.reset();
      voice->rms_r.reset();
      voice->rms_l.setWinSizeBits(winSizeBits);
      voice->rms_r.setWinSizeBits(winSizeBits);
   }

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modDrive = (shared->params[PARAM_DRIVE]-0.5f)*2.0f + voice->mods[MOD_DRIVE];
   modDrive = powf(10.0f, modDrive * 2.0f);

   float modRise = shared->params[PARAM_RISE] * powf(10.0f, voice->mods[MOD_RISE]);
   modRise = Dstplugin_clamp(modRise, 0.0f, 1.0f);

   float modFall = shared->params[PARAM_FALL] * powf(10.0f, voice->mods[MOD_FALL]);
   modFall = Dstplugin_clamp(modFall, 0.0f, 1.0f);

   float modFreq = _freqHz * powf(10.0f, (shared->params[PARAM_HPF]-0.5f)*2.0f*2.0f);
   if(modFreq >= (voice->sample_rate*0.5f))
      modFreq = 1.0f;
   else
      modFreq /= (voice->sample_rate*0.5f);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_drive_inc  = (modDrive  - voice->mod_drive_cur)  * recBlockSize;
      voice->mod_rise_inc   = (modRise   - voice->mod_rise_cur)   * recBlockSize;
      voice->mod_fall_inc   = (modFall   - voice->mod_fall_cur)   * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      voice->mod_drive_cur  = modDrive;
      voice->mod_drive_inc  = 0.0f;
      voice->mod_rise_cur   = modRise;
      voice->mod_rise_inc   = 0.0f;
      voice->mod_fall_cur   = modFall;
      voice->mod_fall_inc   = 0.0f;

      _numFrames = 1u;
   }

   voice->rms_l.hpf.calcParams(_numFrames,
                               StBiquad::HPF,
                               0.0f/*gainDB*/,
                               modFreq,
                               0.0/*modQ*/
                               );

   voice->rms_r.hpf.calcParams(_numFrames,
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
   ST_PLUGIN_VOICE_CAST(rms_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(rms_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];

         float outL = l * voice->mod_drive_cur;
         outL = voice->rms_l.process(outL, voice->mod_rise_cur, voice->mod_fall_cur);

         outL = l + (outL - l) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outL;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_drive_cur  += voice->mod_drive_inc;
         voice->mod_rise_cur   += voice->mod_rise_inc;
         voice->mod_fall_cur   += voice->mod_fall_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float r = _samplesIn[k + 1u];

         float outL = l * voice->mod_drive_cur;
         outL = voice->rms_l.process(outL, voice->mod_rise_cur, voice->mod_fall_cur);
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

         float outR = r * voice->mod_drive_cur;
         outR = voice->rms_r.process(outR, voice->mod_rise_cur, voice->mod_fall_cur);

         outL = l + (outL - l) * voice->mod_drywet_cur;
         outR = r + (outR - r) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);
         outR = Dstplugin_fix_denorm_32(outR);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_drive_cur  += voice->mod_drive_inc;
         voice->mod_rise_cur   += voice->mod_rise_inc;
         voice->mod_fall_cur   += voice->mod_fall_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   rms_shared_t *ret = (rms_shared_t *)malloc(sizeof(rms_shared_t));
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

static st_plugin_voice_t *ST_PLUGIN_API loc_voice_new(st_plugin_info_t *_info, unsigned int _voiceIdx) {
   (void)_voiceIdx;
   rms_voice_t *ret = (rms_voice_t *)malloc(sizeof(rms_voice_t));
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
st_plugin_info_t *rms_init(void) {
   rms_info_t *ret = (rms_info_t *)malloc(sizeof(rms_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp rms";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "rms";
      ret->base.short_name  = "rms";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
      ret->base.category    = ST_PLUGIN_CAT_UTILITY;
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
