// ----
// ---- file   : je_wavefolder.c
// ---- author : Julien Eres (adapted for stfx by bsp)
// ---- legal  : Copyright (c) 2017 Julien Eres
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// ----
// ---- info   : simple wave folder
// ----           see <http://smc2017.aalto.fi/media/materials/proceedings/SMC17_p336.pdf>
// ----
// ---- created: 07Jun2020
// ---- changed: 08Jun2020
// ----
// ----
// ----


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cmath>
#include <limits>

#include "../../../plugin.h"

#include "constants.h"
#include "meta.h"
#include "ext/LambertW/LambertW.h"

#define PARAM_DRYWET       0
#define PARAM_DRIVE1       1
#define PARAM_OFFSET       2
#define PARAM_R            3
#define PARAM_RI           4
#define PARAM_DRIVE2       5
#define NUM_PARAMS         6
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Drive",
   "Offset",
   "R",
   "RI",
   "Out Drive",
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.4f,  // DRIVE1
   0.5f,  // OFFSET
   0.7f,  // R
   0.2f,  // RI
   0.79f, // DRIVE2
};

#define MOD_DRYWET  0
#define MOD_DRIVE1  1
#define MOD_OFFSET  2
#define MOD_R       3
#define MOD_RI      4
#define MOD_DRIVE2  5
#define NUM_MODS    6
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Drive",
   "Offset",
   "R",
   "RI",
   "Out Drive",
};

typedef struct je_wavefolder_info_s {
   st_plugin_info_t base;
   unsigned int cfg_outmode;
} je_wavefolder_info_t;

typedef struct je_wavefolder_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} je_wavefolder_shared_t;

struct je_wavefolder_state_t {

private:
	const float m_thermalVoltage = 0.026f;
	const float m_saturationCurrent = 10e-17f;
	float m_resistor = 15000.f;
	float m_loadResistor = 7500.f;
	float m_loadResistor2 = m_loadResistor * 2.f;
	// Derived values
	float m_alpha = m_loadResistor2 / m_resistor;
	float m_beta = (m_resistor + m_loadResistor2) / (m_thermalVoltage * m_resistor);
	float m_delta = (m_loadResistor * m_saturationCurrent) / m_thermalVoltage;

public:

   void reset(void) {
      m_resistor = 15000.0f;
      m_loadResistor = 7500.0f;
      m_loadResistor2 = m_loadResistor * 2.f;
      m_alpha = m_loadResistor2 / m_resistor;
      m_beta = (m_resistor + m_loadResistor2) / (m_thermalVoltage * m_resistor);
      m_delta = (m_loadResistor * m_saturationCurrent) / m_thermalVoltage;
   }

   inline void updateResistors(const float _r, const float _ri) {
      if (meta::updateIfDifferent(m_resistor, _r))
      {
         m_alpha = m_loadResistor2 / m_resistor;
         m_beta = (m_resistor + m_loadResistor2) / (m_thermalVoltage * m_resistor);
      }

      if (meta::updateIfDifferent(m_loadResistor, _ri))
      {
         m_loadResistor2 = m_loadResistor * 2.f;
         m_alpha = m_loadResistor2 / m_resistor;
         m_beta = (m_resistor + m_loadResistor2) / (m_thermalVoltage * m_resistor);
         m_delta = (m_loadResistor * m_saturationCurrent) / m_thermalVoltage;
      }
   }

   // inline float getGainedOffsetedInputValue(const float _inSmp) {
	// 	return (_inSmp * meta::clamp(_inLvl, 0.f, 1.f)) +
   //       (_offset) / 2.f;
	// }

   inline float waveFolder(const float in) {
      const float theta = (in >= 0.f) ? 1.f : -1.f;
      return (float)(theta * m_thermalVoltage * utl::LambertW<0>(m_delta * meta::exp(theta * m_beta * in)) - m_alpha * in);
   }

