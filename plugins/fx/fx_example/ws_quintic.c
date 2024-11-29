// ----
// ---- file   : ws_quintic.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020-2024 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a quintic waveshaper that supports per-sample-frame parameter interpolation
// ----           idea based on DHE's 'cubic' VCV Rack module
// ----
// ---- created: 07Jun2020
// ---- changed: 08Jun2020, 21Jan2024
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET   0
#define PARAM_X0       1
#define PARAM_X1       2
#define PARAM_X2       3
#define PARAM_X3       4
#define PARAM_X4       5
#define PARAM_X5       6
#define PARAM_DRIVE    7
#define NUM_PARAMS     8
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "X^0",
   "X^1",
   "X^2",
   "X^3",
   "X^4",
   "X^5",
   "Drive",
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.0f,  // X0
   0.5f,  // X1
   0.5f,  // X2
   0.5f,  // X3
   0.5f,  // X4
   0.5f,  // X5
   0.5f,  // DRIVE
};

#define MOD_DRYWET   0
#define MOD_X0       1
#define MOD_X1       2
#define MOD_X2       3
#define MOD_X3       4
#define MOD_X4       5
#define MOD_X5       6
#define MOD_DRIVE    7
#define NUM_MODS     8
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "X^0",
   "X^1",
   "X^2",
   "X^3",
   "X^4",
   "X^5",
   "Drive",
};

typedef struct ws_quintic_info_s {
   st_plugin_info_t base;
} ws_quintic_info_t;

typedef struct ws_quintic_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ws_quintic_shared_t;

