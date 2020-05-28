// ----
// ---- file   : ringmod.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : a ring modulator that tracks the current voice pitch (tksampler voice plugin FX example)
// ----
// ---- created: 16May2020
// ---- changed: 17May2020, 18May2020, 19May2020, 20May2020, 21May2020, 22May2020
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET     0
#define PARAM_FREQ_MUL   1
#define PARAM_FREQ_MIN   2
#define PARAM_FREQ_MAX   3
#define NUM_PARAMS       4
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Freq Mul",
   "Freq Min",
   "Freq Max"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // FREQ_MUL
   0.0f,  // FREQ_MIN
   1.0f,  // FREQ_MAX
};


#define MOD_DRYWET    0
#define MOD_FREQ_MUL  1
#define NUM_MODS      2
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Freq Mul"
};

typedef struct ringmod_info_s {
   st_plugin_info_t base;
} ringmod_info_t;

typedef struct ringmod_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ringmod_shared_t;

typedef struct ringmod_voice_s {
   st_plugin_voice_t base;
   float phase;  // wrap-around in 0..1 range
   float sample_rate;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_freq_cur;
   float mod_freq_inc;
} ringmod_voice_t;


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
   ST_PLUGIN_VOICE_CAST(ringmod_voice_t);
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
   ST_PLUGIN_SHARED_CAST(ringmod_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ringmod_shared_t);
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
   ST_PLUGIN_VOICE_CAST(ringmod_voice_t);
   (void)_bGlide;
   (void)_voiceIdx;
   (void)_activeNoteIdx;
   (void)_note;
   (void)_noteHz;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned int       _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(ringmod_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ringmod_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ringmod_shared_t);
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modFreq   = shared->params[PARAM_FREQ_MUL] + voice->mods[MOD_FREQ_MUL];
   modFreq = loc_bipolar_to_scale(modFreq, 4.0f, 4.0f);  // 0..1 => 0.25..4.0  (0.5=>1.0)

   float freqMin = shared->params[PARAM_FREQ_MIN];
   freqMin *= freqMin;
   freqMin *= freqMin;
   freqMin = (freqMin * 20000.0f) + 10.0f;

   float freqMax = shared->params[PARAM_FREQ_MAX];
   freqMax *= freqMax;
   freqMax *= freqMax;
   freqMax = (freqMax * 20000.0f) + 10.0f;

   float freqHz = freqMin + ((_freqHz-10.0f) / 20000.0f) * (freqMax - freqMin);
   freqHz *= modFreq;

   float freq = freqHz / voice->sample_rate;

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = 1.0f / _numFrames;
      voice->mod_drywet_inc  = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_freq_inc    = (freq      - voice->mod_freq_cur)   * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      voice->mod_freq_cur   = freq;
      voice->mod_freq_inc   = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_fx_replace(st_plugin_voice_t  *_voice,
                                                 int                 _bMonoIn,
                                                 const float        *_samplesIn,
                                                 float              *_samplesOut, 
                                                 unsigned int        _numFrames
                                                 ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(ringmod_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ringmod_shared_t);

   float phase    = voice->phase;
   unsigned int k = 0u;

   float pi2 = ST_PLUGIN_2PI_F;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float a = sinf(phase * pi2) * 0.5f + 0.5f;
         float out = l * (1.0f + (a - 1.0f) * voice->mod_drywet_cur);
         _samplesOut[k]      = out;
         _samplesOut[k + 1u] = out;
         // Next frame
         k += 2u;
         phase += voice->mod_freq_cur;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_freq_cur   += voice->mod_freq_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float r = _samplesIn[k + 1u];
         float a = sinf(phase * pi2) * 0.5f + 0.5f;
         float outL = l * (1.0f + (a - 1.0f) * voice->mod_drywet_cur);
         float outR = r * (1.0f + (a - 1.0f) * voice->mod_drywet_cur);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;
         // Next frame
         k += 2u;
         phase += voice->mod_freq_cur;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_freq_cur   += voice->mod_freq_inc;
      }
   }
   if(phase >= 1.0f)
      phase -= 1.0f;
   voice->phase = phase;
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ringmod_shared_t *ret = malloc(sizeof(ringmod_shared_t));
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
   ringmod_voice_t *ret = malloc(sizeof(ringmod_voice_t));
   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));
      ret->base.info   = _info;
      ret->phase = 0.0f;
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   free(_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {
   free(_info);
}

st_plugin_info_t *ringmod_init(void) {
   ringmod_info_t *ret = NULL;

   ret = malloc(sizeof(ringmod_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ring mod";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "ring modulator";
      ret->base.short_name  = "ring mod";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
      ret->base.category    = ST_PLUGIN_CAT_RINGMOD;
      ret->base.num_params  = NUM_PARAMS;
      ret->base.num_mods    = NUM_MODS;

      ret->base.shared_new         = &loc_shared_new;
      ret->base.shared_delete      = &loc_shared_delete;
      ret->base.voice_new          = &loc_voice_new;
      ret->base.voice_delete       = &loc_voice_delete;
      ret->base.get_param_name     = &loc_get_param_name;
      ret->base.get_param_reset    = &loc_get_param_reset;
      ret->base.get_param_value    = &loc_get_param_value;
      ret->base.set_param_value    = &loc_set_param_value;
      ret->base.get_mod_name       = &loc_get_mod_name;
      ret->base.set_sample_rate    = &loc_set_sample_rate;
      ret->base.note_on            = &loc_note_on;
      ret->base.set_mod_value      = &loc_set_mod_value;
      ret->base.prepare_block      = &loc_prepare_block;
      ret->base.process_fx_replace = &loc_process_fx_replace;
      ret->base.plugin_exit        = &loc_plugin_exit;
   }

   return &ret->base;
}
