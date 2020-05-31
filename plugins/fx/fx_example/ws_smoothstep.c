// ----
// ---- file   : ws_smoothstep.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a tanh waveshaper that supports per-sample-frame parameter interpolation
// ----
// ---- created: 10May2009
// ---- changed: 23May2010, 30Sep2010, 07Oct2010, 17May2020, 18May2020, 19May2020, 20May2020
// ----          21May2020, 24May2020, 31May2020
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET   0
#define PARAM_DRIVE    1
#define PARAM_SHAPE_1  2
#define PARAM_SHAPE_2  3
#define PARAM_LEVEL    4
#define NUM_PARAMS     5
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Drive",
   "Shape 1",
   "Shape 2",
   "Level"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.0f,  // DRIVE
   0.5f,  // SHAPE_1
   0.5f,  // SHAPE_2
   1.0f,  // LEVEL
};

#define MOD_DRYWET   0
#define MOD_DRIVE    1
#define MOD_SHAPE_1  2
#define MOD_SHAPE_2  3
#define NUM_MODS     4
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Drive",
   "Shape 1",
   "Shape 2"
};

typedef struct ws_smoothstep_info_s {
   st_plugin_info_t base;
} ws_smoothstep_info_t;

typedef struct ws_smoothstep_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ws_smoothstep_shared_t;

typedef struct ws_smoothstep_voice_s {
   st_plugin_voice_t base;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_drive_cur;
   float mod_drive_inc;
   float mod_shape_1_cur;
   float mod_shape_1_inc;
   float mod_shape_2_cur;
   float mod_shape_2_inc;
   float mod_level_cur;
   float mod_level_inc;
} ws_smoothstep_voice_t;


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
   ST_PLUGIN_SHARED_CAST(ws_smoothstep_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ws_smoothstep_shared_t);
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
                                      unsigned int        _voiceIdx,
                                      unsigned int        _activeNoteIdx,
                                      unsigned char       _note,
                                      float               _noteHz,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(ws_smoothstep_voice_t);
   (void)_bGlide;
   (void)_voiceIdx;
   (void)_activeNoteIdx;
   (void)_note;
   (void)_noteHz;
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
   ST_PLUGIN_VOICE_CAST(ws_smoothstep_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_smoothstep_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_smoothstep_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modDrive = shared->params[PARAM_DRIVE] + voice->mods[MOD_DRIVE];
   modDrive = powf(10.0f, modDrive * 2.0f);

   float modShape1 = (shared->params[PARAM_SHAPE_1] - 0.5f) * 2.0f + voice->mods[MOD_SHAPE_1];
   modShape1 = Dstplugin_clamp(modShape1, -1.0f, 1.0f);
   modShape1 *= 2.0f;
   modShape1 += 3.0f;

   float modShape2 = (shared->params[PARAM_SHAPE_2] - 0.5f) * 2.0f + voice->mods[MOD_SHAPE_2];
   modShape2 = Dstplugin_clamp(modShape2, -1.0f, 1.0f);
   modShape2 *= 2.0f;
   modShape2 -= 2.0f;

   float modLevel = (-1.0f + shared->params[PARAM_LEVEL]) * 4.0f;
   modLevel = powf(10.0f, modLevel);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc  = (modDryWet - voice->mod_drywet_cur)  * recBlockSize;
      voice->mod_drive_inc   = (modDrive  - voice->mod_drive_cur)   * recBlockSize;
      voice->mod_shape_1_inc = (modShape1 - voice->mod_shape_1_cur) * recBlockSize;
      voice->mod_shape_2_inc = (modShape2 - voice->mod_shape_2_cur) * recBlockSize;
      voice->mod_level_inc   = (modLevel  - voice->mod_level_cur)   * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur  = modDryWet;
      voice->mod_drywet_inc  = 0.0f;
      voice->mod_drive_cur   = modDrive;
      voice->mod_drive_inc   = 0.0f;
      voice->mod_shape_1_cur = modShape1;
      voice->mod_shape_1_inc = 0.0f;
      voice->mod_shape_2_cur = modShape2;
      voice->mod_shape_2_inc = 0.0f;
      voice->mod_level_cur   = modLevel;
      voice->mod_level_inc   = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(ws_smoothstep_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_smoothstep_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float a = l * voice->mod_drive_cur;
         if(a > 1.0f)
            a = 1.0f;
         else if(a < -1.0f)
            a = -1.0f;
         a = (a * 0.5f) + 0.5f;
         // a = a * a * (3.0f - 2.0f * a);
         a = a * a * (voice->mod_shape_1_cur + voice->mod_shape_2_cur * a);
         a = (a * 2.0f) - 1.0f;
         a *= voice->mod_level_cur;

         float out = l + (a - l) * voice->mod_drywet_cur;
         _samplesOut[k]      = out;
         _samplesOut[k + 1u] = out;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur  += voice->mod_drywet_inc;
         voice->mod_drive_cur   += voice->mod_drive_inc;
         voice->mod_shape_1_cur += voice->mod_shape_1_inc;
         voice->mod_shape_2_cur += voice->mod_shape_2_inc;
         voice->mod_level_cur   += voice->mod_level_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float al = l * voice->mod_drive_cur;
         if(al > 1.0f)
            al = 1.0f;
         else if(al < -1.0f)
            al = -1.0f;
         al = (al * 0.5f) + 0.5f;
         // al = al * al * (3.0f - 2.0f * al);
         al = al * al * (voice->mod_shape_1_cur + voice->mod_shape_2_cur * al);
         al = (al * 2.0f) - 1.0f;
         al *= voice->mod_level_cur;

         float r = _samplesIn[k + 1u];
         float ar = r * voice->mod_drive_cur;
         if(ar > 1.0f)
            ar = 1.0f;
         else if(ar < -1.0f)
            ar = -1.0f;
         ar = (ar * 0.5f) + 0.5f;
         // ar = ar * ar * (3.0f - 2.0f * ar);
         ar = ar * ar * (voice->mod_shape_1_cur + voice->mod_shape_2_cur * ar);
         ar = (ar * 2.0f) - 1.0f;
         ar *= voice->mod_level_cur;

         float outL = l + (al - l) * voice->mod_drywet_cur;
         float outR = r + (ar - r) * voice->mod_drywet_cur;
         _samplesOut[k]      = Dstplugin_fix_denorm_32(outL);
         _samplesOut[k + 1u] = Dstplugin_fix_denorm_32(outR);

         // Next frame
         k += 2u;
         voice->mod_drywet_cur  += voice->mod_drywet_inc;
         voice->mod_drive_cur   += voice->mod_drive_inc;
         voice->mod_shape_1_cur += voice->mod_shape_1_inc;
         voice->mod_shape_2_cur += voice->mod_shape_2_inc;
         voice->mod_level_cur   += voice->mod_level_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ws_smoothstep_shared_t *ret = malloc(sizeof(ws_smoothstep_shared_t));
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

static st_plugin_voice_t *ST_PLUGIN_API loc_voice_new(st_plugin_info_t *_info) {
   ws_smoothstep_voice_t *ret = malloc(sizeof(ws_smoothstep_voice_t));
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

st_plugin_info_t *ws_smoothstep_init(void) {
   ws_smoothstep_info_t *ret = NULL;

   ret = malloc(sizeof(ws_smoothstep_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ws smoothstep";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "smoothstep shaper";
      ret->base.short_name  = "smoothstep";
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
