// ----
// ---- file   : ws_lin_cpx.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2024 by Bastian Spiegel.
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a tanh waveshaper that supports per-sample-frame parameter interpolation
// ----
// ---- created: 14Oct2024
// ---- changed:
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET      0
#define PARAM_DRIVE_PRE   1
#define PARAM_TH_MIN      2
#define PARAM_TH_MAX      3
#define PARAM_DRIVE_POST  4
#define PARAM_SAT_AMT     5
#define PARAM_FADE_AMT    6
#define NUM_PARAMS        7
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Drive Pre",
   "Th.Min",
   "Th.Max",
   "Drive Post",
   "Sat Amt",
   "Fade Amt",
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // DRIVE_PRE
   0.25f, // TH.MIN
   0.75f, // TH.MAX
   0.5f,  // DRIVE_PRE
   0.5f,  // SAT_AMT
   1.0f,  // FADE_AMT
};

#define MOD_DRYWET      0
#define MOD_DRIVE_PRE   1
#define MOD_TH_MIN      2
#define MOD_TH_MAX      3
#define MOD_DRIVE_POST  4
#define MOD_SAT_AMT     5
#define MOD_FADE_AMT    6
#define NUM_MODS        7
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Drive Pre",
   "Th.Min",
   "Th.Max",
   "Drive Post",
   "Sat Amt",
   "Fade Amt"
};

typedef struct ws_lin_cpx_info_s {
   st_plugin_info_t base;
} ws_lin_cpx_info_t;

typedef struct ws_lin_cpx_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ws_lin_cpx_shared_t;

typedef struct ws_lin_cpx_voice_s {
   st_plugin_voice_t base;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_drive_pre_cur;
   float mod_drive_pre_inc;
   float mod_threshold_min_cur;
   float mod_threshold_min_inc;
   float mod_threshold_max_cur;
   float mod_threshold_max_inc;
   float mod_drive_post_cur;
   float mod_drive_post_inc;
   float mod_sat_amt_cur;
   float mod_sat_amt_inc;
   float mod_fade_amt_cur;
   float mod_fade_amt_inc;
} ws_lin_cpx_voice_t;


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
   ST_PLUGIN_SHARED_CAST(ws_lin_cpx_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ws_lin_cpx_shared_t);
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
   ST_PLUGIN_VOICE_CAST(ws_lin_cpx_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_lin_cpx_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_lin_cpx_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_lin_cpx_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modDrivePre = ((shared->params[PARAM_DRIVE_PRE] - 0.5f) * 2.0f) + voice->mods[MOD_DRIVE_PRE];
   modDrivePre = powf(10.0f, modDrivePre * 2.0f);

   float modThresholdMin = shared->params[PARAM_TH_MIN] + voice->mods[MOD_TH_MIN];
   if(modThresholdMin < 0.0f)
      modThresholdMin  = -modThresholdMin;

   float modThresholdMax = shared->params[PARAM_TH_MAX] + voice->mods[MOD_TH_MAX];
   if(modThresholdMax < 0.0f)
      modThresholdMax  = -modThresholdMax;

   float modDrivePost = ((shared->params[PARAM_DRIVE_POST] - 0.5f) * 2.0f) + voice->mods[MOD_DRIVE_POST];
   modDrivePost = powf(10.0f, modDrivePost * 2.0f);

   float modSatAmt = shared->params[PARAM_SAT_AMT]   + voice->mods[MOD_SAT_AMT];
   modSatAmt = Dstplugin_clamp(modSatAmt, 0.0f, 1.0f);

   float modFadeAmt = shared->params[PARAM_FADE_AMT]   + voice->mods[MOD_FADE_AMT];
   modFadeAmt = Dstplugin_clamp(modFadeAmt, 0.0f, 1.0f);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc        = (modDryWet       - voice->mod_drywet_cur)         * recBlockSize;
      voice->mod_drive_pre_inc     = (modDrivePre     - voice->mod_drive_pre_cur)      * recBlockSize;
      voice->mod_threshold_min_inc = (modThresholdMin - voice->mod_threshold_min_cur)  * recBlockSize;
      voice->mod_threshold_max_inc = (modThresholdMax - voice->mod_threshold_max_cur)  * recBlockSize;
      voice->mod_drive_post_inc    = (modDrivePost    - voice->mod_drive_post_cur)     * recBlockSize;
      voice->mod_sat_amt_inc       = (modSatAmt       - voice->mod_sat_amt_cur)        * recBlockSize;
      voice->mod_fade_amt_inc      = (modFadeAmt      - voice->mod_fade_amt_cur)       * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur         = modDryWet;
      voice->mod_drywet_inc         = 0.0f;
      voice->mod_drive_pre_cur      = modDrivePre;
      voice->mod_drive_pre_inc      = 0.0f;
      voice->mod_threshold_min_cur  = modThresholdMin;
      voice->mod_threshold_min_inc  = 0.0f;
      voice->mod_threshold_max_cur  = modThresholdMax;
      voice->mod_threshold_max_inc  = 0.0f;
      voice->mod_drive_post_cur     = modDrivePost;
      voice->mod_drive_post_inc     = 0.0f;
      voice->mod_sat_amt_cur        = modSatAmt;
      voice->mod_sat_amt_inc        = 0.0f;
      voice->mod_fade_amt_cur       = modFadeAmt;
      voice->mod_fade_amt_inc       = 0.0f;
   }
}

