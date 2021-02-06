// ----
// ---- file   : ws_fm.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a sine / FM waveshaper that supports per-sample-frame parameter interpolation
// ----
// ---- created: 20May2020
// ---- changed: 21May2020, 24May2020, 31May2020, 08Jun2020
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET    0
#define PARAM_FREQ      1
#define PARAM_PHASE     2
#define PARAM_LEVEL     3
#define PARAM_LFO_FREQ  4
#define PARAM_LFO_LEVEL 5
#define PARAM_LFO_START 6  // 1.0 = free running / "random"
#define PARAM_ZERO_TH   7
#define NUM_PARAMS      8
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Freq",
   "Phase",
   "Level",
   "LFO Freq",
   "LFO Level",
   "LFO Start",
   "Zero Thold"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // FREQ
   0.5f,  // PHASE
   0.5f,  // LEVEL
   0.25f, // LFO_FREQ
   0.0f,  // LFO_LEVEL
   0.0f,  // LFO_START
   0.0f,  // ZERO_TH
};

#define MOD_DRYWET  0
#define MOD_FREQ    1
#define MOD_LEVEL   2
#define MOD_ZERO_TH 3
#define NUM_MODS    4
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Freq",
   "Level",
   "Zero Thold",
};

typedef union {
   float        f;
   int          i;
   unsigned int u;
} ws_fm_fi_u;

typedef struct biquad_s {
   float x1, x2;
   float y1, y2;
   float a0, a1, a2, a3, a4;
} biquad_t;

typedef struct ws_fm_info_s {
   st_plugin_info_t base;
} ws_fm_info_t;

typedef struct ws_fm_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ws_fm_shared_t;

typedef struct ws_fm_voice_s {
   st_plugin_voice_t base;
   float    sample_rate;
   float    mods[NUM_MODS];
   float    mod_drywet_cur;
   float    mod_drywet_inc;
   float    mod_phase_cur;
   float    mod_phase_inc;
   float    mod_freq_cur;
   float    mod_freq_inc;
   float    mod_level_cur;
   float    mod_level_inc;
   float    mod_zero_th_cur;
   float    mod_zero_th_inc;
   float    lfo_phase;
   float    lfo_speed;
   float    lfo_level;
   // fixed frequency dc-filter:
   biquad_t hpf_l;
   biquad_t hpf_r;
} ws_fm_voice_t;


static void biquad_init(biquad_t *_f) {
   // (note) hardcoded sample rate but this does not really matter for this purpose
   float bandwidth = 4.3f;
   float freq = 0.0002f * (0.5f * 44100.0f);

   float omega, sn, cs, alpha;
   float a0, a1, a2, b0, b1, b2;

   // A = powf(10, dbGain /40);
   omega = (float) (ST_PLUGIN_2PI_F * freq / 44100.0f);
   sn = sinf(omega);
   cs = cosf(omega);
   alpha = (float) (sn * sinh(ST_PLUGIN_LN2 /2 * bandwidth * omega /sn));

   // HPF
   b0 = (1 + cs) / 2;
   b1 = -(1 + cs);
   b2 = (1 + cs) / 2;
   a0 = 1 + alpha;
   a1 = -2 * cs;
   a2 = 1 - alpha;

   _f->a0 = b0 / a0;
   _f->a1 = b1 / a0;
   _f->a2 = b2 / a0;
   _f->a3 = a1 / a0;
   _f->a4 = a2 / a0;
}

