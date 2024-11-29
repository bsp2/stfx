// ----
// ---- file   : x_mul.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020-2024 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a simple voice bus ring modulator
// ----
// ---- created: 08Jun2020
// ---- changed: 14Jan2024, 19Jan2024, 21Jan2024
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "../../../plugin.h"

#define PARAM_DRYWET    0
#define PARAM_VOICEBUS  1
#define PARAM_LVL_IN    2
#define PARAM_LVL_BUS   3
#define PARAM_MIX_IN    4
#define PARAM_MIX_BUS   5
#define NUM_PARAMS      6
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Voice Bus",
   "Lvl In",
   "Lvl Bus",
   "Mix In",
   "Mix Bus"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.0f,  // VOICEBUS (0=prev, 0.01=bus 1, 0.02=bus 2, ..0.32=bus 32)
   0.5f,  // LVL_IN
   0.5f,  // LVL_BUS
   0.5f,  // MIX_IN
   0.5f,  // MIX_BUS
};

#define MOD_DRYWET    0
#define MOD_VOICEBUS  1
#define MOD_LVL_IN    2
#define MOD_LVL_BUS   3
#define MOD_MIX_IN    4
#define MOD_MIX_BUS   5
#define NUM_MODS      6
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Voice Bus",
   "Lvl In",
   "Lvl Bus",
   "Mix In",
   "Mix Bus",
};

typedef struct x_mul_info_s {
   st_plugin_info_t base;
} x_mul_info_t;

typedef struct x_mul_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} x_mul_shared_t;

typedef struct x_mul_voice_s {
   st_plugin_voice_t base;
   float        mods[NUM_MODS];
   float        mod_drywet_cur;
   float        mod_drywet_inc;
   unsigned int mod_voicebus_idx;
   float        mod_lvl_in_cur;
   float        mod_lvl_in_inc;
   float        mod_lvl_bus_cur;
   float        mod_lvl_bus_inc;
   float        mod_mix_in_cur;
   float        mod_mix_in_inc;
   float        mod_mix_bus_cur;
   float        mod_mix_bus_inc;
} x_mul_voice_t;


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
   ST_PLUGIN_SHARED_CAST(x_mul_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(x_mul_shared_t);
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
   ST_PLUGIN_VOICE_CAST(x_mul_voice_t);
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
   ST_PLUGIN_VOICE_CAST(x_mul_voice_t);
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
   ST_PLUGIN_VOICE_CAST(x_mul_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(x_mul_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modVoiceBus = shared->params[PARAM_VOICEBUS] + voice->mods[MOD_VOICEBUS];
   Dstplugin_voicebus_f(voice->mod_voicebus_idx, modVoiceBus);

   float modLvlIn  = (shared->params[PARAM_LVL_IN] -0.5f)*2.0f + voice->mods[MOD_LVL_IN];
   float modLvlBus = (shared->params[PARAM_LVL_BUS]-0.5f)*2.0f + voice->mods[MOD_LVL_BUS];
   float modMixIn  = (shared->params[PARAM_MIX_IN] -0.5f)*2.0f + voice->mods[MOD_MIX_IN];
   float modMixBus = (shared->params[PARAM_MIX_BUS]-0.5f)*2.0f + voice->mods[MOD_MIX_BUS];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc   = (modDryWet   - voice->mod_drywet_cur)   * recBlockSize;
      voice->mod_lvl_in_inc   = (modLvlIn    - voice->mod_lvl_in_cur)   * recBlockSize;
      voice->mod_lvl_bus_inc  = (modLvlBus   - voice->mod_lvl_bus_cur)  * recBlockSize;
      voice->mod_mix_in_inc   = (modMixIn    - voice->mod_mix_in_cur)   * recBlockSize;
      voice->mod_mix_bus_inc  = (modMixBus   - voice->mod_mix_bus_cur)  * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur   = modDryWet;
      voice->mod_drywet_inc   = 0.0f;
      voice->mod_lvl_in_cur   = modLvlIn;
      voice->mod_lvl_in_inc   = 0.0f;
      voice->mod_lvl_bus_cur  = modLvlBus;
      voice->mod_lvl_bus_inc  = 0.0f;
      voice->mod_mix_in_cur   = modMixIn;
      voice->mod_mix_in_inc   = 0.0f;
      voice->mod_mix_bus_cur  = modMixBus;
      voice->mod_mix_bus_inc  = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(x_mul_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(x_mul_shared_t);

   unsigned int k = 0u;

   // (note) always a valid ptr (points to "0"-filled buffer if layer does not exist)
   const float *samplesBus = voice->base.voice_bus_buffers[voice->mod_voicebus_idx] + voice->base.voice_bus_read_offset;

   // Stereo input, stereo output
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      const float l = _samplesIn[k];
      const float r = _samplesIn[k + 1u];

      const float lAmp = l * voice->mod_lvl_in_cur;
      const float rAmp = r * voice->mod_lvl_in_cur;

      const float busL = samplesBus[k];
      const float busR = samplesBus[k + 1u];

      const float busLAmp = busL * voice->mod_lvl_bus_cur;
      const float busRAmp = busR * voice->mod_lvl_bus_cur;

      // printf("xxx bus[%u] = %f    l=%f\n", k, busL, l);

      // (todo) combine mod_lvl_bus_cur * mod_lvl_in_cur
      float outL =
         (lAmp * busRAmp)
         + (l    * voice->mod_mix_in_cur)
         + (busL * voice->mod_mix_bus_cur)
         ;
      float outR =
         (r * voice->mod_lvl_in_cur) * (busR * voice->mod_lvl_bus_cur)
         + (r    * voice->mod_mix_in_cur)
         + (busR * voice->mod_mix_bus_cur)
         ;

      outL = l + (outL - l) * voice->mod_drywet_cur;
      outR = r + (outR - r) * voice->mod_drywet_cur;

      // outL = busL;
      // outR = busR;

      outL = Dstplugin_fix_denorm_32(outL);
      outR = Dstplugin_fix_denorm_32(outR);

      _samplesOut[k]      = outL;
      _samplesOut[k + 1u] = outR;

      // Next frame
      k += 2u;
      voice->mod_drywet_cur  += voice->mod_drywet_inc;
      voice->mod_lvl_in_cur  += voice->mod_lvl_in_inc;
      voice->mod_lvl_bus_cur += voice->mod_lvl_bus_inc;
      voice->mod_mix_in_cur  += voice->mod_mix_in_inc;
      voice->mod_mix_bus_cur += voice->mod_mix_bus_inc;
   }

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   x_mul_shared_t *ret = malloc(sizeof(x_mul_shared_t));
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
   x_mul_voice_t *ret = malloc(sizeof(x_mul_voice_t));
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

st_plugin_info_t *x_mul_init(void) {
   x_mul_info_t *ret = NULL;

   ret = malloc(sizeof(x_mul_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp x mul";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "x mul";
      ret->base.short_name  = "x mul";
      ret->base.flags       = ST_PLUGIN_FLAG_FX | ST_PLUGIN_FLAG_XMOD | ST_PLUGIN_FLAG_TRUE_STEREO_OUT;
      ret->base.category    = ST_PLUGIN_CAT_RINGMOD;
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
