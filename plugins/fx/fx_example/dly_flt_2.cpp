// ----
// ---- file   : dly_flt_2.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a cross feedback delay line with variable shape (sweepable multimode) filtering
// ----
// ---- created: 25May2020
// ---- changed: 31May2020
// ----
// ----
// ----

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../plugin.h"

#include "delay.h"
#include "biquad.h"

#define PARAM_DRYWET       0
#define PARAM_TIME         1  // 0..1 => 0..~371ms
#define PARAM_TIME_SCL_R   2  // *0.01..*10
#define PARAM_FB           3
#define PARAM_XFB          4
#define PARAM_FLT_CUTOFF   5
#define PARAM_FLT_SHAPE    6
#define PARAM_FLT_Q        7
#define NUM_PARAMS         8
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Time",
   "Time * R",
   "Feedback",
   "X-Feedback",
   "Flt Cutoff",
   "Flt Shape",
   "Flt Q"
};
static float loc_param_resets[NUM_PARAMS] = {
   0.5f,  // DRYWET
   0.0f,  // TIME
   0.5f,  // TIME_SCL_R
   0.0f,  // FB
   0.0f,  // XFB
   1.0f,  // FLT_CUTOFF
   0.0f,  // FLT_SHAPE
   0.35f  // FLT_Q
};

#define MOD_TIME        0
#define MOD_FB          1
#define MOD_FLT_CUTOFF  2
#define MOD_FLT_SHAPE   3
#define NUM_MODS        4
static const char *loc_mod_names[NUM_MODS] = {
   "Time",
   "Feedback",
   "Flt Cutoff",
   "Flt Shape"
};

typedef struct dly_flt_2_info_s {
   st_plugin_info_t base;
} dly_flt_2_info_t;

typedef struct dly_flt_2_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} dly_flt_2_shared_t;

typedef struct dly_flt_2_voice_s {
   st_plugin_voice_t base;
   float         sample_rate;
   float         mods[NUM_MODS];
   float         mod_time_smooth;
   float         mod_time_l_cur;
   float         mod_time_l_inc;
   float         mod_time_r_cur;
   float         mod_time_r_inc;
   float         mod_fb_cur;
   float         mod_fb_inc;
   float         mod_xfb_cur;
   float         mod_xfb_inc;
   StDelay       dly_l;
   StDelay       dly_r;
   StBiquadCoeff coeff_lpf;
   StBiquadCoeff coeff_bpf;
   StBiquadCoeff coeff_hpf;
   StBiquad      flt_1_l;
   StBiquad      flt_1_r;

   void calcFltCoeff(StBiquadCoeff &d, float _shape, float _freq, float _q) {

      coeff_bpf.calcParams(StBiquad::BPF,
                           0.0f/*gainDB*/,
                           _freq,
                           _q
                           );

      if(_shape < 0.5f)
      {
         coeff_lpf.calcParams(StBiquad::LPF,
                              0.0f/*gainDB*/,
                              _freq,
                              _q
                              );

         d.lerp(coeff_lpf, coeff_bpf, _shape*2.0f);
      }
      else
      {
         coeff_hpf.calcParams(StBiquad::HPF,
                              0.0f/*gainDB*/,
                              _freq,
                              _q
                              );

         d.lerp(coeff_bpf, coeff_hpf, (_shape-0.5f)*2.0f);
      }
   }

} dly_flt_2_voice_t;


