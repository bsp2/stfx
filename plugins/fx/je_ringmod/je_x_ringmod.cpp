// ----
// ---- file   : je_x_ringmod.c
// ---- author : Julien Eres (adapted for stfx by bsp)
// ---- legal  : Copyright (c) 2017 Julien Eres
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// ----
// ---- info   : ring modulator (voicebus crossmod version)
// ----
// ---- created: 06Jun2020
// ---- changed: 08Jun2020, 09Jun2020, 19Jan2024, 21Jan2024, 19Sep2024
// ----
// ----
// ----


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits>

#include "../../../plugin.h"

#include "constants.h"
#include "meta.h"
#include "diode.h"


#define PARAM_DRYWET       0
#define PARAM_VOICEBUS     1
#define PARAM_INPUT_LVL    2
#define PARAM_CARRIER_LVL  3
#define PARAM_OFFSET       4
#define PARAM_VB           5
#define PARAM_VLVB         6
#define PARAM_CONFIG       7   // >=0.5: same as <0.5 but with swapped carrier/modulator
#define NUM_PARAMS         8
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Voice Bus",
   "Input Lvl",
   "Carrier Lvl",
   "Offset",
   "VB",
   "VL-VB",
   "Configuration"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.0f,  // VOICEBUS
   0.5f,  // INPUT_LVL
   0.5f,  // CARRIER_LVL
   0.5f,  // OFFSET
   0.10f, // VB
   0.14f, // VLVB
   0.5f,  // CONFIG
};

#define MOD_DRYWET    0
#define MOD_VOICEBUS  1
#define MOD_LVL1      2
#define MOD_LVL2      3
#define MOD_OFFSET    4
#define MOD_VB        5
#define MOD_VLVB      6
#define NUM_MODS      7
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Voice Bus",
   "Level 1",
   "Level 2",
   "Offset",
   "VB",
   "VL-VB",
};

typedef struct je_x_ringmod_info_s {
   st_plugin_info_t base;
   unsigned int cfg_outmode;
} je_x_ringmod_info_t;

typedef struct je_x_ringmod_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} je_x_ringmod_shared_t;

struct je_x_ringmod_state_t {

	Diode m_diode;

   void reset(void) {
   }

	inline void updateDiodeCharacteristics(const float _vb, const float _vlMinusVb, const float _hSlope)
	{
		m_diode.setVb(_vb);
		m_diode.setVlMinusVb(_vlMinusVb);
		m_diode.setH(_hSlope);
	}

	inline float getLeveledPolarizedInputValue(const float _smp, const float _lvl, const float _polarity)
	{
		const float inputPolarity = _polarity;
		const float inputValue = _smp * _lvl;

		if (inputPolarity < 0.5f)
			return (inputValue < 0.0f) ? -m_diode(inputValue) : 0.f;
		else if (inputPolarity > 1.5f)
			return (inputValue > 0.0f) ? m_diode(inputValue) : 0.f;
		return inputValue;
	}

   inline float process(const float _in1Smp,
                        const float _in1Lvl,
                        const float _in1Polarity,
                        const float _in2Smp,
                        const float _in2Lvl,
                        const float _in2Polarity,
                        const float _offset,
                        const unsigned char _outMode
                       ) {

		const float vhin = getLeveledPolarizedInputValue(_in1Smp, _in1Lvl, _in1Polarity) * 0.5f;
		const float vc   = getLeveledPolarizedInputValue(_in2Smp, _in2Lvl, _in2Polarity) + _offset;

		const float vc_plus_vhin = vc + vhin;
		const float vc_minus_vhin = vc - vhin;

		const float outSum  = m_diode(vc_plus_vhin);
		const float outDiff = m_diode(vc_minus_vhin);

		float out;
      switch(_outMode & 7u/*compiler hint*/)
      {
         case 0u:  // ring
         default:
            out = outSum - outDiff;
            break;

         case 1u:  // sum
            out = outSum;
            break;

         case 2u:  // diff
            out = outDiff;
            break;

         case 3u:  // min
            out = outSum < outDiff ? outSum : outDiff;
            break;

         case 4u:  // max
            out = outSum > outDiff ? outSum : outDiff;
            break;
      }
      return out;
   }
};

