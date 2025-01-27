// ----
// ---- file   : osc_voice_1b.cpp
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
// ----           $ g++ -Wall -Wno-unused-function -Wno-unused-variable -I../../tksampler -c osc_voice_1b.cpp -o osc_voice_1b.o
// ---- created: 08Aug2023 12:49:07
// ----
// ----
// ----

#include <plugin.h>

#include <stdlib.h>
#include <math.h>
#include <string.h>


#define OVERSAMPLE_FACTOR  16.0f

#define PARAM_FREQ_2             0
#define PARAM_FM                 1
#define PARAM_PHASE_RAND         2
#define PARAM_CUTOFF             3
#define PARAM_RES                4
#define PARAM_VSYNC_OFF          5
#define PARAM_FLT_OFF            6
#define PARAM_COLOR              7
#define NUM_PARAMS               8
static const char *loc_param_names[NUM_PARAMS] = {
   "freq_2",                  // 0: FREQ_2
   "fm",                      // 1: FM
   "phase_rand",              // 2: PHASE_RAND
   "cutoff",                  // 3: CUTOFF
   "res",                     // 4: RES
   "vsync_off",               // 5: VSYNC_OFF
   "flt_off",                 // 6: FLT_OFF
   "color",                   // 7: COLOR

};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,                      // 0: FREQ_2
   0.0f,                      // 1: FM
   0.0f,                      // 2: PHASE_RAND
   0.5f,                      // 3: CUTOFF
   0.5f,                      // 4: RES
   0.0f,                      // 5: VSYNC_OFF
   0.0f,                      // 6: FLT_OFF
   0.0f,                      // 7: COLOR

};

#define MOD_FREQ_2               0
#define MOD_FM                   1
#define MOD_CUTOFF               2
#define MOD_RES                  3
#define MOD_VSYNC_OFF            4
#define MOD_FLT_OFF              5
#define MOD_COLOR                6
#define MOD_DETUNE               7
#define NUM_MODS                 8
static const char *loc_mod_names[NUM_MODS] = {
   "freq_2",       // 0: FREQ_2
   "fm",           // 1: FM
   "cutoff",       // 2: CUTOFF
   "res",          // 3: RES
   "vsync_off",    // 4: VSYNC_OFF
   "flt_off",      // 5: FLT_OFF
   "color",        // 6: COLOR
   "detune",       // 7: DETUNE

};

typedef struct osc_voice_1b_info_s {
   st_plugin_info_t base;
} osc_voice_1b_info_t;

typedef struct osc_voice_1b_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];

} osc_voice_1b_shared_t;