static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(dly_flt_2_voice_t);
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
   ST_PLUGIN_SHARED_CAST(dly_flt_2_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(dly_flt_2_shared_t);
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
   ST_PLUGIN_VOICE_CAST(dly_flt_2_voice_t);
   (void)_bGlide;
   (void)_voiceIdx;
   (void)_activeNoteIdx;
   (void)_note;
   (void)_noteHz;
   (void)_vel;
   // printf("xxx dly_flt_2: note_on(bGlide=%d voiceIdx=%u activeNoteIdx=%u note=%u noteHz=%f vel=%f)\n", _bGlide, _voiceIdx, _activeNoteIdx, _note, _noteHz, _vel);
   // fflush(stdout);
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
      voice->flt_1_l.reset();
      voice->flt_1_r.reset();
      voice->dly_l.reset();
      voice->dly_r.reset();
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(dly_flt_2_voice_t);
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
   ST_PLUGIN_VOICE_CAST(dly_flt_2_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(dly_flt_2_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;
   
   float maxMs = (float(ST_DELAY_SIZE) / 88.200f);    // maxMs @ 88.2kHz = ~371

   float newTime = shared->params[PARAM_TIME] * powf(10.0f, voice->mods[MOD_TIME]);

   if(_numFrames > 0u)
      voice->mod_time_smooth = (float)(voice->mod_time_smooth + (newTime - voice->mod_time_smooth) * 0.0075f);
   else
      voice->mod_time_smooth = newTime;

   float modTimeSclR = (shared->params[PARAM_TIME_SCL_R] - 0.5f) * 2.0f;
   modTimeSclR = powf(10.0f, modTimeSclR);

   float modTimeL = voice->mod_time_smooth;
   modTimeL = Dstplugin_clamp(modTimeL, 0.0f, 1.0f);
   modTimeL = (modTimeL * maxMs * (voice->sample_rate / 1000.0f));
   if(modTimeL >= (float)(ST_DELAY_SIZE - 1u))
      modTimeL = (float)(ST_DELAY_SIZE - 1u);

   float modTimeR = voice->mod_time_smooth * modTimeSclR;
   modTimeR = Dstplugin_clamp(modTimeR, 0.0f, 1.0f);
   modTimeR = (modTimeR * maxMs * (voice->sample_rate / 1000.0f));
   if(modTimeR >= (float)(ST_DELAY_SIZE - 1u))
      modTimeR = (float)(ST_DELAY_SIZE - 1u);
   
   float modFbAmt = powf(10.0f, voice->mods[MOD_FB]);

#define MAX_FB 1.0f
   float modFb = shared->params[PARAM_FB] * MAX_FB * modFbAmt;
   modFb = Dstplugin_clamp(modFb, 0.0f, MAX_FB);

   float modXfb = shared->params[PARAM_XFB] * MAX_FB * modFbAmt;
   modXfb = Dstplugin_clamp(modXfb, 0.0f, MAX_FB);

   float modFltShape = shared->params[PARAM_FLT_SHAPE] + voice->mods[MOD_FLT_SHAPE];
   modFltShape = Dstplugin_clamp(modFltShape, 0.0f, 1.0f);

   float modFltFreq = shared->params[PARAM_FLT_CUTOFF] + voice->mods[MOD_FLT_CUTOFF];
   modFltFreq = Dstplugin_clamp(modFltFreq, 0.0f, 1.0f);
   modFltFreq = (powf(2.0f, modFltFreq * 7.0f) - 1.0f) / 127.0f;

   float modFltQ = shared->params[PARAM_FLT_Q];
   modFltQ = Dstplugin_clamp(modFltQ, 0.0f, 1.0f);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_time_l_inc = (modTimeL  - voice->mod_time_l_cur) * recBlockSize;
      voice->mod_time_r_inc = (modTimeR  - voice->mod_time_r_cur) * recBlockSize;
      voice->mod_fb_inc     = (modFb     - voice->mod_fb_cur)     * recBlockSize;
      voice->mod_xfb_inc    = (modXfb    - voice->mod_xfb_cur)    * recBlockSize;

      voice->flt_1_l.shuffleCoeff();
      voice->flt_1_r.shuffleCoeff();
      voice->calcFltCoeff(voice->flt_1_l.next, modFltShape, modFltFreq, modFltQ);
      voice->flt_1_l.calcStep(_numFrames);
      voice->flt_1_r.next = voice->flt_1_l.next;
      voice->flt_1_r.step = voice->flt_1_l.step;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_time_l_cur = modTimeL;
      voice->mod_time_l_inc = 0.0f;
      voice->mod_time_r_cur = modTimeR;
      voice->mod_time_r_inc = 0.0f;
      voice->mod_fb_cur     = modFb;
      voice->mod_fb_inc     = 0.0f;
      voice->mod_xfb_cur    = modXfb;
      voice->mod_xfb_inc    = 0.0f;

      voice->calcFltCoeff(voice->flt_1_l.next, modFltShape, modFltFreq, modFltQ);
      voice->flt_1_r.next = voice->flt_1_l.next;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(dly_flt_2_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(dly_flt_2_shared_t);

   unsigned int k = 0u;

   // Stereo input, stereo output
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
#if 0
      // sounds weird when both fb+xfb are used (sample delay?)
      float l = _samplesIn[k];
      float fltL = voice->flt_1_l.filter(l);
      float dlyLastL = voice->dly_l.last_out;
      voice->dly_l.push(fltL + voice->dly_r.last_out * voice->mod_xfb_cur, voice->mod_fb_cur);
      float outL = voice->dly_l.readLinear(voice->mod_time_l_cur);
      outL = Dstplugin_fix_denorm_32(outL);

      float r = _samplesIn[k + 1u];
      float fltR = voice->flt_1_r.filter(r);
      voice->dly_r.push(fltR + dlyLastL * voice->mod_xfb_cur, voice->mod_fb_cur);
      float outR = voice->dly_r.readLinear(voice->mod_time_r_cur);
      outR = Dstplugin_fix_denorm_32(outR);
#elif 0
      // filter on input only
      //  (note) useful to create "send-level" type effects (by using highpass to thin out input)
      //  (note) but this also be achieved via a serial effect setup (mmf -> dly_2)
      float l = _samplesIn[k];
      float fltL = voice->flt_1_l.filter(l);
      voice->dly_l.push(fltL, voice->mod_fb_cur);
      float outL = voice->dly_l.readLinear(voice->mod_time_l_cur);
      outL = Dstplugin_fix_denorm_32(outL);

      float r = _samplesIn[k + 1u];
      float fltR = voice->flt_1_r.filter(r);
      voice->dly_r.push(fltR, voice->mod_fb_cur);
      float outR = voice->dly_r.readLinear(voice->mod_time_r_cur);
      outR = Dstplugin_fix_denorm_32(outR);

      float lastL = voice->dly_l.last_out;
      voice->dly_l.add(voice->dly_r.last_out * voice->mod_xfb_cur);
      voice->dly_r.add(lastL * voice->mod_xfb_cur);
#else
      // filter in feedback loop
      float l = _samplesIn[k];
      float lastOutL = voice->dly_l.last_out;
      float fltL = voice->flt_1_l.filter(l + lastOutL * voice->mod_fb_cur + voice->dly_r.last_out * voice->mod_xfb_cur);
      voice->dly_l.pushRaw(fltL);
      float outL = voice->dly_l.readLinear(voice->mod_time_l_cur);
      outL = Dstplugin_fix_denorm_32(outL);

      float r = _samplesIn[k + 1u];
      float fltR = voice->flt_1_r.filter(r + voice->dly_r.last_out * voice->mod_fb_cur + lastOutL * voice->mod_xfb_cur);
      voice->dly_r.pushRaw(fltR);
      float outR = voice->dly_r.readLinear(voice->mod_time_r_cur);
      outR = Dstplugin_fix_denorm_32(outR);
#endif

      outL = l + (outL - l) * shared->params[PARAM_DRYWET];
      outR = r + (outR - r) * shared->params[PARAM_DRYWET];

      _samplesOut[k]      = outL;
      _samplesOut[k + 1u] = outR;

      // Next frame
      k += 2u;
      voice->mod_time_l_cur += voice->mod_time_l_inc;
      voice->mod_time_r_cur += voice->mod_time_r_inc;
      voice->mod_fb_cur     += voice->mod_fb_inc;
      voice->mod_xfb_cur    += voice->mod_xfb_inc;
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   dly_flt_2_shared_t *ret = (dly_flt_2_shared_t *)malloc(sizeof(dly_flt_2_shared_t));
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
   dly_flt_2_voice_t *ret = (dly_flt_2_voice_t *)malloc(sizeof(dly_flt_2_voice_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info   = _info;
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
st_plugin_info_t *dly_flt_2_init(void) {

   dly_flt_2_info_t *ret = (dly_flt_2_info_t *)malloc(sizeof(dly_flt_2_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp dly flt 2";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "delay filter 2";
      ret->base.short_name  = "dly flt 2";
      ret->base.flags       = ST_PLUGIN_FLAG_FX | ST_PLUGIN_FLAG_TRUE_STEREO_OUT;
      ret->base.category    = ST_PLUGIN_CAT_DELAY;
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
}
