// ----
// ---- file   : modfm.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2024 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : modfm **EXPERIMENTAL**   <https://core.ac.uk/download/pdf/297015586.pdf>
// ----           (note) strictly speaking, this implementation is not quite correct (I think at least an abs() is missing) but
// ----                   it sounds very nice and somewhat "organic"
// ----           (note) see modfm_* Cycle voice plugin patches for a mathematically correct (but "clinical" sounding) implementation
// ----
// ---- created: 05Feb2024
// ---- changed: 
// ----
// ----
// ----

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_AMP      0
#define PARAM_K        1
#define PARAM_N        2
#define PARAM_R        3
#define NUM_PARAMS     4
static const char *loc_param_names[NUM_PARAMS] = {
   "Amp",
   "k",
   "n",
   "r",
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // AMP
   0.5f,  // k
   0.5f,  // n
   0.5f,  // r
};

#define MOD_AMP     0
#define MOD_K       1
#define MOD_N       2
#define MOD_R       3
#define NUM_MODS    4
static const char *loc_mod_names[NUM_MODS] = {
   "Amp",
   "k",
   "n",
   "r"
};

typedef struct modfm_info_s {
   st_plugin_info_t base;
} modfm_info_t;

typedef struct modfm_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} modfm_shared_t;

typedef struct modfm_voice_s {
   st_plugin_voice_t base;
   float    mods[NUM_MODS];
   float    mod_amp_cur;
   float    mod_amp_inc;
   float    mod_k_cur;
   float    mod_k_inc;
   float    mod_n_cur;
   float    mod_n_inc;
   float    mod_r_cur;
   float    mod_r_inc;

   float sample_rate;
   float phase_c;  // 0..2PI
   float phase_m;  // 0..2PI
   float w_c;
} modfm_voice_t;


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
   ST_PLUGIN_SHARED_CAST(modfm_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(modfm_shared_t);
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
   ST_PLUGIN_VOICE_CAST(modfm_voice_t);
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(modfm_voice_t);
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
   ST_PLUGIN_VOICE_CAST(modfm_voice_t);
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
   ST_PLUGIN_VOICE_CAST(modfm_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(modfm_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float noteSpeed = _freqHz / voice->sample_rate;
   voice->w_c = noteSpeed * ST_PLUGIN_2PI_F;

   float modAmp = shared->params[PARAM_AMP]   + voice->mods[MOD_AMP];
   modAmp = Dstplugin_clamp(modAmp, 0.0f, 1.0f);

   float modK = ((shared->params[PARAM_K] - 0.5f) * 2.0f) + voice->mods[MOD_K];
   modK = powf(10.0f, modK * 2.0f);

   float modN = 1.0f + shared->params[PARAM_N] * 32.0f;

   float modR = ((shared->params[PARAM_R] - 0.5f) * 2.0f) + voice->mods[MOD_R];
   modR *= 16.0f;
   if(modR < 0.0f)
      modR = 1.0f / -modR;
   
   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_amp_inc = (modAmp - voice->mod_amp_cur) * recBlockSize;
      voice->mod_k_inc  = (modK  - voice->mod_k_cur)  * recBlockSize;
      voice->mod_n_inc  = (modN  - voice->mod_n_cur)  * recBlockSize;
      voice->mod_r_inc  = (modR  - voice->mod_r_cur)  * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_amp_cur = modAmp;
      voice->mod_amp_inc = 0.0f;
      voice->mod_k_cur  = modK;
      voice->mod_k_inc  = 0.0f;
      voice->mod_n_cur  = modN;
      voice->mod_n_inc  = 0.0f;
      voice->mod_r_cur  = modR;
      voice->mod_r_inc  = 0.0f;
   }
}

static unsigned int loc_fac(unsigned int i) {
   unsigned int r = i;
   while(i > 2u)
      r *= --i;
   return r;
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(modfm_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(modfm_shared_t);

   unsigned int outIdx = 0u;

   // Mono output
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      // // float l = _samplesIn[outIdx];
      double k = voice->mod_k_cur;
      if(k < 0.0)
         k = 1.0;

      float nf = voice->mod_n_cur;
      if(nf < 0.0f)
         nf = 0.0f;
      unsigned int nMax = (unsigned int)(nf+0.5f);

      // In(k) = e^k / sqrt(2pi*k)
      // A/e^k * sum{ In(k) * cos(wc*t + n*wm*t)
      //
      //  k=modulation index (modamt, e.g. 5)
      //  n=bessel function order

      double ek = pow(ST_PLUGIN_E, k);
      double sqk = sqrt(ST_PLUGIN_2PI * k);
      // double inkNorm = ek / sqk;  // orig
      double inkNorm = ek / k;
      // double inkNorm = 1.0f;
      double aek = 1.0f / ek;
      inkNorm *= voice->mod_amp_cur;

      double outL = 0.0;
      for(unsigned int n = 0u; n < nMax; n++)
      {
         double ink = 0.0;
         for(unsigned int m = 1u; m < 2u; m++)
         {
            ink += pow(k*0.5, n+2*m) / ( loc_fac(m) * loc_fac(m + n) );
         }
         static int xxx = 0;
         // if(0 == (++xxx & 16383))
         //    printf("xxx ink=%f inkNorm=%f aek=%f\n", ink, inkNorm, aek);

         outL += inkNorm * ink * cos(voice->phase_c + n * voice->phase_m);
      }

      outL *= aek;
      outL *= (1.0 / pow(1.75, k));
      // outL *= (1.0f / powf(1.75f, k));

      // outL = l + (outL - l) * voice->mod_drywet_cur;

      if(outL > 16.0f)
         outL = 16.0f;
      else if(outL < -16.0f)
         outL = -16.0f;

      _samplesOut[outIdx]      = (float)outL;
      _samplesOut[outIdx + 1u] = (float)outL;

      voice->phase_c += voice->w_c;
      if(voice->phase_c >= ST_PLUGIN_2PI_F)
         voice->phase_c -= voice->phase_c;

      voice->phase_m += voice->w_c * voice->mod_r_cur;
      if(voice->phase_m >= ST_PLUGIN_2PI_F)
         voice->phase_m -= voice->phase_m;

      // Next frame
      outIdx += 2u;
      voice->mod_amp_cur += voice->mod_amp_inc;
      voice->mod_k_cur  += voice->mod_k_inc;
      voice->mod_n_cur  += voice->mod_n_inc;

   }

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   modfm_shared_t *ret = (modfm_shared_t *)malloc(sizeof(modfm_shared_t));
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
   modfm_voice_t *ret = (modfm_voice_t *)malloc(sizeof(modfm_voice_t));
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

st_plugin_info_t *modfm_init(void) {
   modfm_info_t *ret = NULL;

   ret = (modfm_info_t *)malloc(sizeof(modfm_info_t));

   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp modfm";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "modfm";
      ret->base.short_name  = "modfm";
      ret->base.flags       = ST_PLUGIN_FLAG_OSC;
      ret->base.category    = ST_PLUGIN_CAT_UNKNOWN;
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
      ret->base.set_sample_rate    = &loc_set_sample_rate;
      ret->base.note_on            = &loc_note_on;
      ret->base.set_mod_value      = &loc_set_mod_value;
      ret->base.prepare_block      = &loc_prepare_block;
      ret->base.process_replace    = &loc_process_replace;
      ret->base.plugin_exit        = &loc_plugin_exit;
   }

   return &ret->base;
}
