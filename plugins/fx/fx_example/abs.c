// ----
// ---- file   : abs.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : abs
// ----
// ---- created: 09Jun2020
// ---- changed: 
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
#define PARAM_ABS      3
#define PARAM_BLEND    4
#define NUM_PARAMS     5
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Drive",
   "Offset",
   "Abs",
   "Blend",
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // DRIVE
   0.5f,  // OFFSET
   1.0f,  // ABS
   0.0f,  // BLEND
};

#define MOD_DRYWET  0
#define MOD_DRIVE   1
#define MOD_OFFSET  2
#define MOD_ABS     3
#define MOD_BLEND   4
#define NUM_MODS    5
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Drive",
   "Offset",
   "Abs",
   "Blend",
};

typedef struct abs_info_s {
   st_plugin_info_t base;
} abs_info_t;

typedef struct abs_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} abs_shared_t;

typedef struct abs_voice_s {
   st_plugin_voice_t base;
   float    mods[NUM_MODS];
   float    mod_drywet_cur;
   float    mod_drywet_inc;
   float    mod_drive_cur;
   float    mod_drive_inc;
   float    mod_offset_cur;
   float    mod_offset_inc;
   float    mod_abs_cur;
   float    mod_abs_inc;
   float    mod_blend_cur;
   float    mod_blend_inc;
} abs_voice_t;


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
   ST_PLUGIN_SHARED_CAST(abs_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(abs_shared_t);
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
   ST_PLUGIN_VOICE_CAST(abs_voice_t);
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
   ST_PLUGIN_VOICE_CAST(abs_voice_t);
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
   ST_PLUGIN_VOICE_CAST(abs_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(abs_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET] + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modDrive = ((shared->params[PARAM_DRIVE] - 0.5f) * 2.0f) + voice->mods[MOD_DRIVE];
   modDrive = powf(10.0f, modDrive * 2.0f);

   float modOffset = (shared->params[PARAM_OFFSET] -0.5f) * 2.0f + voice->mods[MOD_OFFSET];

   float modAbs = shared->params[PARAM_ABS] + voice->mods[MOD_ABS];
   modAbs = Dstplugin_clamp(modAbs, 0.0f, 1.0f);

   float modBlend = shared->params[PARAM_BLEND] + voice->mods[MOD_BLEND];
   modBlend = Dstplugin_clamp(modBlend, 0.0f, 1.0f);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc  = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_drive_inc   = (modDrive  - voice->mod_drive_cur)  * recBlockSize;
      voice->mod_offset_inc  = (modOffset - voice->mod_offset_cur) * recBlockSize;
      voice->mod_abs_inc     = (modAbs    - voice->mod_abs_cur)    * recBlockSize;
      voice->mod_blend_inc   = (modBlend  - voice->mod_blend_cur)  * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur  = modDryWet;
      voice->mod_drywet_inc  = 0.0f;
      voice->mod_drive_cur   = modDrive;
      voice->mod_drive_inc   = 0.0f;
      voice->mod_offset_cur  = modOffset;
      voice->mod_offset_inc  = 0.0f;
      voice->mod_abs_cur     = modAbs;
      voice->mod_abs_inc     = 0.0f;
      voice->mod_blend_cur   = modBlend;
      voice->mod_blend_inc   = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(abs_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(abs_shared_t);

   unsigned int k = 0u;

   typedef union uf_u {
      float f;
      unsigned int u;
   } uf_t;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         uf_t absL; absL.f = l;
         absL.u &= 0x7FFFffffu;

         float outL = (l + voice->mod_offset_cur) * voice->mod_drive_cur;
         uf_t absOutL; absOutL.f = outL;
         absOutL.u &= 0x7FFFffffu;

         outL = outL + (absOutL.f - outL) * voice->mod_abs_cur;
         outL = outL + (outL*absL.f - outL) * voice->mod_blend_cur;

         outL = l + (outL - l) * voice->mod_drywet_cur;
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outL;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur  += voice->mod_drywet_inc;
         voice->mod_offset_cur  += voice->mod_offset_inc;
         voice->mod_drive_cur   += voice->mod_drive_inc;
         voice->mod_abs_cur     += voice->mod_abs_inc;
         voice->mod_blend_cur   += voice->mod_blend_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float r = _samplesIn[k + 1u];

         // left
         uf_t absL; absL.f = l;
         absL.u &= 0x7FFFffffu;

         float outL = (l + voice->mod_offset_cur) * voice->mod_drive_cur;
         uf_t absOutL; absOutL.f = outL;
         absOutL.u &= 0x7FFFffffu;

         outL = outL + (absOutL.f - outL) * voice->mod_abs_cur;
         outL = outL + (outL*absL.f - outL) * voice->mod_blend_cur;

         // right
         uf_t absR; absR.f = r;
         absR.u &= 0x7FFFffffu;

         float outR = (r + voice->mod_offset_cur) * voice->mod_drive_cur;
         uf_t absOutR; absOutR.f = outR;
         absOutR.u &= 0x7FFFffffu;

         outR = outR + (absOutR.f - outR) * voice->mod_abs_cur;
         outR = outR + (outR*absR.f - outR) * voice->mod_blend_cur;

         outL = l + (outL - l) * voice->mod_drywet_cur;
         outR = r + (outR - r) * voice->mod_drywet_cur;
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur  += voice->mod_drywet_inc;
         voice->mod_offset_cur  += voice->mod_offset_inc;
         voice->mod_drive_cur   += voice->mod_drive_inc;
         voice->mod_abs_cur     += voice->mod_abs_inc;
         voice->mod_blend_cur   += voice->mod_blend_inc;
      }
   }

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   abs_shared_t *ret = (abs_shared_t *)malloc(sizeof(abs_shared_t));
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
   abs_voice_t *ret = (abs_voice_t *)malloc(sizeof(abs_voice_t));
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

st_plugin_info_t *abs_init(void) {
   abs_info_t *ret = NULL;

   ret = (abs_info_t *)malloc(sizeof(abs_info_t));

   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp abs"; // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "abs";
      ret->base.short_name  = "abs";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
      ret->base.category    = ST_PLUGIN_CAT_WAVESHAPER;
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
