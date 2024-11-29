// ----
// ---- file   : ws_fold_sine.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020-2024 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a tanh waveshaper that supports per-sample-frame parameter interpolation
// ----
// ---- created: 08Feb2021
// ---- changed: 21Jan2024, 19Sep2024
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET     0
#define PARAM_DRIVE      1
#define PARAM_BIAS       2
#define PARAM_FOLD       3
#define PARAM_THRESHOLD  4
#define PARAM_LEVEL      5
#define NUM_PARAMS       6
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Drive",
   "Bias",
   "Fold",
   "Threshold",
   "Level"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.0f,  // DRIVE
   0.0f,  // BIAS
   0.25f, // FOLD
   0.8f,  // THRESHOLD
   0.0f,  // LEVEL
};

#define MOD_DRYWET     0
#define MOD_DRIVE      1
#define MOD_BIAS       2
#define MOD_FOLD       3
#define MOD_THRESHOLD  4
#define MOD_LEVEL      5
#define NUM_MODS       6
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Drive",
   "Bias",
   "Fold",
   "Threshold",
   "Level"
};

typedef struct ws_fold_sine_info_s {
   st_plugin_info_t base;
} ws_fold_sine_info_t;

typedef struct ws_fold_sine_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ws_fold_sine_shared_t;

