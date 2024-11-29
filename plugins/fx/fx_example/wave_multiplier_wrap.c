// ----
// ---- file   : wave_multiplier_wrap.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2021-2024 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : pseudo (saw) phase shifter
// ----
// ---- created: 13Oct2021
// ---- changed: 21Jan2024
// ----
// ----
// ----

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET   0
#define PARAM_INLVL    1
#define PARAM_SHIFT1   2
#define PARAM_LEVEL1   3
#define PARAM_SHIFT2   4
#define PARAM_LEVEL2   5
#define PARAM_SHIFT3   6
#define PARAM_LEVEL3   7
#define NUM_PARAMS     8
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Input Lvl",
   "Shift 1",
   "Level 1",
   "Shift 2",
   "Level 2",
   "Shift 3",
   "Level 3",
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // INLVL
   0.5f,  // SHIFT1
   0.0f,  // LEVEL1
   0.5f,  // SHIFT2
   0.0f,  // LEVEL2
   0.5f,  // SHIFT3
   0.0f,  // LEVEL3
};

#define MOD_DRYWET  0
#define MOD_INLVL   1
#define MOD_SHIFT1  2
#define MOD_LEVEL1  3
#define MOD_SHIFT2  4
#define MOD_LEVEL2  5
#define MOD_SHIFT3  6
#define MOD_LEVEL3  7
#define NUM_MODS    8
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Input Lvl",
   "Shift 1",
   "Level 1",
   "Shift 2",
   "Level 2",
   "Shift 3",
   "Level 3",
};

typedef struct wave_multiplier_wrap_info_s {
   st_plugin_info_t base;
} wave_multiplier_wrap_info_t;

typedef struct wave_multiplier_wrap_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} wave_multiplier_wrap_shared_t;

