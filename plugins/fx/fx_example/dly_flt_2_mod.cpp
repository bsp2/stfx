// ----
// ---- file   : dly_flt_2_mod.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020-2024 by Bastian Spiegel.
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a cross feedback delay line with variable shape (sweepable multimode) filtering
// ----           - based on dly_flt_2
// ----           - quad delay buffer size (0..1484ms)
// ----           - input level mod ('send' level)
// ----           - separate lowpass and highpass filters
// ----           - time modulation LFO
// ----
// ---- created: 25May2020
// ---- changed: 31May2020, 08Jun2020, 10Feb2021, 21Jan2024, 27Sep2024
// ----
// ----
// ----

#include <stdio.h>  // snprintf
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../plugin.h"

#define ST_DELAY_SIZE (32768u * 4u)
#include "delay.h"
#include "biquad.h"

#define PARAM_DRYWET       0
#define PARAM_IN_LVL       1
#define PARAM_TIME         2  // 0..1 => 0..~1486ms
#define PARAM_TIME_SCL_R   3  // *0.01..*10
#define PARAM_LFO_SPEED    4  // 0..1 => 0..50Hz
#define PARAM_LFO_AMT      5
#define PARAM_FB           6
#define PARAM_LPF_CUTOFF   7
#define PARAM_HPF_CUTOFF   8
#define PARAM_FLT_Q        9
#define PARAM_XFB         10
#define NUM_PARAMS        11
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",   //  0
   "In Lvl",      //  1
   "Time",        //  2
   "Time * R",    //  3
   "Mod Speed",   //  4
   "Mod Amount",  //  5
   "Feedback",    //  6
   "LPF Cutoff",  //  7
   "HPF Cutoff",  //  8
   "Flt Q",       //  9
   "X-Feedback",  // 10
};
static float loc_param_resets[NUM_PARAMS] = {
   0.2f,  //  0: DRYWET
   1.0f,  //  1: IN_LVL
   0.17f, //  2: TIME
   0.51f, //  3: TIME_SCL_R
   0.1f,  //  4: LFO_SPEED
   0.1f,  //  5: LFO_AMT
   0.4f,  //  6: FB
   1.0f,  //  7: LPF_CUTOFF
   0.05f, //  8: HPF_CUTOFF
   0.35f, //  9: FLT_Q
   0.0f,  // 10: XFB
};

#define MOD_DRYWET      0
#define MOD_IN_LVL      1
#define MOD_TIME        2
#define MOD_FB          3
#define MOD_LFO_SPEED   4
#define MOD_LFO_AMT     5
#define MOD_LPF_CUTOFF  6
#define MOD_HPF_CUTOFF  7
#define NUM_MODS        8
static const char *loc_mod_names[NUM_MODS] = {
   "Dry/Wet",     // 0
   "In Lvl",      // 1
   "Time",        // 2
   "Feedback",    // 3
   "Mod Speed",   // 4
   "Mod Amount",  // 5
   "LPF Cutoff",  // 6
   "HPF Cutoff",  // 7
};

typedef struct dly_flt_2_mod_info_s {
   st_plugin_info_t base;
} dly_flt_2_mod_info_t;

typedef struct dly_flt_2_mod_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} dly_flt_2_mod_shared_t;

typedef struct dly_flt_2_mod_voice_s {
   st_plugin_voice_t base;
   float         sample_rate;
   float         mods[NUM_MODS];
   float         mod_drywet_cur;
   float         mod_drywet_inc;
   float         mod_in_lvl_cur;
   float         mod_in_lvl_inc;
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
   StBiquadCoeff coeff_tmp;
   StBiquad      lpf_l;
   StBiquad      lpf_r;
   StBiquad      hpf_l;
   StBiquad      hpf_r;
   float         lfo_phase;  // updated @1kHz rate (prepare_block())
} dly_flt_2_mod_voice_t;