typedef struct ws_fold_sine_voice_s {
   st_plugin_voice_t base;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_drive_cur;
   float mod_drive_inc;
   float mod_bias_cur;
   float mod_bias_inc;
   float mod_fold_cur;
   float mod_fold_inc;
   float mod_threshold_cur;
   float mod_threshold_inc;
   float mod_level_cur;
   float mod_level_inc;
} ws_fold_sine_voice_t;


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
   ST_PLUGIN_SHARED_CAST(ws_fold_sine_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ws_fold_sine_shared_t);
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
   ST_PLUGIN_VOICE_CAST(ws_fold_sine_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_fold_sine_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_fold_sine_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_fold_sine_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modDrive = shared->params[PARAM_DRIVE] + voice->mods[MOD_DRIVE];
   modDrive = powf(10.0f, modDrive * 2.0f);

   float modBias = shared->params[PARAM_BIAS];
   modBias = Dstplugin_scale(modBias, 0.0f, 0.3f);
   modBias += voice->mods[MOD_BIAS] * 0.3f;
   modBias = Dstplugin_clamp(modBias, 0.0f, 0.3f);  // <0 and >0.3: heavy distortion (=> clamp)

   float modFold = (shared->params[PARAM_FOLD]-0.25f)*2.0f + voice->mods[MOD_FOLD]*2.0f;
   // float modDrive2 = shared->params[PARAM_DRIVE2] + voice->mods[MOD_DRIVE2];
   modFold = powf(10.0f, modFold * 2.0f);

   float modThreshold = shared->params[PARAM_THRESHOLD] + voice->mods[MOD_THRESHOLD];

   float modLevel = shared->params[PARAM_LEVEL] + voice->mods[MOD_LEVEL];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc    = (modDryWet    - voice->mod_drywet_cur)    * recBlockSize;
      voice->mod_drive_inc     = (modDrive     - voice->mod_drive_cur)     * recBlockSize;
      voice->mod_bias_inc      = (modBias      - voice->mod_bias_cur)      * recBlockSize;
      voice->mod_fold_inc      = (modFold      - voice->mod_fold_cur)      * recBlockSize;
      voice->mod_threshold_inc = (modThreshold - voice->mod_threshold_cur) * recBlockSize;
      voice->mod_level_inc     = (modLevel     - voice->mod_level_cur)     * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur    = modDryWet;
      voice->mod_drywet_inc    = 0.0f;
      voice->mod_drive_cur     = modDrive;
      voice->mod_drive_inc     = 0.0f;
      voice->mod_bias_cur      = modBias;
      voice->mod_bias_inc      = 0.0f;
      voice->mod_fold_cur      = modFold;
      voice->mod_fold_inc      = 0.0f;
      voice->mod_threshold_cur = modThreshold;
      voice->mod_threshold_inc = 0.0f;
      voice->mod_level_cur     = modLevel;
      voice->mod_level_inc     = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   ST_PLUGIN_VOICE_CAST(ws_fold_sine_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_fold_sine_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         union {
            float f;
            unsigned int ui;
         } a;
         a.f = l;
         union {
            float f;
            unsigned int ui;
         } t;
         t.f = a.f;
         t.f *= voice->mod_drive_cur;
         t.f += voice->mod_bias_cur;
         t.ui &= 0x7FFFffffu;
         if(t.f > voice->mod_threshold_cur)
         {
            float p = (t.f - voice->mod_threshold_cur) * voice->mod_fold_cur;
            t.f = voice->mod_threshold_cur - sinf(p) * voice->mod_level_cur;
         }
         t.f -= voice->mod_bias_cur;
         t.ui ^= a.ui & 0x80000000u;
         float out = l + (t.f - l) * voice->mod_drywet_cur;
         out = Dstplugin_fix_denorm_32(out);
         _samplesOut[k]      = out;
         _samplesOut[k + 1u] = out;
         // Next frame
         k += 2u;
         voice->mod_drywet_cur    += voice->mod_drywet_inc;
         voice->mod_drive_cur     += voice->mod_drive_inc;
         voice->mod_bias_cur      += voice->mod_bias_inc;
         voice->mod_threshold_cur += voice->mod_threshold_inc;
         voice->mod_fold_cur      += voice->mod_fold_inc;
         voice->mod_level_cur     += voice->mod_level_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         union {
            float f;
            unsigned int ui;
         } t;

         union {
            float f;
            unsigned int ui;
         } a;

         // left
         float l = _samplesIn[k];
         a.f = l;
         t.f = a.f;
         t.f *= voice->mod_drive_cur;
         t.f += voice->mod_bias_cur;
         t.ui &= 0x7FFFffffu;
         if(t.f > voice->mod_threshold_cur)
         {
            float p = (t.f - voice->mod_threshold_cur) * voice->mod_fold_cur;
            t.f = voice->mod_threshold_cur - sinf(p) * voice->mod_level_cur;
         }
         t.f -= voice->mod_bias_cur;
         t.ui ^= a.ui & 0x80000000u;
         float outL = l + (t.f - l) * voice->mod_drywet_cur;

         // right
         float r = _samplesIn[k + 1];
         a.f = r;
         t.f = a.f;
         t.f *= voice->mod_drive_cur;
         t.f += voice->mod_bias_cur;
         t.ui &= 0x7FFFffffu;
         if(t.f > voice->mod_threshold_cur)
         {
            float p = (t.f - voice->mod_threshold_cur) * voice->mod_fold_cur;
            t.f = voice->mod_threshold_cur - sinf(p) * voice->mod_level_cur;
         }
         t.f -= voice->mod_bias_cur;
         t.ui ^= a.ui & 0x80000000u;
         float outR = r + (t.f - r) * voice->mod_drywet_cur;

         outL = Dstplugin_fix_denorm_32(outL);
         outR = Dstplugin_fix_denorm_32(outR);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;
         // Next frame
         k += 2u;
         voice->mod_drywet_cur    += voice->mod_drywet_inc;
         voice->mod_drive_cur     += voice->mod_drive_inc;
         voice->mod_bias_cur      += voice->mod_bias_inc;
         voice->mod_threshold_cur += voice->mod_threshold_inc;
         voice->mod_fold_cur      += voice->mod_fold_inc;
         voice->mod_level_cur     += voice->mod_level_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ws_fold_sine_shared_t *ret = malloc(sizeof(ws_fold_sine_shared_t));
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
   ws_fold_sine_voice_t *ret = malloc(sizeof(ws_fold_sine_voice_t));
   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));
      ret->base.info   = _info;
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   free(_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {
   free(_info);
}

st_plugin_info_t *ws_fold_sine_init(void) {
   ws_fold_sine_info_t *ret = NULL;

   ret = malloc(sizeof(ws_fold_sine_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ws fold sine";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "fold sine shaper";
      ret->base.short_name  = "fold sine";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
      ret->base.category    = ST_PLUGIN_CAT_WAVESHAPER;
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
   }

   return &ret->base;
}
