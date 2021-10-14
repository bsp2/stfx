// ----
// ---- file   : ws_slew.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2021 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : waveshaper with configurable rise/fall slew rates and log..lin..exp curve
// ----
// ---- created: 14Apr2021
// ---- changed: 
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET   0
#define PARAM_RISE     1
#define PARAM_FALL     2
#define PARAM_CURVE    3
#define NUM_PARAMS     4
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Rise",
   "Fall",
   "Curve",
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // RISE
   0.5f,  // FALL
   0.5f,  // CURVE
};

#define MOD_DRYWET   0
#define MOD_RISE     1
#define MOD_FALL     2
#define MOD_CURVE    3
#define NUM_MODS     4
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Rise",
   "Fall",
   "Curve",
};

typedef struct ws_slew_info_s {
   st_plugin_info_t base;
} ws_slew_info_t;

typedef struct ws_slew_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ws_slew_shared_t;

typedef struct ws_slew_voice_s {
   st_plugin_voice_t base;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_rise_cur;
   float mod_rise_inc;
   float mod_fall_cur;
   float mod_fall_inc;
   float mod_curve_cur;
   float mod_curve_inc;
   float last_l;
   float last_r;
} ws_slew_voice_t;


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
   ST_PLUGIN_SHARED_CAST(ws_slew_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ws_slew_shared_t);
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
   ST_PLUGIN_VOICE_CAST(ws_slew_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_slew_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_slew_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_slew_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modRise = shared->params[PARAM_RISE] + voice->mods[MOD_RISE];
   if(modRise > 1.0f)
      modRise = 1.0f;
   else if(modRise < 0.0f)
      modRise = 0.0f;

   float modFall = shared->params[PARAM_FALL] + voice->mods[MOD_FALL];
   if(modFall > 1.0f)
      modFall = 1.0f;
   else if(modFall < 0.0f)
      modFall = 0.0f;

   float modCurve = (shared->params[PARAM_CURVE]-0.5f)*2.0f + voice->mods[MOD_CURVE];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc  = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_rise_inc    = (modRise   - voice->mod_rise_cur)   * recBlockSize;
      voice->mod_fall_inc    = (modFall   - voice->mod_fall_cur)   * recBlockSize;
      voice->mod_curve_inc   = (modCurve  - voice->mod_curve_cur)  * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur  = modDryWet;
      voice->mod_drywet_inc  = 0.0f;
      voice->mod_rise_cur    = modRise;
      voice->mod_rise_inc    = 0.0f;
      voice->mod_fall_cur    = modFall;
      voice->mod_fall_inc    = 0.0f;
      voice->mod_curve_cur   = modCurve;
      voice->mod_curve_inc   = 0.0f;
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
   u.ui &= 0x7fffFFFFu;
   u.f = powf(u.f, powf(2.0f, _c));
   u.ui |= uSign.ui & 0x80000000u;
   return u.f;
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(ws_slew_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_slew_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];

         if(l >= voice->last_l)
         {
            float fRise = voice->mod_rise_cur;
            fRise = 1.0f - fRise;
            fRise = fRise * fRise * fRise;
            voice->last_l += (l - voice->last_l) * fRise;
         }
         else
         {
            float fFall = voice->mod_fall_cur;
            fFall = 1.0f - fFall;
            fFall = fFall * fFall * fFall;
            voice->last_l += (l - voice->last_l) * fFall;
         }

         float outL = loc_mathLogLinExpf(voice->last_l, voice->mod_curve_cur);

         outL = l + (outL - l) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outL;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_rise_cur   += voice->mod_rise_inc;
         voice->mod_fall_cur   += voice->mod_fall_inc;
         voice->mod_curve_cur  += voice->mod_curve_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float r = _samplesIn[k + 1u];

         float fRise = voice->mod_rise_cur;
         fRise = 1.0f - fRise;
         fRise = fRise * fRise * fRise;

         float fFall = voice->mod_fall_cur;
         fFall = 1.0f - fFall;
         fFall = fFall * fFall * fFall;

         // Left
         if(l >= voice->last_l)
         {
            voice->last_l += (l - voice->last_l) * fRise;
         }
         else
         {
            voice->last_l += (l - voice->last_l) * fFall;
         }
         float outL = loc_mathLogLinExpf(voice->last_l, voice->mod_curve_cur);

         // Right
         if(r >= voice->last_r)
         {
            voice->last_r += (r - voice->last_r) * fRise;
         }
         else
         {
            voice->last_r += (r - voice->last_r) * fFall;
         }
         float outR = loc_mathLogLinExpf(voice->last_r, voice->mod_curve_cur);

         // Output
         outL = l + (outL - l) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);

         outR = r + (outR - r) * voice->mod_drywet_cur;
         outR = Dstplugin_fix_denorm_32(outR);

         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_rise_cur   += voice->mod_rise_inc;
         voice->mod_fall_cur   += voice->mod_fall_inc;
         voice->mod_curve_cur  += voice->mod_curve_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ws_slew_shared_t *ret = malloc(sizeof(ws_slew_shared_t));
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
   ws_slew_voice_t *ret = malloc(sizeof(ws_slew_voice_t));
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

st_plugin_info_t *ws_slew_init(void) {
   ws_slew_info_t *ret = NULL;

   ret = malloc(sizeof(ws_slew_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ws slew";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "slew shaper";
      ret->base.short_name  = "slew";
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
