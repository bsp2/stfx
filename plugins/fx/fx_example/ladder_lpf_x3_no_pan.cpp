// ----
// ---- file   : ladder_lpf_x3_no_pan.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a Ladder-like low pass filter that supports per-sample-frame parameter interpolation
// ----           see <http://musicdsp.org/showArchiveComment.php?ArchiveID=24>
// ----
// ---- created: 05Jun2020
// ---- changed: 06Jun2020, 08Jun2020
// ----
// ----
// ----

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../plugin.h"

#include "ladder_lpf.h"

#define NUM_STAGES  (3u)

#define PARAM_DRYWET   0
#define PARAM_DRIVE1   1
#define PARAM_FREQ     2
#define PARAM_Q        3
#define PARAM_DIV      4
#define PARAM_DRIVE2   5
#define PARAM_DRIVE3   6
#define PARAM_DRIVE4   7
#define NUM_PARAMS     8
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Drive 1",
   "Freq",
   "Q",
   "Divergence",
   "Drive 2",
   "Drive 3",
   "Drive 4",
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.37f, // DRIVE1
   1.0f,  // FREQ
   0.0f,  // Q
   0.5f,  // DIVERGENCE
   0.46f, // DRIVE2
   0.37f, // DRIVE3
   0.6f,  // DRIVE4
};

#define MOD_DRIVE   0
#define MOD_FREQ    1
#define MOD_Q       2
#define MOD_DRIVE2  3
#define NUM_MODS    4
static const char *loc_mod_names[NUM_MODS] = {
   "Drive",
   "Freq",
   "Q",
   "Drive2"
};

typedef struct ladder_lpf_x3_no_pan_info_s {
   st_plugin_info_t base;
} ladder_lpf_x3_no_pan_info_t;

typedef struct ladder_lpf_x3_no_pan_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ladder_lpf_x3_no_pan_shared_t;

typedef struct ladder_lpf_x3_no_pan_voice_s {
   st_plugin_voice_t base;
   float     sample_rate;
   float     mods[NUM_MODS];
   float     mod_drywet_cur;
   float     mod_drywet_inc;
   float     mod_drive1_cur;
   float     mod_drive1_inc;
   float     mod_drive2_cur;
   float     mod_drive2_inc;
   StLadderLPF lpf_l[NUM_STAGES];
   StLadderLPF lpf_r[NUM_STAGES];
} ladder_lpf_x3_no_pan_voice_t;


