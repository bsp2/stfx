// ----
// ---- file   : ws_sin_exp.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2023-2024 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : experimental waveshaper
// ----
// ---- created: 04Jan2023
// ---- changed: 21Jan2024
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET    0
#define PARAM_DRIVE     1
#define PARAM_EXP       2
#define PARAM_EXP2      3
#define PARAM_DRIVEPOST 4
#define PARAM_DRIVEDRY  5
#define NUM_PARAMS      6
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "DrivePre",
   "Exp1",
   "Exp2",
   "DrivePost",
   "DriveDry",
};
static float loc_param_resets[NUM_PARAMS] = {
   0.5f,  // DRYWET
   0.5f,  // Drive
   0.5f,  // Exp
   0.25f, // Exp2
   0.5f,  // DrivePost
   0.5f,  // DriveDry
};

#define MOD_DRYWET    0
#define MOD_DRIVE     1
#define MOD_EXP       2
#define MOD_EXP2      3
#define MOD_DRIVEPOST 4
#define MOD_DRIVEDRY  5
#define NUM_MODS      6
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Drive",
   "Exp",
   "Exp2",
   "DrivePost",
   "DriveDry",
};

typedef struct ws_sin_exp_info_s {
   st_plugin_info_t base;
} ws_sin_exp_info_t;

typedef struct ws_sin_exp_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ws_sin_exp_shared_t;

typedef struct ws_sin_exp_voice_s {
   st_plugin_voice_t base;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_drive_cur;
   float mod_drive_inc;
   float mod_exp_cur;
   float mod_exp_inc;
   float mod_exp2_cur;
   float mod_exp2_inc;
   float mod_drivepost_cur;
   float mod_drivepost_inc;
   float mod_drivedry_cur;
   float mod_drivedry_inc;
} ws_sin_exp_voice_t;


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
   ST_PLUGIN_SHARED_CAST(ws_sin_exp_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ws_sin_exp_shared_t);
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
   ST_PLUGIN_VOICE_CAST(ws_sin_exp_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_sin_exp_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_sin_exp_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_sin_exp_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modDrive = shared->params[PARAM_DRIVE] + voice->mods[MOD_DRIVE];
   modDrive = powf(10.0f, modDrive * 4.0f - 2.0f);

   float modExp = shared->params[PARAM_EXP] + voice->mods[MOD_EXP];
   modExp = Dstplugin_clamp(modExp, 0.0f, 1.0f);
   modExp = 1.0f - modExp;
   modExp *= modExp;
   modExp *= modExp;
   modExp = 1.0f - modExp;
   modExp = (modExp * 2.0f) - 1.0f;
   modExp *= 0.49999f;
   // modExp *= 1.5f;
   if(modExp < 0.0001f)
      modExp = 0.0001f;

   float modExp2 = shared->params[PARAM_EXP2] + voice->mods[MOD_EXP2];
   modExp2 *= 1.5f;
   if(modExp2 < 0.0001f)
      modExp2 = 0.0001f;

   float modDrivePost = shared->params[PARAM_DRIVEPOST] + voice->mods[MOD_DRIVEPOST];
   modDrivePost = powf(10.0f, modDrivePost * 8.0f - 4.0f);

   float modDriveDry = shared->params[PARAM_DRIVEDRY] + voice->mods[MOD_DRIVEDRY];
   modDriveDry = powf(10.0f, modDriveDry * 4.0f - 2.0f);


   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc    = (modDryWet    - voice->mod_drywet_cur)    * recBlockSize;
      voice->mod_drive_inc     = (modDrive     - voice->mod_drive_cur)     * recBlockSize;
      voice->mod_exp_inc       = (modExp       - voice->mod_exp_cur)       * recBlockSize;
      voice->mod_exp2_inc      = (modExp2      - voice->mod_exp2_cur)      * recBlockSize;
      voice->mod_drivepost_inc = (modDrivePost - voice->mod_drivepost_cur) * recBlockSize;
      voice->mod_drivedry_inc  = (modDriveDry  - voice->mod_drivedry_cur)  * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur    = modDryWet;
      voice->mod_drywet_inc    = 0.0f;
      voice->mod_drive_cur     = modDrive;
      voice->mod_drive_inc     = 0.0f;
      voice->mod_exp_cur       = modExp;
      voice->mod_exp_inc       = 0.0f;
      voice->mod_exp2_cur      = modExp2;
      voice->mod_exp2_inc      = 0.0f;
      voice->mod_drivepost_cur = modDrivePost;
      voice->mod_drivepost_inc = 0.0f;
      voice->mod_drivedry_cur  = modDriveDry;
      voice->mod_drivedry_inc  = 0.0f;
   }
}

