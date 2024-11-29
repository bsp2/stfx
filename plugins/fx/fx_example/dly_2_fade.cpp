// ----
// ---- file   : dly_2_fade.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020-2024 by Bastian Spiegel.
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a cross feedback delay line
// ----
// ---- created: 24May2020
// ---- changed: 25May2020, 31May2020, 08Jun2020, 11Feb2021, 21Jan2024, 27Sep2024
// ----
// ----
// ----

// must be a power of two
#define FADE_LEN  (256u)

#include <stdio.h>  // snprintf
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../plugin.h"

#include "delay.h"

#define PARAM_DRYWET   0
#define PARAM_TIME_L   1  // 0..1 => 0..~371ms
#define PARAM_FB_L     2
#define PARAM_TIME_R   3  // 0..1 => 0..~371ms
#define PARAM_FB_R     4
#define PARAM_FB_L2R   5
#define PARAM_FB_R2L   6
#define PARAM_SMOOTH   7
#define NUM_PARAMS     8
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Time L",
   "Feedback L",
   "Time R",
   "Feedback R",
   "Feedback L->R",
   "Feedback R->L",
   "Mod T Smooth"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.0f,  // TIME_L
   0.5f,  // FB_L
   0.0f,  // TIME_R
   0.5f,  // FB_R
   0.5f,  // FB_L2R
   0.5f,  // FB_R2L
   0.8f,  // SMOOTH
};

#define MOD_DRYWET   0
#define MOD_TIME     1
#define MOD_FB       2
#define MOD_FB_XAMT  3
#define NUM_MODS     4
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Time",
   "Feedback",
   "X-Fb Amount"
};

typedef struct dly_2_fade_info_s {
   st_plugin_info_t base;
} dly_2_fade_info_t;

typedef struct dly_2_fade_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} dly_2_fade_shared_t;

typedef struct dly_2_fade_voice_s {
   st_plugin_voice_t base;
   float        sample_rate;
   float        mods[NUM_MODS];
   float        mod_drywet_cur;
   float        mod_drywet_inc;
   float        mod_time_l_smooth;
   float        mod_time_r_smooth;
   float        mod_time_l_cur;
   float        mod_time_l_next;
   float        mod_time_l_target;
   float        time_fade_amt;
   unsigned int time_fade_idx;
   float        mod_time_r_cur;
   float        mod_time_r_next;
   float        mod_time_r_target;
   float        mod_fb_l_cur;
   float        mod_fb_l_inc;
   float        mod_fb_r_cur;
   float        mod_fb_r_inc;
   float        mod_fb_l2r_cur;
   float        mod_fb_l2r_inc;
   float        mod_fb_r2l_cur;
   float        mod_fb_r2l_inc;
   StDelay      dly_l;
   StDelay      dly_r;
} dly_2_fade_voice_t;