typedef struct wave_multiplier_wrap_voice_s {
   st_plugin_voice_t base;
   float    mods[NUM_MODS];
   float    mod_drywet_cur;
   float    mod_drywet_inc;
   float    mod_inlevel_cur;
   float    mod_inlevel_inc;
   float    mod_shift1_cur;
   float    mod_shift1_inc;
   float    mod_level1_cur;
   float    mod_level1_inc;
   float    mod_shift2_cur;
   float    mod_shift2_inc;
   float    mod_level2_cur;
   float    mod_level2_inc;
   float    mod_shift3_cur;
   float    mod_shift3_inc;
   float    mod_level3_cur;
   float    mod_level3_inc;
} wave_multiplier_wrap_voice_t;


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
   ST_PLUGIN_SHARED_CAST(wave_multiplier_wrap_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(wave_multiplier_wrap_shared_t);
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
   ST_PLUGIN_VOICE_CAST(wave_multiplier_wrap_voice_t);
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
   ST_PLUGIN_VOICE_CAST(wave_multiplier_wrap_voice_t);
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
   ST_PLUGIN_VOICE_CAST(wave_multiplier_wrap_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(wave_multiplier_wrap_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modInLevel = ((shared->params[PARAM_INLVL] - 0.5f) * 2.0f) + voice->mods[MOD_INLVL];
   modInLevel = powf(10.0f, modInLevel * 2.0f);

   float modShift1 = (2.0f * shared->params[PARAM_SHIFT1] - 1.0f) + voice->mods[MOD_SHIFT1];
   float modLevel1 = shared->params[PARAM_LEVEL1] + voice->mods[MOD_LEVEL1];
   modLevel1 = Dstplugin_clamp(modLevel1, -1.0f, 1.0f);

   float modShift2 = (2.0f * shared->params[PARAM_SHIFT2] - 1.0f) + voice->mods[MOD_SHIFT2];
   float modLevel2 = shared->params[PARAM_LEVEL2] + voice->mods[MOD_LEVEL2];
   modLevel2 = Dstplugin_clamp(modLevel2, -1.0f, 1.0f);

   float modShift3 = (2.0f * shared->params[PARAM_SHIFT3] - 1.0f) + voice->mods[MOD_SHIFT3];
   float modLevel3 = shared->params[PARAM_LEVEL3] + voice->mods[MOD_LEVEL3];
   modLevel3 = Dstplugin_clamp(modLevel3, -1.0f, 1.0f);


   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc  = (modDryWet  - voice->mod_drywet_cur)  * recBlockSize;
      voice->mod_inlevel_inc = (modInLevel - voice->mod_inlevel_cur) * recBlockSize;
      voice->mod_shift1_inc  = (modShift1  - voice->mod_shift1_cur)  * recBlockSize;
      voice->mod_level1_inc  = (modLevel1  - voice->mod_level1_cur)  * recBlockSize;
      voice->mod_shift2_inc  = (modShift2  - voice->mod_shift2_cur)  * recBlockSize;
      voice->mod_level2_inc  = (modLevel2  - voice->mod_level2_cur)  * recBlockSize;
      voice->mod_shift3_inc  = (modShift3  - voice->mod_shift3_cur)  * recBlockSize;
      voice->mod_level3_inc  = (modLevel3  - voice->mod_level3_cur)  * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur  = modDryWet;
      voice->mod_drywet_inc  = 0.0f;
      voice->mod_inlevel_cur = modInLevel;
      voice->mod_inlevel_inc = 0.0f;
      voice->mod_shift1_cur  = modShift1;
      voice->mod_shift1_inc  = 0.0f;
      voice->mod_level1_cur  = modLevel1;
      voice->mod_level1_inc  = 0.0f;
      voice->mod_shift2_cur  = modShift2;
      voice->mod_shift2_inc  = 0.0f;
      voice->mod_level2_cur  = modLevel2;
      voice->mod_level2_inc  = 0.0f;
      voice->mod_shift3_cur  = modShift3;
      voice->mod_shift3_inc  = 0.0f;
      voice->mod_level3_cur  = modLevel3;
      voice->mod_level3_inc  = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(wave_multiplier_wrap_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(wave_multiplier_wrap_shared_t);

   unsigned int k = 0u;

   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      for(unsigned int ch = 0u; ch < 2u; ch++)
      {
         float l = _samplesIn[k + ch];
         // float cmp;
         // if(l >= voice->mod_shift1_cur)
         // {
         //    cmp = 1.0f;
         // }
         // else
         // {
         //    cmp = -1.0f;
         // }

         // outL = l * voice->mod_inlevel_cur + outL * voice->mod_level1_cur;
         // if(outL < -1.0f)
         //    outL += 2.0f;
         // else if(outL > 1.0f)
         //    outL -= 2.0f;

         // float outL = 5.0f/6.0f - l * voice->mod_inlevel_cur - cmp * voice->mod_level1_cur;

         // float outL = l * voice->mod_inlevel_cur + cmp * l * voice->mod_level1_cur;

         float lAmp = l * voice->mod_inlevel_cur;
         float outL = 0.0f;

         float out1 = lAmp + voice->mod_shift1_cur;
         if(out1 < -1.0f)
            out1 += 2.0f;
         else if(out1 > 1.0f)
            out1 -= 2.0f;

         float out2 = lAmp + voice->mod_shift2_cur;
         if(out2 < -1.0f)
            out2 += 2.0f;
         else if(out2 > 1.0f)
            out2 -= 2.0f;

         float out3 = lAmp + voice->mod_shift3_cur;
         if(out3 < -1.0f)
            out3 += 2.0f;
         else if(out3 > 1.0f)
            out3 -= 2.0f;

         outL += out1 * voice->mod_level1_cur;
         outL += out2 * voice->mod_level2_cur;
         outL += out3 * voice->mod_level3_cur;

         outL = l + (outL - l) * voice->mod_drywet_cur;

         _samplesOut[k + ch] = outL;
      }

      // Next frame
      k += 2u;
      voice->mod_drywet_cur  += voice->mod_drywet_inc;
      voice->mod_inlevel_cur += voice->mod_inlevel_inc;
      voice->mod_shift1_cur  += voice->mod_shift1_inc;
      voice->mod_level1_cur  += voice->mod_level1_inc;
      voice->mod_shift2_cur  += voice->mod_shift2_inc;
      voice->mod_level2_cur  += voice->mod_level2_inc;
      voice->mod_shift3_cur  += voice->mod_shift3_inc;
      voice->mod_level3_cur  += voice->mod_level3_inc;
   }

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   wave_multiplier_wrap_shared_t *ret = (wave_multiplier_wrap_shared_t *)malloc(sizeof(wave_multiplier_wrap_shared_t));
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
   wave_multiplier_wrap_voice_t *ret = (wave_multiplier_wrap_voice_t *)malloc(sizeof(wave_multiplier_wrap_voice_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info = _info;
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   free((void*)_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {
   free((void*)_info);
}

st_plugin_info_t *wave_multiplier_wrap_init(void) {
   wave_multiplier_wrap_info_t *ret = NULL;

   ret = (wave_multiplier_wrap_info_t *)malloc(sizeof(wave_multiplier_wrap_info_t));

   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp wave_multiplier_wrap";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "wave_multiplier_wrap";
      ret->base.short_name  = "wave_multiplier_wrap";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
      ret->base.category    = ST_PLUGIN_CAT_CHORUS;
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
      ret->base.note_on            = &loc_note_on;
      ret->base.set_mod_value      = &loc_set_mod_value;
      ret->base.prepare_block      = &loc_prepare_block;
      ret->base.process_replace    = &loc_process_replace;
      ret->base.plugin_exit        = &loc_plugin_exit;
   }

   return &ret->base;
}
