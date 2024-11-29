// ----
// ---- file   : ws_fold_wrap2.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2024 by Bastian Spiegel.
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a waveshaper that supports per-sample-frame parameter interpolation and min/max range
// ----
// ---- created: 08Nov2024
// ---- changed:
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET      0
#define PARAM_DRIVE_PRE   1  // pre (0..*100)
#define PARAM_DRIVE_POST  2  // post (/10..*1..*10)
#define PARAM_MIN         3  // 0..1 => -1..1
#define PARAM_MAX         4  // 0..1 => -1..1
#define PARAM_OFFSET      5  // DC offset. 0..1 => -1..1
#define NUM_PARAMS        6
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Drive Pre",
   "Drive Post",
   "Min",
   "Max",
   "Offset",
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.0f,  // DRIVE_PRE
   0.5f,  // DRIVE_POST
   0.0f,  // MIN
   1.0f,  // MAX
   0.5f,  // OFFSET
};

#define MOD_DRYWET      0
#define MOD_DRIVE_PRE   1
#define MOD_DRIVE_POST  2
#define MOD_MIN         3
#define MOD_MAX         4
#define MOD_OFFSET      5
#define NUM_MODS        6
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Drive Pre",
   "Drive Post",
   "Min",
   "Max",
   "Offset",
};

typedef struct ws_fold_wrap2_info_s {
   st_plugin_info_t base;
} ws_fold_wrap2_info_t;

typedef struct ws_fold_wrap2_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ws_fold_wrap2_shared_t;

typedef struct ws_fold_wrap2_voice_s {
   st_plugin_voice_t base;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_drive_pre_cur;
   float mod_drive_pre_inc;
   float mod_drive_post_cur;
   float mod_drive_post_inc;
   float mod_min_cur;
   float mod_min_inc;
   float mod_max_cur;
   float mod_max_inc;
   float mod_offset_cur;
   float mod_offset_inc;
} ws_fold_wrap2_voice_t;


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
   ST_PLUGIN_SHARED_CAST(ws_fold_wrap2_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ws_fold_wrap2_shared_t);
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
   ST_PLUGIN_VOICE_CAST(ws_fold_wrap2_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_fold_wrap2_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_fold_wrap2_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_fold_wrap2_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET] + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modDrivePre = shared->params[PARAM_DRIVE_PRE] + voice->mods[MOD_DRIVE_PRE];
   modDrivePre = powf(10.0f, modDrivePre * 2.0f);

   float modDrivePost = shared->params[PARAM_DRIVE_POST] + voice->mods[MOD_DRIVE_POST];
   modDrivePost = powf(10.0f, 2.0f*modDrivePost - 1.0f);

   float modMin = shared->params[PARAM_MIN] + voice->mods[MOD_MIN];
   modMin = Dstplugin_clamp(modMin, 0.0f, 1.0f);
   modMin = Dstplugin_scale(modMin, -1.0f, 1.0f);

   float modMax = shared->params[PARAM_MAX] + voice->mods[MOD_MAX];
   modMax = Dstplugin_clamp(modMax, 0.0f, 1.0f);
   modMax = Dstplugin_scale(modMax, -1.0f, 1.0f);

   float modOffset = shared->params[PARAM_OFFSET] + voice->mods[MOD_OFFSET];
   modOffset = Dstplugin_clamp(modOffset, 0.0f, 1.0f);
   modOffset = Dstplugin_scale(modOffset, -1.0f, 1.0f);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc     = (modDryWet    - voice->mod_drywet_cur)     * recBlockSize;
      voice->mod_drive_pre_inc  = (modDrivePre  - voice->mod_drive_pre_cur)  * recBlockSize;
      voice->mod_drive_post_inc = (modDrivePost - voice->mod_drive_post_cur) * recBlockSize;
      voice->mod_min_inc        = (modMin       - voice->mod_min_cur)        * recBlockSize;
      voice->mod_max_inc        = (modMax       - voice->mod_max_cur)        * recBlockSize;
      voice->mod_offset_inc     = (modOffset    - voice->mod_offset_cur)     * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur     = modDryWet;
      voice->mod_drywet_inc     = 0.0f;
      voice->mod_drive_pre_cur  = modDrivePre;
      voice->mod_drive_pre_inc  = 0.0f;
      voice->mod_drive_post_cur = modDrivePost;
      voice->mod_drive_post_inc = 0.0f;
      voice->mod_min_cur        = modMin;
      voice->mod_min_inc        = 0.0f;
      voice->mod_max_cur        = modMax;
      voice->mod_max_inc        = 0.0f;
      voice->mod_offset_cur     = modOffset;
      voice->mod_offset_inc     = 0.0f;
   }
}

