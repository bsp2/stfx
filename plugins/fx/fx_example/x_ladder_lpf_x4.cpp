// ----
// ---- file   : x_ladder_lpf_x4.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a Ladder-like low pass filter that supports per-sample-frame parameter interpolation and filter FM
// ----           see <http://musicdsp.org/showArchiveComment.php?ArchiveID=24>
// ----
// ---- created: 05Jun2020
// ---- changed: 08Jun2020, 09Jun2020
// ----
// ----
// ----

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../plugin.h"

#include "ladder_lpf.h"

#define NUM_STAGES  (4u)

#define PARAM_DRYWET   0
#define PARAM_DRIVE    1
#define PARAM_FREQ     2
#define PARAM_Q        3
#define PARAM_VOICEBUS 4
#define PARAM_BUSLEVEL 5
#define PARAM_DRIVEDIV 6
#define PARAM_FREQDIV  7
#define NUM_PARAMS     8
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Drive",
   "Freq",
   "Q",
   "Voice Bus",
   "Bus Level",
   "Drive Div",
   "Freq Div"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // DRIVE
   1.0f,  // FREQ
   0.0f,  // Q
   0.0f,  // VOICEBUS
   1.0f,  // BUSLEVEL
   0.5f,  // DRIVEDIV
   0.5f,  // FREQDIV
};

#define MOD_DRYWET    0
#define MOD_DRIVE     1
#define MOD_FREQ      2
#define MOD_Q         3
#define MOD_PAN       4
#define MOD_BUSLEVEL  5
#define MOD_DRIVEDIV  6
#define MOD_FREQDIV   7
#define NUM_MODS      8
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Drive",
   "Freq",
   "Q",
   "Pan",
   "Bus Level",
   "Drive Div",
   "Freq Div"
};


typedef struct x_ladder_lpf_x4_info_s {
   st_plugin_info_t base;
} x_ladder_lpf_x4_info_t;

typedef struct x_ladder_lpf_x4_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} x_ladder_lpf_x4_shared_t;

typedef struct x_ladder_lpf_x4_voice_s {
   st_plugin_voice_t base;
   float        sample_rate;
   float        mods[NUM_MODS];
   float        mod_drywet_cur;
   float        mod_drywet_inc;
   float        mod_drive_cur[NUM_STAGES];
   float        mod_drive_inc[NUM_STAGES];
   float        mod_freql_cur[NUM_STAGES];
   float        mod_freql_inc[NUM_STAGES];
   float        mod_freqr_cur[NUM_STAGES];
   float        mod_freqr_inc[NUM_STAGES];
   float        mod_q_cur;
   float        mod_q_inc;
   unsigned int mod_voicebus_idx;
   float        mod_bus_lvl_cur;
   float        mod_bus_lvl_inc;
   StLadderLPF  lpf_l[NUM_STAGES];
   StLadderLPF  lpf_r[NUM_STAGES];
} x_ladder_lpf_x4_voice_t;


