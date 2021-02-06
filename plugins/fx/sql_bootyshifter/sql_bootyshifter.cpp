// ----
// ---- file   : sql_bootyshifter.c
// ---- author : squinkylabs & bsp
// ---- legal  : (c) 2018-2020 by squinkylabs
// ----
// Copyright (c) 2018 squinkylabs
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ----
// ---- info   : frequency shifter
// ----           (adapted for 'stfx' plugin API by bsp)
// ----
// ---- created: 07Jun2020
// ---- changed: 08Jun2020
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#include "composites/FrequencyShifter.h"

#define PARAM_DRYWET    0
#define PARAM_FREQ      1
#define PARAM_RANGE     2
#define NUM_PARAMS      3
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Freq",
   "Range"
   
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // FREQ
   0.5f,  // RANGE (0=5Hz, 0.2=50Hz, 0.4=500Hz, 0.6=5kHz, 1=exp)
};

#define MOD_DRYWET  0
#define MOD_FREQ    1
#define NUM_MODS    2
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Freq"
};

typedef struct sql_bootyshifter_info_s {
   st_plugin_info_t base;
} sql_bootyshifter_info_t;

typedef struct sql_bootyshifter_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} sql_bootyshifter_shared_t;

typedef struct sql_bootyshifter_voice_s {
   st_plugin_voice_t base;
   float            sample_rate;
   float            mods[NUM_MODS];
   float            mod_drywet_cur;
   float            mod_drywet_inc;
   float            mod_freq_cur;
   float            mod_freq_inc;
   FrequencyShifter fs_l;
   FrequencyShifter fs_r;
} sql_bootyshifter_voice_t;


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
   ST_PLUGIN_SHARED_CAST(sql_bootyshifter_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(sql_bootyshifter_shared_t);
   shared->params[_paramIdx] = _value;
}

static const char *ST_PLUGIN_API loc_get_mod_name(st_plugin_info_t *_info,
                                                  unsigned int      _modIdx
                                                  ) {
   (void)_info;
   return loc_mod_names[_modIdx];
}

static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(sql_bootyshifter_voice_t);
   voice->sample_rate = _sampleRate;
   voice->fs_l.setSampleRate(_sampleRate);
   voice->fs_r.setSampleRate(_sampleRate);
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(sql_bootyshifter_voice_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
      voice->fs_l.init();
      voice->fs_r.init();
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(sql_bootyshifter_voice_t);
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
   ST_PLUGIN_VOICE_CAST(sql_bootyshifter_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(sql_bootyshifter_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modFreq = (shared->params[PARAM_FREQ]-0.5f)*2.0f + voice->mods[MOD_FREQ];
   modFreq *= 5.0f;

   unsigned int rangeIdx = (unsigned int)(shared->params[PARAM_RANGE] * 4.0f + 0.5f);
   static const float ranges[5] = { 5, 50, 500, 5000, 0/*exp*/ };
   float modRange = ranges[rangeIdx];
   voice->fs_l.freqRange = modRange;
   voice->fs_r.freqRange = modRange;

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_freq_inc   = (modFreq   - voice->mod_freq_cur)   * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      voice->mod_freq_cur   = modFreq;
      voice->mod_freq_inc   = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(sql_bootyshifter_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(sql_bootyshifter_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float outL = voice->fs_l.process(l, voice->mod_freq_cur);
         outL = l + (outL - l) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outL;
         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_freq_cur   += voice->mod_freq_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float r = _samplesIn[k + 1u];
         float outL = voice->fs_l.process(l, voice->mod_freq_cur);
         outL = l + (outL - l) * voice->mod_drywet_cur;
         float outR = voice->fs_r.process(r, voice->mod_freq_cur);
         outR = r + (outR - r) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);
         outR = Dstplugin_fix_denorm_32(outR);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;
         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_freq_cur   += voice->mod_freq_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   sql_bootyshifter_shared_t *ret = (sql_bootyshifter_shared_t *)malloc(sizeof(sql_bootyshifter_shared_t));
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
   sql_bootyshifter_voice_t *ret = (sql_bootyshifter_voice_t *)malloc(sizeof(sql_bootyshifter_voice_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info   = _info;
      new(&ret->fs_l)FrequencyShifter();
      new(&ret->fs_r)FrequencyShifter();
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
st_plugin_info_t *sql_bootyshifter_init(void) {
   sql_bootyshifter_info_t *ret = (sql_bootyshifter_info_t *)malloc(sizeof(sql_bootyshifter_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "sql booty shifter";  // unique id. don't change this in future builds.
      ret->base.author      = "squinkylabs & bsp";
      ret->base.name        = "sql booty shifter";
      ret->base.short_name  = "sql booty shifter";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
      ret->base.category    = ST_PLUGIN_CAT_PITCH;
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

} // extern "C"