static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(dly_2_fade_voice_t);
   voice->sample_rate = _sampleRate;
}

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
   ST_PLUGIN_SHARED_CAST(dly_2_fade_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_get_param_value_string(st_plugin_shared_t  *_shared,
                                                     unsigned int         _paramIdx,
                                                     char                *_buf,
                                                     unsigned int         _bufSize
                                                     ) {
   ST_PLUGIN_SHARED_CAST(dly_2_fade_shared_t);
   if(PARAM_TIME_L == _paramIdx || PARAM_TIME_R == _paramIdx)
   {
      const float maxMs = (float(ST_DELAY_SIZE) / 88.200f);    // maxMs @ 88.2kHz = ~371ms
      const float ms = shared->params[_paramIdx] * maxMs;
      snprintf(_buf, _bufSize, "%4.2f ms", ms);
   }
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(dly_2_fade_shared_t);
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
   ST_PLUGIN_VOICE_CAST(dly_2_fade_voice_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
      voice->dly_l.reset();
      voice->dly_r.reset();
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(dly_2_fade_voice_t);
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
   ST_PLUGIN_VOICE_CAST(dly_2_fade_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(dly_2_fade_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET] + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float maxMs = (float(ST_DELAY_SIZE) / 88.200f);    // maxMs @ 88.2kHz = ~371

   double modSmooth = shared->params[PARAM_SMOOTH];
   modSmooth  = 1.0 - modSmooth;
   modSmooth *= modSmooth;
   modSmooth *= modSmooth;

   float modTime = powf(10.0f, voice->mods[MOD_TIME]);
   float modTimeL = shared->params[PARAM_TIME_L] * modTime;
   float modTimeR = shared->params[PARAM_TIME_R] * modTime;

   if(_numFrames > 0u)
   {
      voice->mod_time_l_smooth = (float)(voice->mod_time_l_smooth + (modTimeL - voice->mod_time_l_smooth) * modSmooth);
      voice->mod_time_r_smooth = (float)(voice->mod_time_r_smooth + (modTimeR - voice->mod_time_r_smooth) * modSmooth);
   }
   else
   {
      voice->mod_time_l_smooth = modTimeL;
      voice->mod_time_r_smooth = modTimeR;
   }

   modTimeL = Dstplugin_clamp(voice->mod_time_l_smooth, 0.0f, 1.0f);
   modTimeL = (modTimeL * maxMs * (voice->sample_rate / 1000.0f));
   if(modTimeL >= (float)(ST_DELAY_SIZE - 1u))
      modTimeL = (float)(ST_DELAY_SIZE - 1u);

   modTimeR = Dstplugin_clamp(voice->mod_time_r_smooth, 0.0f, 1.0f);
   modTimeR = (modTimeR * maxMs * (voice->sample_rate / 1000.0f));
   if(modTimeR >= (float)(ST_DELAY_SIZE - 1u))
      modTimeR = (float)(ST_DELAY_SIZE - 1u);

   float modFb = powf(10.0f, voice->mods[MOD_FB]);

   float modFbL = (shared->params[PARAM_FB_L]-0.5f)*2.0f * modFb;
   modFbL = Dstplugin_clamp(modFbL, -1.0f, 1.0f);

   float modFbR = (shared->params[PARAM_FB_R]-0.5f)*2.0f * modFb;
   modFbR = Dstplugin_clamp(modFbR, -1.0f, 1.0f);

   float modFbXAmt = powf(10.0f, voice->mods[MOD_FB_XAMT]);
   float modFbL2R = (shared->params[PARAM_FB_L2R]-0.5f)*2.0f * modFbXAmt;
   modFbL2R = Dstplugin_clamp(modFbL2R, -1.0f, 1.0f);

   float modFbR2L = (shared->params[PARAM_FB_R2L]-0.5f)*2.0f * powf(10.0f, voice->mods[MOD_FB_XAMT]);
   modFbR2L = Dstplugin_clamp(modFbR2L, -1.0f, 1.0f);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc    = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_time_l_target = modTimeL;
      voice->mod_time_r_target = modTimeR;
      voice->mod_fb_l_inc      = (modFbL    - voice->mod_fb_l_cur)   * recBlockSize;
      voice->mod_fb_r_inc      = (modFbR    - voice->mod_fb_r_cur)   * recBlockSize;
      voice->mod_fb_l2r_inc    = (modFbL2R  - voice->mod_fb_l2r_cur) * recBlockSize;
      voice->mod_fb_r2l_inc    = (modFbR2L  - voice->mod_fb_r2l_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur    = modDryWet;
      voice->mod_drywet_inc    = 0.0f;
      voice->mod_time_l_cur    = modTimeL;
      voice->mod_time_l_next   = modTimeL;
      voice->mod_time_l_target = modTimeL;
      voice->time_fade_amt     = 0.0f;
      voice->time_fade_idx     = 0u;
      voice->mod_time_r_cur    = modTimeR;
      voice->mod_time_r_next   = modTimeR;
      voice->mod_time_r_target = modTimeR;
      voice->mod_fb_l_cur      = modFbL;
      voice->mod_fb_l_inc      = 0.0f;
      voice->mod_fb_r_cur      = modFbR;
      voice->mod_fb_r_inc      = 0.0f;
      voice->mod_fb_l2r_cur    = modFbL2R;
      voice->mod_fb_l2r_inc    = 0.0f;
      voice->mod_fb_r2l_cur    = modFbR2L;
      voice->mod_fb_r2l_inc    = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut,
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(dly_2_fade_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(dly_2_fade_shared_t);

   unsigned int k = 0u;
   float c;
   float n;

   // Stereo input, stereo output
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      float l = _samplesIn[k];
      float lastOutL = voice->dly_l.last_out;
      c = voice->dly_l.readLinear(voice->mod_time_l_cur);
      n = voice->dly_l.readLinear(voice->mod_time_l_next);
      float outL = c + (n - c) * voice->time_fade_amt;
      voice->dly_l.pushRaw(l + voice->dly_r.last_out * voice->mod_fb_r2l_cur + outL * voice->mod_fb_l_cur);
      voice->dly_l.last_out = outL;

      float r = _samplesIn[k + 1u];
      c = voice->dly_r.readLinear(voice->mod_time_r_cur);
      n = voice->dly_r.readLinear(voice->mod_time_r_next);
      float outR = c + (n - c) * voice->time_fade_amt;
      voice->dly_r.pushRaw(r + lastOutL * voice->mod_fb_l2r_cur + outR * voice->mod_fb_r_cur);
      voice->dly_l.last_out = outR;

      outL = l + (outL - l) * voice->mod_drywet_cur;
      outR = r + (outR - r) * voice->mod_drywet_cur;

      _samplesOut[k]      = outL;
      _samplesOut[k + 1u] = outR;

      // Next frame
      k += 2u;
      voice->mod_drywet_cur += voice->mod_drywet_inc;
      voice->mod_fb_l_cur   += voice->mod_fb_l_inc;
      voice->mod_fb_r_cur   += voice->mod_fb_r_inc;
      voice->mod_fb_l2r_cur += voice->mod_fb_l2r_inc;
      voice->mod_fb_r2l_cur += voice->mod_fb_r2l_inc;

      if(0u == (++voice->time_fade_idx & (FADE_LEN-1)))
      {
         voice->mod_time_l_cur  = voice->mod_time_l_next;
         voice->mod_time_l_next = voice->mod_time_l_target;
         voice->mod_time_r_cur  = voice->mod_time_r_next;
         voice->mod_time_r_next = voice->mod_time_r_target;
         voice->time_fade_amt = 0.0f;
      }
      else
      {
         voice->time_fade_amt += (1.0f / FADE_LEN);
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   dly_2_fade_shared_t *ret = (dly_2_fade_shared_t *)malloc(sizeof(dly_2_fade_shared_t));
   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));
      ret->base.info  = _info;
      memcpy((void*)ret->params, (void*)loc_param_resets, NUM_PARAMS * sizeof(float));
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_shared_delete(st_plugin_shared_t *_shared) {
   free(_shared);
}

static st_plugin_voice_t *ST_PLUGIN_API loc_voice_new(st_plugin_info_t *_info, unsigned int _voiceIdx) {
   (void)_voiceIdx;
   dly_2_fade_voice_t *ret = (dly_2_fade_voice_t *)malloc(sizeof(dly_2_fade_voice_t));
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
st_plugin_info_t *dly_2_fade_init(void) {

   dly_2_fade_info_t *ret = (dly_2_fade_info_t *)malloc(sizeof(dly_2_fade_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp dly 2 fade";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "delay 2 fade";
      ret->base.short_name  = "dly 2 fd";
      ret->base.flags       = ST_PLUGIN_FLAG_FX | ST_PLUGIN_FLAG_TRUE_STEREO_OUT;
      ret->base.category    = ST_PLUGIN_CAT_DELAY;
      ret->base.num_params  = NUM_PARAMS;
      ret->base.num_mods    = NUM_MODS;

      ret->base.shared_new             = &loc_shared_new;
      ret->base.shared_delete          = &loc_shared_delete;
      ret->base.voice_new              = &loc_voice_new;
      ret->base.voice_delete           = &loc_voice_delete;
      ret->base.get_param_name         = &loc_get_param_name;
      ret->base.get_param_reset        = &loc_get_param_reset;
      ret->base.get_param_value        = &loc_get_param_value;
      ret->base.get_param_value_string = &loc_get_param_value_string;
      ret->base.set_param_value        = &loc_set_param_value;
      ret->base.get_mod_name           = &loc_get_mod_name;
      ret->base.set_sample_rate        = &loc_set_sample_rate;
      ret->base.note_on                = &loc_note_on;
      ret->base.set_mod_value          = &loc_set_mod_value;
      ret->base.prepare_block          = &loc_prepare_block;
      ret->base.process_replace        = &loc_process_replace;
      ret->base.plugin_exit            = &loc_plugin_exit;
   }

   return &ret->base;
}
}
