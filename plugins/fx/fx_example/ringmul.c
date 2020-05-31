// ----
// ---- file   : ringmul.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a ring multiplier that supports per-sample-frame parameter interpolation
// ----
// ---- created: 31May2020
// ---- changed: 
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET   0
#define PARAM_FREQ     1
#define PARAM_W1       2
#define PARAM_FACTOR   3
#define PARAM_W2       4
#define NUM_PARAMS     5
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Freq",
   "Width 1",
   "Factor",
   "Width 2"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,    // DRYWET
   0.5f,    // FREQ
   0.333f,  // W1
   1.0f,    // FACTOR
   0.333f,  // W2
};

#define MOD_FREQ    0
#define MOD_W1      1
#define MOD_FACTOR  2
#define MOD_W2      3
#define NUM_MODS    4
static const char *loc_mod_names[NUM_MODS] = {
   "Freq",
   "Width 1",
   "Factor",
   "Width 2"
};

typedef struct ringmul_info_s {
   st_plugin_info_t base;
} ringmul_info_t;

typedef struct ringmul_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ringmul_shared_t;

typedef struct ringmul_voice_s {
   st_plugin_voice_t base;
   float sample_rate;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_freq_cur;
   float mod_freq_inc;
   float mod_phase1_cur;
   float mod_phase1_inc;
   float mod_factor_cur;
   float mod_factor_inc;
   float mod_phase2_cur;
   float mod_phase2_inc;
   float phase;
} ringmul_voice_t;

static float loc_bipolar_to_scale(float _t, float _mul, float _div) {
   // t (0..1) => /_div .. *_mul   (0.5 = *1.0)
   if(_t < 0.0f)
      _t = 0.0f;
   else if(_t > 1.0f)
      _t = 1.0f;
   float s;
   if(_t < 0.5f)
   {
      s = (1.0f / _div);
      s = s + ( (1.0f - s) * (_t / 0.5f) );
   }
   else
   {
      s = 1.0f + ((_t - 0.5f) / (0.5f / (_mul - 1.0f)));
   }
   return s;
}

static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(ringmul_voice_t);
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
   ST_PLUGIN_SHARED_CAST(ringmul_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ringmul_shared_t);
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
   ST_PLUGIN_VOICE_CAST(ringmul_voice_t);
   (void)_bGlide;
   (void)_voiceIdx;
   (void)_activeNoteIdx;
   (void)_note;
   (void)_noteHz;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
      voice->phase = 0.0f;
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(ringmul_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ringmul_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ringmul_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modFreq   = shared->params[PARAM_FREQ] + voice->mods[MOD_FREQ];
   modFreq = loc_bipolar_to_scale(modFreq, 4.0f, 4.0f);  // 0..1 => 0.25..4.0  (0.5=>1.0)
   modFreq = ((modFreq * _freqHz) / voice->sample_rate);

   float modW1 = shared->params[PARAM_W1] + voice->mods[MOD_W1];
   if(modW1 < 0.0f)
      modW1 = 0.0f;

   float modFactor = (shared->params[PARAM_FACTOR]-0.5f)*2.0f + voice->mods[MOD_FACTOR];

   float modW2 = shared->params[PARAM_W2] + voice->mods[MOD_W2];
   if(modW2 < 0.0f)
      modW2 = 0.0f;

   if((modW1 + modW2) > 1.0f)
   {
      float s = 1.0f / (modW1 + modW2);
      modW1 *= s;
      modW2 *= s;
   }

   float modPhase1 = modW1;
   float modPhase2 = modPhase1 + modW2;


   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc = (modDryWet - voice->mod_drywet_cur)  * recBlockSize;
      voice->mod_freq_inc   = (modFreq   - voice->mod_freq_cur )   * recBlockSize;
      voice->mod_phase1_inc = (modPhase1 - voice->mod_phase1_cur ) * recBlockSize;
      voice->mod_factor_inc = (modFactor - voice->mod_factor_cur ) * recBlockSize;
      voice->mod_phase2_inc = (modPhase2 - voice->mod_phase2_cur ) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      voice->mod_freq_cur   = modFreq;
      voice->mod_freq_inc   = 0.0f;
      voice->mod_phase1_cur = modPhase1;
      voice->mod_phase1_inc = 0.0f;
      voice->mod_factor_cur = modFactor;
      voice->mod_factor_inc = 0.0f;
      voice->mod_phase2_cur = modPhase2;
      voice->mod_phase2_inc = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(ringmul_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ringmul_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float outL = l;
         if(voice->phase >= voice->mod_phase1_cur)
         {
            if(voice->phase < voice->mod_phase2_cur)
            {
               outL *= voice->mod_factor_cur;
            }
         }
         outL = l + (outL - l) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outL;
         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->phase          += voice->mod_freq_cur;
         if(voice->phase >= 1.0)
            voice->phase -= 1.0;
         voice->mod_freq_cur   += voice->mod_freq_inc;
         voice->mod_phase1_cur += voice->mod_phase1_inc;
         voice->mod_factor_cur += voice->mod_factor_inc;
         voice->mod_phase2_cur += voice->mod_phase2_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float r = _samplesIn[k + 1u];
         float outL = l;
         float outR = r;
         if(voice->phase >= voice->mod_phase1_cur)
         {
            if(voice->phase < voice->mod_phase2_cur)
            {
               outL *= voice->mod_factor_cur;
               outR *= voice->mod_factor_cur;
            }
         }

         outL = l + (outL - l) * voice->mod_drywet_cur;
         outR = r + (outR - r) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);
         outR = Dstplugin_fix_denorm_32(outR);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->phase          += voice->mod_freq_cur;
         if(voice->phase >= 1.0)
            voice->phase -= 1.0;
         voice->mod_freq_cur   += voice->mod_freq_inc;
         voice->mod_phase1_cur += voice->mod_phase1_inc;
         voice->mod_factor_cur += voice->mod_factor_inc;
         voice->mod_phase2_cur += voice->mod_phase2_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ringmul_shared_t *ret = malloc(sizeof(ringmul_shared_t));
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
   ringmul_voice_t *ret = malloc(sizeof(ringmul_voice_t));
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

st_plugin_info_t *ringmul_init(void) {
   ringmul_info_t *ret = NULL;

   ret = malloc(sizeof(ringmul_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ringmul";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "ring multiplier";
      ret->base.short_name  = "ringmul";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
      ret->base.category    = ST_PLUGIN_CAT_RINGMOD;
      ret->base.num_params  = NUM_PARAMS;
      ret->base.num_mods    = NUM_MODS;

      ret->base.shared_new       = &loc_shared_new;
      ret->base.shared_delete    = &loc_shared_delete;
      ret->base.voice_new        = &loc_voice_new;
      ret->base.voice_delete     = &loc_voice_delete;
      ret->base.set_sample_rate  = &loc_set_sample_rate;
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
