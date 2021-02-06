// ----
// ---- file   : clip.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : clipping amplifier
// ----
// ---- created: 31May2020
// ---- changed: 08Jun2020
// ----
// ----
// ----

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET   0
#define PARAM_DRIVE    1
#define PARAM_OFFSET   2
#define PARAM_MIN      3
#define PARAM_MAX      4
#define PARAM_NORM     5
#define NUM_PARAMS     6
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Drive",
   "Offset",
   "Min",
   "Max",
   "Normalize"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // DRIVE
   0.5f,  // OFFSET
   0.0f,  // MIN
   1.0f,  // MAX
   0.0f,  // NORMALIZE
};

#define MOD_DRYWET  0
#define MOD_OFFSET  1
#define MOD_DRIVE   2
#define MOD_RANGE   3
#define NUM_MODS    4
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Offset",
   "Drive",
   "Range",
};

typedef struct clip_info_s {
   st_plugin_info_t base;
} clip_info_t;

typedef struct clip_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} clip_shared_t;

typedef struct clip_voice_s {
   st_plugin_voice_t base;
   float    mods[NUM_MODS];
   float    mod_drywet_cur;
   float    mod_drywet_inc;
   float    mod_drive_cur;
   float    mod_drive_inc;
   float    mod_offset_pre_cur;
   float    mod_offset_pre_inc;
   float    mod_min_cur;
   float    mod_min_inc;
   float    mod_max_cur;
   float    mod_max_inc;
   float    mod_offset_post_cur;
   float    mod_offset_post_inc;
   float    mod_gain_cur;
   float    mod_gain_inc;
} clip_voice_t;


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
   ST_PLUGIN_SHARED_CAST(clip_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(clip_shared_t);
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
   ST_PLUGIN_VOICE_CAST(clip_voice_t);
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
   ST_PLUGIN_VOICE_CAST(clip_voice_t);
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
   ST_PLUGIN_VOICE_CAST(clip_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(clip_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET] + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modOffsetPre = (shared->params[PARAM_OFFSET] -0.5f) * 2.0f + voice->mods[MOD_OFFSET];

   float modDrive = ((shared->params[PARAM_DRIVE] - 0.5f) * 2.0f) + voice->mods[MOD_DRIVE];
   modDrive = powf(10.0f, modDrive * 4.0f);

   float modMin = (shared->params[PARAM_MIN] -0.5f) * 2.0f;
   float modMax = (shared->params[PARAM_MAX] -0.5f) * 2.0f;
   modMin += voice->mods[MOD_RANGE];
   modMax += voice->mods[MOD_RANGE];
   float modGain;
   float modOffsetPost;
   if(modMin > modMax)
   {
      float t = modMin;
      modMin = modMax;
      modMax = t;
      modOffsetPost = (modMax + modMin) * 0.5f;
      modGain       = 1.0f / ((modMax - modMin) * 0.5f);
   }
   else if(modMin < modMax)
   {
      modOffsetPost = (modMax + modMin) * 0.5f;
      modGain       = 1.0f / ((modMax - modMin) * 0.5f);
   }
   else
   {
      modOffsetPost = 0.0f;
      modGain       = 0.0f;
   }

   float modNorm = shared->params[PARAM_NORM];// + voice->mods[MOD_NORM];
   modOffsetPost *= modNorm;
   modGain = 1.0f + (modGain - 1.0f) * modNorm;

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc      = (modDryWet     - voice->mod_drywet_cur)      * recBlockSize;
      voice->mod_drive_inc       = (modDrive      - voice->mod_drive_cur)       * recBlockSize;
      voice->mod_offset_pre_inc  = (modOffsetPre  - voice->mod_offset_pre_cur)  * recBlockSize;
      voice->mod_min_inc         = (modMin        - voice->mod_min_cur)         * recBlockSize;
      voice->mod_max_inc         = (modMax        - voice->mod_max_cur)         * recBlockSize;
      voice->mod_offset_post_inc = (modOffsetPost - voice->mod_offset_post_cur) * recBlockSize;
      voice->mod_gain_inc        = (modGain       - voice->mod_gain_cur)        * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur      = modDryWet;
      voice->mod_drywet_inc      = 0.0f;
      voice->mod_drive_cur       = modDrive;
      voice->mod_drive_inc       = 0.0f;
      voice->mod_offset_pre_cur  = modOffsetPre;
      voice->mod_offset_pre_inc  = 0.0f;
      voice->mod_min_cur         = modMin;
      voice->mod_min_inc         = 0.0f;
      voice->mod_max_cur         = modMax;
      voice->mod_max_inc         = 0.0f;
      voice->mod_offset_post_cur = modOffsetPost;
      voice->mod_offset_post_inc = 0.0f;
      voice->mod_gain_cur        = modGain;
      voice->mod_gain_inc        = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(clip_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(clip_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float outL = (l + voice->mod_offset_pre_cur) * voice->mod_drive_cur;
         if(outL < voice->mod_min_cur)
            outL = voice->mod_min_cur;
         else if(outL > voice->mod_max_cur)
            outL = voice->mod_max_cur;
         outL -= voice->mod_offset_post_cur;
         outL *= voice->mod_gain_cur;
         outL = l + (outL - l) * voice->mod_drywet_cur;
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outL;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur      += voice->mod_drywet_inc;
         voice->mod_offset_pre_cur  += voice->mod_offset_pre_inc;
         voice->mod_drive_cur       += voice->mod_drive_inc;
         voice->mod_min_cur         += voice->mod_min_inc;
         voice->mod_max_cur         += voice->mod_max_inc;
         voice->mod_offset_post_cur += voice->mod_offset_post_inc;
         voice->mod_gain_cur        += voice->mod_gain_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float outL = (l + voice->mod_offset_pre_cur) * voice->mod_drive_cur;
         if(outL < voice->mod_min_cur)
            outL = voice->mod_min_cur;
         else if(outL > voice->mod_max_cur)
            outL = voice->mod_max_cur;
         outL -= voice->mod_offset_post_cur;
         outL *= voice->mod_gain_cur;

         float r = _samplesIn[k + 1u];
         float outR = (r + voice->mod_offset_pre_cur) * voice->mod_drive_cur;
         if(outR < voice->mod_min_cur)
            outR = voice->mod_min_cur;
         else if(outR > voice->mod_max_cur)
            outR = voice->mod_max_cur;
         outR -= voice->mod_offset_post_cur;
         outR *= voice->mod_gain_cur;

         outL = l + (outL - l) * voice->mod_drywet_cur;
         outR = r + (outR - r) * voice->mod_drywet_cur;
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur      += voice->mod_drywet_inc;
         voice->mod_offset_pre_cur  += voice->mod_offset_pre_inc;
         voice->mod_drive_cur       += voice->mod_drive_inc;
         voice->mod_min_cur         += voice->mod_min_inc;
         voice->mod_max_cur         += voice->mod_max_inc;
         voice->mod_offset_post_cur += voice->mod_offset_post_inc;
         voice->mod_gain_cur        += voice->mod_gain_inc;
      }
   }

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   clip_shared_t *ret = (clip_shared_t *)malloc(sizeof(clip_shared_t));
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
   clip_voice_t *ret = (clip_voice_t *)malloc(sizeof(clip_voice_t));
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

st_plugin_info_t *clip_init(void) {
   clip_info_t *ret = NULL;

   ret = (clip_info_t *)malloc(sizeof(clip_info_t));

   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp clip"; // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "clip";
      ret->base.short_name  = "clip";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
      ret->base.category    = ST_PLUGIN_CAT_AMP;
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
