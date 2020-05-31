// ----
// ---- file   : bitflipper.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a bit flipper with configurable resolution
// ----
// ---- created: 26May2020
// ---- changed: 31May2020
// ----
// ----
// ----

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../plugin.h"

#include "biquad.h"

#define PARAM_DRYWET    0
#define PARAM_BITS      1
#define PARAM_MASK      2
#define PARAM_LPF       3
#define PARAM_LPF_Q     4
#define NUM_PARAMS      5
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Bits",
   "Mask",
   "LPF",
   "Q",
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   1.0f,  // BITS
   0.0f,  // MASK
   0.5f,  // LPF
   0.0f,  // Q
};

#define MOD_DRYWET  0
#define MOD_BITS    1
#define MOD_MASK    2
#define MOD_LPF     3
#define NUM_MODS    4
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Bits",
   "Mask",
   "LPF",
};

typedef struct bitflipper_info_s {
   st_plugin_info_t base;
} bitflipper_info_t;

typedef struct bitflipper_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} bitflipper_shared_t;

typedef struct bitflipper_voice_s {
   st_plugin_voice_t base;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_bits_cur;
   float mod_bits_inc;
   float mod_mask_cur;
   float mod_mask_inc;
   StBiquad lpf_l_1;
   StBiquad lpf_l_2;
   StBiquad lpf_r_1;
   StBiquad lpf_r_2;
} bitflipper_voice_t;


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
   ST_PLUGIN_SHARED_CAST(bitflipper_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(bitflipper_shared_t);
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
   ST_PLUGIN_VOICE_CAST(bitflipper_voice_t);
   (void)_bGlide;
   (void)_voiceIdx;
   (void)_activeNoteIdx;
   (void)_note;
   (void)_noteHz;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
      voice->lpf_l_1.reset();
      voice->lpf_l_2.reset();
      voice->lpf_r_1.reset();
      voice->lpf_r_2.reset();
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned int       _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(bitflipper_voice_t);
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
   ST_PLUGIN_VOICE_CAST(bitflipper_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(bitflipper_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modBits = shared->params[PARAM_BITS] + voice->mods[MOD_BITS];
   modBits = Dstplugin_clamp(modBits, 0.0f, 1.0f);
   modBits = float(1 << int(Dstplugin_val_to_range(modBits, 0.0f, 23.0f)));

   float modMask = shared->params[PARAM_MASK] + voice->mods[MOD_MASK];
   modMask = Dstplugin_clamp(modMask, 0.0f, 1.0f);
   modMask = Dstplugin_val_to_range(modMask, 0.0f, modMask * (modBits - 1.0f));

   float modFreq = shared->params[PARAM_LPF] + voice->mods[MOD_LPF];
   modFreq = Dstplugin_clamp(modFreq, 0.0f, 1.0f);
   modFreq = (powf(2.0f, modFreq * 7.0f) - 1.0f) / 127.0f;

   float modQ = shared->params[PARAM_LPF_Q];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_mask_inc   = (modMask   - voice->mod_mask_cur)   * recBlockSize;
      voice->mod_bits_inc   = (modBits   - voice->mod_bits_cur)   * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      voice->mod_mask_cur   = modMask;
      voice->mod_mask_inc   = 0.0f;
      voice->mod_bits_cur   = modBits;
      voice->mod_bits_inc   = 0.0f;
   }

   if(0 == _numFrames)
      _numFrames = 1u;

   voice->lpf_l_1.calcParams(_numFrames,
                             StBiquad::LPF,
                             0.0f/*gainDB*/,
                             modFreq,
                             modQ
                             );

   voice->lpf_l_2.calcParams(_numFrames,
                             StBiquad::LPF,
                             0.0f/*gainDB*/,
                             modFreq,
                             modQ
                             );

   voice->lpf_r_1.calcParams(_numFrames,
                             StBiquad::LPF,
                             0.0f/*gainDB*/,
                             modFreq,
                             modQ
                           );

   voice->lpf_r_2.calcParams(_numFrames,
                             StBiquad::LPF,
                             0.0f/*gainDB*/,
                             modFreq,
                             modQ
                             );
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(bitflipper_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(bitflipper_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         // if(l > 1.0f)
         //    l = 1.0f;
         // else if(l < -1.0f)
         //    l = -1.0f;
         int il = (int)(l * voice->mod_bits_cur);
         if(il >= 0)
         {
            il ^= (int)(voice->mod_mask_cur);
         }
         else
         {
            il = -il;
            il ^= (int)(voice->mod_mask_cur);
            il = il;
         }
         float out = (il / voice->mod_bits_cur);
         out = l + (out - l) * voice->mod_drywet_cur;
         out = voice->lpf_l_1.filter(out);
         out = voice->lpf_l_2.filter(out);

         _samplesOut[k]      = out;
         _samplesOut[k + 1u] = out;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_bits_cur   += voice->mod_bits_inc;
         voice->mod_mask_cur   += voice->mod_mask_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         // if(l > 1.0f)
         //    l = 1.0f;
         // else if(l < -1.0f)
         //    l = -1.0f;
         int il = (int)(l * voice->mod_bits_cur);
         if(il >= 0)
            il ^= (int)(voice->mod_mask_cur);
         else
         {
            il = -il;
            il ^= (int)(voice->mod_mask_cur);
            il = -il;
         }
         float outL = (il / voice->mod_bits_cur);
         outL = voice->lpf_l_1.filter(outL);
         outL = voice->lpf_l_2.filter(outL);

         float r = _samplesIn[k + 1u];
         // if(r > 1.0f)
         //    r = 1.0f;
         // else if(r < -1.0f)
         //    r = -1.0f;
         int ir = (int)(r * voice->mod_bits_cur);
         if(ir >= 0)
            ir ^= (int)(voice->mod_mask_cur);
         else
         {
            ir = -ir;
            ir ^= (int)(voice->mod_mask_cur);
            ir = -ir;
         }
         float outR = (ir / voice->mod_bits_cur);
         outR = voice->lpf_r_1.filter(outR);
         outR = voice->lpf_r_2.filter(outR);

         outL = l + (outL - l) * voice->mod_drywet_cur;
         outR = r + (outR - r) * voice->mod_drywet_cur;

         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_bits_cur  += voice->mod_bits_inc;
         voice->mod_mask_cur  += voice->mod_mask_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   bitflipper_shared_t *ret = (bitflipper_shared_t *)malloc(sizeof(bitflipper_shared_t));
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
   bitflipper_voice_t *ret = (bitflipper_voice_t *)malloc(sizeof(bitflipper_voice_t));
   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));
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
st_plugin_info_t *bitflipper_init(void) {

   bitflipper_info_t *ret = (bitflipper_info_t *)malloc(sizeof(bitflipper_info_t));

   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp bit flipper";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "bit flipper";
      ret->base.short_name  = "bit flipper";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
      ret->base.category    = ST_PLUGIN_CAT_LOFI;
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
}