static inline float loc_shape(float _t, float _exp, float _exp2, float _drivePost) {
   float norm = powf(sinf(_exp * ST_PLUGIN_2PI_F), _exp2) / powf(2.0f, _exp);
   if(norm != 0.0f)
      norm = _drivePost / norm;
   else
      norm = 999999.0f;
   float t;
   if(_t < 0.0f)
   {
      t = -_t;
      norm = -norm;
   }
   else
      t = _t;
   float r = norm*t*powf(sinf(_exp*t*ST_PLUGIN_2PI_F), _exp2) / powf(t+1.0f, _exp);
   int i = (int)(r * 32767);
   if(i > 32767)
      i = 32767;
   else if(i < -32767)
      i = -32767;
   r = i / 32767.0f;
   return r;
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   ST_PLUGIN_VOICE_CAST(ws_sin_exp_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_sin_exp_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float a = loc_shape(l * voice->mod_drive_cur, voice->mod_exp_cur, voice->mod_exp2_cur, voice->mod_drivepost_cur);
         float ld = l * voice->mod_drivedry_cur;
         float out = ld + (a - ld) * voice->mod_drywet_cur;
         out = l + (out - l) * voice->mod_drywet_cur;
         out = Dstplugin_fix_denorm_32(out);
         _samplesOut[k]      = out;
         _samplesOut[k + 1u] = out;
         // Next frame
         k += 2u;
         voice->mod_drywet_cur    += voice->mod_drywet_inc;
         voice->mod_drive_cur     += voice->mod_drive_inc;
         voice->mod_exp_cur       += voice->mod_exp_inc;
         voice->mod_exp2_cur      += voice->mod_exp2_inc;
         voice->mod_drivepost_cur += voice->mod_drivepost_inc;
         voice->mod_drivedry_cur  += voice->mod_drivedry_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float r = _samplesIn[k + 1u];
         float outL = loc_shape(l * voice->mod_drive_cur, voice->mod_exp_cur, voice->mod_exp2_cur, voice->mod_drivepost_cur);
         float outR = loc_shape(r * voice->mod_drive_cur, voice->mod_exp_cur, voice->mod_exp2_cur, voice->mod_drivepost_cur);
         float ld = l * voice->mod_drivedry_cur;
         float rd = r * voice->mod_drivedry_cur;
         outL = ld + (outL - ld) * voice->mod_drywet_cur;
         outR = rd + (outR - rd) * voice->mod_drywet_cur;
         outL = l + (outL - l) * voice->mod_drywet_cur;
         outR = r + (outR - r) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);
         outR = Dstplugin_fix_denorm_32(outR);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;
         // Next frame
         k += 2u;
         voice->mod_drywet_cur    += voice->mod_drywet_inc;
         voice->mod_drive_cur     += voice->mod_drive_inc;
         voice->mod_exp_cur       += voice->mod_exp_inc;
         voice->mod_exp2_cur      += voice->mod_exp2_inc;
         voice->mod_drivepost_cur += voice->mod_drivepost_inc;
         voice->mod_drivedry_cur  += voice->mod_drivedry_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ws_sin_exp_shared_t *ret = malloc(sizeof(ws_sin_exp_shared_t));
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
   ws_sin_exp_voice_t *ret = malloc(sizeof(ws_sin_exp_voice_t));
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

st_plugin_info_t *ws_sin_exp_init(void) {
   ws_sin_exp_info_t *ret = NULL;

   ret = malloc(sizeof(ws_sin_exp_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ws sin exp";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "sin exp shaper";
      ret->base.short_name  = "sin exp";
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
