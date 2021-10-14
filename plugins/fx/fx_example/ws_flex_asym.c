// ----
// ---- file   : ws_flex_asym.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2021 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a quintic waveshaper that supports per-sample-frame parameter interpolation
// ----           idea based on DHE's 'cubic' VCV Rack module
// ----
// ---- created: 23Feb2021
// ---- changed: 
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "../../../plugin.h"

#define PARAM_DRYWET   0
#define PARAM_OFF      1
#define PARAM_DRIVE    2
#define PARAM_POS      3
#define PARAM_NEG      4
#define NUM_PARAMS     5
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Offset",
   "Drive",
   "Pos",
   "Neg",
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // OFF
   0.5f,  // DRIVE
   0.5f,  // POS
   0.5f,  // NEG
};

#define MOD_DRYWET   0
#define MOD_OFF      1
#define MOD_DRIVE    2
#define MOD_POS      3
#define MOD_NEG      4
#define NUM_MODS     5
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Offset",
   "Drive",
   "Pos",
   "Neg",
};

typedef struct ws_flex_asym_info_s {
   st_plugin_info_t base;
} ws_flex_asym_info_t;

typedef struct ws_flex_asym_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ws_flex_asym_shared_t;

typedef struct ws_flex_asym_voice_s {
   st_plugin_voice_t base;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_off_cur;
   float mod_off_inc;
   float mod_drive_cur;
   float mod_drive_inc;
   float mod_pos_cur;
   float mod_pos_inc;
   float mod_neg_cur;
   float mod_neg_inc;
} ws_flex_asym_voice_t;


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
   ST_PLUGIN_SHARED_CAST(ws_flex_asym_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ws_flex_asym_shared_t);
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
   ST_PLUGIN_VOICE_CAST(ws_flex_asym_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_flex_asym_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_flex_asym_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_flex_asym_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modOff = (shared->params[PARAM_OFF]-0.5f)*2.0f + voice->mods[MOD_OFF];

   float modDrive = (shared->params[PARAM_DRIVE]-0.5f)*2.0f + voice->mods[MOD_DRIVE];
   modDrive = powf(10.0f, modDrive * 3.0f);

   float modPos = (shared->params[PARAM_POS]-0.5f)*2.0f + voice->mods[MOD_POS];
   float modNeg = (shared->params[PARAM_NEG]-0.5f)*2.0f + voice->mods[MOD_NEG];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc  = (modDryWet  - voice->mod_drywet_cur)  * recBlockSize;
      voice->mod_off_inc     = (modOff     - voice->mod_off_cur)     * recBlockSize;
      voice->mod_drive_inc   = (modDrive   - voice->mod_drive_cur)   * recBlockSize;
      voice->mod_pos_inc     = (modPos     - voice->mod_pos_cur)     * recBlockSize;
      voice->mod_neg_inc     = (modNeg     - voice->mod_neg_cur)     * recBlockSize;
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
      voice->mod_pos_cur     = modPos;
      voice->mod_pos_inc     = 0.0f;
      voice->mod_neg_cur     = modNeg;
      voice->mod_neg_inc     = 0.0f;
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
   u.ui &= 0x7fffFFFFu;
   u.f *= (1.0f/0.4f);  // => 0..1
   float linF = u.f;
   stplugin_fi_t cAbs;
   cAbs.f = c;
   cAbs.ui &= 0x7fffFFFFu;
   if(cAbs.f > 1.0f)
      cAbs.f = 1.0f;
   u.f = logf(1.0f + powf(u.f, 0.255f + powf(10.0f, c * 2.0f))) * 1.44269504089f/*div log(2)*/;
   u.f = linF + (u.f - linF) * cAbs.f;
   u.f *= 0.4f;
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
   ST_PLUGIN_VOICE_CAST(ws_flex_asym_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_flex_asym_shared_t);

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

         if(outL >= 0.0f)
         {
            // pos
            outL = loc_bend(outL, voice->mod_pos_cur);
         }
         else
         {
            // neg
            outL = -loc_bend(-outL, voice->mod_neg_cur);
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
         voice->mod_pos_cur     += voice->mod_pos_inc;
         voice->mod_neg_cur     += voice->mod_neg_inc;
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

         if(outL >= 0.0f)
         {
            // pos
            outL = loc_bend(outL, voice->mod_pos_cur);
         }
         else
         {
            // neg
            outL = -loc_bend(-outL, voice->mod_neg_cur);
         }

         outL -= voice->mod_off_cur;

         // Right
         float outR = r + voice->mod_off_cur;

         outR *= voice->mod_drive_cur;
         if(outR >= 1.0f)
            outR = 1.0f;
         else if(outR < -1.0f)
            outR = -1.0f;

         if(outR >= 0.0f)
         {
            // pos
            outR = loc_bend(outR, voice->mod_pos_cur);
         }
         else
         {
            // neg
            outR = -loc_bend(-outR, voice->mod_neg_cur);
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
         voice->mod_pos_cur     += voice->mod_pos_inc;
         voice->mod_neg_cur     += voice->mod_neg_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ws_flex_asym_shared_t *ret = malloc(sizeof(ws_flex_asym_shared_t));
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
   ws_flex_asym_voice_t *ret = malloc(sizeof(ws_flex_asym_voice_t));
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

st_plugin_info_t *ws_flex_asym_init(void) {
   ws_flex_asym_info_t *ret = NULL;

   ret = malloc(sizeof(ws_flex_asym_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ws flex asym";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "flex shaper asym";
      ret->base.short_name  = "flex asym";
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