static float loc_mathFoldf(float _a, float _b, float _c) {
   float b, c;
   if(_b < _c)
   {
      b = _b;
      c = _c;
   }
   else
   {
      b = _c;
      c = _b;
   }
   float r = (c - b);
   if(0.0f != r)
   {
      if(_a < b)
      {
         float d = b - _a;
         if(d < r)
         {
            return b + d;
         }
         else
         {
            const float dr = d / r;
            d = d - r * floorf(dr);
            const int num = (int)dr;
            if(num & 1)
               return c - d;
            return b + d;
         }
      }
      else if(_a >= c)
      {
         float d = _a - c;
         if(d < r)
         {
            return c - d;
         }
         else
         {
            const float dr = d / r;
            d = d - r * floorf(dr);
            const int num = (int)dr;
            if(num & 1)
               return b + d;
            return c - d;
         }
      }
      return _a;
   }
   // b == c
   return b;
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut,
                                              unsigned int        _numFrames
                                              ) {
   ST_PLUGIN_VOICE_CAST(ws_fold_wrap2_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_fold_wrap2_shared_t);

   unsigned int k = 0u;

   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      float l = _samplesIn[k];
      float aL = l * voice->mod_drive_pre_cur;
      aL = loc_mathFoldf(aL, voice->mod_min_cur, voice->mod_max_cur);
      aL += voice->mod_offset_cur;
      aL *= voice->mod_drive_post_cur;
      float outL = l + (aL - l) * voice->mod_drywet_cur;
      outL = Dstplugin_fix_denorm_32(outL);
      _samplesOut[k + 0u] = outL;

      float r = _samplesIn[k + 1u];
      float aR = r * voice->mod_drive_pre_cur;
      aR = loc_mathFoldf(aR, voice->mod_min_cur, voice->mod_max_cur);
      aR += voice->mod_offset_cur;
      aR *= voice->mod_drive_post_cur;
      float outR = r + (aR - r) * voice->mod_drywet_cur;
      outR = Dstplugin_fix_denorm_32(outR);
      _samplesOut[k + 1u] = outR;

      // Next frame
      k += 2u;
      voice->mod_drywet_cur     += voice->mod_drywet_inc;
      voice->mod_drive_pre_cur  += voice->mod_drive_pre_inc;
      voice->mod_drive_post_cur += voice->mod_drive_post_inc;
      voice->mod_min_cur        += voice->mod_min_inc;
      voice->mod_max_cur        += voice->mod_max_inc;
      voice->mod_offset_cur     += voice->mod_offset_inc;
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ws_fold_wrap2_shared_t *ret = malloc(sizeof(ws_fold_wrap2_shared_t));
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
   ws_fold_wrap2_voice_t *ret = malloc(sizeof(ws_fold_wrap2_voice_t));
   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));
      ret->base.info = _info;
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   free(_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {
   free(_info);
}

st_plugin_info_t *ws_fold_wrap2_init(void) {
   ws_fold_wrap2_info_t *ret = NULL;

   ret = malloc(sizeof(ws_fold_wrap2_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ws fold wrap2";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "fold wrap2 shaper";
      ret->base.short_name  = "fold wrap2";
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
