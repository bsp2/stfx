// ----
// ---- file   : osc_crossfm_tanh_vsync.cpp
// ---- author : bsp
// ---- legal  : Distributed under terms of the MIT license (https://opensource.org/licenses/MIT)
// ----
// ----          Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
// ----          associated documentation files (the "Software"), to deal in the Software without restriction, including 
// ----          without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
// ----          copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to 
// ----          the following conditions:
// ----  
// ----          The above copyright notice and this permission notice shall be included in all copies or substantial 
// ----          portions of the Software.
// ----
// ----          THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// ----          NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// ----          IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// ----          WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
// ----          SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ----
// ---- info   : auto-generated by "Cycle"
// ----           $ g++ -Wall -Wno-unused-function -Wno-unused-variable -I../../tksampler -c osc_crossfm_tanh_vsync.cpp -o osc_crossfm_tanh_vsync.o
// ---- created: 11Nov2023 18:41:17
// ----
// ----
// ----

#include <plugin.h>

#include <stdlib.h>
#include <math.h>
#include <string.h>


#define OVERSAMPLE_FACTOR  16.0f

#define PARAM_COLOR_1            0
#define PARAM_COLOR_2            1
#define PARAM_VSYNC              2
#define PARAM_RATIO_2            3
#define PARAM_FM_2TO1            4
#define PARAM_FM_1TO2            5
#define PARAM_LEVEL_1            6
#define PARAM_LEVEL_2            7
#define NUM_PARAMS               8
static const char *loc_param_names[NUM_PARAMS] = {
   "color_1",                 // 0: COLOR_1
   "color_2",                 // 1: COLOR_2
   "vsync",                   // 2: VSYNC
   "ratio_2",                 // 3: RATIO_2
   "fm_2to1",                 // 4: FM_2TO1
   "fm_1to2",                 // 5: FM_1TO2
   "level_1",                 // 6: LEVEL_1
   "level_2",                 // 7: LEVEL_2

};
static float loc_param_resets[NUM_PARAMS] = {
   0.5f,                      // 0: COLOR_1
   0.5f,                      // 1: COLOR_2
   0.5f,                      // 2: VSYNC
   0.5f,                      // 3: RATIO_2
   0.0f,                      // 4: FM_2TO1
   0.0f,                      // 5: FM_1TO2
   1.0f,                      // 6: LEVEL_1
   0.0f,                      // 7: LEVEL_2

};

#define MOD_COLOR_1              0
#define MOD_COLOR_2              1
#define MOD_VSYNC_2_OFF          2
#define MOD_RATIO_2              3
#define MOD_FM_2TO1              4
#define MOD_FM_1TO2              5
#define MOD_LEVEL_1              6
#define MOD_LEVEL_2              7
#define NUM_MODS                 8
static const char *loc_mod_names[NUM_MODS] = {
   "color_1",      // 0: COLOR_1
   "color_2",      // 1: COLOR_2
   "vsync_2_off",  // 2: VSYNC_2_OFF
   "ratio_2",      // 3: RATIO_2
   "fm_2to1",      // 4: FM_2TO1
   "fm_1to2",      // 5: FM_1TO2
   "level_1",      // 6: LEVEL_1
   "level_2",      // 7: LEVEL_2

};

typedef struct osc_crossfm_tanh_vsync_info_s {
   st_plugin_info_t base;
} osc_crossfm_tanh_vsync_info_t;

typedef struct osc_crossfm_tanh_vsync_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];

   float lut_tanh[4096];
   int b_lut_tanh;

} osc_crossfm_tanh_vsync_shared_t;

