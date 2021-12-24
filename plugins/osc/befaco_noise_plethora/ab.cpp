// ----
// ---- file   : ab.cpp
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
// ---- info   : Befaco Noise Plethora osc (section A/B)
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
#include "noise-plethora/plugins/ProgramSelector.hpp" 

#define NUM_BANKS     ( 3u)
#define NUM_PROGRAMS  (10u)  // per bank

#define PARAM_BANK      0
#define PARAM_PROGRAM   1
#define PARAM_X         2
#define PARAM_Y         3
#define NUM_PARAMS      4
static const char *loc_param_names[NUM_PARAMS] = {
   "Bank",
   "Program",
   "X",
   "Y",  
};
static float loc_param_resets[NUM_PARAMS] = {
   0.0f,  // BANK (0..1 => 0..2 (A/B/C)
   0.0f,  // PROGRAM (0..1 => 0..9)
   0.0f,  // CV X
   0.0f,  // CV Y
};

#define MOD_BANK     0
#define MOD_PROGRAM  1
#define MOD_X        2
#define MOD_Y        3
#define NUM_MODS     4
static const char *loc_mod_names[NUM_MODS] = {
   "Bank",
   "Program",
   "X",
   "Y",
};

typedef struct noiseplethora_ab_info_s {
   st_plugin_info_t base;
} noiseplethora_ab_info_t;

typedef struct noiseplethora_ab_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} noiseplethora_ab_shared_t;

typedef struct noiseplethora_ab_voice_s {
   st_plugin_voice_t base;
   float            sample_rate;
   float            mods[NUM_MODS];
   unsigned int     bank;     // 0..2 => A/B/C
   unsigned int     program;  // 0..9
   float            mod_x_cur;
   float            mod_x_inc;
   float            mod_y_cur;
   float            mod_y_inc;
   std::shared_ptr<NoisePlethoraPlugin> np_algorithm[NUM_BANKS][NUM_PROGRAMS];
} noiseplethora_ab_voice_t;

float teensy_sample_rate;  // used by noise-plethora/teensy/

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
   ST_PLUGIN_SHARED_CAST(noiseplethora_ab_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(noiseplethora_ab_shared_t);
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
   ST_PLUGIN_VOICE_CAST(noiseplethora_ab_voice_t);
   voice->sample_rate = teensy_sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(noiseplethora_ab_voice_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(noiseplethora_ab_voice_t);
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
   ST_PLUGIN_VOICE_CAST(noiseplethora_ab_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(noiseplethora_ab_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modBank = shared->params[PARAM_BANK] + voice->mods[MOD_BANK];
   modBank = Dstplugin_clamp(modBank, 0.0f, 1.0f);
   voice->bank = (unsigned int) (modBank * NUM_BANKS);
   if(voice->bank > (NUM_BANKS - 1u))
      voice->bank = (NUM_BANKS - 1u);

   float modProgram = shared->params[PARAM_PROGRAM] + voice->mods[MOD_PROGRAM];
   modProgram = Dstplugin_clamp(modProgram, 0.0f, 1.0f);
   voice->program = (unsigned int) (modProgram * NUM_PROGRAMS);
   if(voice->program > (NUM_PROGRAMS - 1u))
      voice->program = (NUM_PROGRAMS - 1u);

   float modX = shared->params[PARAM_X] + voice->mods[MOD_X];
   modX = Dstplugin_clamp(modX, 0.0f, 1.0f);

   float modY = shared->params[PARAM_Y] + voice->mods[MOD_Y];
   modY = Dstplugin_clamp(modY, 0.0f, 1.0f);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_x_inc = (modX - voice->mod_x_cur) * recBlockSize;
      voice->mod_y_inc = (modY - voice->mod_y_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_x_cur = modX;
      voice->mod_x_inc = 0.0f;
      voice->mod_y_cur = modY;
      voice->mod_y_inc = 0.0f;
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

   ST_PLUGIN_VOICE_CAST(noiseplethora_ab_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(noiseplethora_ab_shared_t);

   NoisePlethoraPlugin *np = voice->np_algorithm[voice->bank][voice->program].get();

   if(NULL != np)
   {
      // Mono output (replicate left to right channel)
      unsigned int k = 0u;
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         np->process(voice->mod_x_cur, voice->mod_y_cur);
         float out = np->processGraph();

         _samplesOut[k]      = out;
         _samplesOut[k + 1u] = out;

         // Next frame
         k += 2u;
         voice->mod_x_cur += voice->mod_x_inc;
         voice->mod_y_cur += voice->mod_y_inc;
      }
   }
   else
   {
      // plugin instantiation failed, output silence
      ::memset((void*)_samplesOut, 0, sizeof(float) * 2 * _numFrames);
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   noiseplethora_ab_shared_t *ret = (noiseplethora_ab_shared_t *)malloc(sizeof(noiseplethora_ab_shared_t));
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
   noiseplethora_ab_voice_t *ret = (noiseplethora_ab_voice_t *)malloc(sizeof(noiseplethora_ab_voice_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info = _info;

      // (note) instantiate all programs so the bank/program number can be modulated
      for(unsigned int bankIdx = 0u; bankIdx < NUM_BANKS; bankIdx++)
      {
         for(unsigned int programIdx = 0u; programIdx < NUM_PROGRAMS; programIdx++)
         {
            const std::string programName = getBankForIndex(bankIdx).getProgramName(programIdx);
            ret->np_algorithm[bankIdx][programIdx] = MyFactory::Instance()->Create(programName);
            NoisePlethoraPlugin *np = ret->np_algorithm[bankIdx][programIdx].get();

            if(NULL != np)
            {
               np->init();
            }
            else
            {
               printf("[---] befaco_noise_plethora::voice_new: failed to instantiate algorithm %u:%u (\"%s\")\n", bankIdx, programIdx, programName.c_str());
            }
         }
      }
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(noiseplethora_ab_voice_t);

   for(unsigned int bankIdx = 0u; bankIdx < NUM_BANKS; bankIdx++)
   {
      for(unsigned int programIdx = 0u; programIdx < NUM_PROGRAMS; programIdx++)
      {
         voice->np_algorithm[bankIdx][programIdx].~shared_ptr<NoisePlethoraPlugin>();
      }
   }   

   free((void*)_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {
   free((void*)_info);
}

extern "C" {
st_plugin_info_t *noiseplethora_ab_init(void) {
   noiseplethora_ab_info_t *ret = (noiseplethora_ab_info_t *)malloc(sizeof(noiseplethora_ab_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "noise plethora a/b";  // unique id. don't change this in future builds.
      ret->base.author      = "Befaco";
      ret->base.name        = "Noise Plethora A/B";
      ret->base.short_name  = "Noise Plethora A/B";
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