typedef struct je_x_ringmod_voice_s {
   st_plugin_voice_t base;
   float                sample_rate;
   float                sample_rate_inv;
   float                mods[NUM_MODS];
   float                mod_drywet_cur;
   float                mod_drywet_inc;
   unsigned int         mod_voicebus_idx;
   float                mod_lvl1_cur;
   float                mod_lvl1_inc;
   float                mod_lvl2_cur;
   float                mod_lvl2_inc;
   float                mod_offset_cur;
   float                mod_offset_inc;
   unsigned int         cfg_polarity_1;  // 0..2
   unsigned int         cfg_polarity_2;  // 0..2
   unsigned int         cfg_wave;        // 0..3 (0=sin,1=tri,2=saw,3=rect)
   unsigned int         cfg_chorder;     // 1=swap mod+carrier
   je_x_ringmod_state_t rm_l;
   je_x_ringmod_state_t rm_r;
} je_x_ringmod_voice_t;


static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(je_x_ringmod_voice_t);
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
   ST_PLUGIN_SHARED_CAST(je_x_ringmod_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(je_x_ringmod_shared_t);
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
   ST_PLUGIN_VOICE_CAST(je_x_ringmod_voice_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));

      voice->rm_l.reset();
      voice->rm_r.reset();
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(je_x_ringmod_voice_t);
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
   ST_PLUGIN_VOICE_CAST(je_x_ringmod_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(je_x_ringmod_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET] + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modVoiceBus = shared->params[PARAM_VOICEBUS] + voice->mods[MOD_VOICEBUS];
   Dstplugin_voicebus_f(voice->mod_voicebus_idx, modVoiceBus);

   float modLvl1 = powf(10.0f, ((shared->params[PARAM_INPUT_LVL]-0.5f)*2.0f + voice->mods[MOD_LVL1])*3.0f);
   float modLvl2 = powf(10.0f, ((shared->params[PARAM_CARRIER_LVL]-0.5f)*2.0f + voice->mods[MOD_LVL2])*3.0f);

   float modVb = shared->params[PARAM_VB] + voice->mods[MOD_VB];
   modVb = Dstplugin_clamp(modVb, 0.0f, 1.0f);
   modVb = Dstplugin_scale(modVb, std::numeric_limits<float>::epsilon(), 1.0f);//g_controlPeakVoltage);

   float modVlVb = shared->params[PARAM_VLVB] + voice->mods[MOD_VLVB];
   modVlVb = Dstplugin_clamp(modVlVb, 0.0f, 1.0f);
   modVlVb = Dstplugin_scale(modVlVb, std::numeric_limits<float>::epsilon(), g_controlPeakVoltage);

   float modOffset = (shared->params[PARAM_OFFSET]-0.5f)*2.0f + voice->mods[MOD_OFFSET];

   float modSlope = 0.8f;//shared->params[PARAM_SLOPE];
   
   unsigned int modConfig = (unsigned int)(shared->params[PARAM_CONFIG] * (4/*waves*/ * (3*3)/*polarities*/ * 2/*chorder*/) + 0.5f);
   if(modConfig >= 72u)
      modConfig = 71u;
   voice->cfg_polarity_1 = ((modConfig+1u) % 3u);
   voice->cfg_polarity_2 = (((modConfig/3u)+1u) % 3u);
   voice->cfg_wave       = ((modConfig/(3u*3u)) % 4u);
   voice->cfg_chorder    = modConfig / (4u * (3u*3u));

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_lvl1_inc   = (modLvl1   - voice->mod_lvl1_cur)   * recBlockSize;
      voice->mod_lvl2_inc   = (modLvl2   - voice->mod_lvl2_cur)   * recBlockSize;
      voice->mod_offset_inc = (modOffset - voice->mod_offset_cur) * recBlockSize;

      voice->rm_l.updateDiodeCharacteristics(modVb, modVlVb, modSlope);
      voice->rm_r.updateDiodeCharacteristics(modVb, modVlVb, modSlope);
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_inc = modDryWet;
      voice->mod_drywet_cur = 0.0f;
      voice->mod_lvl1_cur   = modLvl1;
      voice->mod_lvl1_inc   = 0.0f;
      voice->mod_lvl2_cur   = modLvl2;
      voice->mod_lvl2_inc   = 0.0f;
      voice->mod_offset_cur = modOffset;
      voice->mod_offset_inc = 0.0f;
   }

}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   ST_PLUGIN_VOICE_CAST(je_x_ringmod_voice_t);
   ST_PLUGIN_VOICE_INFO_CAST(je_x_ringmod_info_t);
   ST_PLUGIN_VOICE_SHARED_CAST(je_x_ringmod_shared_t);

   // (note) always a valid ptr (points to "0"-filled buffer if layer does not exist)
   const float *samplesBus = voice->base.voice_bus_buffers[voice->mod_voicebus_idx] + voice->base.voice_bus_read_offset;

   unsigned int k = 0u;

   // Stereo input, stereo output
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      float l = _samplesIn[k];
      float r = _samplesIn[k + 1u];

      float oscValL = samplesBus[k];
      float oscValR = samplesBus[k + 1u];

      float in1Smp;
      float in1Lvl;
      float in1Polarity;

      float in2Smp;
      float in2Lvl;
      float in2Polarity;

      in1Lvl = voice->mod_lvl1_cur;
      in1Polarity = float(voice->cfg_polarity_1);

      in2Lvl = voice->mod_lvl2_cur;
      in2Polarity = float(voice->cfg_polarity_2);

      if(0u == voice->cfg_chorder)
      {
         in1Smp = l;
         in2Smp = oscValL;
      }
      else
      {
         in1Smp = oscValL;
         in2Smp = l;
      }
      float outL = voice->rm_l.process(in1Smp, in1Lvl, in1Polarity,
                                       in2Smp, in2Lvl, in2Polarity,
                                       voice->mod_offset_cur,
                                       info->cfg_outmode
                                       );

      if(0u == voice->cfg_chorder)
      {
         in1Smp = r;
         in2Smp = oscValR;
      }
      else
      {
         in1Smp = oscValR;
         in2Smp = r;
      }
      float outR = voice->rm_r.process(in1Smp, in1Lvl, in1Polarity,
                                       in2Smp, in2Lvl, in2Polarity,
                                       voice->mod_offset_cur,
                                       info->cfg_outmode
                                       );

      outL = l + (outL - l) * voice->mod_drywet_cur;
      outR = r + (outR - r) * voice->mod_drywet_cur;
      _samplesOut[k]      = outL;
      _samplesOut[k + 1u] = outR;

      // Next frame
      k += 2u;
      voice->mod_drywet_cur += voice->mod_drywet_inc;
      voice->mod_lvl1_cur   += voice->mod_lvl1_inc;
      voice->mod_lvl2_cur   += voice->mod_lvl2_inc;
      voice->mod_offset_cur += voice->mod_offset_inc;
   }

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   je_x_ringmod_shared_t *ret = (je_x_ringmod_shared_t *)malloc(sizeof(je_x_ringmod_shared_t));
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

static st_plugin_voice_t *ST_PLUGIN_API loc_voice_new(st_plugin_info_t *_info, unsigned int _voiceIdx) {
   (void)_voiceIdx;
   je_x_ringmod_voice_t *ret = (je_x_ringmod_voice_t *)malloc(sizeof(je_x_ringmod_voice_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info = _info;
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
st_plugin_info_t *je_x_ringmod_init(unsigned int  _outputMode,
                                  const char   *_id,
                                  const char   *_name
                                  ) {
   je_x_ringmod_info_t *ret = NULL;

   ret = (je_x_ringmod_info_t *)malloc(sizeof(je_x_ringmod_info_t));

   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));

      ret->cfg_outmode = _outputMode;

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = _id;  // unique id. don't change this in future builds.
      ret->base.author      = "Julien Eres & bsp";
      ret->base.name        = _name;
      ret->base.short_name  = _id;
      ret->base.flags       = ST_PLUGIN_FLAG_FX | ST_PLUGIN_FLAG_XMOD | ST_PLUGIN_FLAG_TRUE_STEREO_OUT;
      ret->base.category    = ST_PLUGIN_CAT_RINGMOD;
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

} // extern "C"
