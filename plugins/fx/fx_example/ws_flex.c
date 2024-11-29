// ----
// ---- file   : ws_flex.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2021-2024 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a quintic waveshaper that supports per-sample-frame parameter interpolation
// ----           idea based on DHE's 'cubic' VCV Rack module
// ----
// ---- created: 23Feb2021
// ---- changed: 21Jan2024, 14Oct2024
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET   0
#define PARAM_OFF      1
#define PARAM_DRIVE    2
#define PARAM_CEIL     3
#define PARAM_MIDUP    4
#define PARAM_HALF     5
#define PARAM_MIDDOWN  6
#define PARAM_FLOOR    7
#define NUM_PARAMS     8
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Offset",
   "Drive",
   "Ceiling",
   "Mid Up",
   "Half-Way",
   "Mid Down",
   "Floor"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // OFF
   0.5f,  // DRIVE
   0.5f,  // CEIL
   0.5f,  // MIDUP
   0.5f,  // HALF
   0.5f,  // MIDDOWN
   0.5f  // FLOOR
};

#define MOD_DRYWET   0
#define MOD_OFF      1
#define MOD_DRIVE    2
#define MOD_CEIL     3
#define MOD_MIDUP    4
#define MOD_HALF     5
#define MOD_MIDDOWN  6
#define MOD_FLOOR    7
#define NUM_MODS     8
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Offset",
   "Drive",
   "Ceiling",
   "Mid Up",
   "Half-Way",
   "Mid Down",
   "Floor"
};

typedef struct ws_flex_info_s {
   st_plugin_info_t base;
} ws_flex_info_t;

typedef struct ws_flex_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ws_flex_shared_t;

typedef struct ws_flex_voice_s {
   st_plugin_voice_t base;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_off_cur;
   float mod_off_inc;
   float mod_drive_cur;
   float mod_drive_inc;
   float mod_ceil_cur;
   float mod_ceil_inc;
   float mod_midup_cur;
   float mod_midup_inc;
   float mod_half_cur;
   float mod_half_inc;
   float mod_middown_cur;
   float mod_middown_inc;
   float mod_floor_cur;
   float mod_floor_inc;
} ws_flex_voice_t;


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
   ST_PLUGIN_SHARED_CAST(ws_flex_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ws_flex_shared_t);
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
   ST_PLUGIN_VOICE_CAST(ws_flex_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_flex_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_flex_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_flex_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modOff = (shared->params[PARAM_OFF]-0.5f)*2.0f + voice->mods[MOD_OFF];

   float modDrive = (shared->params[PARAM_DRIVE]-0.5f)*2.0f + voice->mods[MOD_DRIVE];
   modDrive = powf(10.0f, modDrive * 3.0f);

   float modCeil    = (shared->params[PARAM_CEIL]   -0.5f)*2.0f + voice->mods[MOD_CEIL];
   float modMidUp   = (shared->params[PARAM_MIDUP]  -0.5f)*2.0f + voice->mods[MOD_MIDUP];
   float modHalf    = (shared->params[PARAM_HALF]   -0.5f)*2.0f + voice->mods[MOD_HALF];
   float modMidDown = (shared->params[PARAM_MIDDOWN]-0.5f)*2.0f + voice->mods[MOD_MIDDOWN];
   float modFloor   = (shared->params[PARAM_FLOOR]  -0.5f)*2.0f + voice->mods[MOD_FLOOR];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc  = (modDryWet  - voice->mod_drywet_cur)  * recBlockSize;
      voice->mod_off_inc     = (modOff     - voice->mod_off_cur)     * recBlockSize;
      voice->mod_drive_inc   = (modDrive   - voice->mod_drive_cur)   * recBlockSize;
      voice->mod_ceil_inc    = (modCeil    - voice->mod_ceil_cur)    * recBlockSize;
      voice->mod_midup_inc   = (modMidUp   - voice->mod_midup_cur)   * recBlockSize;
      voice->mod_half_inc    = (modHalf    - voice->mod_half_cur)    * recBlockSize;
      voice->mod_middown_inc = (modMidDown - voice->mod_middown_cur) * recBlockSize;
      voice->mod_floor_inc   = (modFloor   - voice->mod_floor_cur)   * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur  = modDryWet;
      voice->mod_drywet_inc  = 0.0f;
      voice->mod_off_cur     = modOff;
      voice->mod_off_inc     = 0.0f;
      voice->mod_drive_cur   = modDrive;
      voice->mod_drive_inc   = 0.0f;
      voice->mod_ceil_cur    = modCeil;
      voice->mod_ceil_inc    = 0.0f;
      voice->mod_midup_cur   = modMidUp;
      voice->mod_midup_inc   = 0.0f;
      voice->mod_half_cur    = modHalf;
      voice->mod_half_inc    = 0.0f;
      voice->mod_middown_cur = modMidDown;
      voice->mod_middown_inc = 0.0f;
      voice->mod_floor_cur   = modFloor;
      voice->mod_floor_inc   = 0.0f;
   }
}

