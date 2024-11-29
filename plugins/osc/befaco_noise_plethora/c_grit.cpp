// ----
// ---- file   : c_white.cpp
// ---- author : Befaco & Ewan Hemingway & bsp
// ---- legal  : (c) 2021 Befaco
// ---- license: GPLv3
//
//        Copyright (c) 2021 Befaco
//
//            This program is free software: you can redistribute it and/or modify
//            it under the terms of the GNU General Public License as published by
//            the Free Software Foundation, either version 3 of the License, or
//            (at your option) any later version.
//        
//            This program is distributed in the hope that it will be useful,
//            but WITHOUT ANY WARRANTY; without even the implied warranty of
//            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//            GNU General Public License for more details.
//        
//            You should have received a copy of the GNU General Public License
//            along with this program.  If not, see <https://www.gnu.org/licenses/>. 
// ----
// ---- info   : Befaco Noise Plethora osc (section C / white noise)
// ----           (adapted from https://github.com/hemmer/Befaco for 'stfx' plugin API by bsp)
// ----
// ---- created: 05Dec2021
// ---- changed: 21Jan2024
// ----
// ----
// ----

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "../../../plugin.h"

#include "noise-plethora/random.h"
#include "noise-plethora/plugins/NoisePlethoraPlugin.hpp"
#include "noise-plethora/svf.h"

#define PARAM_MODE_1    0   // lp,hp,bp
#define PARAM_POLES_1   1   // <0.5: 2 poles, >=0.5: 4 poles
#define PARAM_FREQ_1    2
#define PARAM_RESO_1    3
#define PARAM_DENSITY   4   // lp,hp,bp
#define NUM_PARAMS      5
static const char *loc_param_names[NUM_PARAMS] = {
   "Mode",
   "Poles",
   "Freq",
   "Reso",
   "Density",
};
static float loc_param_resets[NUM_PARAMS] = {
   0.0f,  // Mode 1
   0.0f,  // Poles 1
   1.0f,  // Freq 1
   0.0f,  // Reso 1
   0.5f,  // Density
};

#define MOD_FREQ_1   0
#define MOD_RESO_1   1
#define MOD_DENSITY  2
#define NUM_MODS     3
static const char *loc_mod_names[NUM_MODS] = {
   "Freq",
   "Reso",
   "Density",
};

typedef struct noiseplethora_cgrit_info_s {
   st_plugin_info_t base;
} noiseplethora_cgrit_info_t;

typedef struct noiseplethora_cgrit_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
   
} noiseplethora_cgrit_shared_t;

