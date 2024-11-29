// ----
// ---- file   : eq3.cpp
// ---- author : 
// ---- legal  : (c) 2021-2024 by Neil C / Etanza Systems / 2K6
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : simple 3-band EQ
// ----
// ---- created: 31May2021
// ---- changed: 21Jan2024
// ----
// ----
// ----

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET   0
#define PARAM_GAIN     1
#define PARAM_FREQLO   2
#define PARAM_GAINLO   3
#define PARAM_FREQHI   4
#define PARAM_GAINHI   5
#define PARAM_GAINMID  6
#define NUM_PARAMS     7
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Gain",
   "Freq Low",
   "Gain Low",
   "Freq High",
   "Gain High",
   "Gain Mid",
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // GAIN
   0.5f,  // FREQLO
   0.5f,  // GAINLO
   0.5f,  // FREQHI
   0.5f,  // GAINHI
   0.5f,  // GAINMID
};

#define MOD_DRYWET   0
#define MOD_GAIN     1
#define MOD_FREQLO   2
#define MOD_GAINLO   3
#define MOD_FREQHI   4
#define MOD_GAINHI   5
#define MOD_GAINMID  6
#define NUM_MODS     7
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Gain",
   "Freq Low",
   "Gain Low",
   "Freq High",
   "Gain High",
   "Gain Mid",
};

typedef struct eq3_info_s {
   st_plugin_info_t base;
} eq3_info_t;

typedef struct eq3_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} eq3_shared_t;


typedef float sF32;
typedef double sF64;

class EQ3Filter {
   //---------------------------------------------------------------------------
   //
   // 3 Band EQ :)
   //
   // EQ.H - Header file for 3 band EQ
   //
   // (c) Neil C / Etanza Systems / 2K6
   //
   // Shouts / Loves / Moans = etanza at lycos dot co dot uk
   //
   // This work is hereby placed in the public domain for all purposes, including
   // use in commercial applications.
   //
   // The author assumes NO RESPONSIBILITY for any problems caused by the use of
   // this software.
   //
   //----------------------------------------------------------------------------
   // NOTES :
   //
   // - Original filter code by Paul Kellet (musicdsp.pdf)
   //
   // - Uses 4 first order filters in series, should give 24dB per octave
   //
   // - Now with P4 Denormal fix :)
   //----------------------------------------------------------------------------

   // Filter #1 (Low band)
   sF64 f1p0; // Poles ...
   sF64 f1p1;
   sF64 f1p2;
   sF64 f1p3;
      
   // Filter #2 (High band)
   sF64 f2p0; // Poles ...
   sF64 f2p1;
   sF64 f2p2;
   sF64 f2p3;
      
   // Sample history buffer
   sF64 sdm1; // Sample data minus 1
   sF64 sdm2; // 2
   sF64 sdm3; // 3

public:
   EQ3Filter(void) {
   }
   ~EQ3Filter() {
   }

   void reset(void) {
      f1p0 = f1p1 = f1p2 = f1p3 = 0.0;
      f2p0 = f2p1 = f2p2 = f2p3 = 0.0;
      sdm1 = sdm2 = sdm3 = 0.0f;
   }
   
   sF32 process(const sF32 _smp,
                const sF32 _freqLo,
                const sF32 _gainLo,
                const sF32 _freqHi,
                const sF32 _gainHi,
                const sF32 _gainMid
                ) {
      sF64 lf = 1.0 * sin(ST_PLUGIN_PI * _freqLo);
      sF64 hf = 1.0 * sin(ST_PLUGIN_PI * _freqHi);

      // // sF64 lf = 1.0 * sin(ST_PLUGIN_PI * _freqLo);
      // // sF64 hf = 1.0 * sin(ST_PLUGIN_PI * _freqHi);

#define vsa (1.0 / 4294967295.0) // Very small amount (Denormal Fix)

      // Filter #1 (lowpass)
      f1p0 += (lf * (_smp - f1p0)) + vsa;
      f1p1 += (lf * (f1p0 - f1p1));
      f1p2 += (lf * (f1p1 - f1p2));
      f1p3 += (lf * (f1p2 - f1p3));

      sF64 l = f1p3;

      // Filter #2 (highpass)
      f2p0 += (hf * (_smp - f2p0)) + vsa;
      f2p1 += (hf * (f2p0 - f2p1));
      f2p2 += (hf * (f2p1 - f2p2));
      f2p3 += (hf * (f2p2 - f2p3));

#undef vsa

      sF64 h = sdm3 - f2p3;

      // Calculate midrange (signal - (low + high))
      sF64 m = sdm3 - (h + l);

      // Scale and Combine
      l *= _gainLo;
      m *= _gainMid;
      h *= _gainHi;
   
      sF64 out = (l + m + h);

      // Shuffle history buffer
      sdm3 = sdm2;
      sdm2 = sdm1;
      sdm1 = _smp;

      return sF32(out);
   }
};