static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(ladder_lpf_x3_no_pan_voice_t);
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
   ST_PLUGIN_SHARED_CAST(ladder_lpf_x3_no_pan_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ladder_lpf_x3_no_pan_shared_t);
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
   ST_PLUGIN_VOICE_CAST(ladder_lpf_x3_no_pan_voice_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));

      for(unsigned int i = 0u; i < NUM_STAGES; i++)
      {
         voice->lpf_l[i].reset();
         voice->lpf_r[i].reset();
      }
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(ladder_lpf_x3_no_pan_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ladder_lpf_x3_no_pan_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ladder_lpf_x3_no_pan_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modDrive1 = ((shared->params[PARAM_DRIVE1] - 0.5f) * 2.0f) + voice->mods[MOD_DRIVE]*0.005f;
   modDrive1 = powf(10.0f, modDrive1 * 2.0f);

   float modDrive2 = ((shared->params[PARAM_DRIVE2] - 0.5f) * 2.0f) + voice->mods[MOD_DRIVE]*0.2f;
   modDrive2 = powf(10.0f, modDrive2 * 2.0f);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_drive1_inc = (modDrive1 - voice->mod_drive1_cur) * recBlockSize;
      voice->mod_drive2_inc = (modDrive2 - voice->mod_drive2_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      voice->mod_drive1_cur = modDrive1;
      voice->mod_drive1_inc = 0.0f;
      voice->mod_drive2_cur = modDrive2;
      voice->mod_drive2_inc = 0.0f;
   }

   for(unsigned int i = 0u; i < NUM_STAGES; i++)
   {
      float div = float(i) / NUM_STAGES;
      div = powf(10.0f, (shared->params[PARAM_DIV]-0.5f)*2.0f * 3.0f * div);

      float modDrive3 = ((shared->params[PARAM_DRIVE3] - 0.5f) * 2.0f) * div + voice->mods[MOD_DRIVE2]*0.025f;
      modDrive3 = powf(10.0f, modDrive3 * 1.0f);

      float modDrive4 = ((shared->params[PARAM_DRIVE4] - 0.5f) * 2.0f) * div + voice->mods[MOD_DRIVE2]*0.1f;
      modDrive4 = powf(10.0f, modDrive4 * 2.0f);

      float modFreq = shared->params[PARAM_FREQ] * div + voice->mods[MOD_FREQ];
      modFreq = Dstplugin_clamp(modFreq, 0.0f, 1.0f);
      modFreq = (powf(2.0f, modFreq * 7.0f) - 1.0f) / 127.0f;

      float modQ = shared->params[PARAM_Q] * div + voice->mods[MOD_Q];
      modQ = Dstplugin_clamp(modQ, 0.0f, 1.0f);

      voice->lpf_l[i].calcCoeff(_numFrames,
                                modFreq,
                                modQ,
                                modDrive3,
                                modDrive4
                                );

      voice->lpf_r[i].calcCoeff(_numFrames,
                                modFreq,
                                modQ,
                                modDrive3,
                                modDrive4
                                );
   }

}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(ladder_lpf_x3_no_pan_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ladder_lpf_x3_no_pan_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];

         float outL = l;

         for(unsigned int i = 0u; i < NUM_STAGES; i++)
         {
#if 1
            outL = tanhf(outL * voice->mod_drive1_cur) * voice->mod_drive2_cur;
// #else
            outL = (float) (
               (exp(sin(outL)*4) - exp(-sin(outL)*4*1.25)) /
               (exp(sin(outL)*4) + exp(-sin(outL)*4))
                            );
#endif
            outL = voice->lpf_l[i].filter(outL);
         }
         outL = l + (outL - l) * voice->mod_drywet_cur;
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outL;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_drive1_cur += voice->mod_drive1_inc;
         voice->mod_drive2_cur += voice->mod_drive2_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float r = _samplesIn[k + 1u];

         float outL = l;
         float outR = r;

         for(unsigned int i = 0u; i < NUM_STAGES; i++)
         {
#if 1
            outL = tanhf(outL * voice->mod_drive1_cur) * voice->mod_drive2_cur;
            outR = tanhf(outR * voice->mod_drive1_cur) * voice->mod_drive2_cur;
// #else
            outL = (float) (
               (exp(sin(outL)*4) - exp(-sin(outL)*4*1.25)) /
               (exp(sin(outL)*4) + exp(-sin(outL)*4))
                            );

            outR = (float) (
               (exp(sin(outR)*4) - exp(-sin(outR)*4*1.25)) /
               (exp(sin(outR)*4) + exp(-sin(outR)*4))
                            );
#endif
            outL = voice->lpf_l[i].filter(outL);
            outR = voice->lpf_r[i].filter(outR);
         }
         outL = l + (outL - l) * voice->mod_drywet_cur;
         outR = r + (outR - r) * voice->mod_drywet_cur;
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_drive1_cur += voice->mod_drive1_inc;
         voice->mod_drive2_cur += voice->mod_drive2_inc;
      }
   }

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ladder_lpf_x3_no_pan_shared_t *ret = (ladder_lpf_x3_no_pan_shared_t *)malloc(sizeof(ladder_lpf_x3_no_pan_shared_t));
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
   ladder_lpf_x3_no_pan_voice_t *ret = (ladder_lpf_x3_no_pan_voice_t *)malloc(sizeof(ladder_lpf_x3_no_pan_voice_t));
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
st_plugin_info_t *ladder_lpf_x3_no_pan_init(void) {
   ladder_lpf_x3_no_pan_info_t *ret = NULL;

   ret = (ladder_lpf_x3_no_pan_info_t *)malloc(sizeof(ladder_lpf_x3_no_pan_info_t));

   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ladder lpf x3 no pan";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "ladder low pass filter x3 no pan";
      ret->base.short_name  = "ladder lpf x3 np";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
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