typedef struct ws_quintic_voice_s {
   st_plugin_voice_t base;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_x0_cur;
   float mod_x0_inc;
   float mod_x1_cur;
   float mod_x1_inc;
   float mod_x2_cur;
   float mod_x2_inc;
   float mod_x3_cur;
   float mod_x3_inc;
   float mod_x4_cur;
   float mod_x4_inc;
   float mod_x5_cur;
   float mod_x5_inc;
   float mod_drive_cur;
   float mod_drive_inc;
} ws_quintic_voice_t;


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
   ST_PLUGIN_SHARED_CAST(ws_quintic_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ws_quintic_shared_t);
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
   ST_PLUGIN_VOICE_CAST(ws_quintic_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_quintic_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_quintic_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_quintic_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modX0 = (shared->params[PARAM_X0]-0.5f)*2.0f + voice->mods[MOD_X0];
   modX0 = powf(10.0f, modX0 * 3.0f);

   float modX1 = (shared->params[PARAM_X1]-0.5f)*2.0f + voice->mods[MOD_X1];
   modX1 = powf(10.0f, modX1 * 3.0f);

   float modX2 = (shared->params[PARAM_X2]-0.5f)*2.0f + voice->mods[MOD_X2];
   modX2 = powf(10.0f, modX2 * 3.0f);

   float modX3 = (shared->params[PARAM_X3]-0.5f)*2.0f + voice->mods[MOD_X3];
   modX3 = powf(10.0f, modX3 * 3.0f);

   float modX4 = (shared->params[PARAM_X4]-0.5f)*2.0f + voice->mods[MOD_X4];
   modX4 = powf(10.0f, modX4 * 3.0f);

   float modX5 = (shared->params[PARAM_X5]-0.5f)*2.0f + voice->mods[MOD_X5];
   modX5 = powf(10.0f, modX5 * 3.0f);

   float modDrive = (shared->params[PARAM_DRIVE]-0.5f)*2.0f + voice->mods[MOD_DRIVE];
   modDrive = powf(10.0f, modDrive * 3.0f);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_x0_inc     = (modX0     - voice->mod_x0_cur)     * recBlockSize;
      voice->mod_x1_inc     = (modX1     - voice->mod_x1_cur)     * recBlockSize;
      voice->mod_x2_inc     = (modX2     - voice->mod_x2_cur)     * recBlockSize;
      voice->mod_x3_inc     = (modX3     - voice->mod_x3_cur)     * recBlockSize;
      voice->mod_x4_inc     = (modX4     - voice->mod_x4_cur)     * recBlockSize;
      voice->mod_x5_inc     = (modX5     - voice->mod_x5_cur)     * recBlockSize;
      voice->mod_drive_inc  = (modDrive  - voice->mod_drive_cur)  * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      voice->mod_x0_cur     = modX0;
      voice->mod_x0_inc     = 0.0f;
      voice->mod_x1_cur     = modX1;
      voice->mod_x1_inc     = 0.0f;
      voice->mod_x2_cur     = modX2;
      voice->mod_x2_inc     = 0.0f;
      voice->mod_x3_cur     = modX3;
      voice->mod_x3_inc     = 0.0f;
      voice->mod_x4_cur     = modX4;
      voice->mod_x4_inc     = 0.0f;
      voice->mod_x5_cur     = modX5;
      voice->mod_x5_inc     = 0.0f;
      voice->mod_drive_cur  = modDrive;
      voice->mod_drive_inc  = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(ws_quintic_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_quintic_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];

         float x0, x1, x2, x3, x4, x5;
         x0 = 1.0f;

         x1 = l;
         x2 = l * l;
         x3 = l * x2;
         x4 = l * x3;
         x5 = l * x4;

         float outL =
            x0 * voice->mod_x0_cur +
            x1 * voice->mod_x1_cur +
            x2 * voice->mod_x2_cur +
            x3 * voice->mod_x3_cur +
            x4 * voice->mod_x4_cur +
            x5 * voice->mod_x5_cur ;

         outL = tanhf(outL * voice->mod_drive_cur);
         outL = l + (outL - l) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outL;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_x0_cur     += voice->mod_x0_inc;
         voice->mod_x1_cur     += voice->mod_x1_inc;
         voice->mod_x2_cur     += voice->mod_x2_inc;
         voice->mod_x3_cur     += voice->mod_x3_inc;
         voice->mod_x4_cur     += voice->mod_x4_inc;
         voice->mod_x5_cur     += voice->mod_x5_inc;
         voice->mod_drive_cur  += voice->mod_drive_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float r = _samplesIn[k + 1u];

         float x0, x1, x2, x3, x4, x5;
         x0 = 1.0f;

         x1 = l;
         x2 = l * l;
         x3 = l * x2;
         x4 = l * x3;
         x5 = l * x4;

         float outL =
            x0 * voice->mod_x0_cur +
            x1 * voice->mod_x1_cur +
            x2 * voice->mod_x2_cur +
            x3 * voice->mod_x3_cur +
            x4 * voice->mod_x4_cur +
            x5 * voice->mod_x5_cur ;

         x1 = r;
         x2 = r * r;
         x3 = r * x2;
         x4 = r * x3;
         x5 = r * x4;

         float outR =
            x0 * voice->mod_x0_cur +
            x1 * voice->mod_x1_cur +
            x2 * voice->mod_x2_cur +
            x3 * voice->mod_x3_cur +
            x4 * voice->mod_x4_cur +
            x5 * voice->mod_x5_cur ;

         outL = tanhf(outL * voice->mod_drive_cur);
         outR = tanhf(outR * voice->mod_drive_cur);
         outL = l + (outL - l) * voice->mod_drywet_cur;
         outR = r + (outR - r) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);
         outR = Dstplugin_fix_denorm_32(outR);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_x0_cur     += voice->mod_x0_inc;
         voice->mod_x1_cur     += voice->mod_x1_inc;
         voice->mod_x2_cur     += voice->mod_x2_inc;
         voice->mod_x3_cur     += voice->mod_x3_inc;
         voice->mod_x4_cur     += voice->mod_x4_inc;
         voice->mod_x5_cur     += voice->mod_x5_inc;
         voice->mod_drive_cur  += voice->mod_drive_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ws_quintic_shared_t *ret = malloc(sizeof(ws_quintic_shared_t));
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
   ws_quintic_voice_t *ret = malloc(sizeof(ws_quintic_voice_t));
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

st_plugin_info_t *ws_quintic_init(void) {
   ws_quintic_info_t *ret = NULL;

   ret = malloc(sizeof(ws_quintic_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ws quintic";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "quintic polynomial shaper";
      ret->base.short_name  = "quintic";
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