typedef struct noiseplethora_cgrit_voice_s {
   st_plugin_voice_t base;
   float            sample_rate;
   float            mods[NUM_MODS];
   FilterMode       svf_mode_1;  // 0=lp, 1=hp, 2=bp
   FilterMode       svf_mode_2;  // 0=lp, 1=hp, 2=bp
   float            mod_freq1_cur;
   float            mod_freq1_inc;
   float            mod_reso1_cur;
   float            mod_reso1_inc;
   float            mod_density_cur;
   float            mod_density_inc;
   AudioSynthNoiseGritFloat *grit_noise;
   StateVariableFilter2ndOrder *svf_1a;  // SVF 1
   StateVariableFilter2ndOrder *svf_1b;
} noiseplethora_cgrit_voice_t;

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
   ST_PLUGIN_SHARED_CAST(noiseplethora_cgrit_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(noiseplethora_cgrit_shared_t);
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
   ST_PLUGIN_VOICE_CAST(noiseplethora_cgrit_voice_t);
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(noiseplethora_cgrit_voice_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));

      voice->svf_1a->reset();
      voice->svf_1b->reset();
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(noiseplethora_cgrit_voice_t);
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
   ST_PLUGIN_VOICE_CAST(noiseplethora_cgrit_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(noiseplethora_cgrit_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   // Filter 1
   float modMode1 = shared->params[PARAM_MODE_1];
   voice->svf_mode_1 = (FilterMode)int(modMode1 * 3.0f);
   if(voice->svf_mode_1 > 2)
      voice->svf_mode_1 = BANDPASS/*2*/;

   float modFreq1 = shared->params[PARAM_FREQ_1] + voice->mods[MOD_FREQ_1];
   modFreq1 = Dstplugin_clamp(modFreq1, 0.0f, 1.0f);
   modFreq1 = (powf(2.0f, modFreq1 * 7.0f) - 1.0f) * (0.499f / 127.0f);

   float modReso1 = shared->params[PARAM_RESO_1] + voice->mods[MOD_RESO_1];
   modReso1 = Dstplugin_clamp(modReso1, 0.0f, 1.0f);
   modReso1 = Dstplugin_scale(modReso1, 1, 10);

   float modDensity = shared->params[PARAM_DENSITY] + voice->mods[MOD_DENSITY];
   modDensity = Dstplugin_clamp(modDensity, 0.0f, 1.0f);
   modDensity = Dstplugin_scale(modDensity, 0.1f, 20000.0f);  // (todo) ?scale to actual samplerate?

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_freq1_inc   = (modFreq1   - voice->mod_freq1_cur)   * recBlockSize;
      voice->mod_reso1_inc   = (modReso1   - voice->mod_reso1_cur)   * recBlockSize;
      voice->mod_density_inc = (modDensity - voice->mod_density_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_freq1_cur   = modFreq1;
      voice->mod_freq1_inc   = 0.0f;
      voice->mod_reso1_cur   = modReso1;
      voice->mod_reso1_inc   = 0.0f;
      voice->mod_density_cur = modDensity;
      voice->mod_density_inc = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   (void)_bMonoIn;
   (void)_samplesIn;

   ST_PLUGIN_VOICE_CAST(noiseplethora_cgrit_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(noiseplethora_cgrit_shared_t);

   // Mono output (replicate left to right channel)
   unsigned int k = 0u;
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      voice->grit_noise->setDensity(voice->mod_density_cur);
      float out = voice->grit_noise->process(1.0f / voice->sample_rate);

#if 1
      // Filter 1        
      if(shared->params[PARAM_POLES_1] >= 0.5f)
      {
         // 4pole
         float outOld = out;
         voice->svf_1a->setParameters(voice->mod_freq1_cur, 1.0f);
         voice->svf_1a->process(out);
         out = voice->svf_1a->output(voice->svf_mode_1);
         // printf("xxx outOld=%f outFlt=%f  freq1_cur=%f\n", outOld, out, voice->mod_freq1_cur);

         voice->svf_1b->setParameters(voice->mod_freq1_cur, voice->mod_reso1_cur);
         voice->svf_1b->process(out);
         out = voice->svf_1b->output(voice->svf_mode_1);
      }
      else
      {
         // 2pole
         voice->svf_1a->setParameters(voice->mod_freq1_cur, voice->mod_reso1_cur);
         voice->svf_1a->process(out);
         out = voice->svf_1a->output(voice->svf_mode_1);
      }

#endif

      _samplesOut[k]      = out;
      _samplesOut[k + 1u] = out;

      // Next frame
      k += 2u;
      voice->mod_freq1_cur   += voice->mod_freq1_inc;
      voice->mod_reso1_cur   += voice->mod_reso1_inc;
      voice->mod_density_cur += voice->mod_density_inc;
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   noiseplethora_cgrit_shared_t *ret = (noiseplethora_cgrit_shared_t *)malloc(sizeof(noiseplethora_cgrit_shared_t));
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
   noiseplethora_cgrit_voice_t *ret = (noiseplethora_cgrit_voice_t *)malloc(sizeof(noiseplethora_cgrit_voice_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info = _info;

      ret->grit_noise = new(std::nothrow) AudioSynthNoiseGritFloat();
      ret->svf_1a = new(std::nothrow) StateVariableFilter2ndOrder();
      ret->svf_1b = new(std::nothrow) StateVariableFilter2ndOrder();
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(noiseplethora_cgrit_voice_t);

   delete voice->grit_noise;
   voice->grit_noise = NULL;

   delete voice->svf_1a;
   voice->svf_1a = NULL;

   delete voice->svf_1b;
   voice->svf_1b = NULL;

   free((void*)_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {

   free((void*)_info);
}

extern "C" {
st_plugin_info_t *noiseplethora_cgrit_init(void) {
   noiseplethora_cgrit_info_t *ret = (noiseplethora_cgrit_info_t *)malloc(sizeof(noiseplethora_cgrit_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "noise plethora c.grit";  // unique id. don't change this in future builds.
      ret->base.author      = "Befaco";
      ret->base.name        = "Noise Plethora C.Grit";
      ret->base.short_name  = "Noise Plethora C.Grit";
      ret->base.flags       = ST_PLUGIN_FLAG_OSC;
      ret->base.category    = ST_PLUGIN_CAT_UNKNOWN;
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

      randomInit();
   }

   return &ret->base;
}
}