typedef struct osc_voice_1b_voice_s {
   st_plugin_voice_t base;
   float             sample_rate;
   float             note_speed_fixed;
   float             note_speed_cur;
   float             note_speed_inc;
   float             note_cur;
   float             note_inc;
   float             mods[NUM_MODS];
   float mod_freq_2_cur;
   float mod_freq_2_inc;
   float mod_fm_cur;
   float mod_fm_inc;
   float mod_cutoff_cur;
   float mod_cutoff_inc;
   float mod_res_cur;
   float mod_res_inc;
   float mod_vsync_off_cur;
   float mod_vsync_off_inc;
   float mod_flt_off_cur;
   float mod_flt_off_inc;
   float mod_color_cur;
   float mod_color_inc;
   float mod_detune_cur;
   float mod_detune_inc;

   short tmp1_i2f;
   unsigned short tmp2_lfsr_state;
   short tmp3_lfsr_state_signed;
   float tmp4_seq;
   short tmp5_i2f;
   unsigned short tmp6_lfsr_state;
   short tmp7_lfsr_state_signed;
   float tmp8_seq;
   float tmp9_kbd;
   float tmp10_off;
   float tmp11_scl;
   float tmp12_seq;
   float tmp13_kbd;
   float tmp14_off;
   float tmp15_scl;
   float tmp16_seq;
   float tmp17_seq;
   float tmp18_seq;
   float tmp19_seq;
   float tmp20_saw_phase;
   float tmp21_saw_speed;
   float tmp22_freq;
   float tmp23_saw_tmp;
   float tmp24_phase;
   float tmp25_vsync;
   float tmp26_seq;
   float tmp27_c;
   float tmp28_sin_phase;
   float tmp29_sin_speed;
   float tmp30_freq;
   float tmp31_sin_tmp;
   float tmp32_phase;
   float tmp33_seq;
   float tmp34_seq;
   float tmp35_svf_lp;
   float tmp36_svf_hp;
   float tmp37_svf_bp;
   float tmp38_res;
   float tmp39_freq;
   float tmp40_svf_lp;
   float tmp41_svf_hp;
   float tmp42_svf_bp;
   float tmp43_res;
   float tmp44_freq;
   float tmp45_exp;
   float tmp46_svf_lp;
   float tmp47_svf_hp;
   float tmp48_svf_bp;
   float tmp49_res;
   float tmp50_freq;
   float tmp51_svf_lp;
   float tmp52_svf_hp;
   float tmp53_svf_bp;
   float tmp54_res;
   float tmp55_freq;
   float var_x;
   float var_v_phrand_1;
   float var_v_phrand_2;
   float var_v_osc_1;
   float var_v_cutoff_1;
   float var_v_cutoff_2;
   float var_v_res;
   float var_v_res_half;
   float var_v_freq_sin;

} osc_voice_1b_voice_t;

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

