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
// ---- changed: 
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
#define PARAM_MODE_2    4   // lp,hp,bp
#define PARAM_POLES_2   5   // <0.5: 2 poles, >=0.5: 4 poles
#define PARAM_FREQ_2    6
#define PARAM_RESO_2    7
#define NUM_PARAMS      8
static const char *loc_param_names[NUM_PARAMS] = {
   "Mode 1",
   "Poles 1",
   "Freq 1",
   "Reso 1",
   "Mode 2",
   "Poles 2",
   "Freq 2",
   "Reso 2",
};
static float loc_param_resets[NUM_PARAMS] = {
   0.0f,  // Mode 1
   0.0f,  // Poles 1
   1.0f,  // Freq 1
   0.0f,  // Reso 1
   0.0f,  // Mode 2
   0.0f,  // Poles 2
   1.0f,  // Freq 2
   0.0f,  // Reso 2
};

#define MOD_FREQ_1   0
#define MOD_RESO_1   1
#define MOD_FREQ_2   2
#define MOD_RESO_2   3
#define NUM_MODS     4
static const char *loc_mod_names[NUM_MODS] = {
   "Freq 1",
   "Reso 1",
   "Freq 2",
   "Reso 2",
};

typedef struct noiseplethora_cwhite_info_s {
   st_plugin_info_t base;
} noiseplethora_cwhite_info_t;

typedef struct noiseplethora_cwhite_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
   
} noiseplethora_cwhite_shared_t;

