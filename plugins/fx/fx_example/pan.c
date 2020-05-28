// ----
// ---- file   : amp.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : amplifier
// ----
// ---- created: 21May2020
// ---- changed: 24May2020
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
#define PARAM_PAN      2
#define NUM_PARAMS     3
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Drive",
   "Pan"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // DRIVE
   0.5f,  // PAN
};

#define MOD_DRYWET  0
#define MOD_DRIVE   1
#define MOD_PAN     2
#define NUM_MODS    3
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Drive",
   "Pan"
};

typedef struct pan_info_s {
   st_plugin_info_t base;
} pan_info_t;

typedef struct pan_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} pan_shared_t;

typedef struct pan_voice_s {
   st_plugin_voice_t base;
   float    mods[NUM_MODS];
   float    mod_drywet_cur;
   float    mod_drywet_inc;
   float    mod_drive_l_cur;
   float    mod_drive_l_inc;
   float    mod_drive_r_cur;
   float    mod_drive_r_inc;
} pan_voice_t;


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
   ST_PLUGIN_SHARED_CAST(pan_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(pan_shared_t);
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
   ST_PLUGIN_VOICE_CAST(pan_voice_t);
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
   ST_PLUGIN_VOICE_CAST(pan_voice_t);
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
   ST_PLUGIN_VOICE_CAST(pan_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(pan_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modDrive = ((shared->params[PARAM_DRIVE] - 0.5f) * 2.0f);
   modDrive = powf(10.0f, modDrive * 2.0f);

   float modPan = (shared->params[PARAM_PAN] - 0.5f) * 2.0f + voice->mods[MOD_PAN];
   modPan = Dstplugin_clamp(modPan, -1.0f, 1.0f);

   // linear panning law
   float modDriveL = modDrive * ( (modPan < 0.0f) ? 1.0f : (1.0f - modPan) );
   float modDriveR = modDrive * ( (modPan > 0.0f) ? 1.0f : (1.0f + modPan) );

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc  = (modDryWet - voice->mod_drywet_cur)  * recBlockSize;
      voice->mod_drive_l_inc = (modDriveL - voice->mod_drive_l_cur) * recBlockSize;
      voice->mod_drive_r_inc = (modDriveR - voice->mod_drive_r_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur  = modDryWet;
      voice->mod_drywet_inc  = 0.0f;
      voice->mod_drive_l_cur = modDriveL;
      voice->mod_drive_l_inc = 0.0f;
      voice->mod_drive_r_cur = modDriveR;
      voice->mod_drive_r_inc = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_fx_replace(st_plugin_voice_t  *_voice,
                                                 int                 _bMonoIn,
                                                 const float        *_samplesIn,
                                                 float              *_samplesOut, 
                                                 unsigned int        _numFrames
                                                 ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(pan_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(pan_shared_t);

   unsigned int k = 0u;

   // Stereo input, stereo output
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      float l = _samplesIn[k];
      float r = _samplesIn[k + 1u];
      float outL = l * voice->mod_drive_l_cur;
      float outR = r * voice->mod_drive_r_cur;
      outL = l + (outL - l) * voice->mod_drywet_cur;
      outR = r + (outR - r) * voice->mod_drywet_cur;
      _samplesOut[k]      = outL;
      _samplesOut[k + 1u] = outR;

      // Next frame
      k += 2u;
      voice->mod_drywet_cur  += voice->mod_drywet_inc;
      voice->mod_drive_l_cur += voice->mod_drive_l_inc;
      voice->mod_drive_r_cur += voice->mod_drive_r_inc;
   }

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   pan_shared_t *ret = (pan_shared_t *)malloc(sizeof(pan_shared_t));
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
   pan_voice_t *ret = (pan_voice_t *)malloc(sizeof(pan_voice_t));
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

st_plugin_info_t *pan_init(void) {
   pan_info_t *ret = NULL;

   ret = (pan_info_t *)malloc(sizeof(pan_info_t));

   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp pan";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "pan";
      ret->base.short_name  = "pan";
      ret->base.flags       = ST_PLUGIN_FLAG_FX | ST_PLUGIN_FLAG_TRUE_STEREO_OUT;
      ret->base.category    = ST_PLUGIN_CAT_PAN;
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
      ret->base.process_fx_replace = &loc_process_fx_replace;
      ret->base.plugin_exit        = &loc_plugin_exit;
   }

   return &ret->base;
}