static inline float loc_lm(float a, float b) {
   stplugin_fi_t ta, tb, tr; ta.f = a; tb.f = b;
   tr.u = (ta.u & 0x7FFFffffu) + (tb.u & 0x7FFFffffu) - 0x3F780000u;
   if(tr.s < 0)
      tr.s = 0;
   tr.u &= 0x7FFFffffu;
   tr.u |= ((ta.u & 0x80000000u) ^ (tb.u & 0x80000000u));
#if 0
   if(0x7F800000u == (tr.u & 0x7F800000u))
      tr.u = 0u;
#endif
   return tr.f;
}
#define Dmulf(a,b) loc_lm(a,b)

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t *_voice,
                                              int                _bMonoIn,
                                              const float       *_samplesIn,
                                              float             *_samplesOut,
                                              unsigned int       _numFrames
                                              ) {
   ST_PLUGIN_VOICE_CAST(ws_lin_cpx_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_lin_cpx_shared_t);

   unsigned int k = 0u;

   // Stereo input, stereo output
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      float l = _samplesIn[k];
      float r = _samplesIn[k + 1u];

      float wetL = loc_lm(l, voice->mod_drive_pre_cur);
      float wetR = loc_lm(r, voice->mod_drive_pre_cur);

      stplugin_fi_t lAbs; lAbs.f = l; lAbs.u &= 0x7FFFffffu;
      stplugin_fi_t rAbs; rAbs.f = r; rAbs.u &= 0x7FFFffffu;

      if(voice->mod_threshold_min_cur > 0.0f)
      {
         if(lAbs.f < voice->mod_threshold_min_cur)
         {
            float ampL = l * voice->mod_drive_pre_cur;
            float amt = (lAbs.f / voice->mod_threshold_min_cur);
            amt = (1.0f - voice->mod_fade_amt_cur) + (voice->mod_fade_amt_cur * amt);
            wetL = ampL + (wetL - ampL) * amt;
         }

         if(rAbs.f < voice->mod_threshold_min_cur)
         {
            float ampR = r * voice->mod_drive_pre_cur;
            float amt = (rAbs.f / voice->mod_threshold_min_cur);
            amt = (1.0f - voice->mod_fade_amt_cur) + (voice->mod_fade_amt_cur * amt);
            wetR = ampR + (wetR - ampR) * amt;
         }
      }

      if(voice->mod_threshold_max_cur > 0.0f)
      {
         if(lAbs.f >= voice->mod_threshold_max_cur)
         {
            float ampL = l * voice->mod_drive_pre_cur;
            float amt = (lAbs.f - voice->mod_threshold_max_cur) / voice->mod_threshold_max_cur;
            if(amt > 1.0f)
               amt = 1.0f;
            amt = amt * voice->mod_fade_amt_cur;
            wetL = wetL + (ampL - wetL) * amt;
         }

         if(rAbs.f >= voice->mod_threshold_max_cur)
         {
            float ampR = r * voice->mod_drive_pre_cur;
            float amt = (rAbs.f - voice->mod_threshold_max_cur) / voice->mod_threshold_max_cur;
            if(amt > 1.0f)
               amt = 1.0f;
            amt = amt * voice->mod_fade_amt_cur;
            wetR = wetR + (ampR - wetR) * amt;
         }
      }

      wetL *= voice->mod_drive_post_cur;
      wetR *= voice->mod_drive_post_cur;

      if(voice->mod_sat_amt_cur > 0.0f)
      {
         wetL = wetL + (tanhf(wetL) - wetL) * voice->mod_sat_amt_cur;
         wetR = wetR + (tanhf(wetR) - wetR) * voice->mod_sat_amt_cur;
      }

      float outL = l + (wetL - l) * voice->mod_drywet_cur;
      float outR = r + (wetR - r) * voice->mod_drywet_cur;

      outL = Dstplugin_fix_denorm_32(outL);
      outR = Dstplugin_fix_denorm_32(outR);

      _samplesOut[k]      = outL;
      _samplesOut[k + 1u] = outR;

      // Next frame
      k += 2u;
      voice->mod_drywet_cur        += voice->mod_drywet_inc;
      voice->mod_drive_pre_cur     += voice->mod_drive_pre_inc;
      voice->mod_threshold_min_cur += voice->mod_threshold_min_inc;
      voice->mod_threshold_max_cur += voice->mod_threshold_max_inc;
      voice->mod_drive_post_cur    += voice->mod_drive_post_inc;
      voice->mod_sat_amt_cur       += voice->mod_sat_amt_inc;
      voice->mod_fade_amt_cur      += voice->mod_fade_amt_inc;
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ws_lin_cpx_shared_t *ret = malloc(sizeof(ws_lin_cpx_shared_t));
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
   ws_lin_cpx_voice_t *ret = malloc(sizeof(ws_lin_cpx_voice_t));
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

st_plugin_info_t *ws_lin_cpx_init(void) {
   ws_lin_cpx_info_t *ret = NULL;

   ret = malloc(sizeof(ws_lin_cpx_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ws lin cpx";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "linear complexity shaper";
      ret->base.short_name  = "ws lin cpx";
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
      ret->base.note_on          = &loc_note_on;
      ret->base.set_mod_value    = &loc_set_mod_value;
      ret->base.prepare_block    = &loc_prepare_block;
      ret->base.process_replace  = &loc_process_replace;
      ret->base.plugin_exit      = &loc_plugin_exit;
   }

   return &ret->base;
}
