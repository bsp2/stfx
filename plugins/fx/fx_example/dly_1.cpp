// ----
// ---- file   : dly_1.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a tanh waveshaper that supports per-sample-frame parameter interpolation
// ----
// ---- created: 24May2020
// ---- changed: 25May2020
// ----
// ----
// ----

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../plugin.h"

#include "delay.h"

#define PARAM_DRYWET   0
#define PARAM_TIME     1  // 0..1 => 0..~371ms
#define PARAM_FB       2
#define PARAM_SMOOTH   3
#define NUM_PARAMS     4
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Time",
   "Feedback",
   "Mod T Smooth"

};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.0f,  // TIME
   0.5f,  // FB
   0.9f,  // SMOOTH
};

#define MOD_DRYWET  0
#define MOD_TIME    1
#define MOD_FB      2
#define MOD_SMOOTH  3
#define NUM_MODS    4
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Time",
   "Feedback",
   "Mod T Smooth"
};

typedef struct dly_1_info_s {
   st_plugin_info_t base;
} dly_1_info_t;

typedef struct dly_1_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} dly_1_shared_t;

typedef struct dly_1_voice_s {
   st_plugin_voice_t base;
   float   sample_rate;
   float   mods[NUM_MODS];
   float   mod_drywet_cur;
   float   mod_drywet_inc;
   float   mod_time_smooth;
   float   mod_time_cur;
   float   mod_time_inc;
   float   mod_fb_cur;
   float   mod_fb_inc;
   StDelay dly_l;
   StDelay dly_r;
} dly_1_voice_t;


static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(dly_1_voice_t);
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
   ST_PLUGIN_SHARED_CAST(dly_1_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(dly_1_shared_t);
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
                                      unsigned int        _voiceIdx,
                                      unsigned int        _activeNoteIdx,
                                      unsigned char       _note,
                                      float               _noteHz,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(dly_1_voice_t);
   (void)_bGlide;
   (void)_voiceIdx;
   (void)_activeNoteIdx;
   (void)_note;
   (void)_noteHz;
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
   ST_PLUGIN_VOICE_CAST(dly_1_voice_t);
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
   ST_PLUGIN_VOICE_CAST(dly_1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(dly_1_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   double modSmooth = shared->params[PARAM_SMOOTH] + voice->mods[MOD_SMOOTH];
   modSmooth = Dstplugin_clamp(modSmooth, 0.0, 1.0);
   modSmooth  = 1.0 - modSmooth;
   modSmooth *= modSmooth;
   modSmooth *= modSmooth;

   float modTime = powf(10.0f, voice->mods[MOD_TIME]);
   modTime *= shared->params[PARAM_TIME];

   if(_numFrames > 0u)
      voice->mod_time_smooth = (float)(voice->mod_time_smooth + (modTime - voice->mod_time_smooth) * modSmooth);
   else
      voice->mod_time_smooth = modTime;

   modTime = Dstplugin_clamp(voice->mod_time_smooth, 0.0f, 1.0f);
   float maxMs = (float(ST_DELAY_SIZE) / 88.200f);    // maxMs @ 88.2kHz = ~371
   modTime = (modTime * maxMs * (voice->sample_rate / 1000.0f));
   if(modTime >= (float)(ST_DELAY_SIZE - 1u))
      modTime = (float)(ST_DELAY_SIZE - 1u);

   float modFb = (shared->params[PARAM_FB]-0.5f)*2.0f + voice->mods[MOD_FB];
   modFb = Dstplugin_clamp(modFb, -1.0f, 1.0f);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_time_inc   = (modTime   - voice->mod_time_cur)   * recBlockSize;
      voice->mod_fb_inc     = (modFb     - voice->mod_fb_cur)     * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      voice->mod_time_cur   = modTime;
      voice->mod_time_inc   = 0.0f;
      voice->mod_fb_cur     = modFb;
      voice->mod_fb_inc     = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_fx_replace(st_plugin_voice_t  *_voice,
                                                 int                 _bMonoIn,
                                                 const float        *_samplesIn,
                                                 float              *_samplesOut, 
                                                 unsigned int        _numFrames
                                                 ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(dly_1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(dly_1_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         voice->dly_l.push(l, voice->mod_fb_cur);
         float out = voice->dly_l.readLinear(voice->mod_time_cur);
         out = l + (out - l) * voice->mod_drywet_cur;
         _samplesOut[k]      = out;
         _samplesOut[k + 1u] = out;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_time_cur   += voice->mod_time_inc;
         voice->mod_fb_cur     += voice->mod_fb_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         voice->dly_l.push(l, voice->mod_fb_cur);
         float outL = voice->dly_l.readLinear(voice->mod_time_cur);

         float r = _samplesIn[k + 1u];
         voice->dly_r.push(r, voice->mod_fb_cur);
         float outR = voice->dly_r.readLinear(voice->mod_time_cur);

         outL = l + (outL - l) * voice->mod_drywet_cur;
         outR = r + (outR - r) * voice->mod_drywet_cur;

         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_time_cur   += voice->mod_time_inc;
         voice->mod_fb_cur     += voice->mod_fb_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   dly_1_shared_t *ret = (dly_1_shared_t *)malloc(sizeof(dly_1_shared_t));
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

static st_plugin_voice_t *ST_PLUGIN_API loc_voice_new(st_plugin_info_t *_info) {
   dly_1_voice_t *ret = (dly_1_voice_t *)malloc(sizeof(dly_1_voice_t));
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
st_plugin_info_t *dly_1_init(void) {

   dly_1_info_t *ret = (dly_1_info_t *)malloc(sizeof(dly_1_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp dly 1";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "delay 1";
      ret->base.short_name  = "dly 1";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
      ret->base.category    = ST_PLUGIN_CAT_DELAY;
      ret->base.num_params  = NUM_PARAMS;
      ret->base.num_mods    = NUM_MODS;

      ret->base.shared_new         = &loc_shared_new;
      ret->base.shared_delete      = &loc_shared_delete;
      ret->base.voice_new          = &loc_voice_new;
      ret->base.voice_delete       = &loc_voice_delete;
      ret->base.get_param_name     = &loc_get_param_name;
      ret->base.get_param_reset    = &loc_get_param_reset;
      ret->base.get_param_value    = &loc_get_param_value;
      ret->base.set_param_value    = &loc_set_param_value;
      ret->base.get_mod_name       = &loc_get_mod_name;
      ret->base.set_sample_rate    = &loc_set_sample_rate;
      ret->base.note_on            = &loc_note_on;
      ret->base.set_mod_value      = &loc_set_mod_value;
      ret->base.prepare_block      = &loc_prepare_block;
      ret->base.process_fx_replace = &loc_process_fx_replace;
      ret->base.plugin_exit        = &loc_plugin_exit;
   }

   return &ret->base;
}
}
