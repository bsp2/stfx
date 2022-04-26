// ----
// ---- file   : mb_rng_convolve.cpp
// ---- author : mb (paper) + bsp (voice plugin implementation)
// ---- legal  : Distributed under terms of the MIT license (https://opensource.org/licenses/MIT)
// ----          Copyright 2020 by mb (paper) + bsp (voice plugin implementation)
// ----
// ----          Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
// ----          associated documentation files (the "Software"), to deal in the Software without restriction, including 
// ----          without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
// ----          copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to 
// ----          the following conditions:
// ----  
// ----          The above copyright notice and this permission notice shall be included in all copies or substantial 
// ----          portions of the Software.
// ----
// ----          THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// ----          NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// ----          IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// ----          WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
// ----          SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ----
// ---- info   : MB's RNG convolver (see https://gearspace.com/board/attachments/electronic-music-instruments-and-electronic-music-production/1014442d1650910676-novel-old-1970s-style-anolge-physical-modeling-efficient-real-time-pulse-convolution-synthesis.pdf)
// ---- created: 26Apr2022
// ---- changed: 
// ----
// ----
// ----

#include "../../../plugin.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "../../../plugin.h"

typedef struct lfsr_s {
   unsigned int state;
} lfsr_t;

static int lfsr_rands(lfsr_t *_lfsr) {
   unsigned int state = _lfsr->state;
   state ^= (state >> 7);
   state ^= (state << 9);
   state ^= (state >> 13);
   _lfsr->state = state;
   return state;
}

static float lfsr_randf(lfsr_t *_lfsr, float _max){
   unsigned short r = lfsr_rands(_lfsr) & 65535u;
   return (_max * r) / 65535.0f;  // * (1.0 / 65535.0)
}

static void lfsr_init(lfsr_t *_lfsr, unsigned int _seed, unsigned int _numPre) {
   _lfsr->state = _seed;
   for(unsigned int i = 0u; i < _numPre; i++)
      lfsr_rands(_lfsr);
   // printf("xxx lfsr_init: state=0x%08x\n", _lfsr->state);
   // fflush(stdout);
}


#define PARAM_SEED1     0
#define PARAM_SEED1SCL  1
#define PARAM_SEED2     2
#define PARAM_SEED2SCL  3
#define PARAM_SEED3     4
#define PARAM_SEED3SCL  5
#define PARAM_SCL1      6
#define PARAM_SCL2      7
#define NUM_PARAMS      8
static const char *loc_param_names[NUM_PARAMS] = {
   "Seed 1",
   "Seed 1 Scl",
   "Seed 2",
   "Seed 2 Scl",
   "Seed 3",
   "Seed 3 Scl",
   "Scl 1",
   "Scl 2",  
};
static float loc_param_resets[NUM_PARAMS] = {
   0.1f,  // SEED1
   0.7f,  // SEED1SCL
   0.2f,  // SEED2
   0.7f,  // SEED2SCL
   0.3f,  // SEED3
   0.6f,  // SEED3SCL
   0.6f,  // SCL1
   0.6f,  // SCL2
};

#define MOD_SEED1    0
#define MOD_SEED1SCL 1
#define MOD_SEED2    2
#define MOD_SEED2SCL 3
#define MOD_SEED3    4
#define MOD_SEED3SCL 5
#define MOD_SCL1     6
#define MOD_SCL2     7
#define NUM_MODS     8
static const char *loc_mod_names[NUM_MODS] = {
   "Seed 1",
   "Seed 1 Scl",
   "Seed 2",
   "Seed 2 Scl",
   "Seed 3",
   "Seed 3 Scl",
   "Scl 1",
   "Scl 2",  
};

typedef struct mb_rng_convolve_info_s {
   st_plugin_info_t base;
} mb_rng_convolve_info_t;

typedef struct mb_rng_convolve_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} mb_rng_convolve_shared_t;