static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(dly_flt_2_mod_voice_t);
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
   ST_PLUGIN_SHARED_CAST(dly_flt_2_mod_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_get_param_value_string(st_plugin_shared_t  *_shared,
                                                     unsigned int         _paramIdx,
                                                     char                *_buf,
                                                     unsigned int         _bufSize
                                                     ) {
   ST_PLUGIN_SHARED_CAST(dly_flt_2_mod_shared_t);
   if(PARAM_TIME == _paramIdx)
   {
      const float maxMs = (float(ST_DELAY_SIZE) / 88.200f);    // maxMs @ 88.2kHz = ~1486ms
      const float ms = shared->params[PARAM_TIME] * maxMs;
      snprintf(_buf, _bufSize, "%4.2f ms", ms);
   }
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(dly_flt_2_mod_shared_t);
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
   ST_PLUGIN_VOICE_CAST(dly_flt_2_mod_voice_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   // printf("xxx dly_flt_2_mod: note_on(bGlide=%d voiceIdx=%u activeNoteIdx=%u note=%u noteHz=%f vel=%f)\n", _bGlide, _voiceIdx, _activeNoteIdx, _note, _noteHz, _vel);
   // fflush(stdout);
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
      voice->lpf_l.reset();
      voice->lpf_r.reset();
      voice->hpf_l.reset();
      voice->hpf_r.reset();
      voice->dly_l.reset();
      voice->dly_r.reset();
      // (note) keep lfo_phase
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(dly_flt_2_mod_voice_t);
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
   ST_PLUGIN_VOICE_CAST(dly_flt_2_mod_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(dly_flt_2_mod_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET] + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modInLvl = shared->params[PARAM_IN_LVL] + voice->mods[MOD_IN_LVL];
   modInLvl = Dstplugin_clamp(modInLvl, 0.0f, 1.0f);

   const float maxMs = (float(ST_DELAY_SIZE) / 88.200f);    // maxMs @ 88.2kHz = ~1486

   float newTime = shared->params[PARAM_TIME] * powf(10.0f, voice->mods[MOD_TIME]);

   // LFO modulation
   {
      const float a = voice->lfo_phase;
      float lfoLvl = (a < 0.5f) ? (-1.0f + a * 4.0f) : (1.0f - (a - 0.5f)*4.0f);  // triangle
      float modLFOAmt = shared->params[PARAM_LFO_AMT] + voice->mods[MOD_LFO_AMT];
      modLFOAmt = Dstplugin_clamp(modLFOAmt, 0.0f, 1.0f);
      lfoLvl *= modLFOAmt;
      newTime += lfoLvl * (1.0f / 16.0f);
   }

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

   float modLPFFreq = shared->params[PARAM_LPF_CUTOFF] + voice->mods[MOD_LPF_CUTOFF];
   modLPFFreq = Dstplugin_clamp(modLPFFreq, 0.0f, 1.0f);
   modLPFFreq = (powf(2.0f, modLPFFreq * 7.0f) - 1.0f) / 127.0f;

   float modHPFFreq = shared->params[PARAM_HPF_CUTOFF] + voice->mods[MOD_HPF_CUTOFF];
   modHPFFreq = Dstplugin_clamp(modHPFFreq, 0.0f, 1.0f);
   modHPFFreq = (powf(2.0f, modHPFFreq * 7.0f) - 1.0f) / 127.0f;

   float modFltQ = shared->params[PARAM_FLT_Q];
   modFltQ = Dstplugin_clamp(modFltQ, 0.0f, 1.0f);

   if(_numFrames > 0u)
   {
      // lerp
      const float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      voice->mod_in_lvl_cur = modInLvl;
      voice->mod_in_lvl_inc = 0.0f;
      voice->mod_time_l_inc = (modTimeL  - voice->mod_time_l_cur) * recBlockSize;
      voice->mod_time_r_inc = (modTimeR  - voice->mod_time_r_cur) * recBlockSize;
      voice->mod_fb_inc     = (modFb     - voice->mod_fb_cur)     * recBlockSize;
      voice->mod_xfb_inc    = (modXfb    - voice->mod_xfb_cur)    * recBlockSize;

      // Lowpass
      voice->lpf_l.shuffleCoeff();
      voice->lpf_r.shuffleCoeff();
      voice->lpf_l.next.calcParams(StBiquad::LPF,
                                   0.0f/*gainDB*/,
                                   modLPFFreq,
                                   modFltQ
                                   );
      voice->lpf_l.calcStep(_numFrames);
      voice->lpf_r.next = voice->lpf_l.next;
      voice->lpf_r.step = voice->lpf_l.step;

      // Highpass
      voice->hpf_l.shuffleCoeff();
      voice->hpf_r.shuffleCoeff();
      voice->hpf_l.next.calcParams(StBiquad::HPF,
                                   0.0f/*gainDB*/,
                                   modHPFFreq,
                                   modFltQ
                                   );
      voice->hpf_l.calcStep(_numFrames);
      voice->hpf_r.next = voice->hpf_l.next;
      voice->hpf_r.step = voice->hpf_l.step;

      float modLFOSpeed = shared->params[PARAM_LFO_SPEED] + voice->mods[MOD_LFO_SPEED];
      Dstplugin_clamp(modLFOSpeed, 0.0f, 1.0f);
      voice->lfo_phase += modLFOSpeed * (1.0f / (1000.0f / 50.0f/*Hz*/));
      if(voice->lfo_phase >= 1.0f)
         voice->lfo_phase -= 1.0f;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      voice->mod_in_lvl_cur = modInLvl;
      voice->mod_in_lvl_inc = 0.0f;
      voice->mod_time_l_cur = modTimeL;
      voice->mod_time_l_inc = 0.0f;
      voice->mod_time_r_cur = modTimeR;
      voice->mod_time_r_inc = 0.0f;
      voice->mod_fb_cur     = modFb;
      voice->mod_fb_inc     = 0.0f;
      voice->mod_xfb_cur    = modXfb;
      voice->mod_xfb_inc    = 0.0f;

      // Lowpass
      voice->lpf_l.next.calcParams(StBiquad::LPF,
                                   0.0f/*gainDB*/,
                                   modLPFFreq,
                                   modFltQ
                                   );
      voice->lpf_r.next = voice->lpf_l.next;

      // Highpass
      voice->hpf_l.next.calcParams(StBiquad::HPF,
                                   0.0f/*gainDB*/,
                                   modHPFFreq,
                                   modFltQ
                                   );
      voice->hpf_l.calcStep(_numFrames);
      voice->hpf_r.next = voice->hpf_l.next;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut,
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(dly_flt_2_mod_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(dly_flt_2_mod_shared_t);

   unsigned int k = 0u;

   // Stereo input, stereo output
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      // filter in feedback loop
      float l = _samplesIn[k];
      float lastOutL = voice->dly_l.last_out;
      float fltL = voice->lpf_l.filter(l * voice->mod_in_lvl_cur + lastOutL * voice->mod_fb_cur + voice->dly_r.last_out * voice->mod_xfb_cur);
      fltL = voice->hpf_l.filter(fltL);
      voice->dly_l.pushRaw(fltL);
      float outL = voice->dly_l.readLinear(voice->mod_time_l_cur);
      outL = Dstplugin_fix_denorm_32(outL);

      float r = _samplesIn[k + 1u];
      float fltR = voice->lpf_r.filter(r * voice->mod_in_lvl_cur + voice->dly_r.last_out * voice->mod_fb_cur + lastOutL * voice->mod_xfb_cur);
      fltR = voice->hpf_r.filter(fltR);
      voice->dly_r.pushRaw(fltR);
      float outR = voice->dly_r.readLinear(voice->mod_time_r_cur);
      outR = Dstplugin_fix_denorm_32(outR);

      outL = l + (outL - l) * voice->mod_drywet_cur;
      outR = r + (outR - r) * voice->mod_drywet_cur;

      _samplesOut[k]      = outL;
      _samplesOut[k + 1u] = outR;

      // Next frame
      k += 2u;
      voice->mod_drywet_cur += voice->mod_drywet_inc;
      voice->mod_in_lvl_cur += voice->mod_in_lvl_inc;
      voice->mod_time_l_cur += voice->mod_time_l_inc;
      voice->mod_time_r_cur += voice->mod_time_r_inc;
      voice->mod_fb_cur     += voice->mod_fb_inc;
      voice->mod_xfb_cur    += voice->mod_xfb_inc;
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   dly_flt_2_mod_shared_t *ret = (dly_flt_2_mod_shared_t *)malloc(sizeof(dly_flt_2_mod_shared_t));
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
   dly_flt_2_mod_voice_t *ret = (dly_flt_2_mod_voice_t *)malloc(sizeof(dly_flt_2_mod_voice_t));
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
st_plugin_info_t *dly_flt_2_mod_init(void) {

   dly_flt_2_mod_info_t *ret = (dly_flt_2_mod_info_t *)malloc(sizeof(dly_flt_2_mod_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp dly flt 2 mod";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "delay filter 2 mod";
      ret->base.short_name  = "dly flt 2 mod";
      ret->base.flags       = ST_PLUGIN_FLAG_FX | ST_PLUGIN_FLAG_TRUE_STEREO_OUT;
      ret->base.category    = ST_PLUGIN_CAT_DELAY;
      ret->base.num_params  = NUM_PARAMS;
      ret->base.num_mods    = NUM_MODS;

      ret->base.shared_new             = &loc_shared_new;
      ret->base.shared_delete          = &loc_shared_delete;
      ret->base.voice_new              = &loc_voice_new;
      ret->base.voice_delete           = &loc_voice_delete;
      ret->base.get_param_name         = &loc_get_param_name;
      ret->base.get_param_reset        = &loc_get_param_reset;
      ret->base.get_param_value        = &loc_get_param_value;
      ret->base.get_param_value_string = &loc_get_param_value_string;
      ret->base.set_param_value        = &loc_set_param_value;
      ret->base.get_mod_name           = &loc_get_mod_name;
      ret->base.set_sample_rate        = &loc_set_sample_rate;
      ret->base.note_on                = &loc_note_on;
      ret->base.set_mod_value          = &loc_set_mod_value;
      ret->base.prepare_block          = &loc_prepare_block;
      ret->base.process_replace        = &loc_process_replace;
      ret->base.plugin_exit            = &loc_plugin_exit;
   }

   return &ret->base;
}
}