static float biquad_filter(biquad_t *_f, const float _inSmp) {
   float outSmp = (float)(
      _f->a0 * _inSmp + 
      _f->a1 * _f->x1 + 
      _f->a2 * _f->x2 -
      _f->a3 * _f->y1 - 
      _f->a4 * _f->y2
                          );

   outSmp = Dstplugin_fix_denorm_32(outSmp);

   // Shuffle history
   _f->x2 = _f->x1;
   _f->x1 = _inSmp;
   _f->x1 = _f->x1;
   _f->y2 = _f->y1;
   _f->y1 = outSmp;

   return outSmp;
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
   ST_PLUGIN_SHARED_CAST(ws_fm_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ws_fm_shared_t);
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
   ST_PLUGIN_VOICE_CAST(ws_fm_voice_t);
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(ws_fm_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_fm_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_fm_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_fm_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modFreq = shared->params[PARAM_FREQ] + voice->mods[MOD_FREQ];
   modFreq = (modFreq - 0.5f) * 2.0f;
   modFreq = powf(10.0f, modFreq * 2.0f);

   float modPhase = (shared->params[PARAM_PHASE] - 0.5f) * 8.0f;

   float modLevel = shared->params[PARAM_LEVEL] + voice->mods[MOD_LEVEL];
   modLevel = (modLevel - 0.5f) * 2.0f;
   modLevel = powf(10.0f, modLevel * 2.0f);

   float lfoSpeed = shared->params[PARAM_LFO_FREQ];
   lfoSpeed *= lfoSpeed;
   lfoSpeed *= lfoSpeed;
   lfoSpeed = (lfoSpeed * 2000.0f) / voice->sample_rate;
   voice->lfo_speed = lfoSpeed;

   float lfoLevel = shared->params[PARAM_LFO_LEVEL];
   lfoLevel = (lfoLevel - 0.5f) * 2.0f;
   lfoLevel = powf(10.0f, lfoLevel * 2.0f);
   voice->lfo_level = lfoLevel;

   float modZeroTh = shared->params[PARAM_ZERO_TH];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc  = (modDryWet - voice->mod_drywet_cur)  * recBlockSize;
      voice->mod_freq_inc    = (modFreq   - voice->mod_freq_cur)    * recBlockSize;
      voice->mod_phase_inc   = (modPhase  - voice->mod_phase_cur)   * recBlockSize;
      voice->mod_level_inc   = (modLevel  - voice->mod_level_cur)   * recBlockSize;
      voice->mod_zero_th_inc = (modZeroTh - voice->mod_zero_th_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur  = modDryWet;
      voice->mod_drywet_inc  = 0.0f;
      voice->mod_freq_cur    = modFreq;
      voice->mod_freq_inc    = 0.0f;
      voice->mod_phase_cur   = modPhase;
      voice->mod_phase_inc   = 0.0f;
      voice->mod_level_cur   = modLevel;
      voice->mod_level_inc   = 0.0f;
      voice->mod_zero_th_cur = modZeroTh;
      voice->mod_zero_th_inc = 0.0f;
      if(shared->params[PARAM_LFO_START] < 0.9999f)
         voice->lfo_phase   = shared->params[PARAM_LFO_START] * ST_PLUGIN_2PI_F;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(ws_fm_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_fm_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float a = 
            sinf( (voice->mod_phase_cur + (sinf(voice->lfo_phase) * voice->lfo_level) + l * voice->mod_freq_cur) * ST_PLUGIN_PI2_F
                  ) * voice->mod_level_cur;
         ws_fm_fi_u lm; lm.f = l; lm.u &= 0x7FFFffffu;
         if(lm.f < voice->mod_zero_th_cur)
            a *= lm.f / voice->mod_zero_th_cur;
         a = biquad_filter(&voice->hpf_l, a);
         float out = l + (a - l) * voice->mod_drywet_cur;
         out = Dstplugin_fix_denorm_32(out);
         _samplesOut[k]      = out;
         _samplesOut[k + 1u] = out;
         // Next frame
         k += 2u;
         voice->mod_drywet_cur  += voice->mod_drywet_inc;
         voice->mod_freq_cur    += voice->mod_freq_inc;
         voice->mod_phase_cur   += voice->mod_phase_inc;
         voice->mod_level_cur   += voice->mod_level_inc;
         voice->mod_zero_th_cur += voice->mod_zero_th_inc;
         voice->lfo_phase += voice->lfo_speed;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float al = 
            sinf( (voice->mod_phase_cur + (sinf(voice->lfo_phase) * voice->lfo_level) + l * voice->mod_freq_cur) * ST_PLUGIN_PI2_F
                  ) * voice->mod_level_cur;
         ws_fm_fi_u lm; lm.f = l; lm.u &= 0x7FFFffffu;
         if(lm.f < voice->mod_zero_th_cur)
            al *= lm.f / voice->mod_zero_th_cur;
         al = biquad_filter(&voice->hpf_l, al);
         float outL = l + (al - l) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);

         float r = _samplesIn[k + 1u];
         float ar =
            sinf( (voice->mod_phase_cur + (sinf(voice->lfo_phase) * voice->lfo_level) + r * voice->mod_freq_cur) * ST_PLUGIN_PI2_F
                  ) * voice->mod_level_cur;
         ws_fm_fi_u rm; rm.f = r; rm.u &= 0x7FFFffffu;
         if(rm.f < voice->mod_zero_th_cur)
            ar *= rm.f / voice->mod_zero_th_cur;
         ar = biquad_filter(&voice->hpf_r, ar);
         float outR = r + (ar - r) * voice->mod_drywet_cur;
         outR = Dstplugin_fix_denorm_32(outR);

         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur  += voice->mod_drywet_inc;
         voice->mod_freq_cur    += voice->mod_freq_inc;
         voice->mod_phase_cur   += voice->mod_phase_inc;
         voice->mod_level_cur   += voice->mod_level_inc;
         voice->mod_zero_th_cur += voice->mod_zero_th_inc;
         voice->lfo_phase += voice->lfo_speed;
      }
   }

   if(voice->lfo_phase >= ST_PLUGIN_2PI_F)
      voice->lfo_phase -= ST_PLUGIN_2PI_F;
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ws_fm_shared_t *ret = malloc(sizeof(ws_fm_shared_t));
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
   ws_fm_voice_t *ret = malloc(sizeof(ws_fm_voice_t));
   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));
      ret->base.info   = _info;
      biquad_init(&ret->hpf_l);
      biquad_init(&ret->hpf_r);
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   free(_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {
   free(_info);
}

st_plugin_info_t *ws_fm_init(void) {
   ws_fm_info_t *ret = NULL;

   ret = malloc(sizeof(ws_fm_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ws fm";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "fm shaper";
      ret->base.short_name  = "fm";
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
      ret->base.set_sample_rate  = &loc_set_sample_rate;
      ret->base.note_on          = &loc_note_on;
      ret->base.set_mod_value    = &loc_set_mod_value;
      ret->base.prepare_block    = &loc_prepare_block;
      ret->base.process_replace  = &loc_process_replace;
      ret->base.plugin_exit      = &loc_plugin_exit;
   }

   return &ret->base;
}