typedef struct noiseplethora_cwhite_voice_s {
   st_plugin_voice_t base;
   float            mods[NUM_MODS];
   FilterMode       svf_mode_1;  // 0=lp, 1=hp, 2=bp
   FilterMode       svf_mode_2;  // 0=lp, 1=hp, 2=bp
   float            mod_freq1_cur;
   float            mod_freq1_inc;
   float            mod_reso1_cur;
   float            mod_reso1_inc;
   float            mod_freq2_cur;
   float            mod_freq2_inc;
   float            mod_reso2_cur;
   float            mod_reso2_inc;
   AudioSynthNoiseWhiteFloat *white_noise;
   StateVariableFilter2ndOrder *svf_1a;  // SVF 1
   StateVariableFilter2ndOrder *svf_1b;
   StateVariableFilter2ndOrder *svf_2a;  // SVF 2
   StateVariableFilter2ndOrder *svf_2b;
} noiseplethora_cwhite_voice_t;

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
   ST_PLUGIN_SHARED_CAST(noiseplethora_cwhite_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(noiseplethora_cwhite_shared_t);
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
   ST_PLUGIN_VOICE_CAST(noiseplethora_cwhite_voice_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));

      voice->svf_1a->reset();
      voice->svf_1b->reset();
      voice->svf_2a->reset();
      voice->svf_2b->reset();
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(noiseplethora_cwhite_voice_t);
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
   ST_PLUGIN_VOICE_CAST(noiseplethora_cwhite_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(noiseplethora_cwhite_shared_t);
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
   modReso1 = Dstplugin_val_to_range(modReso1, 1, 10);

   // Filter 2
   float modMode2 = shared->params[PARAM_MODE_2];
   voice->svf_mode_2 = (FilterMode)int(modMode2 * 3.0f);
   if(voice->svf_mode_2 > 2)
      voice->svf_mode_2 = BANDPASS/*2*/;

   float modFreq2 = shared->params[PARAM_FREQ_2] + voice->mods[MOD_FREQ_2];
   modFreq2 = Dstplugin_clamp(modFreq2, 0.0f, 1.0f);
   modFreq2 = (powf(2.0f, modFreq2 * 7.0f) - 1.0f) * (0.499f / 127.0f);

   float modReso2 = shared->params[PARAM_RESO_2] + voice->mods[MOD_RESO_2];
   modReso2 = Dstplugin_clamp(modReso2, 0.0f, 1.0f);
   modReso2 = Dstplugin_val_to_range(modReso2, 1, 10);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_freq1_inc = (modFreq1 - voice->mod_freq1_cur) * recBlockSize;
      voice->mod_reso1_inc = (modReso1 - voice->mod_reso1_cur) * recBlockSize;
      voice->mod_freq2_inc = (modFreq2 - voice->mod_freq2_cur) * recBlockSize;
      voice->mod_reso2_inc = (modReso2 - voice->mod_reso2_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_freq1_cur = modFreq1;
      voice->mod_freq1_inc = 0.0f;
      voice->mod_reso1_cur = modReso1;
      voice->mod_reso1_inc = 0.0f;
      voice->mod_freq2_cur = modFreq2;
      voice->mod_freq2_inc = 0.0f;
      voice->mod_reso2_cur = modReso2;
      voice->mod_reso2_inc = 0.0f;
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

   ST_PLUGIN_VOICE_CAST(noiseplethora_cwhite_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(noiseplethora_cwhite_shared_t);

   // Mono output (replicate left to right channel)
   unsigned int k = 0u;
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      float out = voice->white_noise->process();

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

      // Filter 2
      if(shared->params[PARAM_POLES_2] >= 0.5f)
      {
         // 4pole
         voice->svf_2a->setParameters(voice->mod_freq2_cur, 1.0f);
         voice->svf_2a->process(out);
         out = voice->svf_2a->output(voice->svf_mode_2);

         voice->svf_2b->setParameters(voice->mod_freq2_cur, voice->mod_reso2_cur);
         voice->svf_2b->process(out);
         out = voice->svf_2b->output(voice->svf_mode_2);
      }
      else
      {
         // 2pole
         voice->svf_2a->setParameters(voice->mod_freq2_cur, voice->mod_reso2_cur);
         voice->svf_2a->process(out);
         out = voice->svf_2a->output(voice->svf_mode_2);
      }
#endif

      _samplesOut[k]      = out;
      _samplesOut[k + 1u] = out;

      // Next frame
      k += 2u;
      voice->mod_freq1_cur += voice->mod_freq1_inc;
      voice->mod_reso1_cur += voice->mod_reso1_inc;
      voice->mod_freq2_cur += voice->mod_freq2_inc;
      voice->mod_reso2_cur += voice->mod_reso2_inc;
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   noiseplethora_cwhite_shared_t *ret = (noiseplethora_cwhite_shared_t *)malloc(sizeof(noiseplethora_cwhite_shared_t));
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
   noiseplethora_cwhite_voice_t *ret = (noiseplethora_cwhite_voice_t *)malloc(sizeof(noiseplethora_cwhite_voice_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info = _info;

      ret->white_noise = new(std::nothrow) AudioSynthNoiseWhiteFloat();
      ret->svf_1a = new(std::nothrow) StateVariableFilter2ndOrder();
      ret->svf_1b = new(std::nothrow) StateVariableFilter2ndOrder();
      ret->svf_2a = new(std::nothrow) StateVariableFilter2ndOrder();
      ret->svf_2b = new(std::nothrow) StateVariableFilter2ndOrder();
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(noiseplethora_cwhite_voice_t);

   delete voice->white_noise;
   voice->white_noise = NULL;

   delete voice->svf_1a;
   voice->svf_1a = NULL;

   delete voice->svf_1b;
   voice->svf_1b = NULL;

   delete voice->svf_2a;
   voice->svf_2a = NULL;

   delete voice->svf_2b;
   voice->svf_2b = NULL;

   free((void*)_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {

   free((void*)_info);
}

extern "C" {
st_plugin_info_t *noiseplethora_cwhite_init(void) {
   noiseplethora_cwhite_info_t *ret = (noiseplethora_cwhite_info_t *)malloc(sizeof(noiseplethora_cwhite_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "noise plethora c.white";  // unique id. don't change this in future builds.
      ret->base.author      = "Befaco";
      ret->base.name        = "Noise Plethora C.white";
      ret->base.short_name  = "Noise Plethora C.white";
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
