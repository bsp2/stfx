// ----
// ---- file   : ws_slew_asym.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2021-2024 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : waveshaper with configurable neg/pos rise/fall slew rates and log..lin..exp curve
// ----
// ---- created: 02May2021
// ---- changed: 21Jan2024, 14Oct2024
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET_POS  0
#define PARAM_RISE_POS    1
#define PARAM_FALL_POS    2
#define PARAM_CURVE_POS   3
#define PARAM_DRYWET_NEG  4
#define PARAM_RISE_NEG    5
#define PARAM_FALL_NEG    6
#define PARAM_CURVE_NEG   7
#define NUM_PARAMS        8
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet +",
   "Rise+",
   "Fall+",
   "Curve+",
   "Dry / Wet -",
   "Rise-",
   "Fall-",
   "Curve-",
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET_POS
   0.5f,  // RISE_POS
   0.5f,  // FALL_POS
   0.5f,  // CURVE_POS
   1.0f,  // DRYWET_NEG
   0.5f,  // RISE_NEG
   0.5f,  // FALL_NEG
   0.5f,  // CURVE_NEG
};

#define MOD_DRYWET_POS  0
#define MOD_RISE_POS    1
#define MOD_FALL_POS    2
#define MOD_CURVE_POS   3
#define MOD_DRYWET_NEG  4
#define MOD_RISE_NEG    5
#define MOD_FALL_NEG    6
#define MOD_CURVE_NEG   7
#define NUM_MODS        8
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet +",
   "Rise+",
   "Fall+",
   "Curve+",
   "Dry / Wet -",
   "Rise-",
   "Fall-",
   "Curve-",
};

typedef struct ws_slew_asym_info_s {
   st_plugin_info_t base;
} ws_slew_asym_info_t;

typedef struct ws_slew_asym_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ws_slew_asym_shared_t;

typedef struct ws_slew_asym_voice_s {
   st_plugin_voice_t base;
   float mods[NUM_MODS];
   float mod_drywet_pos_cur;
   float mod_drywet_pos_inc;
   float mod_rise_pos_cur;
   float mod_rise_pos_inc;
   float mod_fall_pos_cur;
   float mod_fall_pos_inc;
   float mod_curve_pos_cur;
   float mod_curve_pos_inc;
   float mod_drywet_neg_cur;
   float mod_drywet_neg_inc;
   float mod_rise_neg_cur;
   float mod_rise_neg_inc;
   float mod_fall_neg_cur;
   float mod_fall_neg_inc;
   float mod_curve_neg_cur;
   float mod_curve_neg_inc;
   float last_l;
   float last_r;
} ws_slew_asym_voice_t;


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
   ST_PLUGIN_SHARED_CAST(ws_slew_asym_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ws_slew_asym_shared_t);
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
   ST_PLUGIN_VOICE_CAST(ws_slew_asym_voice_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
      voice->last_l = 0.0f;
      voice->last_r = 0.0f;
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(ws_slew_asym_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_slew_asym_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_slew_asym_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   // +
   float modDryWetPos = shared->params[PARAM_DRYWET_POS]   + voice->mods[MOD_DRYWET_POS];
   modDryWetPos = Dstplugin_clamp(modDryWetPos, 0.0f, 1.0f);

   float modRisePos = shared->params[PARAM_RISE_POS] + voice->mods[MOD_RISE_POS];
   if(modRisePos > 1.0f)
      modRisePos = 1.0f;
   else if(modRisePos < 0.0f)
      modRisePos = 0.0f;

   float modFallPos = shared->params[PARAM_FALL_POS] + voice->mods[MOD_FALL_POS];
   if(modFallPos > 1.0f)
      modFallPos = 1.0f;
   else if(modFallPos < 0.0f)
      modFallPos = 0.0f;

   float modCurvePos = (shared->params[PARAM_CURVE_POS]-0.5f)*2.0f + voice->mods[MOD_CURVE_POS];

   // -
   float modDryWetNeg = shared->params[PARAM_DRYWET_NEG]   + voice->mods[MOD_DRYWET_NEG];
   modDryWetNeg = Dstplugin_clamp(modDryWetNeg, 0.0f, 1.0f);

   float modRiseNeg = shared->params[PARAM_RISE_NEG] + voice->mods[MOD_RISE_NEG];
   if(modRiseNeg > 1.0f)
      modRiseNeg = 1.0f;
   else if(modRiseNeg < 0.0f)
      modRiseNeg = 0.0f;

   float modFallNeg = shared->params[PARAM_FALL_NEG] + voice->mods[MOD_FALL_NEG];
   if(modFallNeg > 1.0f)
      modFallNeg = 1.0f;
   else if(modFallNeg < 0.0f)
      modFallNeg = 0.0f;

   float modCurveNeg = (shared->params[PARAM_CURVE_NEG]-0.5f)*2.0f + voice->mods[MOD_CURVE_NEG];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_pos_inc  = (modDryWetPos - voice->mod_drywet_pos_cur) * recBlockSize;
      voice->mod_rise_pos_inc    = (modRisePos   - voice->mod_rise_pos_cur)   * recBlockSize;
      voice->mod_fall_pos_inc    = (modFallPos   - voice->mod_fall_pos_cur)   * recBlockSize;
      voice->mod_curve_pos_inc   = (modCurvePos  - voice->mod_curve_pos_cur)  * recBlockSize;
      voice->mod_drywet_neg_inc  = (modDryWetNeg - voice->mod_drywet_neg_cur) * recBlockSize;
      voice->mod_rise_neg_inc    = (modRiseNeg   - voice->mod_rise_neg_cur)   * recBlockSize;
      voice->mod_fall_neg_inc    = (modFallNeg   - voice->mod_fall_neg_cur)   * recBlockSize;
      voice->mod_curve_neg_inc   = (modCurveNeg  - voice->mod_curve_neg_cur)  * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_pos_cur  = modDryWetPos;
      voice->mod_drywet_pos_inc  = 0.0f;
      voice->mod_rise_pos_cur    = modRisePos;
      voice->mod_rise_pos_inc    = 0.0f;
      voice->mod_fall_pos_cur    = modFallPos;
      voice->mod_fall_pos_inc    = 0.0f;
      voice->mod_curve_pos_cur   = modCurvePos;
      voice->mod_curve_pos_inc   = 0.0f;
      voice->mod_drywet_neg_cur  = modDryWetNeg;
      voice->mod_drywet_neg_inc  = 0.0f;
      voice->mod_rise_neg_cur    = modRiseNeg;
      voice->mod_rise_neg_inc    = 0.0f;
      voice->mod_fall_neg_cur    = modFallNeg;
      voice->mod_fall_neg_inc    = 0.0f;
      voice->mod_curve_neg_cur   = modCurveNeg;
      voice->mod_curve_neg_inc   = 0.0f;
   }
}