typedef struct osc_crossfm_tanh_vsync_voice_s {
   st_plugin_voice_t base;
   float             sample_rate;
   float             note_speed_fixed;
   float             note_speed_cur;
   float             note_speed_inc;
   float             note_cur;
   float             note_inc;
   float             mods[NUM_MODS];
   float mod_color_1_cur;
   float mod_color_1_inc;
   float mod_color_2_cur;
   float mod_color_2_inc;
   float mod_vsync_2_off_cur;
   float mod_vsync_2_off_inc;
   float mod_ratio_2_cur;
   float mod_ratio_2_inc;
   float mod_fm_2to1_cur;
   float mod_fm_2to1_inc;
   float mod_fm_1to2_cur;
   float mod_fm_1to2_inc;
   float mod_level_1_cur;
   float mod_level_1_inc;
   float mod_level_2_cur;
   float mod_level_2_inc;

   float tmp1_seq;
   float tmp2_seq;
   float tmp3_seq;
   float tmp4_seq;
   float tmp5_exp;
   float tmp6_exp;
   float tmp7_seq;
   float tmp8_sin_phase;
   float tmp9_win_phase;
   float tmp10_sin_speed;
   float tmp11_freq;
   float tmp12_sin_tmp;
   float tmp13_phase;
   float tmp14_seq;
   float tmp15_seq;
   float tmp16_vsync;
   float tmp17_window;
   float tmp18_seq;
   float tmp19_lut_f;
   short tmp20_lut_idx;
   float tmp21_lut_frac;
   float tmp22_lut_a;
   float tmp23_lut_b;
   float tmp24_b;
   float tmp25_amount;
   float tmp26_amount_a;
   float tmp27_amount_b;
   float tmp28_sin_phase;
   float tmp29_win_phase;
   float tmp30_sin_speed;
   float tmp31_freq;
   float tmp32_sin_tmp;
   float tmp33_phase;
   float tmp34_seq;
   float tmp35_seq;
   float tmp36_vsync;
   float tmp37_window;
   float tmp38_seq;
   float tmp39_lut_f;
   short tmp40_lut_idx;
   float tmp41_lut_frac;
   float tmp42_lut_a;
   float tmp43_lut_b;
   float tmp44_b;
   float tmp45_amount;
   float tmp46_amount_a;
   float tmp47_amount_b;
   float tmp48_seq;
   float tmp49_seq;
   float tmp50_seq;
   float var_x;
   float var_v_freq_2;
   float var_v_xfd_1;
   float var_v_xfd_2;
   float var_v_drive_1;
   float var_v_drive_2;
   float var_v_sin;
   float var_v_osc_1;
   float var_v_osc_2;
   float var_v_vsync_1;
   float var_v_vsync_2;

} osc_crossfm_tanh_vsync_voice_t;

#define loop(X)  for(unsigned int i = 0u; i < (X); i++)
#define clamp(a,b,c) (((a)<(b))?(b):(((a)>(c))?(c):(a)))

static inline float mathLerpf(float _a, float _b, float _t) { return _a + (_b - _a) * _t; }
static inline float mathClampf(float a, float b, float c) { return (((a)<(b))?(b):(((a)>(c))?(c):(a))); }
static inline float mathMinf(float a, float b) { return (a<b)?a:b; }
static inline float mathMaxf(float a, float b) { return (a>b)?a:b; }
static inline float mathAbsMaxf(float _x, float _y) { return ( ( (_x<0.0f)?-_x:_x)>((_y<0.0f)?-_y:_y)?_x:_y ); }
static inline float mathAbsMinf(float _x, float _y) { return ( ((_x<0.0f)?-_x:_x)<((_y<0.0f)?-_y:_y)?_x:_y ); }

static inline float winLinear(const float *_s, float _index) {
   int idx = (int)_index;
   float r = _index - (float)idx;
   return mathLerpf(_s[idx], _s[idx+1], r);
}

static float ffrac_s(float _f) { int i; if(_f >= 0.0f) { i = (int)_f; return _f - (float)i; } else { i = (int)-_f; return 1.0f - (-_f - (float)i); } }

#define USE_CYCLE_SINE_TBL  defined
static float cycle_sine_tbl_f[16384];

static float mathLogf(const float _x) {
   union {
      float f;
      unsigned int u;
   } bx;
   bx.f = _x;
   const unsigned int ex = bx.u >> 23;
   const signed int t = (signed int)ex - (signed int)127;
   const unsigned int s = (t < 0) ? (-t) : t;
   bx.u = 1065353216u | (bx.u & 8388607u);
   return
      -1.7417939f + (2.8212026f + (-1.4699568f + (0.44717955f - 0.056570851f * bx.f)*bx.f)*bx.f)*bx.f
      + 0.6931471806f * t;
}

static float mathPowerf(float _x, float _y) {
   float r;
   if(_y != 0.0f)
   {
      if(_x < 0.0f)
      {
         r = (float)( -expf(_y*mathLogf(-_x)) );
      }
      else if(_x > 0.0f)
      {
         r = (float)( expf(_y*mathLogf(_x)) );
      }
      else
      {
         r = 0.0f;
      }
   }
   else
   {
      r = 1.0f;
   }
   return Dstplugin_fix_denorm_32(r);
}