   inline float process(const float _inSmp,
                        const float _inLvl,
                        const float _offset,
                        const float _r,
                        const float _ri,
                        const float _outLvl
                       ) {

      updateResistors(_r, _ri);
      
      float inSmp = _inSmp * _inLvl + _offset*0.5f;
		float out = (float)(tanh(waveFolder(inSmp)) * _outLvl);

#if 0
      {
         static int xxx = 0;
         if(0 == (++xxx & 1023))
         {
            // printf("xxx out=%f alpha=%f beta=%f delta=%f m_thermalVoltage=%f\n", out, m_alpha, m_beta, m_delta, m_thermalVoltage);
            printf("xxx _r=%f _ri=%f\n", _r, _ri);
            fflush(stdout);
         }
      }
#endif

      return out;
   }
};

typedef struct je_wavefolder_voice_s {
   st_plugin_voice_t base;
   float                 sample_rate;
   float                 sample_rate_inv;
   float                 mods[NUM_MODS];
   float                 mod_drywet_cur;
   float                 mod_drywet_inc;
   float                 mod_drive1_cur;
   float                 mod_drive1_inc;
   float                 mod_offset_cur;
   float                 mod_offset_inc;
   float                 mod_r_cur;
   float                 mod_r_inc;
   float                 mod_ri_cur;
   float                 mod_ri_inc;
   float                 mod_drive2_cur;
   float                 mod_drive2_inc;
   je_wavefolder_state_t wf_l;
   je_wavefolder_state_t wf_r;
} je_wavefolder_voice_t;


