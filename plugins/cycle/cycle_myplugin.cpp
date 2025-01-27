// ----
// ---- file   : myplugin.cpp
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
// ----           $ g++ -Wall -Wno-unused-function -Wno-unused-variable -I../../tksampler -c myplugin.cpp -o myplugin.o
// ---- created: 08Aug2023 12:58:25
// ----
// ----
// ----

#include <plugin.h>

#include <stdlib.h>
#include <math.h>
#include <string.h>




#define PARAM_CUTOFF             0
#define PARAM_RES                1
#define PARAM_PW                 2
#define PARAM_PW_MOD             3
#define PARAM_DETUNE             4
#define PARAM_DEPHASE            5
#define PARAM_VSYNC_1            6
#define PARAM_VSYNC_2            7
#define NUM_PARAMS               8
static const char *loc_param_names[NUM_PARAMS] = {
   "cutoff",                  // 0: CUTOFF
   "res",                     // 1: RES
   "pw",                      // 2: PW
   "pw_mod",                  // 3: PW_MOD
   "detune",                  // 4: DETUNE
   "dephase",                 // 5: DEPHASE
   "vsync_1",                 // 6: VSYNC_1
   "vsync_2",                 // 7: VSYNC_2

};
static float loc_param_resets[NUM_PARAMS] = {
   0.9f,                      // 0: CUTOFF
   0.3f,                      // 1: RES
   0.5f,                      // 2: PW
   0.1f,                      // 3: PW_MOD
   0.0f,                      // 4: DETUNE
   0.0f,                      // 5: DEPHASE
   0.0f,                      // 6: VSYNC_1
   0.0f,                      // 7: VSYNC_2

};

#define MOD_CUTOFF               0
#define MOD_RES                  1
#define MOD_PW_MOD               2
#define MOD_PW                   3
#define MOD_DETUNE               4
#define MOD_DEPHASE              5
#define MOD_VSYNC_1              6
#define MOD_VSYNC_2              7
#define NUM_MODS                 8
static const char *loc_mod_names[NUM_MODS] = {
   "cutoff",       // 0: CUTOFF
   "res",          // 1: RES
   "pw_mod",       // 2: PW_MOD
   "pw",           // 3: PW
   "detune",       // 4: DETUNE
   "dephase",      // 5: DEPHASE
   "vsync_1",      // 6: VSYNC_1
   "vsync_2",      // 7: VSYNC_2

};

typedef struct myplugin_info_s {
   st_plugin_info_t base;
} myplugin_info_t;

typedef struct myplugin_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];

} myplugin_shared_t;

