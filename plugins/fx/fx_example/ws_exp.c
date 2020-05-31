// ----
// ---- file   : ws_exp.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : an exp/sin waveshaper that supports per-sample-frame parameter interpolation
// ---            see <http://www.csounds.com/ezine/winter1999/processing/>
// ----
// ---- created: 10May2009
// ---- changed: 23May2010, 30Sep2010, 07Oct2010, 20May2020, 21May2020, 24May2020, 31May2020
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET        0
#define PARAM_DRIVE         1
#define PARAM_INPUT_LEVEL   2
#define PARAM_MAX_OUTPUT    3
#define NUM_PARAMS          4
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Drive",
   "Input Level",
   "Max Output"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.0f,  // DRIVE
   0.5f,  // INPUT_LEVEL
   0.15f, // MAX_OUTPUT
};

#define MOD_DRYWET  0
#define MOD_DRIVE   1
#define NUM_MODS    2
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Drive"
};

typedef struct ws_exp_info_s {
   st_plugin_info_t base;
} ws_exp_info_t;

typedef struct ws_exp_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ws_exp_shared_t;

typedef struct ws_exp_voice_s {
   st_plugin_voice_t base;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_drive_cur;
   float mod_drive_inc;
   float mod_max_cur;
   float mod_max_inc;
} ws_exp_voice_t;


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
   ST_PLUGIN_SHARED_CAST(ws_exp_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ws_exp_shared_t);
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
   ST_PLUGIN_VOICE_CAST(ws_exp_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_exp_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_exp_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_exp_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modDriveN = shared->params[PARAM_DRIVE] + voice->mods[MOD_DRIVE];
   float modDrive = powf(10.0f, modDriveN * 2.0f);
   float inputLvl = shared->params[PARAM_INPUT_LEVEL] * 0.25f;
   inputLvl = 1.0f - inputLvl;
   inputLvl *= inputLvl;
   inputLvl = 1.0f - inputLvl;
   modDrive *= inputLvl;

   float modMax = Dstplugin_clamp(modDriveN, 0.0f, 1.0f);
   modMax = 1.0f + (shared->params[PARAM_MAX_OUTPUT] - 1.0f) * modDriveN;

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_drive_inc  = (modDrive  - voice->mod_drive_cur)  * recBlockSize;
      voice->mod_max_inc    = (modMax    - voice->mod_max_cur)    * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      voice->mod_drive_cur  = modDrive;
      voice->mod_drive_inc  = 0.0f;
      voice->mod_max_cur    = modMax;
      voice->mod_max_inc    = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(ws_exp_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_exp_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float a = l * voice->mod_drive_cur;
         a = (float) (
            (exp(sin(a)*4) - exp(-sin(a)*4*1.25)) /
            (exp(sin(a)*4) + exp(-sin(a)*4))
                      );
         a *= voice->mod_max_cur;
         float out = l + (a - l) * voice->mod_drywet_cur;
         out = Dstplugin_fix_denorm_32(out);
         _samplesOut[k]      = out;
         _samplesOut[k + 1u] = out;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_drive_cur  += voice->mod_drive_inc;
         voice->mod_max_cur    += voice->mod_max_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float al = l * voice->mod_drive_cur;
         al = (float) (
            (exp(sin(al)*4) - exp(-sin(al)*4*1.25)) /
            (exp(sin(al)*4) + exp(-sin(al)*4))
                      );
         al *= voice->mod_max_cur;
         float outL = l + (al - l) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);

         float r = _samplesIn[k + 1u];
         float ar = r * voice->mod_drive_cur;
         ar = (float) (
            (exp(sin(ar)*4) - exp(-sin(ar)*4*1.25)) /
            (exp(sin(ar)*4) + exp(-sin(ar)*4))
                      );
         ar *= voice->mod_max_cur;
         float outR = r + (ar - r) * voice->mod_drywet_cur;
         outR = Dstplugin_fix_denorm_32(outR);

         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_drive_cur  += voice->mod_drive_inc;
         voice->mod_max_cur    += voice->mod_max_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ws_exp_shared_t *ret = malloc(sizeof(ws_exp_shared_t));
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
   ws_exp_voice_t *ret = malloc(sizeof(ws_exp_voice_t));
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

st_plugin_info_t *ws_exp_init(void) {
   ws_exp_info_t *ret = NULL;

   ret = malloc(sizeof(ws_exp_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ws exp";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "exp/sin shaper";
      ret->base.short_name  = "exp";
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