static float loc_bipolar_to_scale(const float _t, const float _div, const float _mul) {
   // t (-1..1) => /_div .. *_mul
   
   float s;

   if(_t < 0.0f)
   {
      s = (1.0f / _div);
      s = 1.0f + (s - 1.0f) * -_t;
      if(s < 0.0f)
         s = 0.0f;
   }
   else
   {
      s = _mul;
      s = 1.0f + (s - 1.0f) * _t;
   }
   
   return s;
}

static void loc_calc_lut_tanh(st_plugin_voice_t *_voice, float *_d);
static void loc_prepare(st_plugin_voice_t *_voice);

void loc_calc_lut_tanh(st_plugin_voice_t *_voice, float *_d) {
   ST_PLUGIN_VOICE_CAST(osc_crossfm_tanh_vsync_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_crossfm_tanh_vsync_shared_t);
   float x = 0.0f;
   loop(4096)
   {
      float out = x;
      // -------- lane "lut_tanh" modIdx=0
      
      // -- mod="fma" dstVar=out
      out = (out * 200.0f) + -100.0f;
      
      // -- mod="tanh" dstVar=out
      out = tanhf(out);
      _d[i] = out;

      // Next x
      x += 0.0002442f;
   } // loop 4096
} /* end loc_calc_lut_tanh() */

void loc_prepare(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_crossfm_tanh_vsync_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_crossfm_tanh_vsync_shared_t);

   float out = 0.0f;
   // -------- lane "prepare" modIdx=0
   
   // -- mod="$m_ratio_2" dstVar=out
   out = voice->mod_ratio_2_cur;
   
   // -- mod="fma" dstVar=out
   out = (out * 2.0f) + -1.0f;
   
   // -- mod="bts" dstVar=out
   out = loc_bipolar_to_scale(out, 4096.0f, 4.0f);
   
   // -- mod="pow" dstVar=out
   out = out * out * out;
   
   // -- mod="qua" dstVar=out
   out = ((int)(out * 100.0f) / 100.0f);
   
   // -- mod="sto v_freq_2" dstVar=out
   voice->var_v_freq_2 = out;
   
   // -- mod="1" dstVar=out
   out = 1.0f;
   
   // -- mod="$m_color_1" dstVar=out
   voice->tmp1_seq = voice->mod_color_1_cur;
   
   // -- mod="clp" dstVar=voice->tmp1_seq
   if(voice->tmp1_seq > 0.25f) voice->tmp1_seq = 0.25f;
   else if(voice->tmp1_seq < 0.0f) voice->tmp1_seq = 0.0f;
   
   // -- mod="4" dstVar=voice->tmp1_seq
   voice->tmp2_seq = 4.0f;
   voice->tmp1_seq *= voice->tmp2_seq;
   out -= voice->tmp1_seq;
   
   // -- mod="sto v_xfd_1" dstVar=out
   voice->var_v_xfd_1 = out;
   
   // -- mod="1" dstVar=out
   out = 1.0f;
   
   // -- mod="$m_color_2" dstVar=out
   voice->tmp3_seq = voice->mod_color_2_cur;
   
   // -- mod="clp" dstVar=voice->tmp3_seq
   if(voice->tmp3_seq > 0.25f) voice->tmp3_seq = 0.25f;
   else if(voice->tmp3_seq < 0.0f) voice->tmp3_seq = 0.0f;
   
   // -- mod="4" dstVar=voice->tmp3_seq
   voice->tmp4_seq = 4.0f;
   voice->tmp3_seq *= voice->tmp4_seq;
   out -= voice->tmp3_seq;
   
   // -- mod="sto v_xfd_2" dstVar=out
   voice->var_v_xfd_2 = out;
   
   // -- mod="10" dstVar=out
   out = 10.0f;
   
   // -- mod="pow" dstVar=out
   
   // ---- mod="pow" input "exp" seq 1/1
   
   // -- mod="$m_color_1" dstVar=voice->tmp5_exp
   voice->tmp5_exp = voice->mod_color_1_cur;
   out = mathPowerf(out, voice->tmp5_exp);
   
   // -- mod="sto v_drive_1" dstVar=out
   voice->var_v_drive_1 = out;
   
   // -- mod="10" dstVar=out
   out = 10.0f;
   
   // -- mod="pow" dstVar=out
   
   // ---- mod="pow" input "exp" seq 1/1
   
   // -- mod="$m_color_2" dstVar=voice->tmp6_exp
   voice->tmp6_exp = voice->mod_color_2_cur;
   out = mathPowerf(out, voice->tmp6_exp);
   
   // -- mod="sto v_drive_2" dstVar=out
   voice->var_v_drive_2 = out;
   
   // -- mod="$p_vsync" dstVar=out
   out = shared->params[PARAM_VSYNC];
   
   // -- mod="fma" dstVar=out
   out = (out * 2.0f) + -1.0f;
   
   // -- mod="bts" dstVar=out
   out = loc_bipolar_to_scale(out, 16.0f, 16.0f);
   
   // -- mod="qua" dstVar=out
   out = ((int)(out * 16.0f) / 16.0f);
   
   // -- mod="sto v_vsync_1" dstVar=out
   voice->var_v_vsync_1 = out;
   
   // -- mod="$p_vsync" dstVar=out
   out = shared->params[PARAM_VSYNC];
   
   // -- mod="$m_vsync_2_off" dstVar=out
   voice->tmp7_seq = voice->mod_vsync_2_off_cur;
   out += voice->tmp7_seq;
   
   // -- mod="fma" dstVar=out
   out = (out * 2.0f) + -1.0f;
   
   // -- mod="bts" dstVar=out
   out = loc_bipolar_to_scale(out, 16.0f, 16.0f);
   
   // -- mod="qua" dstVar=out
   out = ((int)(out * 16.0f) / 16.0f);
   
   // -- mod="sto v_vsync_2" dstVar=out
   voice->var_v_vsync_2 = out;
} /* end prepare */

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
   ST_PLUGIN_SHARED_CAST(osc_crossfm_tanh_vsync_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(osc_crossfm_tanh_vsync_shared_t);
   shared->params[_paramIdx] = _value;
}

