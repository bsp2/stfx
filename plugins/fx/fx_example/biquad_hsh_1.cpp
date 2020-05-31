// ----
// ---- file   : biquad_hsh_1.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a biquad high shelf filter that supports per-sample-frame parameter interpolation
// ----
// ---- created: 21May2020
// ---- changed: 24May2020, 31May2020
// ----
// ----
// ----

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../plugin.h"

#include "biquad.h"

#define PARAM_DRYWET   0
#define PARAM_DRIVE    1
#define PARAM_FREQ     2
#define PARAM_Q        3  // unused
#define PARAM_PAN      4
#define PARAM_GAIN     5
#define NUM_PARAMS     6
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Drive",
   "Freq",
   "-",
   "Pan",
   "Gain"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // DRIVE
   0.5f,  // FREQ
   0.0f,  // Q
   0.5f,  // PAN
   0.5f,  // GAIN
};

#define MOD_DRYWET  0
#define MOD_FREQ    1
#define MOD_Q       2  // unused
#define MOD_GAIN    3
#define NUM_MODS    4
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Freq",
   "-",
   "Gain"
};

typedef struct biquad_hsh_1_info_s {
   st_plugin_info_t base;
} biquad_hsh_1_info_t;

typedef struct biquad_hsh_1_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} biquad_hsh_1_shared_t;

typedef struct biquad_hsh_1_voice_s {
   st_plugin_voice_t base;
   float    sample_rate;
   float    mods[NUM_MODS];
   float    mod_drywet_cur;
   float    mod_drywet_inc;
   float    mod_drive_cur;
   float    mod_drive_inc;
   StBiquad hsh_1_l;
   StBiquad hsh_1_r;
} biquad_hsh_1_voice_t;


static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(biquad_hsh_1_voice_t);
   voice->sample_rate = _sampleRate;
}

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
   ST_PLUGIN_SHARED_CAST(biquad_hsh_1_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(biquad_hsh_1_shared_t);
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
   ST_PLUGIN_VOICE_CAST(biquad_hsh_1_voice_t);
   (void)_bGlide;
   (void)_voiceIdx;
   (void)_activeNoteIdx;
   (void)_note;
   (void)_noteHz;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));

      voice->hsh_1_r.reset();
      voice->hsh_1_l.reset();
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(biquad_hsh_1_voice_t);
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
   ST_PLUGIN_VOICE_CAST(biquad_hsh_1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(biquad_hsh_1_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modDrive = ((shared->params[PARAM_DRIVE] - 0.5f) * 2.0f);
   modDrive = powf(10.0f, modDrive * 2.0f);

   float modFreq = shared->params[PARAM_FREQ] + voice->mods[MOD_FREQ];
   float modPan  = ((shared->params[PARAM_PAN] - 0.5f) * 2.0f);
   float modFreqL = Dstplugin_clamp(modFreq - modPan, 0.0f, 0.99f);
   float modFreqR = Dstplugin_clamp(modFreq + modPan, 0.0f, 0.99f);
   modFreqL = (powf(2.0f, modFreqL * 7.0f) - 1.0f) / 127.0f;
   modFreqR = (powf(2.0f, modFreqR * 7.0f) - 1.0f) / 127.0f;

   float modGain = shared->params[PARAM_GAIN] + voice->mods[MOD_GAIN];
   modGain = Dstplugin_clamp(modGain, 0.0f, 1.0f);
   if(modGain < 0.5f)
   {
      modGain *= 2.0f;
      modGain = -90.0f + 90.0f * modGain;
   }
   else
   {
      modGain = (modGain - 0.5f) * 2.0f * 12.0f;
   }

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_drive_inc  = (modDrive  - voice->mod_drive_cur)  * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      voice->mod_drive_cur  = modDrive;
      voice->mod_drive_inc  = 0.0f;

      _numFrames = 1u;
   }

   voice->hsh_1_l.calcParams(_numFrames,
                             StBiquad::HSH,
                             modGain,
                             modFreqL,
                             0.0f/*q*/
                             );

   voice->hsh_1_r.calcParams(_numFrames,
                             StBiquad::HSH,
                             modGain,
                             modFreqR,
                             0.0f/*q*/
                             );
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(biquad_hsh_1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(biquad_hsh_1_shared_t);

   unsigned int k = 0u;

   // Stereo input, stereo output
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      float l = _samplesIn[k];
      float r = _samplesIn[k + 1u];
      float outL = voice->hsh_1_l.filter(l * voice->mod_drive_cur);
      float outR = voice->hsh_1_r.filter(r * voice->mod_drive_cur);
      outL = l + (outL - l) * voice->mod_drywet_cur;
      outR = r + (outR - r) * voice->mod_drywet_cur;
      _samplesOut[k]      = outL;
      _samplesOut[k + 1u] = outR;

      // Next frame
      k += 2u;
      voice->mod_drywet_cur += voice->mod_drywet_inc;
      voice->mod_drive_cur  += voice->mod_drive_inc;
   }

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   biquad_hsh_1_shared_t *ret = (biquad_hsh_1_shared_t *)malloc(sizeof(biquad_hsh_1_shared_t));
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
   biquad_hsh_1_voice_t *ret = (biquad_hsh_1_voice_t *)malloc(sizeof(biquad_hsh_1_voice_t));
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

extern "C" {
st_plugin_info_t *biquad_hsh_1_init(void) {
   biquad_hsh_1_info_t *ret = NULL;

   ret = (biquad_hsh_1_info_t *)malloc(sizeof(biquad_hsh_1_info_t));

   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp biquad hsh 1";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "biquad high shelf 1";
      ret->base.short_name  = "bq hsh 1";
      ret->base.flags       = ST_PLUGIN_FLAG_FX | ST_PLUGIN_FLAG_TRUE_STEREO_OUT;
      ret->base.category    = ST_PLUGIN_CAT_FILTER;
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
      ret->base.set_sample_rate  = &loc_set_sample_rate;
      ret->base.note_on          = &loc_note_on;
      ret->base.set_mod_value    = &loc_set_mod_value;
      ret->base.prepare_block    = &loc_prepare_block;
      ret->base.process_replace  = &loc_process_replace;
      ret->base.plugin_exit      = &loc_plugin_exit;
   }

   return &ret->base;
}
} // extern "C"