typedef struct eq3_voice_s {
   st_plugin_voice_t base;
   float    sample_rate;
   float    mods[NUM_MODS];
   float    mod_drywet_cur;
   float    mod_drywet_inc;
   float    mod_gain_cur;
   float    mod_gain_inc;
   float    mod_freqlo_cur;
   float    mod_freqlo_inc;
   float    mod_gainlo_cur;
   float    mod_gainlo_inc;
   float    mod_freqhi_cur;
   float    mod_freqhi_inc;
   float    mod_gainhi_cur;
   float    mod_gainhi_inc;
   float    mod_gainmid_cur;
   float    mod_gainmid_inc;

   EQ3Filter eq_l;
   EQ3Filter eq_r;

   unsigned int tick_nr;

} eq3_voice_t;


static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(eq3_voice_t);
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
   ST_PLUGIN_SHARED_CAST(eq3_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(eq3_shared_t);
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
   ST_PLUGIN_VOICE_CAST(eq3_voice_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));

      voice->eq_l.reset();
      voice->eq_r.reset();

      voice->tick_nr = 0u;
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(eq3_voice_t);
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
   ST_PLUGIN_VOICE_CAST(eq3_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(eq3_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modGain = (( (shared->params[PARAM_GAIN] + voice->mods[MOD_GAIN]) - 0.5f) * 2.0f);
   modGain = powf(10.0f, modGain * 2.0f);

   float modFreqLo = shared->params[PARAM_FREQLO] + voice->mods[MOD_FREQLO];
   modFreqLo = powf(2.0f, modFreqLo * 8.0f);
   modFreqLo *= _freqHz;
#define fmax (2.0f * 0.089f)
   if(modFreqLo > (voice->sample_rate * fmax))
      modFreqLo = voice->sample_rate * fmax;
#undef fmax
   modFreqLo /= voice->sample_rate;
   if(modFreqLo < 0.0f)
      modFreqLo = 0.0f;
  

   float modGainLo = (( (shared->params[PARAM_GAINLO] + voice->mods[MOD_GAINLO]) - 0.5f) * 2.0f);
   modGainLo = powf(10.0f, modGainLo * 2.0f);

   float modFreqHi = shared->params[PARAM_FREQHI] + voice->mods[MOD_FREQHI];
   modFreqHi = Dstplugin_clamp(modFreqHi, 0.0f, 1.0f);
   // // if(modFreqHi > 0.5f)
   // //    modFreqHi = 0.5f + (modFreqHi - 0.5f) * 0.5f;  // => 0..0.75
   // // modFreqHi = (powf(2.0f, modFreqHi * 7.0f) - 1.0f) / 127.0f;
   modFreqHi = powf(2.0f, modFreqHi * 8.0f);
   modFreqHi *= 0.5f*_freqHz;
#define fmax (2.0f * 0.291768721876f)
   if(modFreqHi > (voice->sample_rate * fmax))
      modFreqHi = voice->sample_rate * fmax;
#undef fmax
   modFreqHi /= voice->sample_rate;
   if(modFreqHi < 0.0f)
      modFreqHi = 0.0f;

   float modGainHi = (( (shared->params[PARAM_GAINHI] + voice->mods[MOD_GAINHI]) - 0.5f) * 2.0f);
   modGainHi = powf(10.0f, modGainHi * 2.0f);

   float modGainMid = (( (shared->params[PARAM_GAINMID] + voice->mods[MOD_GAINMID]) - 0.5f) * 2.0f);
   modGainMid = powf(10.0f, modGainMid * 2.0f);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc  = (modDryWet  - voice->mod_drywet_cur)  * recBlockSize;
      voice->mod_gain_inc    = (modGain    - voice->mod_gain_cur)    * recBlockSize;
      voice->mod_freqlo_inc  = (modFreqLo  - voice->mod_freqlo_cur)  * recBlockSize;
      voice->mod_gainlo_inc  = (modGainLo  - voice->mod_gainlo_cur)  * recBlockSize;
      voice->mod_freqhi_inc  = (modFreqHi  - voice->mod_freqhi_cur)  * recBlockSize;
      voice->mod_gainhi_inc  = (modGainHi  - voice->mod_gainhi_cur)  * recBlockSize;
      voice->mod_gainmid_inc = (modGainMid - voice->mod_gainmid_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur  = modDryWet;
      voice->mod_drywet_inc  = 0.0f;
      voice->mod_gain_cur    = modGain;
      voice->mod_gain_inc    = 0.0f;
      voice->mod_freqlo_cur  = modFreqLo;
      voice->mod_freqlo_inc  = 0.0f;
      voice->mod_gainlo_cur  = modGainLo;
      voice->mod_gainlo_inc  = 0.0f;
      voice->mod_freqhi_cur  = modFreqHi;
      voice->mod_freqhi_inc  = 0.0f;
      voice->mod_gainhi_cur  = modGainHi;
      voice->mod_gainhi_inc  = 0.0f;
      voice->mod_gainmid_cur = modGainMid;
      voice->mod_gainmid_inc = 0.0f;

      _numFrames = 1u;
   }

   // if(voice->tick_nr < 50)
   // {
   //    printf("xxx tick_nr=%u freqlo_cur=%f gainlo_cur=%f\n", voice->tick_nr, voice->mod_freqlo_cur, voice->mod_gainlo_cur);
   // }

   voice->tick_nr++;
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(eq3_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(eq3_shared_t);

   unsigned int k = 0u;

   // Stereo input, stereo output
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      float l = _samplesIn[k];
      float r = _samplesIn[k + 1u];
      float outL = voice->eq_l.process(l * voice->mod_gain_cur,
                                       voice->mod_freqlo_cur,
                                       voice->mod_gainlo_cur,
                                       voice->mod_freqhi_cur,
                                       voice->mod_gainhi_cur,
                                       voice->mod_gainmid_cur
                                       );
      float outR = voice->eq_r.process(r * voice->mod_gain_cur,
                                       voice->mod_freqlo_cur,
                                       voice->mod_gainlo_cur,
                                       voice->mod_freqhi_cur,
                                       voice->mod_gainhi_cur,
                                       voice->mod_gainmid_cur
                                       );
      outL = l + (outL - l) * voice->mod_drywet_cur;
      outR = r + (outR - r) * voice->mod_drywet_cur;
      _samplesOut[k]      = outL;
      _samplesOut[k + 1u] = outR;

      // Next frame
      k += 2u;
      voice->mod_drywet_cur  += voice->mod_drywet_inc;
      voice->mod_gain_cur    += voice->mod_gain_inc;
      voice->mod_freqlo_cur  += voice->mod_freqlo_inc;
      voice->mod_gainlo_cur  += voice->mod_gainlo_inc;
      voice->mod_freqhi_cur  += voice->mod_freqhi_inc;
      voice->mod_gainhi_cur  += voice->mod_gainhi_inc;
      voice->mod_gainmid_cur += voice->mod_gainmid_inc;
   }

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   eq3_shared_t *ret = (eq3_shared_t *)malloc(sizeof(eq3_shared_t));
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
   eq3_voice_t *ret = (eq3_voice_t *)malloc(sizeof(eq3_voice_t));
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
st_plugin_info_t *eq3_init(void) {
   eq3_info_t *ret = NULL;

   ret = (eq3_info_t *)malloc(sizeof(eq3_info_t));

   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp eq3";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp & neilc & paulk";
      ret->base.name        = "eq3";
      ret->base.short_name  = "eq3";
      ret->base.flags       = ST_PLUGIN_FLAG_FX | ST_PLUGIN_FLAG_TRUE_STEREO_OUT;
      ret->base.category    = ST_PLUGIN_CAT_EQ;
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