typedef struct myplugin_voice_s {
   st_plugin_voice_t base;
   float             sample_rate;
   float             note_speed_fixed;
   float             note_speed_cur;
   float             note_speed_inc;
   float             note_cur;
   float             note_inc;
   float             mods[NUM_MODS];
   float mod_cutoff_cur;
   float mod_cutoff_inc;
   float mod_res_cur;
   float mod_res_inc;
   float mod_pw_mod_cur;
   float mod_pw_mod_inc;
   float mod_pw_cur;
   float mod_pw_inc;
   float mod_detune_cur;
   float mod_detune_inc;
   float mod_dephase_cur;
   float mod_dephase_inc;
   float mod_vsync_1_cur;
   float mod_vsync_1_inc;
   float mod_vsync_2_cur;
   float mod_vsync_2_inc;

   float tmp1_pul_phase;
   float tmp2_pul_speed;
   float tmp3_freq;
   float tmp4_pul_tmp;
   float tmp5_vsync;
   float tmp6_seq;
   float tmp7_seq;
   float tmp8_width;
   float tmp9_seq;
   float tmp10_seq;
   float tmp11_seq;
   float tmp12_pul_phase;
   float tmp13_pul_speed;
   float tmp14_freq;
   float tmp15_seq;
   float tmp16_pul_tmp;
   float tmp17_vsync;
   float tmp18_seq;
   float tmp19_seq;
   float tmp20_width;
   float tmp21_seq;
   float tmp22_seq;
   float tmp23_svf_lp;
   float tmp24_svf_hp;
   float tmp25_svf_bp;
   float tmp26_res;
   float tmp27_freq;
   float tmp28_svf_lp;
   float tmp29_svf_hp;
   float tmp30_svf_bp;
   float tmp31_freq;
   float tmp32_seq;
   float tmp33_seq;
   float tmp34_hbx_last;
   float tmp35_seq;
   float var_x;
   float var_v_osc1_tmp;
   float var_v_osc2_last;

} myplugin_voice_t;

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
   ST_PLUGIN_SHARED_CAST(myplugin_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(myplugin_shared_t);
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
   ST_PLUGIN_VOICE_CAST(myplugin_voice_t);
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(myplugin_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(myplugin_shared_t);
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


      voice->tmp12_pul_phase = 0.0f;
      voice->tmp23_svf_lp = 0.0f;
      voice->tmp24_svf_hp = 0.0f;
      voice->tmp25_svf_bp = 0.0f;
      voice->tmp28_svf_lp = 0.0f;
      voice->tmp29_svf_hp = 0.0f;
      voice->tmp30_svf_bp = 0.0f;
      voice->tmp34_hbx_last = 0.0f;

      voice->var_x = 0.0f;
      voice->var_v_osc1_tmp = 0.0f;
      voice->var_v_osc2_last = 0.0f;


   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(myplugin_voice_t);
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
   ST_PLUGIN_VOICE_CAST(myplugin_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(myplugin_shared_t);
   (void)_note;
   (void)_vol;
   (void)_pan;

#ifdef OVERSAMPLE_FACTOR
   float noteSpeed = _freqHz / (voice->sample_rate * OVERSAMPLE_FACTOR);
#else
   float noteSpeed = _freqHz / voice->sample_rate;
#endif // OVERSAMPLE_FACTOR

   float modcutoff       = shared->params[PARAM_CUTOFF      ]                       + voice->mods[MOD_CUTOFF       ];
   float modres          = shared->params[PARAM_RES         ]                       + voice->mods[MOD_RES          ];
   float modpw_mod       = shared->params[PARAM_PW_MOD      ]                       + voice->mods[MOD_PW_MOD       ];
   float modpw           = shared->params[PARAM_PW          ]                       + voice->mods[MOD_PW           ];
   float moddetune       = shared->params[PARAM_DETUNE      ]                       + voice->mods[MOD_DETUNE       ];
   float moddephase      = shared->params[PARAM_DEPHASE     ]                       + voice->mods[MOD_DEPHASE      ];
   float modvsync_1      = shared->params[PARAM_VSYNC_1     ]                       + voice->mods[MOD_VSYNC_1      ];
   float modvsync_2      = shared->params[PARAM_VSYNC_2     ]                       + voice->mods[MOD_VSYNC_2      ];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->note_speed_inc = (noteSpeed - voice->note_speed_cur) * recBlockSize;
      voice->note_inc       = (_note - voice->note_cur) * recBlockSize;
      voice->mod_cutoff_inc       = (modcutoff          - voice->mod_cutoff_cur        ) * recBlockSize;
      voice->mod_res_inc          = (modres             - voice->mod_res_cur           ) * recBlockSize;
      voice->mod_pw_mod_inc       = (modpw_mod          - voice->mod_pw_mod_cur        ) * recBlockSize;
      voice->mod_pw_inc           = (modpw              - voice->mod_pw_cur            ) * recBlockSize;
      voice->mod_detune_inc       = (moddetune          - voice->mod_detune_cur        ) * recBlockSize;
      voice->mod_dephase_inc      = (moddephase         - voice->mod_dephase_cur       ) * recBlockSize;
      voice->mod_vsync_1_inc      = (modvsync_1         - voice->mod_vsync_1_cur       ) * recBlockSize;
      voice->mod_vsync_2_inc      = (modvsync_2         - voice->mod_vsync_2_cur       ) * recBlockSize;

   
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->note_speed_cur = noteSpeed;
      voice->note_speed_inc = 0.0f;
      voice->note_cur       = _note;
      voice->note_inc       = 0.0f;
      voice->mod_cutoff_cur       = modcutoff;
      voice->mod_cutoff_inc       = 0.0f;
      voice->mod_res_cur          = modres;
      voice->mod_res_inc          = 0.0f;
      voice->mod_pw_mod_cur       = modpw_mod;
      voice->mod_pw_mod_inc       = 0.0f;
      voice->mod_pw_cur           = modpw;
      voice->mod_pw_inc           = 0.0f;
      voice->mod_detune_cur       = moddetune;
      voice->mod_detune_inc       = 0.0f;
      voice->mod_dephase_cur      = moddephase;
      voice->mod_dephase_inc      = 0.0f;
      voice->mod_vsync_1_cur      = modvsync_1;
      voice->mod_vsync_1_inc      = 0.0f;
      voice->mod_vsync_2_cur      = modvsync_2;
      voice->mod_vsync_2_inc      = 0.0f;

   
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

   ST_PLUGIN_VOICE_CAST(myplugin_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(myplugin_shared_t);

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
      
      // -- mod="pul" dstVar=out
      
      // ---- mod="pul" input "freq" seq 1/1
      
      // -- mod="1" dstVar=voice->tmp3_freq
      voice->tmp3_freq = 1.0f;
      voice->tmp2_pul_speed = voice->note_speed_cur * voice->tmp3_freq;
      
      // ---- mod="pul" input "vsync" seq 1/1
      
      // -- mod="$m_vsync_1" dstVar=voice->tmp5_vsync
      voice->tmp5_vsync = voice->mod_vsync_1_cur;
      
      // -- mod="0.1" dstVar=voice->tmp5_vsync
      voice->tmp6_seq = 0.1f;
      voice->tmp5_vsync += voice->tmp6_seq;
      
      // -- mod="12" dstVar=voice->tmp5_vsync
      voice->tmp7_seq = 12.0f;
      voice->tmp5_vsync *= voice->tmp7_seq;
      
      // ---- mod="pul" input "width" seq 1/1
      
      // -- mod="$m_pw" dstVar=voice->tmp8_width
      voice->tmp8_width = voice->mod_pw_cur;
      
      // -- mod="$v_osc2_last" dstVar=voice->tmp8_width
      voice->tmp9_seq = voice->var_v_osc2_last;
      
      // -- mod="$m_pw_mod" dstVar=voice->tmp9_seq
      voice->tmp10_seq = voice->mod_pw_mod_cur;
      
      // -- mod="0.25" dstVar=voice->tmp10_seq
      voice->tmp11_seq = 0.25f;
      voice->tmp10_seq *= voice->tmp11_seq;
      voice->tmp9_seq *= voice->tmp10_seq;
      voice->tmp8_width += voice->tmp9_seq;
      voice->tmp4_pul_tmp = (voice->tmp1_pul_phase);
      voice->tmp4_pul_tmp = voice->tmp4_pul_tmp * voice->tmp5_vsync;
      voice->tmp4_pul_tmp = ffrac_s(voice->tmp4_pul_tmp);
      out = (voice->tmp4_pul_tmp >= voice->tmp8_width) ? 1.0f : -1.0f;
      voice->tmp1_pul_phase = ffrac_s(voice->tmp1_pul_phase + voice->tmp2_pul_speed);
      
      // -- mod="sto v_osc1_tmp" dstVar=out
      voice->var_v_osc1_tmp = out;
      
      // -- mod="pul" dstVar=out
      
      // ---- mod="pul" input "freq" seq 1/1
      
      // -- mod="1" dstVar=voice->tmp14_freq
      voice->tmp14_freq = 1.0f;
      
      // -- mod="$m_detune" dstVar=voice->tmp14_freq
      voice->tmp15_seq = voice->mod_detune_cur;
      voice->tmp14_freq += voice->tmp15_seq;
      voice->tmp13_pul_speed = voice->note_speed_cur * voice->tmp14_freq;
      
      // ---- mod="pul" input "vsync" seq 1/1
      
      // -- mod="$m_vsync_2" dstVar=voice->tmp17_vsync
      voice->tmp17_vsync = voice->mod_vsync_2_cur;
      
      // -- mod="0.1" dstVar=voice->tmp17_vsync
      voice->tmp18_seq = 0.1f;
      voice->tmp17_vsync += voice->tmp18_seq;
      
      // -- mod="12" dstVar=voice->tmp17_vsync
      voice->tmp19_seq = 12.0f;
      voice->tmp17_vsync *= voice->tmp19_seq;
      
      // ---- mod="pul" input "width" seq 1/1
      
      // -- mod="$m_pw" dstVar=voice->tmp20_width
      voice->tmp20_width = voice->mod_pw_cur;
      
      // -- mod="$m_dephase" dstVar=voice->tmp20_width
      voice->tmp21_seq = voice->mod_dephase_cur;
      voice->tmp20_width += voice->tmp21_seq;
      voice->tmp16_pul_tmp = (voice->tmp12_pul_phase);
      voice->tmp16_pul_tmp = voice->tmp16_pul_tmp * voice->tmp17_vsync;
      voice->tmp16_pul_tmp = ffrac_s(voice->tmp16_pul_tmp);
      out = (voice->tmp16_pul_tmp >= voice->tmp20_width) ? 1.0f : -1.0f;
      voice->tmp12_pul_phase = ffrac_s(voice->tmp12_pul_phase + voice->tmp13_pul_speed);
      
      // -- mod="sto v_osc2_last" dstVar=out
      voice->var_v_osc2_last = out;
      
      // -- mod="$v_osc1_tmp" dstVar=out
      voice->tmp22_seq = voice->var_v_osc1_tmp;
      out += voice->tmp22_seq;
      
      // -- mod="svf" dstVar=out
      
      // ---- mod="svf" input "res" seq 1/1
      
      // -- mod="$m_res" dstVar=voice->tmp26_res
      voice->tmp26_res = voice->mod_res_cur;
      
      // ---- mod="svf" input "freq" seq 1/2
      
      // -- mod="0.5" dstVar=voice->tmp27_freq
      voice->tmp27_freq = 0.5f;
      
      // ---- mod="svf" input "freq" seq 2/2
      
      // -- mod="$m_cutoff" dstVar=voice->tmp27_freq
      voice->tmp27_freq = voice->mod_cutoff_cur;
      
      // -- mod="clp" dstVar=voice->tmp27_freq
      if(voice->tmp27_freq > 0.999f) voice->tmp27_freq = 0.999f;
      else if(voice->tmp27_freq < -0.999f) voice->tmp27_freq = -0.999f;
      voice->tmp23_svf_lp = voice->tmp23_svf_lp + (voice->tmp25_svf_bp * voice->tmp27_freq);
      voice->tmp24_svf_hp = out - voice->tmp23_svf_lp - (voice->tmp25_svf_bp * voice->tmp26_res);
      voice->tmp25_svf_bp = voice->tmp25_svf_bp + (voice->tmp24_svf_hp * voice->tmp27_freq);
      out = voice->tmp23_svf_lp;
      
      // -- mod="svf" dstVar=out
      
      // ---- mod="svf" input "freq" seq 1/2
      
      // -- mod="0.5" dstVar=voice->tmp31_freq
      voice->tmp31_freq = 0.5f;
      
      // ---- mod="svf" input "freq" seq 2/2
      
      // -- mod="$m_cutoff" dstVar=voice->tmp31_freq
      voice->tmp31_freq = voice->mod_cutoff_cur;
      
      // -- mod="1" dstVar=voice->tmp31_freq
      voice->tmp32_seq = 1.0f;
      voice->tmp31_freq -= voice->tmp32_seq;
      
      // -- mod="clp" dstVar=voice->tmp31_freq
      if(voice->tmp31_freq > 0.5f) voice->tmp31_freq = 0.5f;
      else if(voice->tmp31_freq < -0.5f) voice->tmp31_freq = -0.5f;
      
      // -- mod="0.5" dstVar=voice->tmp31_freq
      voice->tmp33_seq = 0.5f;
      voice->tmp31_freq += voice->tmp33_seq;
      voice->tmp28_svf_lp = voice->tmp28_svf_lp + (voice->tmp30_svf_bp * voice->tmp31_freq);
      voice->tmp29_svf_hp = out - voice->tmp28_svf_lp - (voice->tmp30_svf_bp * 1.0f);
      voice->tmp30_svf_bp = voice->tmp30_svf_bp + (voice->tmp29_svf_hp * voice->tmp31_freq);
      out = voice->tmp28_svf_lp;
      
      // -- mod="hbx" dstVar=out
      voice->tmp34_hbx_last = mathLerpf(voice->tmp34_hbx_last, out, 0.000106589f);
      out = out - voice->tmp34_hbx_last;
      
      // -- mod="$FLT_GAIN" dstVar=out
      voice->tmp35_seq = 0.49805f;
      out *= voice->tmp35_seq;

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
      voice->mod_cutoff_cur        += voice->mod_cutoff_inc;
      voice->mod_res_cur           += voice->mod_res_inc;
      voice->mod_pw_mod_cur        += voice->mod_pw_mod_inc;
      voice->mod_pw_cur            += voice->mod_pw_inc;
      voice->mod_detune_cur        += voice->mod_detune_inc;
      voice->mod_dephase_cur       += voice->mod_dephase_inc;
      voice->mod_vsync_1_cur       += voice->mod_vsync_1_inc;
      voice->mod_vsync_2_cur       += voice->mod_vsync_2_inc;
   } /* loop numFrames */
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   myplugin_shared_t *ret = (myplugin_shared_t *)malloc(sizeof(myplugin_shared_t));
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
   myplugin_voice_t *voice = (myplugin_voice_t *)malloc(sizeof(myplugin_voice_t));
   if(NULL != voice)
   {
      memset((void*)voice, 0, sizeof(*voice));
      voice->base.info = _info;
      voice->tmp1_pul_phase = 0.0f;
   }
   return &voice->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(myplugin_voice_t);

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
st_plugin_info_t *myplugin_init(void) {
   myplugin_info_t *ret = (myplugin_info_t *)malloc(sizeof(myplugin_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "myplugin";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "my plugin v1";
      ret->base.short_name  = "my plugin v1";
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
         return myplugin_init();
   }
   return NULL;
}
#endif // STFX_SKIP_MAIN_INIT

} // extern "C"
