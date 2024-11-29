// ----
// ---- file   : ws_tanh_asym.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020-2024 by Bastian Spiegel.
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : an asymetric tanh waveshaper that supports per-sample-frame parameter interpolation
// ----
// ---- created: 10May2009
// ---- changed: 23May2010, 30Sep2010, 07Oct2010, 17May2020, 18May2020, 19May2020, 20May2020
// ----          21May2020, 24May2020, 25May2020, 31May2020, 08Jun2020, 08Aug2021, 21Jan2024
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET      0
#define PARAM_BIAS        1
#define PARAM_DRIVE_M     2
#define PARAM_DRIVE_P     3
#define PARAM_BIAS_PAN    4
#define PARAM_OUTPUT_LVL  5
#define NUM_PARAMS        6
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Bias",
   "Drive -",
   "Drive +",
   "Bias Pan",
   "Output Lvl"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // BIAS
   0.0f,  // DRIVE_M
   0.0f,  // DRIVE_P
   0.5f,  // BIAS_PAN
   1.0f,  // OUTPUT_LVL
};

#define MOD_DRYWET    0
#define MOD_BIAS_PAN  1
#define MOD_DRIVE_M   2
#define MOD_DRIVE_P   3
#define NUM_MODS      4
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Bias Pan",
   "Drive -",
   "Drive +"
};

typedef struct ws_tanh_asym_info_s {
   st_plugin_info_t base;
} ws_tanh_asym_info_t;

typedef struct ws_tanh_asym_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ws_tanh_asym_shared_t;

typedef struct ws_tanh_asym_voice_s {
   st_plugin_voice_t base;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_bias_l_cur;
   float mod_bias_l_inc;
   float mod_bias_r_cur;
   float mod_bias_r_inc;
   float mod_drivem_cur;
   float mod_drivem_inc;
   float mod_drivep_cur;
   float mod_drivep_inc;
} ws_tanh_asym_voice_t;


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
   ST_PLUGIN_SHARED_CAST(ws_tanh_asym_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ws_tanh_asym_shared_t);
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
   ST_PLUGIN_VOICE_CAST(ws_tanh_asym_voice_t);
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
                                            unsigned int       _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(ws_tanh_asym_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_tanh_asym_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_tanh_asym_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modBias = (shared->params[PARAM_BIAS] * 2.0f - 1.0f);
   float modBiasPan = (shared->params[PARAM_BIAS_PAN] - 0.5f) * 2.0f + voice->mods[MOD_BIAS_PAN];
   float modBiasL = modBias * ( (modBiasPan > 0.0f) ? 1.0f : (1.0f + modBiasPan) );
   float modBiasR = modBias * ( (modBiasPan < 0.0f) ? 1.0f : (1.0f - modBiasPan) );

   float modDriveM = shared->params[PARAM_DRIVE_M] + voice->mods[MOD_DRIVE_M];
   modDriveM = powf(10.0f, modDriveM * 2.0f);

   float modDriveP = shared->params[PARAM_DRIVE_P] + voice->mods[MOD_DRIVE_P];
   modDriveP = powf(10.0f, modDriveP * 2.0f);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc  = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_bias_l_inc  = (modBiasL  - voice->mod_bias_l_cur) * recBlockSize;
      voice->mod_bias_r_inc  = (modBiasR  - voice->mod_bias_r_cur) * recBlockSize;
      voice->mod_drivem_inc  = (modDriveM - voice->mod_drivem_cur) * recBlockSize;
      voice->mod_drivep_inc  = (modDriveP - voice->mod_drivep_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      voice->mod_bias_l_cur = modBiasL;
      voice->mod_bias_l_inc = 0.0f;
      voice->mod_bias_r_cur = modBiasR;
      voice->mod_bias_r_inc = 0.0f;
      voice->mod_drivem_cur = modDriveM;
      voice->mod_drivem_inc = 0.0f;
      voice->mod_drivep_cur = modDriveP;
      voice->mod_drivep_inc = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut,
                                              unsigned int        _numFrames
                                              ) {
   ST_PLUGIN_VOICE_CAST(ws_tanh_asym_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_tanh_asym_shared_t);

   unsigned int k = 0u;

   // Stereo input, stereo output
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      float l = _samplesIn[k];
      float r = _samplesIn[k + 1u];

      float outL = l;
      outL += voice->mod_bias_l_cur;
      if(outL < 0.0f)
         outL = tanhf(outL * voice->mod_drivem_cur);
      else
         outL = tanhf(outL * voice->mod_drivep_cur);
      outL -= voice->mod_bias_l_cur;
      outL *= shared->params[PARAM_OUTPUT_LVL];

      float outR = r;
      outR += voice->mod_bias_r_cur;
      if(outR < 0.0f)
         outR = tanhf(outR * voice->mod_drivem_cur);
      else
         outR = tanhf(outR * voice->mod_drivep_cur);
      outR -= voice->mod_bias_r_cur;
      outR *= shared->params[PARAM_OUTPUT_LVL];

      outL = l + (outL - l) * voice->mod_drywet_cur;
      outR = r + (outR - r) * voice->mod_drywet_cur;
      outL = Dstplugin_fix_denorm_32(outL);
      outR = Dstplugin_fix_denorm_32(outR);
      _samplesOut[k]      = outL;
      _samplesOut[k + 1u] = outR;

      // Next frame
      voice->mod_drywet_cur += voice->mod_drywet_inc;
      voice->mod_bias_l_cur += voice->mod_bias_l_inc;
      voice->mod_bias_r_cur += voice->mod_bias_r_inc;
      voice->mod_drivem_cur += voice->mod_drivem_inc;
      voice->mod_drivep_cur += voice->mod_drivep_inc;
      k += 2u;
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ws_tanh_asym_shared_t *ret = malloc(sizeof(ws_tanh_asym_shared_t));
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
   ws_tanh_asym_voice_t *ret = malloc(sizeof(ws_tanh_asym_voice_t));
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

st_plugin_info_t *ws_tanh_asym_init(void) {
   ws_tanh_asym_info_t *ret = NULL;

   ret = malloc(sizeof(ws_tanh_asym_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ws tanh asym";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "tanh asym shaper";
      ret->base.short_name  = "tanh asym";
      ret->base.flags       = ST_PLUGIN_FLAG_FX | ST_PLUGIN_FLAG_TRUE_STEREO_OUT;
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
