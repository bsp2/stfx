// ----
// ---- file   : lrt_laika.cpp
// ---- author : Patrick Lindenberg & bsp
// ---- legal  : (c) 2017-2020 by Patrick Lindenberg
// ----
// ----
// ---- info   : diode ladder filter
// ----           (adapted for 'stfx' plugin API by bsp)
// ----
// ---- created: 07Jun2020
// ---- changed: 21Jan2024
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#include "dsp/DiodeLadder.h"

#define PARAM_DRYWET      0
#define PARAM_RESVD1      1 
#define PARAM_FREQ        2
#define PARAM_Q           3
#define PARAM_SATURATION  4
#define NUM_PARAMS        5
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "-",
   "Freq",
   "Q",
   "Saturation"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // RESVD1
   1.0f,  // FREQ
   0.0f,  // Q
   0.0f,  // SATURATION
};

#define MOD_DRYWET      0
#define MOD_FREQ        1
#define MOD_Q           2
#define MOD_SATURATION  3
#define NUM_MODS        4
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Freq",
   "Q",
   "Saturation",
};

typedef struct lrt_laika_info_s {
   st_plugin_info_t base;
} lrt_laika_info_t;

typedef struct lrt_laika_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} lrt_laika_shared_t;

struct LaikaFilter {
   dsp::DiodeLadderFilter filter;

   LaikaFilter(void) : filter(44100.0f) {
   }

   ~LaikaFilter(void) {
   }

   inline void setSampleRate(float _sampleRate) {
      filter.setSamplerate(_sampleRate);
   }

   inline float process(const float _inSmp,
                        const float _freq,
                        const float _q,
                        const float _saturation
                        ) {

      filter.setFrequency(_freq);
      filter.setResonance(_q);
      filter.setSaturation(_saturation);

      filter.low = true;

      filter.setIn(_inSmp);
      filter.invalidate();
      filter.process();
      
      return filter.getOut();
   }
      
};

typedef struct lrt_laika_voice_s {
   st_plugin_voice_t base;
   float      sample_rate;
   float      mods[NUM_MODS];
   float      mod_drywet_cur;
   float      mod_drywet_inc;
   float      mod_freq_cur;
   float      mod_freq_inc;
   float      mod_q_cur;
   float      mod_q_inc;
   float      mod_q_gain_cur;
   float      mod_q_gain_inc;
   float      mod_saturation_cur;
   float      mod_saturation_inc;
   LaikaFilter flt_l;
   LaikaFilter flt_r;
} lrt_laika_voice_t;


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
   ST_PLUGIN_SHARED_CAST(lrt_laika_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(lrt_laika_shared_t);
   shared->params[_paramIdx] = _value;
}

static const char *ST_PLUGIN_API loc_get_mod_name(st_plugin_info_t *_info,
                                                  unsigned int      _modIdx
                                                  ) {
   (void)_info;
   return loc_mod_names[_modIdx];
}