static float loc_mathLogLinExpf(float _f, float _c) {
   // c: <0: log
   //     0: lin
   //    >0: exp
   stplugin_fi_t uSign;
   uSign.f = _f;
   stplugin_fi_t u;
   u.f = _f;
   u.u &= 0x7fffFFFFu;
   u.f = powf(u.f, powf(2.0f, _c));
   u.u |= uSign.u & 0x80000000u;
   return u.f;
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(ws_slew_asym_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_slew_asym_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];

         if(l >= 0.0f)
         {
            if(l >= voice->last_l)
            {
               float fRise = voice->mod_rise_pos_cur;
               fRise = 1.0f - fRise;
               fRise = fRise * fRise * fRise;
               voice->last_l += (l - voice->last_l) * fRise;
            }
            else
            {
               float fFall = voice->mod_fall_pos_cur;
               fFall = 1.0f - fFall;
               fFall = fFall * fFall * fFall;
               voice->last_l += (l - voice->last_l) * fFall;
            }

            float outL = loc_mathLogLinExpf(voice->last_l, voice->mod_curve_pos_cur);

            outL = l + (outL - l) * voice->mod_drywet_pos_cur;
            outL = Dstplugin_fix_denorm_32(outL);
            _samplesOut[k]      = outL;
            _samplesOut[k + 1u] = outL;
         }
         else
         {
            if(l >= voice->last_l)
            {
               float fRise = voice->mod_rise_neg_cur;
               fRise = 1.0f - fRise;
               fRise = fRise * fRise * fRise;
               voice->last_l += (l - voice->last_l) * fRise;
            }
            else
            {
               float fFall = voice->mod_fall_neg_cur;
               fFall = 1.0f - fFall;
               fFall = fFall * fFall * fFall;
               voice->last_l += (l - voice->last_l) * fFall;
            }

            float outL = loc_mathLogLinExpf(voice->last_l, voice->mod_curve_neg_cur);

            outL = l + (outL - l) * voice->mod_drywet_neg_cur;
            outL = Dstplugin_fix_denorm_32(outL);
            _samplesOut[k]      = outL;
            _samplesOut[k + 1u] = outL;
         }

         // Next frame
         k += 2u;
         voice->mod_drywet_pos_cur += voice->mod_drywet_pos_inc;
         voice->mod_rise_pos_cur   += voice->mod_rise_pos_inc;
         voice->mod_fall_pos_cur   += voice->mod_fall_pos_inc;
         voice->mod_curve_pos_cur  += voice->mod_curve_pos_inc;
         voice->mod_drywet_neg_cur += voice->mod_drywet_neg_inc;
         voice->mod_rise_neg_cur   += voice->mod_rise_neg_inc;
         voice->mod_fall_neg_cur   += voice->mod_fall_neg_inc;
         voice->mod_curve_neg_cur  += voice->mod_curve_neg_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         // Left
         float l = _samplesIn[k];

         if(l >= 0.0f)
         {
            if(l >= voice->last_l)
            {
               float fRise = voice->mod_rise_pos_cur;
               fRise = 1.0f - fRise;
               fRise = fRise * fRise * fRise;
               voice->last_l += (l - voice->last_l) * fRise;
            }
            else
            {
               float fFall = voice->mod_fall_pos_cur;
               fFall = 1.0f - fFall;
               fFall = fFall * fFall * fFall;
               voice->last_l += (l - voice->last_l) * fFall;
            }

            float outL = loc_mathLogLinExpf(voice->last_l, voice->mod_curve_pos_cur);

            outL = l + (outL - l) * voice->mod_drywet_pos_cur;
            outL = Dstplugin_fix_denorm_32(outL);
            _samplesOut[k] = outL;
         }
         else
         {
            if(l >= voice->last_l)
            {
               float fRise = voice->mod_rise_neg_cur;
               fRise = 1.0f - fRise;
               fRise = fRise * fRise * fRise;
               voice->last_l += (l - voice->last_l) * fRise;
            }
            else
            {
               float fFall = voice->mod_fall_neg_cur;
               fFall = 1.0f - fFall;
               fFall = fFall * fFall * fFall;
               voice->last_l += (l - voice->last_l) * fFall;
            }

            float outL = loc_mathLogLinExpf(voice->last_l, voice->mod_curve_neg_cur);

            outL = l + (outL - l) * voice->mod_drywet_neg_cur;
            outL = Dstplugin_fix_denorm_32(outL);
            _samplesOut[k] = outL;
         }

         // Right
         float r = _samplesIn[k + 1u];

         if(r >= 0.0f)
         {
            if(r >= voice->last_r)
            {
               float fRise = voice->mod_rise_pos_cur;
               fRise = 1.0f - fRise;
               fRise = fRise * fRise * fRise;
               voice->last_r += (r - voice->last_r) * fRise;
            }
            else
            {
               float fFall = voice->mod_fall_pos_cur;
               fFall = 1.0f - fFall;
               fFall = fFall * fFall * fFall;
               voice->last_r += (r - voice->last_r) * fFall;
            }

            float outR = loc_mathLogLinExpf(voice->last_r, voice->mod_curve_pos_cur);

            outR = r + (outR - r) * voice->mod_drywet_pos_cur;
            outR = Dstplugin_fix_denorm_32(outR);
            _samplesOut[k + 1u] = outR;
         }
         else
         {
            if(r >= voice->last_r)
            {
               float fRise = voice->mod_rise_neg_cur;
               fRise = 1.0f - fRise;
               fRise = fRise * fRise * fRise;
               voice->last_r += (r - voice->last_r) * fRise;
            }
            else
            {
               float fFall = voice->mod_fall_neg_cur;
               fFall = 1.0f - fFall;
               fFall = fFall * fFall * fFall;
               voice->last_r += (r - voice->last_r) * fFall;
            }

            float outR = loc_mathLogLinExpf(voice->last_r, voice->mod_curve_neg_cur);

            outR = r + (outR - r) * voice->mod_drywet_neg_cur;
            outR = Dstplugin_fix_denorm_32(outR);
            _samplesOut[k + 1u] = outR;
         }

         // Next frame
         k += 2u;
         voice->mod_drywet_pos_cur += voice->mod_drywet_pos_inc;
         voice->mod_rise_pos_cur   += voice->mod_rise_pos_inc;
         voice->mod_fall_pos_cur   += voice->mod_fall_pos_inc;
         voice->mod_curve_pos_cur  += voice->mod_curve_pos_inc;
         voice->mod_drywet_neg_cur += voice->mod_drywet_neg_inc;
         voice->mod_rise_neg_cur   += voice->mod_rise_neg_inc;
         voice->mod_fall_neg_cur   += voice->mod_fall_neg_inc;
         voice->mod_curve_neg_cur  += voice->mod_curve_neg_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ws_slew_asym_shared_t *ret = malloc(sizeof(ws_slew_asym_shared_t));
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
   ws_slew_asym_voice_t *ret = malloc(sizeof(ws_slew_asym_voice_t));
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

st_plugin_info_t *ws_slew_asym_init(void) {
   ws_slew_asym_info_t *ret = NULL;

   ret = malloc(sizeof(ws_slew_asym_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ws slew asym";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "slew asym shaper";
      ret->base.short_name  = "slew asym";
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