static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(x_ladder_lpf_x4_voice_t);
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
   ST_PLUGIN_SHARED_CAST(x_ladder_lpf_x4_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(x_ladder_lpf_x4_shared_t);
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
   ST_PLUGIN_VOICE_CAST(x_ladder_lpf_x4_voice_t);
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
   ST_PLUGIN_VOICE_CAST(x_ladder_lpf_x4_voice_t);
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
   ST_PLUGIN_VOICE_CAST(x_ladder_lpf_x4_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(x_ladder_lpf_x4_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET] + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modDriveBase = ((shared->params[PARAM_DRIVE] - 0.5f) * 2.0f) + voice->mods[MOD_DRIVE];
   float modDrive[NUM_STAGES];
   float modFreqL[NUM_STAGES];
   float modFreqR[NUM_STAGES];
   float modFreq = shared->params[PARAM_FREQ] + voice->mods[MOD_FREQ];
   float modPan  = voice->mods[MOD_PAN];
   for(unsigned int i = 0u; i < NUM_STAGES; i++)
   {
      modDrive[i] = powf(10.0f, modDriveBase * 2.0f);
      modDriveBase += ((shared->params[PARAM_DRIVEDIV]-0.5f) * 2.0f + voice->mods[MOD_DRIVEDIV])*0.25f;
      modFreqL[i] = modFreq - modPan;
      modFreqR[i] = modFreq + modPan;
      modFreq += ((shared->params[PARAM_FREQDIV]-0.5f) * 2.0f + voice->mods[MOD_FREQDIV])*0.25f;
   }

   float modQ = shared->params[PARAM_Q] + voice->mods[MOD_Q];
   modQ = Dstplugin_clamp(modQ, 0.0f, 1.0f);

   float modVoiceBus = shared->params[PARAM_VOICEBUS];
   Dstplugin_voicebus(voice->mod_voicebus_idx, modVoiceBus);

   float modBusLvl = (shared->params[PARAM_BUSLEVEL]-0.5f)*2.0f + voice->mods[MOD_BUSLEVEL];

   // printf("xxx freq=(%f; %f) q=%f\n", modFreqL, modFreqR, modQ);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc = (modDryWet - voice->mod_drywet_cur)   * recBlockSize;
      for(unsigned int i = 0u; i < NUM_STAGES; i++)
      {
         voice->mod_drive_inc[i] = (modDrive[i] - voice->mod_drive_cur[i]) * recBlockSize;
         voice->mod_freql_inc[i] = (modFreqL[i] - voice->mod_freql_cur[i]) * recBlockSize;
         voice->mod_freqr_inc[i] = (modFreqR[i] - voice->mod_freqr_cur[i]) * recBlockSize;
      }
      voice->mod_q_inc       = (modQ      - voice->mod_q_cur)       * recBlockSize;
      voice->mod_bus_lvl_inc = (modBusLvl - voice->mod_bus_lvl_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      for(unsigned int i = 0u; i < NUM_STAGES; i++)
      {
         voice->mod_drive_cur[i] = modDrive[i];
         voice->mod_drive_inc[i] = 0.0f;
         voice->mod_freql_cur[i] = modFreqL[i];
         voice->mod_freql_inc[i] = 0.0f;
         voice->mod_freqr_cur[i] = modFreqR[i];
         voice->mod_freqr_inc[i] = 0.0f;
      }
      voice->mod_q_cur       = modQ;
      voice->mod_q_inc       = 0.0f;
      voice->mod_bus_lvl_cur = modBusLvl;
      voice->mod_bus_lvl_inc = 0.0f;
   }

}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(x_ladder_lpf_x4_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(x_ladder_lpf_x4_shared_t);

   // printf("xxx voice->mod_voice_bus_idx=%u  voice->base.voice_bus_buffers=%p\n", voice->mod_voicebus_idx, voice->base.voice_bus_buffers);
   // fflush(stdout);
   // // return;

   // (note) always a valid ptr (points to "0"-filled buffer if layer does not exist)
   const float *samplesBus = voice->base.voice_bus_buffers[voice->mod_voicebus_idx] + voice->base.voice_bus_read_offset;

   // printf("xxx voice->mod_voice_bus_idx=%u  samplesBus=%p\n", voice->mod_voicebus_idx, samplesBus);
   // fflush(stdout);
   // return;

   unsigned int k = 0u;

   // Stereo input, stereo output
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      float l = _samplesIn[k];
      float r = _samplesIn[k + 1u];

      float busL = samplesBus[k];
      float busR = samplesBus[k + 1u];

      float outL = l;
      float outR = r;

      for(unsigned int i = 0u; i < NUM_STAGES; i++)
      {
         float modFreqL = Dstplugin_clamp(voice->mod_freql_cur[i] + busL * voice->mod_bus_lvl_cur, 0.0f, 1.0f);
         float modFreqR = Dstplugin_clamp(voice->mod_freqr_cur[i] + busR * voice->mod_bus_lvl_cur, 0.0f, 1.0f);
         modFreqL = (powf(2.0f, modFreqL * 7.0f) - 1.0f) / 127.0f;
         modFreqR = (powf(2.0f, modFreqR * 7.0f) - 1.0f) / 127.0f;

         voice->lpf_l[i].calcCoeff(1u,
                                   modFreqL,
                                   voice->mod_q_cur,
                                   1.0f/*drive3*/,
                                   1.0f/*drive4*/
                                   );

         voice->lpf_r[i].calcCoeff(1u,
                                   modFreqR,
                                   voice->mod_q_cur,
                                   1.0f/*drive3*/,
                                   1.0f/*drive4*/
                                   );


         outL = tanhf(outL * voice->mod_drive_cur[i]);
         outR = tanhf(outR * voice->mod_drive_cur[i]);

         outL = voice->lpf_l[i].filter(outL);
         outR = voice->lpf_r[i].filter(outR);

         voice->mod_drive_cur[i] += voice->mod_drive_inc[i];
         voice->mod_freql_cur[i] += voice->mod_freql_inc[i];
         voice->mod_freqr_cur[i] += voice->mod_freqr_inc[i];
      }

      outL = l + (outL - l) * voice->mod_drywet_cur;
      outR = r + (outR - r) * voice->mod_drywet_cur;
      _samplesOut[k]      = outL;
      _samplesOut[k + 1u] = outR;

      // Next frame
      k += 2u;
      voice->mod_drywet_cur  += voice->mod_drywet_inc;
      voice->mod_q_cur       += voice->mod_q_inc;
      voice->mod_bus_lvl_cur += voice->mod_bus_lvl_inc;
   }

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   x_ladder_lpf_x4_shared_t *ret = (x_ladder_lpf_x4_shared_t *)malloc(sizeof(x_ladder_lpf_x4_shared_t));
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
   x_ladder_lpf_x4_voice_t *ret = (x_ladder_lpf_x4_voice_t *)malloc(sizeof(x_ladder_lpf_x4_voice_t));
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
st_plugin_info_t *x_ladder_lpf_x4_init(void) {
   x_ladder_lpf_x4_info_t *ret = NULL;

   ret = (x_ladder_lpf_x4_info_t *)malloc(sizeof(x_ladder_lpf_x4_info_t));

   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp x ladder lpf x4";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "x ladder low pass filter x4";
      ret->base.short_name  = "x ladder lpf x4";
      ret->base.flags       = ST_PLUGIN_FLAG_FX | ST_PLUGIN_FLAG_XMOD | ST_PLUGIN_FLAG_TRUE_STEREO_OUT;
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