static const char *ST_PLUGIN_API loc_get_mod_name(st_plugin_info_t *_info,
                                                  unsigned int      _modIdx
                                                  ) {
   (void)_info;
   return loc_mod_names[_modIdx];
}

static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(osc_crossfm_tanh_vsync_voice_t);
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(osc_crossfm_tanh_vsync_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_crossfm_tanh_vsync_shared_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
#ifdef OVERSAMPLE_FACTOR
      voice->note_speed_fixed = (0.5f / (voice->sample_rate * OVERSAMPLE_FACTOR));
#else
      voice->note_speed_fixed = (0.5f / voice->sample_rate);
#endif // OVERSAMPLE_FACTOR


      voice->tmp8_sin_phase = 0.0f;
      voice->tmp9_win_phase = 0.0f;
      voice->tmp28_sin_phase = 0.0f;
      voice->tmp29_win_phase = 0.0f;

      voice->var_x = 0.0f;
      voice->var_v_freq_2 = 0.0f;
      voice->var_v_xfd_1 = 0.0f;
      voice->var_v_xfd_2 = 0.0f;
      voice->var_v_drive_1 = 0.0f;
      voice->var_v_drive_2 = 0.0f;
      voice->var_v_sin = 0.0f;
      voice->var_v_osc_1 = 0.0f;
      voice->var_v_osc_2 = 0.0f;
      voice->var_v_vsync_1 = 0.0f;
      voice->var_v_vsync_2 = 0.0f;


      if(!shared->b_lut_tanh) { loc_calc_lut_tanh(&voice->base, shared->lut_tanh); shared->b_lut_tanh = 1; }
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(osc_crossfm_tanh_vsync_voice_t);
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
   ST_PLUGIN_VOICE_CAST(osc_crossfm_tanh_vsync_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_crossfm_tanh_vsync_shared_t);
   (void)_note;
   (void)_vol;
   (void)_pan;

#ifdef OVERSAMPLE_FACTOR
   float noteSpeed = _freqHz / (voice->sample_rate * OVERSAMPLE_FACTOR);
#else
   float noteSpeed = _freqHz / voice->sample_rate;
#endif // OVERSAMPLE_FACTOR

   float modcolor_1      = shared->params[PARAM_COLOR_1     ]                       + voice->mods[MOD_COLOR_1      ];
   float modcolor_2      = shared->params[PARAM_COLOR_2     ]                       + voice->mods[MOD_COLOR_2      ];
   float modvsync_2_off  = voice->mods[MOD_VSYNC_2_OFF  ];
   float modratio_2      = shared->params[PARAM_RATIO_2     ]                       + voice->mods[MOD_RATIO_2      ];
   float modfm_2to1      = shared->params[PARAM_FM_2TO1     ]                       + voice->mods[MOD_FM_2TO1      ];
   float modfm_1to2      = shared->params[PARAM_FM_1TO2     ]                       + voice->mods[MOD_FM_1TO2      ];
   float modlevel_1      = shared->params[PARAM_LEVEL_1     ]                       + voice->mods[MOD_LEVEL_1      ];
   float modlevel_2      = shared->params[PARAM_LEVEL_2     ]                       + voice->mods[MOD_LEVEL_2      ];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->note_speed_inc = (noteSpeed - voice->note_speed_cur) * recBlockSize;
      voice->note_inc       = (_note - voice->note_cur) * recBlockSize;
      voice->mod_color_1_inc      = (modcolor_1         - voice->mod_color_1_cur       ) * recBlockSize;
      voice->mod_color_2_inc      = (modcolor_2         - voice->mod_color_2_cur       ) * recBlockSize;
      voice->mod_vsync_2_off_inc  = (modvsync_2_off     - voice->mod_vsync_2_off_cur   ) * recBlockSize;
      voice->mod_ratio_2_inc      = (modratio_2         - voice->mod_ratio_2_cur       ) * recBlockSize;
      voice->mod_fm_2to1_inc      = (modfm_2to1         - voice->mod_fm_2to1_cur       ) * recBlockSize;
      voice->mod_fm_1to2_inc      = (modfm_1to2         - voice->mod_fm_1to2_cur       ) * recBlockSize;
      voice->mod_level_1_inc      = (modlevel_1         - voice->mod_level_1_cur       ) * recBlockSize;
      voice->mod_level_2_inc      = (modlevel_2         - voice->mod_level_2_cur       ) * recBlockSize;

      loc_prepare(&voice->base);
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->note_speed_cur = noteSpeed;
      voice->note_speed_inc = 0.0f;
      voice->note_cur       = _note;
      voice->note_inc       = 0.0f;
      voice->mod_color_1_cur      = modcolor_1;
      voice->mod_color_1_inc      = 0.0f;
      voice->mod_color_2_cur      = modcolor_2;
      voice->mod_color_2_inc      = 0.0f;
      voice->mod_vsync_2_off_cur  = modvsync_2_off;
      voice->mod_vsync_2_off_inc  = 0.0f;
      voice->mod_ratio_2_cur      = modratio_2;
      voice->mod_ratio_2_inc      = 0.0f;
      voice->mod_fm_2to1_cur      = modfm_2to1;
      voice->mod_fm_2to1_inc      = 0.0f;
      voice->mod_fm_1to2_cur      = modfm_1to2;
      voice->mod_fm_1to2_inc      = 0.0f;
      voice->mod_level_1_cur      = modlevel_1;
      voice->mod_level_1_inc      = 0.0f;
      voice->mod_level_2_cur      = modlevel_2;
      voice->mod_level_2_inc      = 0.0f;

      loc_prepare(&voice->base);
   }

   // printf("xxx note_cur=%f\n", voice->note_cur);
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   (void)_bMonoIn;
   (void)_samplesIn;

   ST_PLUGIN_VOICE_CAST(osc_crossfm_tanh_vsync_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_crossfm_tanh_vsync_shared_t);

   // Mono output (replicate left to right channel)
   unsigned int j = 0u;
   unsigned int jStep = _bMonoIn ? 1u : 2u;
   unsigned int k = 0u;
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      float out = _samplesIn[j];
      j += jStep;