typedef struct mb_rng_convolve_voice_s {
   st_plugin_voice_t base;
   float             sample_rate;
   float             mods[NUM_MODS];
   lfsr_t            lfsr_1;
   lfsr_t            lfsr_2;
   lfsr_t            lfsr_3;
   unsigned int      cycle_idx;
   unsigned int      cycle_len;
   float             mod_seed1_cur;
   float             mod_seed1_inc;
   float             mod_seed2_cur;
   float             mod_seed2_inc;
   float             mod_seed3_cur;
   float             mod_seed3_inc;
   float             mod_seed1scl_cur;
   float             mod_seed1scl_inc;
   float             mod_seed2scl_cur;
   float             mod_seed2scl_inc;
   float             mod_seed3scl_cur;
   float             mod_seed3scl_inc;
   float             mod_scl1_cur;
   float             mod_scl1_inc;
   float             mod_scl2_cur;
   float             mod_scl2_inc;
} mb_rng_convolve_voice_t;

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
   ST_PLUGIN_SHARED_CAST(mb_rng_convolve_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(mb_rng_convolve_shared_t);
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
   ST_PLUGIN_VOICE_CAST(mb_rng_convolve_voice_t);
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(mb_rng_convolve_voice_t);
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
   ST_PLUGIN_VOICE_CAST(mb_rng_convolve_voice_t);
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
   ST_PLUGIN_VOICE_CAST(mb_rng_convolve_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(mb_rng_convolve_shared_t);
   (void)_note;
   (void)_vol;
   (void)_pan;

   voice->cycle_len = (unsigned int)(voice->sample_rate / _freqHz);

   float modSeed1 = shared->params[PARAM_SEED1] + voice->mods[MOD_SEED1];
   modSeed1 = Dstplugin_clamp(modSeed1, 0.0f, 1.0f);

   float modSeed2 = shared->params[PARAM_SEED2] + voice->mods[MOD_SEED2];
   modSeed2 = Dstplugin_clamp(modSeed2, 0.0f, 1.0f);

   float modSeed3 = shared->params[PARAM_SEED3] + voice->mods[MOD_SEED3];
   modSeed3 = Dstplugin_clamp(modSeed3, 0.0f, 1.0f);

   float modSeed1Scl = ((shared->params[PARAM_SEED1SCL] - 0.5f) * 2.0f) + voice->mods[MOD_SEED1SCL];
   float modSeed2Scl = ((shared->params[PARAM_SEED2SCL] - 0.5f) * 2.0f) + voice->mods[MOD_SEED2SCL];
   float modSeed3Scl = ((shared->params[PARAM_SEED3SCL] - 0.5f) * 2.0f) + voice->mods[MOD_SEED3SCL];

   float modScl1 = ((shared->params[PARAM_SCL1] - 0.5f) * 2.0f) + voice->mods[MOD_SCL1];
   float modScl2 = ((shared->params[PARAM_SCL2] - 0.5f) * 2.0f) + voice->mods[MOD_SCL2];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_seed1_inc    = (modSeed1 - voice->mod_seed1_cur) * recBlockSize;
      voice->mod_seed2_inc    = (modSeed2 - voice->mod_seed2_cur) * recBlockSize;
      voice->mod_seed3_inc    = (modSeed3 - voice->mod_seed3_cur) * recBlockSize;
      voice->mod_seed1scl_inc = (modSeed1Scl - voice->mod_seed1scl_cur) * recBlockSize;
      voice->mod_seed2scl_inc = (modSeed2Scl - voice->mod_seed2scl_cur) * recBlockSize;
      voice->mod_seed3scl_inc = (modSeed3Scl - voice->mod_seed3scl_cur) * recBlockSize;
      voice->mod_scl1_inc     = (modScl1 - voice->mod_scl1_cur) * recBlockSize;
      voice->mod_scl2_inc     = (modScl2 - voice->mod_scl2_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_seed1_cur    = modSeed1;
      voice->mod_seed1_inc    = 0.0f;
      voice->mod_seed2_cur    = modSeed2;
      voice->mod_seed2_inc    = 0.0f;
      voice->mod_seed3_cur    = modSeed3;
      voice->mod_seed3_inc    = 0.0f;
      voice->mod_seed1scl_cur = modSeed1Scl;
      voice->mod_seed1scl_inc = 0.0f;
      voice->mod_seed2scl_cur = modSeed2Scl;
      voice->mod_seed2scl_inc = 0.0f;
      voice->mod_seed3scl_cur = modSeed3Scl;
      voice->mod_seed3scl_inc = 0.0f;
      voice->mod_scl1_cur = modScl1;
      voice->mod_scl1_inc = 0.0f;
      voice->mod_scl2_cur = modScl2;
      voice->mod_scl2_inc = 0.0f;

      voice->cycle_idx = 0u;  // Oscillator reset
      // printf("xxx freqHz=%f sr=%f cycle_len=%u\n", _freqHz, voice->sample_rate, voice->cycle_len);
      // fflush(stdout);
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

   ST_PLUGIN_VOICE_CAST(mb_rng_convolve_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(mb_rng_convolve_shared_t);

   // Mono output (replicate left to right channel)
   unsigned int k = 0u;
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      float out;

      if(0u == (voice->cycle_idx % voice->cycle_len))
      {
         unsigned int seed;
         voice->cycle_idx = 0u;

         seed = 1u + (unsigned int) (voice->mod_seed1_cur * 0x7fffFFFe);
         lfsr_init(&voice->lfsr_1, seed, 8u/*numPre*/);

         seed = 1u + (unsigned int) (voice->mod_seed2_cur * 0x7fffFFFe);
         lfsr_init(&voice->lfsr_2, seed, 8u/*numPre*/);

         seed = 1u + (unsigned int) (voice->mod_seed3_cur * 0x7fffFFFe);
         lfsr_init(&voice->lfsr_3, seed, 8u/*numPre*/);
      }

      float r1 = (lfsr_randf(&voice->lfsr_1, 2.0f/*max*/) - 1.0f) * voice->mod_seed1scl_cur;
      float r2 = (lfsr_randf(&voice->lfsr_2, 2.0f/*max*/) - 1.0f) * voice->mod_seed2scl_cur;
      float r3 = (lfsr_randf(&voice->lfsr_3, 2.0f/*max*/) - 1.0f) * voice->mod_seed3scl_cur;

      float r1mr2 = (r1 - r2) * voice->mod_scl1_cur;
      float r2mr3 = (r2 - r3) * voice->mod_scl2_cur;

      out = r2mr3 - r1mr2;

      if(out > 1.0f)
         out = 1.0f;
      else if(out < -1.0f)
         out = -1.0f;

      _samplesOut[k]      = out;
      _samplesOut[k + 1u] = out;

      // Next frame
      k += 2u;
      voice->mod_seed1_cur    += voice->mod_seed1_inc;
      voice->mod_seed2_cur    += voice->mod_seed2_inc;
      voice->mod_seed3_cur    += voice->mod_seed3_inc;
      voice->mod_seed1scl_cur += voice->mod_seed1scl_inc;
      voice->mod_seed2scl_cur += voice->mod_seed2scl_inc;
      voice->mod_seed3scl_cur += voice->mod_seed3scl_inc;
      voice->mod_scl1_cur     += voice->mod_scl1_inc;
      voice->mod_scl2_cur     += voice->mod_scl2_inc;
      voice->cycle_idx++;
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   mb_rng_convolve_shared_t *ret = (mb_rng_convolve_shared_t *)malloc(sizeof(mb_rng_convolve_shared_t));
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
   mb_rng_convolve_voice_t *ret = (mb_rng_convolve_voice_t *)malloc(sizeof(mb_rng_convolve_voice_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info = _info;
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(mb_rng_convolve_voice_t);

   free((void*)_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {
   free((void*)_info);
}

extern "C" {
st_plugin_info_t *mb_rng_convolve_init(void) {
   mb_rng_convolve_info_t *ret = (mb_rng_convolve_info_t *)malloc(sizeof(mb_rng_convolve_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "mb rng convolve";  // unique id. don't change this in future builds.
      ret->base.author      = "mb + bsp";
      ret->base.name        = "MB RNG Convolve";
      ret->base.short_name  = "MB RNG";
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
   }

   return &ret->base;
}

ST_PLUGIN_APICALL st_plugin_info_t *ST_PLUGIN_API st_plugin_init(unsigned int _pluginIdx) {
   switch(_pluginIdx)
   {
      case 0u:
         return mb_rng_convolve_init();
   }
   return NULL;
}

} // extern "C"