static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(lrt_laika_voice_t);
   voice->sample_rate = _sampleRate;
   voice->flt_l.setSampleRate(_sampleRate);
   voice->flt_r.setSampleRate(_sampleRate);
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(lrt_laika_voice_t);
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
   ST_PLUGIN_VOICE_CAST(lrt_laika_voice_t);
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
   ST_PLUGIN_VOICE_CAST(lrt_laika_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(lrt_laika_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modFreq = shared->params[PARAM_FREQ] + voice->mods[MOD_FREQ];
   modFreq = Dstplugin_clamp(modFreq, 0.0f, 1.0f);

   float modQ = shared->params[PARAM_Q] + voice->mods[MOD_Q];
   modQ = Dstplugin_clamp(modQ, 0.0f, 1.0f);
   float s = modQ * modQ;
   float pGain = powf(10.0f, s*2.5f);
   pGain = Dstplugin_clamp(pGain, 0.0f, 2.3f);
   float lGain = modQ*1.4f;//modQ*1.0f;
   float thres = 0.5f;
   if(modQ >= thres)
   {
      s = (modQ - thres) / (1.0f - thres);
      s = s*s * (3.0f - 2.0f * s);
      pGain = pGain + (lGain - pGain) * s;
   }
   float modQGain = 0.2f + pGain;
   modQGain *= 1.5f;
   // modQGain = 1.0f;
   modQ *= dsp::DiodeLadderFilter::MAX_RESONANCE;

   float modSaturation = shared->params[PARAM_SATURATION] + voice->mods[MOD_SATURATION];
   modSaturation = Dstplugin_clamp(modSaturation, 0.0f, 1.0f);
   modSaturation = 1.0f + modSaturation * 14.0f;

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc     = (modDryWet     - voice->mod_drywet_cur)     * recBlockSize;
      voice->mod_freq_inc       = (modFreq       - voice->mod_freq_cur)       * recBlockSize;
      voice->mod_q_inc          = (modQ          - voice->mod_q_cur)          * recBlockSize;
      voice->mod_q_gain_inc     = (modQGain      - voice->mod_q_gain_cur)     * recBlockSize;
      voice->mod_saturation_inc = (modSaturation - voice->mod_saturation_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur     = modDryWet;
      voice->mod_drywet_inc     = 0.0f;
      voice->mod_freq_cur       = modFreq;
      voice->mod_freq_inc       = 0.0f;
      voice->mod_q_cur          = modQ;
      voice->mod_q_inc          = 0.0f;
      voice->mod_q_gain_cur     = modQGain;
      voice->mod_q_gain_inc     = 0.0f;
      voice->mod_saturation_cur = modSaturation;
      voice->mod_saturation_inc = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(lrt_laika_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(lrt_laika_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];

         float outL = voice->flt_l.process(l,
                                           voice->mod_freq_cur,
                                           voice->mod_q_cur,
                                           voice->mod_saturation_cur
                                           );
         outL *= voice->mod_q_gain_cur;

         outL = l + (outL - l) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outL;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur     += voice->mod_drywet_inc;
         voice->mod_freq_cur       += voice->mod_freq_inc;
         voice->mod_q_cur          += voice->mod_q_inc;
         voice->mod_q_gain_cur     += voice->mod_q_gain_inc;
         voice->mod_saturation_cur += voice->mod_saturation_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float r = _samplesIn[k + 1u];

         float outL = voice->flt_l.process(l,
                                           voice->mod_freq_cur,
                                           voice->mod_q_cur,
                                           voice->mod_saturation_cur
                                           );
         outL *= voice->mod_q_gain_cur;

         float outR = voice->flt_r.process(r,
                                           voice->mod_freq_cur,
                                           voice->mod_q_cur,
                                           voice->mod_saturation_cur
                                           );
         outR *= voice->mod_q_gain_cur;

         outL = l + (outL - l) * voice->mod_drywet_cur;
         outR = r + (outR - r) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);
         outR = Dstplugin_fix_denorm_32(outR);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur     += voice->mod_drywet_inc;
         voice->mod_freq_cur       += voice->mod_freq_inc;
         voice->mod_q_cur          += voice->mod_q_inc;
         voice->mod_q_gain_cur     += voice->mod_q_gain_inc;
         voice->mod_saturation_cur += voice->mod_saturation_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   lrt_laika_shared_t *ret = (lrt_laika_shared_t *)malloc(sizeof(lrt_laika_shared_t));
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

static st_plugin_voice_t *ST_PLUGIN_API loc_voice_new(st_plugin_info_t *_info, unsigned int _voiceIdx) {
   (void)_voiceIdx;
   lrt_laika_voice_t *ret = (lrt_laika_voice_t *)malloc(sizeof(lrt_laika_voice_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info   = _info;
      new(&ret->flt_l)LaikaFilter();
      new(&ret->flt_r)LaikaFilter();
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(lrt_laika_voice_t);
   voice->flt_l.~LaikaFilter();
   voice->flt_r.~LaikaFilter();
   free((void*)_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {
   free((void*)_info);
}

extern "C" {
st_plugin_info_t *lrt_laika_init(void) {
   lrt_laika_info_t *ret = (lrt_laika_info_t *)malloc(sizeof(lrt_laika_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "lrt laika diode filter";  // unique id. don't change this in future builds.
      ret->base.author      = "lrt & bsp";
      ret->base.name        = "lrt laika diode filter";
      ret->base.short_name  = "lrt laika lpf";
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