#ifdef OVERSAMPLE_FACTOR
      float outOS = 0.0f;
#endif // OVERSAMPLE_FACTOR

#ifdef OVERSAMPLE_FACTOR
      for(unsigned int osi = 0u; osi < OVERSAMPLE_FACTOR; osi++)
#endif // OVERSAMPLE_FACTOR
      {
         float tmp_f;
         float tmp2_f;
         
         // ========
         // ======== lane "out" modIdx=0
         // ========
         
         // -- mod="sin" dstVar=out
         voice->tmp11_freq = 1;
         voice->tmp10_sin_speed = voice->note_speed_cur * voice->tmp11_freq;
         
         // ---- mod="sin" input "phase" seq 1/1
         
         // -- mod="$v_osc_2" dstVar=voice->tmp13_phase
         voice->tmp13_phase = voice->var_v_osc_2;
         
         // -- mod="$m_fm_2to1" dstVar=voice->tmp13_phase
         voice->tmp14_seq = voice->mod_fm_2to1_cur;
         voice->tmp13_phase *= voice->tmp14_seq;
         
         // -- mod="32" dstVar=voice->tmp13_phase
         voice->tmp15_seq = 32.0f;
         voice->tmp13_phase *= voice->tmp15_seq;
         
         // ---- mod="sin" input "vsync" seq 1/1
         
         // -- mod="$v_vsync_1" dstVar=voice->tmp16_vsync
         voice->tmp16_vsync = voice->var_v_vsync_1;
         voice->tmp12_sin_tmp = ((voice->tmp8_sin_phase + voice->tmp13_phase));
         voice->tmp12_sin_tmp = ffrac_s(voice->tmp12_sin_tmp);
         out = cycle_sine_tbl_f[(unsigned short)(16384 * voice->tmp12_sin_tmp)&16383u];
         voice->tmp17_window = cycle_sine_tbl_f[(unsigned short)(16384 * voice->tmp9_win_phase)&16383u];
         voice->tmp17_window *= voice->tmp17_window;
         out *= voice->tmp17_window;
         voice->tmp8_sin_phase = ffrac_s(voice->tmp8_sin_phase + voice->tmp10_sin_speed * voice->tmp16_vsync);
         tmp_f = voice->tmp9_win_phase;
         voice->tmp9_win_phase = ffrac_s(voice->tmp9_win_phase + voice->tmp10_sin_speed);
         if(tmp_f > voice->tmp9_win_phase) voice->tmp8_sin_phase = voice->tmp9_win_phase * voice->tmp16_vsync; 
         
         // -- mod="sto v_sin" dstVar=out
         voice->var_v_sin = out;
         
         // -- mod="$v_drive_1" dstVar=out
         voice->tmp18_seq = voice->var_v_drive_1;
         out *= voice->tmp18_seq;
         
         // -- mod="fma" dstVar=out
         out = (out * 0.005f) + 0.5f;
         
         // -- mod="lut" dstVar=out
         voice->tmp19_lut_f = (out * 4095);
         voice->tmp19_lut_f = clamp(voice->tmp19_lut_f, 0.0f, (float)4095);
         voice->tmp20_lut_idx = (int)voice->tmp19_lut_f;
         voice->tmp21_lut_frac = voice->tmp19_lut_f - (float)voice->tmp20_lut_idx;
         voice->tmp22_lut_a = shared->lut_tanh[((unsigned int)voice->tmp20_lut_idx) & 4095];
         voice->tmp23_lut_b = shared->lut_tanh[((unsigned int)voice->tmp20_lut_idx + 1) & 4095];
         out = voice->tmp22_lut_a + (voice->tmp23_lut_b - voice->tmp22_lut_a) * voice->tmp21_lut_frac;
         
         // -- mod="xfd" dstVar=out
         
         // ---- mod="xfd" input "b" seq 1/1
         
         // -- mod="$v_sin" dstVar=voice->tmp24_b
         voice->tmp24_b = voice->var_v_sin;
         
         // ---- mod="xfd" input "amount" seq 1/1
         
         // -- mod="$v_xfd_1" dstVar=voice->tmp25_amount
         voice->tmp25_amount = voice->var_v_xfd_1;
         voice->tmp26_amount_a = (voice->tmp25_amount < 0.0f) ? 1.0f : (1.0f - voice->tmp25_amount);
         voice->tmp27_amount_b = (voice->tmp25_amount > 0.0f) ? 1.0f : (1.0f + voice->tmp25_amount);
         out = out*voice->tmp26_amount_a + voice->tmp24_b*voice->tmp27_amount_b;
         
         // -- mod="sto v_osc_1" dstVar=out
         voice->var_v_osc_1 = out;
         
         // -- mod="sin" dstVar=out
         
         // ---- mod="sin" input "freq" seq 1/1
         
         // -- mod="$v_freq_2" dstVar=voice->tmp31_freq
         voice->tmp31_freq = voice->var_v_freq_2;
         voice->tmp30_sin_speed = voice->note_speed_cur * voice->tmp31_freq;
         
         // ---- mod="sin" input "phase" seq 1/1
         
         // -- mod="$v_osc_2" dstVar=voice->tmp33_phase
         voice->tmp33_phase = voice->var_v_osc_2;
         
         // -- mod="$m_fm_1to2" dstVar=voice->tmp33_phase
         voice->tmp34_seq = voice->mod_fm_1to2_cur;
         voice->tmp33_phase *= voice->tmp34_seq;
         
         // -- mod="32" dstVar=voice->tmp33_phase
         voice->tmp35_seq = 32.0f;
         voice->tmp33_phase *= voice->tmp35_seq;
         
         // ---- mod="sin" input "vsync" seq 1/1
         
         // -- mod="$v_vsync_2" dstVar=voice->tmp36_vsync
         voice->tmp36_vsync = voice->var_v_vsync_2;
         voice->tmp32_sin_tmp = ((voice->tmp28_sin_phase + voice->tmp33_phase));
         voice->tmp32_sin_tmp = ffrac_s(voice->tmp32_sin_tmp);
         out = cycle_sine_tbl_f[(unsigned short)(16384 * voice->tmp32_sin_tmp)&16383u];
         voice->tmp37_window = cycle_sine_tbl_f[(unsigned short)(16384 * voice->tmp29_win_phase)&16383u];
         voice->tmp37_window *= voice->tmp37_window;
         out *= voice->tmp37_window;
         voice->tmp28_sin_phase = ffrac_s(voice->tmp28_sin_phase + voice->tmp30_sin_speed * voice->tmp36_vsync);
         tmp_f = voice->tmp29_win_phase;
         voice->tmp29_win_phase = ffrac_s(voice->tmp29_win_phase + voice->tmp30_sin_speed);
         if(tmp_f > voice->tmp29_win_phase) voice->tmp28_sin_phase = voice->tmp29_win_phase * voice->tmp36_vsync; 
         
         // -- mod="sto v_sin" dstVar=out
         voice->var_v_sin = out;
         
         // -- mod="$v_drive_2" dstVar=out
         voice->tmp38_seq = voice->var_v_drive_2;
         out *= voice->tmp38_seq;
         
         // -- mod="fma" dstVar=out
         out = (out * 0.005f) + 0.5f;
         
         // -- mod="lut" dstVar=out
         voice->tmp39_lut_f = (out * 4095);
         voice->tmp39_lut_f = clamp(voice->tmp39_lut_f, 0.0f, (float)4095);
         voice->tmp40_lut_idx = (int)voice->tmp39_lut_f;
         voice->tmp41_lut_frac = voice->tmp39_lut_f - (float)voice->tmp40_lut_idx;
         voice->tmp42_lut_a = shared->lut_tanh[((unsigned int)voice->tmp40_lut_idx) & 4095];
         voice->tmp43_lut_b = shared->lut_tanh[((unsigned int)voice->tmp40_lut_idx + 1) & 4095];
         out = voice->tmp42_lut_a + (voice->tmp43_lut_b - voice->tmp42_lut_a) * voice->tmp41_lut_frac;
         
         // -- mod="xfd" dstVar=out
         
         // ---- mod="xfd" input "b" seq 1/1
         
         // -- mod="$v_sin" dstVar=voice->tmp44_b
         voice->tmp44_b = voice->var_v_sin;
         
         // ---- mod="xfd" input "amount" seq 1/1
         
         // -- mod="$v_xfd_2" dstVar=voice->tmp45_amount
         voice->tmp45_amount = voice->var_v_xfd_2;
         voice->tmp46_amount_a = (voice->tmp45_amount < 0.0f) ? 1.0f : (1.0f - voice->tmp45_amount);
         voice->tmp47_amount_b = (voice->tmp45_amount > 0.0f) ? 1.0f : (1.0f + voice->tmp45_amount);
         out = out*voice->tmp46_amount_a + voice->tmp44_b*voice->tmp47_amount_b;
         
         // -- mod="sto v_osc_2" dstVar=out
         voice->var_v_osc_2 = out;
         
         // -- mod="$m_level_2" dstVar=out
         voice->tmp48_seq = voice->mod_level_2_cur;
         out *= voice->tmp48_seq;
         
         // -- mod="$v_osc_1" dstVar=out
         voice->tmp49_seq = voice->var_v_osc_1;
         
         // -- mod="$m_level_1" dstVar=voice->tmp49_seq
         voice->tmp50_seq = voice->mod_level_1_cur;
         voice->tmp49_seq *= voice->tmp50_seq;
         out += voice->tmp49_seq;
   
         /* end calc */

#ifdef OVERSAMPLE_FACTOR
         outOS += out;
#endif // OVERSAMPLE_FACTOR
      }