static float mathPowerf(float _x, float _y) {
   float r;
   if(_y != 0.0f)
   {
      if(_x < 0.0f)
      {
         r = (float)( -expf(_y*logf(-_x)) );
      }
      else if(_x > 0.0f)
      {
         r = (float)( expf(_y*logf(_x)) );
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

static float mathLogLinExpf(float _f, float _c) {
   // c: <0: log
   //     0: lin
   //    >0: exp
   stplugin_fi_t uSign;
   uSign.f = _f;
   stplugin_fi_t u;
   u.f = _f;
   u.u &= 0x7fffFFFFu;
   u.f = powf(u.f, powf(2.0f, _c));
   u.u |= uSign.u & 0x80000000u;
   return u.f;
}

static void loc_init(st_plugin_voice_t *_voice);
static void loc_prepare(st_plugin_voice_t *_voice);

void loc_init(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_voice_1b_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_voice_1b_shared_t);

   float out = 0.0f;
   // -------- lane "init" modIdx=0
   
   // -- mod="fsr" dstVar=out
   voice->tmp1_i2f = (int)(out * 2048);  // IntFallback: F2I
   voice->tmp2_lfsr_state ^= voice->tmp2_lfsr_state >> 7;
   voice->tmp2_lfsr_state ^= voice->tmp2_lfsr_state << 9;
   voice->tmp2_lfsr_state ^= voice->tmp2_lfsr_state >> 13;
   voice->tmp3_lfsr_state_signed = (voice->tmp2_lfsr_state & 65520);
   voice->tmp1_i2f = voice->tmp3_lfsr_state_signed >> 4;
   out = voice->tmp1_i2f / ((float)(2048));  // IntFallback: I2F
   
   // -- mod="$p_phase_rand" dstVar=out
   voice->tmp4_seq = shared->params[PARAM_PHASE_RAND];
   out *= voice->tmp4_seq;
   
   // -- mod="sto v_phrand_1" dstVar=out
   voice->var_v_phrand_1 = out;
   
   // -- mod="fsr" dstVar=out
   voice->tmp5_i2f = (int)(out * 2048);  // IntFallback: F2I
   voice->tmp6_lfsr_state ^= voice->tmp6_lfsr_state >> 7;
   voice->tmp6_lfsr_state ^= voice->tmp6_lfsr_state << 9;
   voice->tmp6_lfsr_state ^= voice->tmp6_lfsr_state >> 13;
   voice->tmp7_lfsr_state_signed = (voice->tmp6_lfsr_state & 65520);
   voice->tmp5_i2f = voice->tmp7_lfsr_state_signed >> 4;
   out = voice->tmp5_i2f / ((float)(2048));  // IntFallback: I2F
   
   // -- mod="$p_phase_rand" dstVar=out
   voice->tmp8_seq = shared->params[PARAM_PHASE_RAND];
   out *= voice->tmp8_seq;
   
   // -- mod="sto v_phrand_2" dstVar=out
   voice->var_v_phrand_2 = out;
} /* end init */

void loc_prepare(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_voice_1b_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_voice_1b_shared_t);

   float out = 0.0f;
   // -------- lane "prepare" modIdx=0
   
   // -- mod="$m_cutoff" dstVar=out
   out = voice->mod_cutoff_cur;
   
   // -- mod="kbd" dstVar=out
   out *= 127.0f;
   
   // ---- mod="kbd" input "kbd" seq 1/1
   
   // -- mod="1" dstVar=voice->tmp9_kbd
   voice->tmp9_kbd = 1.0f;
   voice->tmp10_off = 0;
   voice->tmp11_scl = 4;
   out += (voice->note_cur + (voice->tmp10_off * 12.0f) - 64.0f) * voice->tmp9_kbd;
   out = clamp(out, 0.0f, 127.0f);
   #ifdef OVERSAMPLE_FACTOR
   out = ((440.0f/32.0f)*expf( ((out-9.0f)/12.0f)*logf(2.0f) )) / (voice->sample_rate * OVERSAMPLE_FACTOR);
   #else
   out = ((440.0f/32.0f)*expf( ((out-9.0f)/12.0f)*logf(2.0f) )) / voice->sample_rate;
   #endif
   out *= voice->tmp11_scl;
   
   // -- mod="clp" dstVar=out
   if(out > 1.0f) out = 1.0f;
   else if(out < 0.0f) out = 0.0f;
   
   // -- mod="sto v_cutoff_1" dstVar=out
   voice->var_v_cutoff_1 = out;
   
   // -- mod="$m_cutoff" dstVar=out
   out = voice->mod_cutoff_cur;
   
   // -- mod="$m_flt_off" dstVar=out
   voice->tmp12_seq = voice->mod_flt_off_cur;
   out += voice->tmp12_seq;
   
   // -- mod="kbd" dstVar=out
   out *= 127.0f;
   
   // ---- mod="kbd" input "kbd" seq 1/1
   
   // -- mod="1" dstVar=voice->tmp13_kbd
   voice->tmp13_kbd = 1.0f;
   voice->tmp14_off = 0;
   voice->tmp15_scl = 4;
   out += (voice->note_cur + (voice->tmp14_off * 12.0f) - 64.0f) * voice->tmp13_kbd;
   out = clamp(out, 0.0f, 127.0f);
   #ifdef OVERSAMPLE_FACTOR
   out = ((440.0f/32.0f)*expf( ((out-9.0f)/12.0f)*logf(2.0f) )) / (voice->sample_rate * OVERSAMPLE_FACTOR);
   #else
   out = ((440.0f/32.0f)*expf( ((out-9.0f)/12.0f)*logf(2.0f) )) / voice->sample_rate;
   #endif
   out *= voice->tmp15_scl;
   
   // -- mod="clp" dstVar=out
   if(out > 1.0f) out = 1.0f;
   else if(out < 0.0f) out = 0.0f;
   
   // -- mod="sto v_cutoff_2" dstVar=out
   voice->var_v_cutoff_2 = out;
   
   // -- mod="$m_res" dstVar=out
   out = voice->mod_res_cur;
   
   // -- mod="clp" dstVar=out
   if(out > 1.0f) out = 1.0f;
   else if(out < 0.04f) out = 0.04f;
   
   // -- mod="sto v_res" dstVar=out
   voice->var_v_res = out;
   
   // -- mod="0.5" dstVar=out
   out = 0.5f;
   
   // -- mod="$v_res" dstVar=out
   voice->tmp16_seq = voice->var_v_res;
   out += voice->tmp16_seq;
   
   // -- mod="0.5" dstVar=out
   voice->tmp17_seq = 0.5f;
   out *= voice->tmp17_seq;
   
   // -- mod="sto v_res_half" dstVar=out
   voice->var_v_res_half = out;
   
   // -- mod="$m_freq_2" dstVar=out
   out = voice->mod_freq_2_cur;
   
   // -- mod="qua" dstVar=out
   out = ((int)(out * 128.0f) / 128.0f);
   
   // -- mod="$m_detune" dstVar=out
   voice->tmp18_seq = voice->mod_detune_cur;
   out += voice->tmp18_seq;
   
   // -- mod="0.01" dstVar=out
   voice->tmp19_seq = 0.01f;
   out *= voice->tmp19_seq;
   
   // -- mod="sto v_freq_sin" dstVar=out
   voice->var_v_freq_sin = out;
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
   ST_PLUGIN_SHARED_CAST(osc_voice_1b_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(osc_voice_1b_shared_t);
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
   ST_PLUGIN_VOICE_CAST(osc_voice_1b_voice_t);
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(osc_voice_1b_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_voice_1b_shared_t);
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


      voice->tmp3_lfsr_state_signed = 0;
      voice->tmp7_lfsr_state_signed = 0;
      voice->tmp20_saw_phase = 0.0f;
      voice->tmp28_sin_phase = 0.0f;
      voice->tmp35_svf_lp = 0.0f;
      voice->tmp36_svf_hp = 0.0f;
      voice->tmp37_svf_bp = 0.0f;
      voice->tmp40_svf_lp = 0.0f;
      voice->tmp41_svf_hp = 0.0f;
      voice->tmp42_svf_bp = 0.0f;
      voice->tmp46_svf_lp = 0.0f;
      voice->tmp47_svf_hp = 0.0f;
      voice->tmp48_svf_bp = 0.0f;
      voice->tmp51_svf_lp = 0.0f;
      voice->tmp52_svf_hp = 0.0f;
      voice->tmp53_svf_bp = 0.0f;

      voice->var_x = 0.0f;
      voice->var_v_phrand_1 = 0.0f;
      voice->var_v_phrand_2 = 0.0f;
      voice->var_v_osc_1 = 0.0f;
      voice->var_v_cutoff_1 = 0.0f;
      voice->var_v_cutoff_2 = 0.0f;
      voice->var_v_res = 0.0f;
      voice->var_v_res_half = 0.0f;
      voice->var_v_freq_sin = 0.0f;

      loc_init(&voice->base);

   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(osc_voice_1b_voice_t);
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
   ST_PLUGIN_VOICE_CAST(osc_voice_1b_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_voice_1b_shared_t);
   (void)_note;
   (void)_vol;
   (void)_pan;

#ifdef OVERSAMPLE_FACTOR
   float noteSpeed = _freqHz / (voice->sample_rate * OVERSAMPLE_FACTOR);
#else
   float noteSpeed = _freqHz / voice->sample_rate;
#endif // OVERSAMPLE_FACTOR

   float modfreq_2       = shared->params[PARAM_FREQ_2      ] * 7.875f + 0.125f     + voice->mods[MOD_FREQ_2       ];
   float modfm           = shared->params[PARAM_FM          ] * 8.0f                + voice->mods[MOD_FM           ];
   float modcutoff       = shared->params[PARAM_CUTOFF      ]                       + voice->mods[MOD_CUTOFF       ];
   float modres          = shared->params[PARAM_RES         ]                       + voice->mods[MOD_RES          ];
   float modvsync_off    = shared->params[PARAM_VSYNC_OFF   ] * 8.0f - 4.0f         + voice->mods[MOD_VSYNC_OFF    ];
   float modflt_off      = shared->params[PARAM_FLT_OFF     ] - 0.5f                + voice->mods[MOD_FLT_OFF      ];
   float modcolor        = shared->params[PARAM_COLOR       ] * 3.99f + 0.01f       + voice->mods[MOD_COLOR        ];
   float moddetune       = voice->mods[MOD_DETUNE       ];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->note_speed_inc = (noteSpeed - voice->note_speed_cur) * recBlockSize;
      voice->note_inc       = (_note - voice->note_cur) * recBlockSize;
      voice->mod_freq_2_inc       = (modfreq_2          - voice->mod_freq_2_cur        ) * recBlockSize;
      voice->mod_fm_inc           = (modfm              - voice->mod_fm_cur            ) * recBlockSize;
      voice->mod_cutoff_inc       = (modcutoff          - voice->mod_cutoff_cur        ) * recBlockSize;
      voice->mod_res_inc          = (modres             - voice->mod_res_cur           ) * recBlockSize;
      voice->mod_vsync_off_inc    = (modvsync_off       - voice->mod_vsync_off_cur     ) * recBlockSize;
      voice->mod_flt_off_inc      = (modflt_off         - voice->mod_flt_off_cur       ) * recBlockSize;
      voice->mod_color_inc        = (modcolor           - voice->mod_color_cur         ) * recBlockSize;
      voice->mod_detune_inc       = (moddetune          - voice->mod_detune_cur        ) * recBlockSize;

      loc_prepare(&voice->base);
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->note_speed_cur = noteSpeed;
      voice->note_speed_inc = 0.0f;
      voice->note_cur       = _note;
      voice->note_inc       = 0.0f;
      voice->mod_freq_2_cur       = modfreq_2;
      voice->mod_freq_2_inc       = 0.0f;
      voice->mod_fm_cur           = modfm;
      voice->mod_fm_inc           = 0.0f;
      voice->mod_cutoff_cur       = modcutoff;
      voice->mod_cutoff_inc       = 0.0f;
      voice->mod_res_cur          = modres;
      voice->mod_res_inc          = 0.0f;
      voice->mod_vsync_off_cur    = modvsync_off;
      voice->mod_vsync_off_inc    = 0.0f;
      voice->mod_flt_off_cur      = modflt_off;
      voice->mod_flt_off_inc      = 0.0f;
      voice->mod_color_cur        = modcolor;
      voice->mod_color_inc        = 0.0f;
      voice->mod_detune_cur       = moddetune;
      voice->mod_detune_inc       = 0.0f;

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

   ST_PLUGIN_VOICE_CAST(osc_voice_1b_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_voice_1b_shared_t);

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
         
         // -- mod="saw" dstVar=out
         voice->tmp22_freq = 1;
         voice->tmp21_saw_speed = voice->note_speed_cur * voice->tmp22_freq;
         
         // ---- mod="saw" input "phase" seq 1/1
         
         // -- mod="$v_phrand_2" dstVar=voice->tmp24_phase
         voice->tmp24_phase = voice->var_v_phrand_2;
         
         // ---- mod="saw" input "vsync" seq 1/1
         
         // -- mod="$m_freq_2" dstVar=voice->tmp25_vsync
         voice->tmp25_vsync = voice->mod_freq_2_cur;
         
         // -- mod="$m_vsync_off" dstVar=voice->tmp25_vsync
         voice->tmp26_seq = voice->mod_vsync_off_cur;
         voice->tmp25_vsync += voice->tmp26_seq;
         voice->tmp23_saw_tmp = ((voice->tmp20_saw_phase + voice->tmp24_phase));
         voice->tmp23_saw_tmp = voice->tmp23_saw_tmp * voice->tmp25_vsync;
         voice->tmp23_saw_tmp = ffrac_s(voice->tmp23_saw_tmp);
         out = 1.0 - (voice->tmp23_saw_tmp * 2.0f);
         voice->tmp20_saw_phase = ffrac_s(voice->tmp20_saw_phase + voice->tmp21_saw_speed);
         
         // -- mod="lle" dstVar=out
         
         // ---- mod="lle" input "c" seq 1/1
         
         // -- mod="$m_color" dstVar=voice->tmp27_c
         voice->tmp27_c = voice->mod_color_cur;
         out = mathLogLinExpf(out, voice->tmp27_c);
         
         // -- mod="sto v_osc_1" dstVar=out
         voice->var_v_osc_1 = out;
         
         // -- mod="sin" dstVar=out
         
         // ---- mod="sin" input "freq" seq 1/1
         
         // -- mod="$v_freq_sin" dstVar=voice->tmp30_freq
         voice->tmp30_freq = voice->var_v_freq_sin;
         voice->tmp29_sin_speed = voice->note_speed_cur * voice->tmp30_freq;
         
         // ---- mod="sin" input "phase" seq 1/1
         
         // -- mod="$v_osc_1" dstVar=voice->tmp32_phase
         voice->tmp32_phase = voice->var_v_osc_1;
         
         // -- mod="$m_fm" dstVar=voice->tmp32_phase
         voice->tmp33_seq = voice->mod_fm_cur;
         voice->tmp32_phase *= voice->tmp33_seq;
         
         // -- mod="$v_phrand_1" dstVar=voice->tmp32_phase
         voice->tmp34_seq = voice->var_v_phrand_1;
         voice->tmp32_phase += voice->tmp34_seq;
         voice->tmp31_sin_tmp = ((voice->tmp28_sin_phase + voice->tmp32_phase));
         voice->tmp31_sin_tmp = ffrac_s(voice->tmp31_sin_tmp);
         out = cycle_sine_tbl_f[(unsigned short)(16384 * voice->tmp31_sin_tmp)&16383u];
         voice->tmp28_sin_phase = ffrac_s(voice->tmp28_sin_phase + voice->tmp29_sin_speed);
         
         // -- mod="svf" dstVar=out
         
         // ---- mod="svf" input "res" seq 1/1
         
         // -- mod="$v_res" dstVar=voice->tmp38_res
         voice->tmp38_res = voice->var_v_res;
         
         // ---- mod="svf" input "freq" seq 1/1
         
         // -- mod="$v_cutoff_1" dstVar=voice->tmp39_freq
         voice->tmp39_freq = voice->var_v_cutoff_1;
         voice->tmp35_svf_lp = voice->tmp35_svf_lp + (voice->tmp37_svf_bp * voice->tmp39_freq);
         voice->tmp36_svf_hp = out - voice->tmp35_svf_lp - (voice->tmp37_svf_bp * voice->tmp38_res);
         voice->tmp37_svf_bp = voice->tmp37_svf_bp + (voice->tmp36_svf_hp * voice->tmp39_freq);
         out = voice->tmp37_svf_bp;
         
         // -- mod="svf" dstVar=out
         
         // ---- mod="svf" input "res" seq 1/1
         
         // -- mod="$v_res_half" dstVar=voice->tmp43_res
         voice->tmp43_res = voice->var_v_res_half;
         
         // ---- mod="svf" input "freq" seq 1/1
         
         // -- mod="$v_cutoff_1" dstVar=voice->tmp44_freq
         voice->tmp44_freq = voice->var_v_cutoff_1;
         voice->tmp40_svf_lp = voice->tmp40_svf_lp + (voice->tmp42_svf_bp * voice->tmp44_freq);
         voice->tmp41_svf_hp = out - voice->tmp40_svf_lp - (voice->tmp42_svf_bp * voice->tmp43_res);
         voice->tmp42_svf_bp = voice->tmp42_svf_bp + (voice->tmp41_svf_hp * voice->tmp44_freq);
         out = voice->tmp42_svf_bp;
         
         // -- mod="pow" dstVar=out
         
         // ---- mod="pow" input "exp" seq 1/1
         
         // -- mod="$m_color" dstVar=voice->tmp45_exp
         voice->tmp45_exp = voice->mod_color_cur;
         out = mathPowerf(out, voice->tmp45_exp);
         
         // -- mod="svf" dstVar=out
         
         // ---- mod="svf" input "res" seq 1/1
         
         // -- mod="$v_res" dstVar=voice->tmp49_res
         voice->tmp49_res = voice->var_v_res;
         
         // ---- mod="svf" input "freq" seq 1/1
         
         // -- mod="$v_cutoff_2" dstVar=voice->tmp50_freq
         voice->tmp50_freq = voice->var_v_cutoff_2;
         voice->tmp46_svf_lp = voice->tmp46_svf_lp + (voice->tmp48_svf_bp * voice->tmp50_freq);
         voice->tmp47_svf_hp = out - voice->tmp46_svf_lp - (voice->tmp48_svf_bp * voice->tmp49_res);
         voice->tmp48_svf_bp = voice->tmp48_svf_bp + (voice->tmp47_svf_hp * voice->tmp50_freq);
         out = voice->tmp48_svf_bp;
         
         // -- mod="svf" dstVar=out
         
         // ---- mod="svf" input "res" seq 1/1
         
         // -- mod="$v_res_half" dstVar=voice->tmp54_res
         voice->tmp54_res = voice->var_v_res_half;
         
         // ---- mod="svf" input "freq" seq 1/1
         
         // -- mod="$v_cutoff_2" dstVar=voice->tmp55_freq
         voice->tmp55_freq = voice->var_v_cutoff_2;
         voice->tmp51_svf_lp = voice->tmp51_svf_lp + (voice->tmp53_svf_bp * voice->tmp55_freq);
         voice->tmp52_svf_hp = out - voice->tmp51_svf_lp - (voice->tmp53_svf_bp * voice->tmp54_res);
         voice->tmp53_svf_bp = voice->tmp53_svf_bp + (voice->tmp52_svf_hp * voice->tmp55_freq);
         out = voice->tmp53_svf_bp;
   
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
      voice->mod_freq_2_cur        += voice->mod_freq_2_inc;
      voice->mod_fm_cur            += voice->mod_fm_inc;
      voice->mod_cutoff_cur        += voice->mod_cutoff_inc;
      voice->mod_res_cur           += voice->mod_res_inc;
      voice->mod_vsync_off_cur     += voice->mod_vsync_off_inc;
      voice->mod_flt_off_cur       += voice->mod_flt_off_inc;
      voice->mod_color_cur         += voice->mod_color_inc;
      voice->mod_detune_cur        += voice->mod_detune_inc;
   } /* loop numFrames */
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   osc_voice_1b_shared_t *ret = (osc_voice_1b_shared_t *)malloc(sizeof(osc_voice_1b_shared_t));
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
   osc_voice_1b_voice_t *voice = (osc_voice_1b_voice_t *)malloc(sizeof(osc_voice_1b_voice_t));
   if(NULL != voice)
   {
      memset((void*)voice, 0, sizeof(*voice));
      voice->base.info = _info;
      voice->tmp2_lfsr_state = 17545;
      voice->tmp6_lfsr_state = 56505;
   }
   return &voice->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_voice_1b_voice_t);

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
st_plugin_info_t *osc_voice_1b_init(void) {
   osc_voice_1b_info_t *ret = (osc_voice_1b_info_t *)malloc(sizeof(osc_voice_1b_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "osc_voice_1b";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "voice osc 1b";
      ret->base.short_name  = "voice osc 1b";
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
         return osc_voice_1b_init();
   }
   return NULL;
}
#endif // STFX_SKIP_MAIN_INIT

} // extern "C"
