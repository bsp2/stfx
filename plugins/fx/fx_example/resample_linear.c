// ----
// ---- file   : resample_linear.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020-2024 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a resampler with linear filtering
// ----
// ---- created: 21May2020
// ---- changed: 24May2020, 31May2020, 08Jun2020, 21Jan2024
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET   0
#define PARAM_RATE     1
#define NUM_PARAMS     2
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Rate"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f   // RATE
};

#define MOD_DRYWET  0
#define MOD_RATE    1
#define NUM_MODS    2
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Rate"
};

typedef struct resample_linear_info_s {
   st_plugin_info_t base;
} resample_linear_info_t;

typedef struct resample_linear_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} resample_linear_shared_t;

typedef struct resample_linear_voice_s {
   st_plugin_voice_t base;
   float          mods[NUM_MODS];
   float          mod_drywet_cur;
   float          mod_drywet_inc;
   float          mod_rate_cur;
   float          mod_rate_inc;
   unsigned short phase;
   float          last_sample_l;
   float          last_sample_r;
   float          next_sample_l;
   float          next_sample_r;
} resample_linear_voice_t;


static float loc_bipolar_to_scale(float _t, float _mul, float _div) {
   // t (0..1) => /_div .. *_mul   (0.5 = *1.0)
   if(_t < 0.0f)
      _t = 0.0f;
   else if(_t > 1.0f)
      _t = 1.0f;
   float s;
   if(_t < 0.5f)
   {
      s = (1.0f / _div);
      s = s + ( (1.0f - s) * (_t / 0.5f) );
   }
   else
   {
      s = 1.0f + ((_t - 0.5f) / (0.5f / (_mul - 1.0f)));
   }
   return s;
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
   ST_PLUGIN_SHARED_CAST(resample_linear_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(resample_linear_shared_t);
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
   ST_PLUGIN_VOICE_CAST(resample_linear_voice_t);
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
   ST_PLUGIN_VOICE_CAST(resample_linear_voice_t);
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
   ST_PLUGIN_VOICE_CAST(resample_linear_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(resample_linear_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modRate = shared->params[PARAM_RATE] + voice->mods[MOD_RATE];
   modRate = Dstplugin_clamp(modRate, 0.0f, 1.0f);
   modRate *= modRate;
   modRate *= 65536.0f;
   if(modRate < 2.0f)
      modRate = 2.0f;

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_rate_inc   = (modRate   - voice->mod_rate_cur)   * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      voice->mod_rate_cur   = modRate;
      voice->mod_rate_inc   = 0.0f;
      voice->phase = (unsigned short)-modRate;
      voice->last_sample_l = 0.0f;
      voice->last_sample_r = 0.0f;
      voice->next_sample_l = 0.0f;
      voice->next_sample_r = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(resample_linear_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(resample_linear_shared_t);

   unsigned int k = 0u;

   int lastPhase = voice->phase;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float out;
         float l = _samplesIn[k];
         voice->phase = (unsigned short)(voice->phase + (unsigned int)voice->mod_rate_cur);

         if(voice->phase <= lastPhase)
         {
            voice->last_sample_l = voice->next_sample_l;
            voice->next_sample_l = l;
         }

         float t = (voice->phase / 65535.0f);
         out = voice->last_sample_l + (voice->next_sample_l - voice->last_sample_l) * t;

         out = l + (out - l) * voice->mod_drywet_cur;

         _samplesOut[k]      = out;
         _samplesOut[k + 1u] = out;

         // Next frame
         k += 2u;
         lastPhase = voice->phase;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_rate_cur   += voice->mod_rate_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float outL;
         float outR;
         float l = _samplesIn[k];
         float r = _samplesIn[k + 1u];
         voice->phase = (unsigned short)(voice->phase + (unsigned int)voice->mod_rate_cur);

         if(voice->phase <= lastPhase)
         {
            voice->last_sample_l = voice->next_sample_l;
            voice->last_sample_r = voice->next_sample_r;
            voice->next_sample_l = l;
            voice->next_sample_r = r;
         }

         float t = (voice->phase / 65535.0f);
         outL = voice->last_sample_l + (voice->next_sample_l - voice->last_sample_l) * t;
         outR = voice->last_sample_r + (voice->next_sample_r - voice->last_sample_r) * t;

         outL = l + (outL - l) * voice->mod_drywet_cur;
         outR = r + (outR - r) * voice->mod_drywet_cur;

         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         lastPhase = voice->phase;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_rate_cur   += voice->mod_rate_inc;
      }
   }

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   resample_linear_shared_t *ret = malloc(sizeof(resample_linear_shared_t));
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
   resample_linear_voice_t *ret = malloc(sizeof(resample_linear_voice_t));
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

st_plugin_info_t *resample_linear_init(void) {
   resample_linear_info_t *ret = NULL;

   ret = malloc(sizeof(resample_linear_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp resample linear";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "resample linear";
      ret->base.short_name  = "res linear";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
      ret->base.category    = ST_PLUGIN_CAT_LOFI;
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