#ifdef OVERSAMPLE_FACTOR
      // Apply lowpass filter before downsampling
      //   (note) normalized Fc = F/Fs = 0.442947 / sqrt(oversample_factor� - 1)
      out = outOS * (1.0f / OVERSAMPLE_FACTOR);
#endif // OVERSAMPLE_FACTOR
      _samplesOut[k]      = out;
      _samplesOut[k + 1u] = out;

      // Next frame
      k += 2u;
      voice->note_speed_cur += voice->note_speed_inc;
      voice->mod_color_1_cur       += voice->mod_color_1_inc;
      voice->mod_color_2_cur       += voice->mod_color_2_inc;
      voice->mod_vsync_2_off_cur   += voice->mod_vsync_2_off_inc;
      voice->mod_ratio_2_cur       += voice->mod_ratio_2_inc;
      voice->mod_fm_2to1_cur       += voice->mod_fm_2to1_inc;
      voice->mod_fm_1to2_cur       += voice->mod_fm_1to2_inc;
      voice->mod_level_1_cur       += voice->mod_level_1_inc;
      voice->mod_level_2_cur       += voice->mod_level_2_inc;
   } /* loop numFrames */
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   osc_crossfm_tanh_vsync_shared_t *ret = (osc_crossfm_tanh_vsync_shared_t *)malloc(sizeof(osc_crossfm_tanh_vsync_shared_t));
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
   osc_crossfm_tanh_vsync_voice_t *voice = (osc_crossfm_tanh_vsync_voice_t *)malloc(sizeof(osc_crossfm_tanh_vsync_voice_t));
   if(NULL != voice)
   {
      memset((void*)voice, 0, sizeof(*voice));
      voice->base.info = _info;
   }
   return &voice->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_crossfm_tanh_vsync_voice_t);

   free((void*)_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {
   free((void*)_info);
}

#ifdef USE_CYCLE_SINE_TBL
static void loc_calc_sine_tbl(void) {
#define QCOS_BITS 16
#define QCOS_ONE  (1 << QCOS_BITS)
#define QCOS_MASK (QCOS_ONE - 1)
   // (note) same as in TSR "C" implementation
   float *qcos = cycle_sine_tbl_f + 4096; // quarter cos tbl

   // calc quarter cos tbl
   int k = 0;
   for(int a = 0; a < (QCOS_ONE/4); a += (QCOS_ONE/16384)/*4*/)  // 4096 elements
   {
      int x = a - ((int)(QCOS_ONE * 0.25 + 0.5)) + ((a + ((int)(QCOS_ONE * 0.25 + 0.5)))&~QCOS_MASK);
      x = ((x * ((x<0?-x:x) - ((int)(QCOS_ONE * 0.5 + 0.5)))) >> QCOS_BITS) << 4;
      x += (((((int)(QCOS_ONE * 0.225 + 0.5)) * x) >> QCOS_BITS) * ((x<0?-x:x) - QCOS_ONE)) >> QCOS_BITS;
      qcos[k++] = x / float(QCOS_ONE);
   }

   // 0�..90� (rev qtbl)
   int j = 4096;
   k = 0;
   loop(4096)
      cycle_sine_tbl_f[k++] = qcos[--j];

   // 90..180� = cos tbl
   k += 4096;

   // 180�..360� (y-flip first half of tbl)
   j = 0;
   loop(8192)
      cycle_sine_tbl_f[k++] = -cycle_sine_tbl_f[j++];

#ifdef USE_CYCLE_SINE_TBL_I
   loop(16384)
      cycle_sine_tbl_i[i] = (short)(cycle_sine_tbl_f[i] * 2048.0f/*FX_ONE*/);
#endif // USE_CYCLE_SINE_TBL_I
}
#endif // USE_CYCLE_SINE_TBL

extern "C" {
st_plugin_info_t *osc_crossfm_tanh_vsync_init(void) {
   osc_crossfm_tanh_vsync_info_t *ret = (osc_crossfm_tanh_vsync_info_t *)malloc(sizeof(osc_crossfm_tanh_vsync_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "osc_crossfm_tanh_vsync";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "crossfm osc tanh vsync";
      ret->base.short_name  = "crossfm osc tanh vsync";
      ret->base.flags       = ST_PLUGIN_FLAG_OSC;
      ret->base.category    = ST_PLUGIN_CAT_UNKNOWN;
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

#ifdef USE_CYCLE_SINE_TBL
      loc_calc_sine_tbl();
#endif // USE_CYCLE_SINE_TBL
   }

   return &ret->base;
}

#ifndef STFX_SKIP_MAIN_INIT
ST_PLUGIN_APICALL st_plugin_info_t *ST_PLUGIN_API st_plugin_init(unsigned int _pluginIdx) {
   switch(_pluginIdx)
   {
      case 0u:
         return osc_crossfm_tanh_vsync_init();
   }
   return NULL;
}
#endif // STFX_SKIP_MAIN_INIT

} // extern "C"