static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(je_wavefolder_voice_t);
   voice->sample_rate = _sampleRate;
   voice->sample_rate_inv = 1.0f / _sampleRate;
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
   ST_PLUGIN_SHARED_CAST(je_wavefolder_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(je_wavefolder_shared_t);
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
   ST_PLUGIN_VOICE_CAST(je_wavefolder_voice_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));

      voice->wf_l.reset();
      voice->wf_r.reset();
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(je_wavefolder_voice_t);
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
   ST_PLUGIN_VOICE_CAST(je_wavefolder_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(je_wavefolder_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET] + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modDrive1 = powf(10.0f, ((shared->params[PARAM_DRIVE1]-0.5f)*2.0f + voice->mods[MOD_DRIVE1])*3.0f);

   float modOffset = (shared->params[PARAM_OFFSET]-0.5f)*2.0f + voice->mods[MOD_OFFSET];

   float modR = shared->params[PARAM_R] + voice->mods[MOD_R];
   modR = Dstplugin_clamp(modR, 0.0f, 1.0f);
   modR = Dstplugin_val_to_range(modR, 10000.0f, 100000.0f);

   float modRI = shared->params[PARAM_RI] + voice->mods[MOD_RI];
   modRI = Dstplugin_clamp(modRI, 0.0f, 1.0f);
   modRI = Dstplugin_val_to_range(modRI, 1000.0f, 10000.0f);

   float modDrive2 = powf(10.0f, ((shared->params[PARAM_DRIVE2]-0.5f)*2.0f + voice->mods[MOD_DRIVE2])*3.0f);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_drive1_inc = (modDrive1 - voice->mod_drive1_cur) * recBlockSize;
      voice->mod_offset_inc = (modOffset - voice->mod_offset_cur) * recBlockSize;
      voice->mod_r_inc      = (modR      - voice->mod_r_cur)      * recBlockSize;
      voice->mod_ri_inc     = (modRI     - voice->mod_ri_cur)     * recBlockSize;
      voice->mod_drive2_inc = (modDrive2 - voice->mod_drive2_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_inc = modDryWet;
      voice->mod_drywet_cur = 0.0f;
      voice->mod_drive1_cur = modDrive1;
      voice->mod_drive1_inc = 0.0f;
      voice->mod_offset_cur = modOffset;
      voice->mod_offset_inc = 0.0f;
      voice->mod_r_cur      = modR;
      voice->mod_r_inc      = 0.0f;
      voice->mod_ri_cur     = modRI;
      voice->mod_ri_inc     = 0.0f;
      voice->mod_drive2_cur = modDrive2;
      voice->mod_drive2_inc = 0.0f;
   }

}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   ST_PLUGIN_VOICE_CAST(je_wavefolder_voice_t);
   ST_PLUGIN_VOICE_INFO_CAST(je_wavefolder_info_t);
   ST_PLUGIN_VOICE_SHARED_CAST(je_wavefolder_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];

         float outL = voice->wf_l.process(l,
                                          voice->mod_drive1_cur,
                                          voice->mod_offset_cur,
                                          voice->mod_r_cur,
                                          voice->mod_ri_cur,
                                          voice->mod_drive2_cur
                                          );

         outL = l + (outL - l) * voice->mod_drywet_cur;
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outL;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_drive1_cur += voice->mod_drive1_inc;
         voice->mod_offset_cur += voice->mod_offset_inc;
         voice->mod_r_cur      += voice->mod_r_inc;
         voice->mod_ri_cur     += voice->mod_ri_inc;
         voice->mod_drive2_cur += voice->mod_drive2_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float r = _samplesIn[k + 1u];

         float outL = voice->wf_l.process(l,
                                          voice->mod_drive1_cur,
                                          voice->mod_offset_cur,
                                          voice->mod_r_cur,
                                          voice->mod_ri_cur,
                                          voice->mod_drive2_cur
                                          );

         float outR = voice->wf_r.process(r,
                                          voice->mod_drive1_cur,
                                          voice->mod_offset_cur,
                                          voice->mod_r_cur,
                                          voice->mod_ri_cur,
                                          voice->mod_drive2_cur
                                          );

         outL = l + (outL - l) * voice->mod_drywet_cur;
         outR = r + (outR - r) * voice->mod_drywet_cur;
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur += voice->mod_drywet_inc;
         voice->mod_drive1_cur += voice->mod_drive1_inc;
         voice->mod_offset_cur += voice->mod_offset_inc;
         voice->mod_r_cur      += voice->mod_r_inc;
         voice->mod_ri_cur     += voice->mod_ri_inc;
         voice->mod_drive2_cur += voice->mod_drive2_inc;
      }
   }

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   je_wavefolder_shared_t *ret = (je_wavefolder_shared_t *)malloc(sizeof(je_wavefolder_shared_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info  = _info;
      memcpy((void*)ret->params, (void*)loc_param_resets, NUM_PARAMS * sizeof(float));
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_shared_delete(st_plugin_shared_t *_shared) {
   free((void*)_shared);
}

static st_plugin_voice_t *ST_PLUGIN_API loc_voice_new(st_plugin_info_t *_info) {
   je_wavefolder_voice_t *ret = (je_wavefolder_voice_t *)malloc(sizeof(je_wavefolder_voice_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info = _info;
      new(&ret->wf_l)je_wavefolder_state_t();
      new(&ret->wf_r)je_wavefolder_state_t();
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
st_plugin_info_t *je_wavefolder_init(void) {
   je_wavefolder_info_t *ret = NULL;

   ret = (je_wavefolder_info_t *)malloc(sizeof(je_wavefolder_info_t));

   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "je wave folder";  // unique id. don't change this in future builds.
      ret->base.author      = "Julien Eres & bsp";
      ret->base.name        = "je wave folder";
      ret->base.short_name  = "je wave folder";
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
      ret->base.set_sample_rate  = &loc_set_sample_rate;
      ret->base.note_on          = &loc_note_on;
      ret->base.set_mod_value    = &loc_set_mod_value;
      ret->base.prepare_block    = &loc_prepare_block;
      ret->base.process_replace  = &loc_process_replace;
      ret->base.plugin_exit      = &loc_plugin_exit;
   }

   return &ret->base;
}

ST_PLUGIN_APICALL st_plugin_info_t *ST_PLUGIN_API st_plugin_init(unsigned int _pluginIdx) {
   switch(_pluginIdx)
   {
      case 0u:
         return je_wavefolder_init();
   }
   return NULL;
}
} // extern "C"