static float loc_bend(float f, float c) {
   // c: <0: log
   //     0: lin
   //     1: exp
   stplugin_fi_t uSign;
   uSign.f = f;
   stplugin_fi_t u;
   u.f = f;
   u.u &= 0x7fffFFFFu;
   u.f *= (1.0f/0.4f);  // => 0..1
   float linF = u.f;
   stplugin_fi_t cAbs;
   cAbs.f = c;
   cAbs.u &= 0x7fffFFFFu;
   if(cAbs.f > 1.0f)
      cAbs.f = 1.0f;
   u.f = logf(1.0f + powf(u.f, 0.255f + powf(10.0f, c * 2.0f))) * 1.44269504089f/*div log(2)*/;
   u.f = linF + (u.f - linF) * cAbs.f;
   u.f *= 0.4f;
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
   ST_PLUGIN_VOICE_CAST(ws_flex_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_flex_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float outL = l + voice->mod_off_cur;

         outL *= voice->mod_drive_cur;
         if(outL >= 1.0f)
            outL = 1.0f;
         else if(outL < -1.0f)
            outL = -1.0f;

         if(outL >= 0.6f)
         {
            // ceil
            outL -= 0.6f; // to range 0..0.4
            outL = loc_bend(outL, voice->mod_ceil_cur);
            outL += 0.6f;
         }
         else if(outL >= 0.2f)
         {
            // mid-up
            outL -= 0.2f; // to range 0..0.4
            outL = loc_bend(outL, voice->mod_midup_cur);
            outL += 0.2f;
         }
         else if(outL >= -0.2f)
         {
            // half
            outL -= -0.2f; // to range 0..0.4
            outL = loc_bend(outL, voice->mod_half_cur);
            outL += -0.2f;
         }
         else if(outL >= -0.6f)
         {
            // mid-down
            outL -= -0.6f; // to range 0..0.4
            outL = loc_bend(outL, voice->mod_middown_cur);
            outL += -0.6f;
         }
         else
         {
            // floor
            outL -= -1.0f; // to range 0..0.4
            outL = loc_bend(outL, voice->mod_floor_cur);
            outL += -1.0f;
         }

         outL -= voice->mod_off_cur;

         outL = l + (outL - l) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outL;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur  += voice->mod_drywet_inc;
         voice->mod_off_cur     += voice->mod_off_inc;
         voice->mod_drive_cur   += voice->mod_drive_inc;
         voice->mod_ceil_cur    += voice->mod_ceil_inc;
         voice->mod_midup_cur   += voice->mod_midup_inc;
         voice->mod_half_cur    += voice->mod_half_inc;
         voice->mod_middown_cur += voice->mod_middown_inc;
         voice->mod_floor_cur   += voice->mod_floor_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float r = _samplesIn[k + 1u];

         // Left
         float outL = l + voice->mod_off_cur;

         outL *= voice->mod_drive_cur;
         if(outL >= 1.0f)
            outL = 1.0f;
         else if(outL < -1.0f)
            outL = -1.0f;

         if(outL >= 0.6f)
         {
            // ceil
            outL -= 0.6f; // to range 0..0.4
            outL = loc_bend(outL, voice->mod_ceil_cur);
            outL += 0.6f;
         }
         else if(outL >= 0.2f)
         {
            // mid-up
            outL -= 0.2f; // to range 0..0.4
            outL = loc_bend(outL, voice->mod_midup_cur);
            outL += 0.2f;
         }
         else if(outL >= -0.2f)
         {
            // half
            outL -= -0.2f; // to range 0..0.4
            outL = loc_bend(outL, voice->mod_half_cur);
            outL += -0.2f;
         }
         else if(outL >= -0.6f)
         {
            // mid-down
            outL -= -0.6f; // to range 0..0.4
            outL = loc_bend(outL, voice->mod_middown_cur);
            outL += -0.6f;
         }
         else
         {
            // floor
            outL -= -1.0f; // to range 0..0.4
            outL = loc_bend(outL, voice->mod_floor_cur);
            outL += -1.0f;
         }

         outL -= voice->mod_off_cur;

         // Right
         float outR = r + voice->mod_off_cur;

         outR *= voice->mod_drive_cur;
         if(outR >= 1.0f)
            outR = 1.0f;
         else if(outR < -1.0f)
            outR = -1.0f;

         if(outR >= 0.6f)
         {
            // ceil
            outR -= 0.6f; // to range 0..0.4
            outR = loc_bend(outR, voice->mod_ceil_cur);
            outR += 0.6f;
         }
         else if(outR >= 0.2f)
         {
            // mid-up
            outR -= 0.2f; // to range 0..0.4
            outR = loc_bend(outR, voice->mod_midup_cur);
            outR += 0.2f;
         }
         else if(outR >= -0.2f)
         {
            // half
            outR -= -0.2f; // to range 0..0.4
            outR = loc_bend(outR, voice->mod_half_cur);
            outR += -0.2f;
         }
         else if(outR >= -0.6f)
         {
            // mid-down
            outR -= -0.6f; // to range 0..0.4
            outR = loc_bend(outR, voice->mod_middown_cur);
            outR += -0.6f;
         }
         else
         {
            // floor
            outR -= -1.0f; // to range 0..0.4
            outR = loc_bend(outR, voice->mod_floor_cur);
            outR += -1.0f;
         }

         outR -= voice->mod_off_cur;

         // Output
         outL = l + (outL - l) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);

         outR = r + (outR - r) * voice->mod_drywet_cur;
         outR = Dstplugin_fix_denorm_32(outR);

         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur  += voice->mod_drywet_inc;
         voice->mod_off_cur     += voice->mod_off_inc;
         voice->mod_drive_cur   += voice->mod_drive_inc;
         voice->mod_ceil_cur    += voice->mod_ceil_inc;
         voice->mod_midup_cur   += voice->mod_midup_inc;
         voice->mod_half_cur    += voice->mod_half_inc;
         voice->mod_middown_cur += voice->mod_middown_inc;
         voice->mod_floor_cur   += voice->mod_floor_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ws_flex_shared_t *ret = malloc(sizeof(ws_flex_shared_t));
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
   ws_flex_voice_t *ret = malloc(sizeof(ws_flex_voice_t));
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

st_plugin_info_t *ws_flex_init(void) {
   ws_flex_info_t *ret = NULL;

   ret = malloc(sizeof(ws_flex_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ws flex";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "flex shaper";
      ret->base.short_name  = "flex";
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
