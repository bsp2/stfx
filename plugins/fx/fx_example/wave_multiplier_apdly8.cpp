// ----
// ---- file   : wave_multiplier_apdly4.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2021 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : multiple randomized+modulated delay lines with allpass filter in feedback loop
// ----
// ---- created: 13Oct2021
// ---- changed: 14Oct2021
// ----
// ----
// ----

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../plugin.h"

#include "allpass.h"

#define ST_DELAY_SIZE 1024
#define ST_DELAY_MASK 1023
#include "delay.h"

#define MAX_PARTS  8
#define NUM_POLES  8

#define PARAM_DRYWET      0
#define PARAM_PAN_RAND    1
#define PARAM_LEVEL       2  // base + rand
#define PARAM_SPEED       3  // freq+delay LFO speed base+rand
#define PARAM_FREQ        4  // allpass freq base+rand
#define PARAM_DELAY       5  // delay time base+rand
#define PARAM_FB          6  // delay feedback
#define PARAM_BIAS        7  // base<=>rand bias
#define NUM_PARAMS        8
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Pan Rand",
   "Level",
   "Speed",
   "Freq",
   "Delay",
   "Feedback",
   "Bias"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,   // DRYWET
   1.0f,   // PAN_RAND
   0.6f,   // LEVEL
   0.1f,   // SPEED
   0.5f,   // FREQ
   0.5f,   // DELAY
   0.3f,   // FB
   0.5f    // BIAS
};

#define MOD_DRYWET      0
#define MOD_PAN_RAND    1
#define MOD_LEVEL_BASE  2
#define MOD_SPEED_BASE  3
#define MOD_FREQ_BASE   4
#define MOD_DELAY_BASE  5
#define MOD_FB          6
#define MOD_BIAS        7
#define NUM_MODS        8
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Pan Rand",
   "Level Base",
   "Speed Base",
   "Freq Base",
   "Delay Base",
   "Feedback",
   "Bias"
};

typedef struct wave_multiplier_apdly8_info_s {
   st_plugin_info_t base;
   float sintbl[65536];
   unsigned int lfsr_state;
} wave_multiplier_apdly8_info_t;

typedef struct wave_multiplier_apdly8_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} wave_multiplier_apdly8_shared_t;

typedef struct wave_multiplier_apdly8_part_s {
   struct {
      float angle;
      float speed;
      float speed_r;
   } lfo_freq;

   struct {
      float angle;
      float speed;
      float speed_r;
   } lfo_delay;

   float    pan_r;
   float    level_r;
   float    freq_r;
   float    delay_r;
   float    level[2];
   float    freq_base;
   float    freq_rand;
   float    delay_base;
   float    delay_rand;
   AllPass  ap[2][NUM_POLES];  // stereo, 4 poles
   StDelay  dly[2];
} wave_multiplier_apdly8_part_t;

typedef struct wave_multiplier_apdly8_voice_s {
   st_plugin_voice_t base;
   float   mods[NUM_MODS];
   float   sample_rate;
   float   mod_drywet_cur;
   float   mod_drywet_inc;
   float   fb;
   wave_multiplier_apdly8_part_t parts[MAX_PARTS];
} wave_multiplier_apdly8_voice_t;


static void loc_init_sintbl(wave_multiplier_apdly8_info_t *_info) {
   float w = (float)(6.28318530718f / 65536.0f);
   float a = 0.0f;
   for(unsigned int i = 0u; i < 65536u; i++)
   {
      _info->sintbl[i] = sinf(a);
      a += w;
   }
}

static unsigned int loc_lfsr_randi(unsigned int &state) {
   state ^= (state >> 7);
   state ^= (state << 9);
   state ^= (state >> 13);

   return state;
}

static float loc_lfsr_randf(unsigned int &state, float _max) {
   unsigned short r = loc_lfsr_randi(state) & 65535u;
   return (_max * r) / 65535.0f;
}

static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(wave_multiplier_apdly8_voice_t);
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
   ST_PLUGIN_SHARED_CAST(wave_multiplier_apdly8_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(wave_multiplier_apdly8_shared_t);
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
   ST_PLUGIN_VOICE_CAST(wave_multiplier_apdly8_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(wave_multiplier_apdly8_shared_t);
   ST_PLUGIN_VOICE_INFO_CAST(wave_multiplier_apdly8_info_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));

      for(unsigned int partIdx = 0u; partIdx < MAX_PARTS; partIdx++)
      {
         wave_multiplier_apdly8_part_t *part = &voice->parts[partIdx];

         part->lfo_freq.angle  = loc_lfsr_randf(info->lfsr_state, 65536.0f/*max*/);
         part->lfo_delay.angle = loc_lfsr_randf(info->lfsr_state, 65536.0f/*max*/);
         part->lfo_freq.speed_r  = loc_lfsr_randf(info->lfsr_state, 2.0f) - 1.0f;
         part->lfo_delay.speed_r = loc_lfsr_randf(info->lfsr_state, 2.0f) - 1.0f;
         part->level_r = loc_lfsr_randf(info->lfsr_state, 2.0f) - 1.0f;
         part->pan_r   = loc_lfsr_randf(info->lfsr_state, 2.0f) - 1.0f;
         part->freq_r  = loc_lfsr_randf(info->lfsr_state, 2.0f) - 1.0f;
         part->delay_r = loc_lfsr_randf(info->lfsr_state, 2.0f) - 1.0f;

         for(unsigned int poleIdx = 0u; poleIdx < NUM_POLES; poleIdx++)
         {
            part->dly[0].reset();
            part->dly[1].reset();
            part->ap[0][poleIdx].reset();
            part->ap[1][poleIdx].reset();
         }
      }
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(wave_multiplier_apdly8_voice_t);
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
   ST_PLUGIN_VOICE_CAST(wave_multiplier_apdly8_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(wave_multiplier_apdly8_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   static int xxx = 0;
   xxx++;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float maxMs = (float(ST_DELAY_SIZE) / 88.200f);    // maxMs @ 88.2kHz = ~11.6099773243ms
   float maxFrames = maxMs * (voice->sample_rate / 1000.0f);

   float bias = shared->params[PARAM_BIAS] + voice->mods[MOD_BIAS];
   bias = Dstplugin_clamp(bias, 0.0f, 1.0f);

   float panRand = shared->params[PARAM_PAN_RAND] + voice->mods[MOD_PAN_RAND];

   float levelBase = shared->params[PARAM_LEVEL] + voice->mods[MOD_LEVEL_BASE];
   levelBase = Dstplugin_clamp(levelBase, 0.0f, 1.0f);
   levelBase *= (0.5f + 0.5f*(1.0f - bias));

   float levelRand = shared->params[PARAM_LEVEL];
   levelRand *= 0.5f*bias;

   float speedBase = shared->params[PARAM_SPEED] + voice->mods[MOD_SPEED_BASE];
   speedBase = Dstplugin_clamp(speedBase, 0.0f, 1.0f);
   speedBase *= (0.5f + 0.5f*(1.0f - bias));

   float speedRand = shared->params[PARAM_SPEED];
   speedRand *= 0.5f*bias;

   float freqBase = shared->params[PARAM_FREQ]*2.0f + voice->mods[MOD_FREQ_BASE];
   freqBase = Dstplugin_clamp(freqBase, 0.0f, 2.0f);
   freqBase *= 0.5f + 0.5f*(1.0f - bias);
   freqBase -= 1.0f;

   float freqRand = shared->params[PARAM_FREQ];
   freqRand *= bias;

   float delayBase = shared->params[PARAM_DELAY] + voice->mods[MOD_DELAY_BASE];
   delayBase = Dstplugin_clamp(delayBase, 0.0f, 1.0f);
   delayBase *= (0.5f + 0.5f*(1.0f - bias));

   float delayRand = shared->params[PARAM_DELAY];
   delayRand *= 0.5f*bias;

   float fb = shared->params[PARAM_FB] + voice->mods[MOD_FB];
   fb = Dstplugin_clamp(fb, 0.0f, 1.0f);
   voice->fb = fb;

   for(unsigned int partIdx = 0u; partIdx < MAX_PARTS; partIdx++)
   {
      wave_multiplier_apdly8_part_t *part = &voice->parts[partIdx];

      float pan = panRand * part->pan_r;
      pan = Dstplugin_clamp(pan, -1.0f, 1.0f);
      pan *= partIdx / float(MAX_PARTS);
      
      float vol = levelBase + part->level_r * levelRand;
      vol = Dstplugin_clamp(vol, 0.0f, 1.0f) * (1.0f - partIdx/float(MAX_PARTS));
      float ap = ((1.0f - pan) * 0.5f);
      part->level[0/*l*/] = vol * ap;
      part->level[1/*r*/] = vol * (1.0f - ap);

      part->lfo_freq.speed  = (16.0f * (speedBase + (speedRand * part->lfo_freq.speed_r)));
      part->lfo_delay.speed = (16.0f * (speedBase + (speedRand * part->lfo_delay.speed_r)));

      part->freq_base = freqBase;
      part->freq_rand = part->freq_r * freqRand;

      part->delay_base = freqBase * maxFrames;
      part->delay_rand = part->delay_r * delayRand * maxFrames;

      // {
      //    if(0 == (xxx & 1023))
      //    {
      //       printf("xxx part[%u] frame_base=%f max_frames=%f pan_r=%f level=%f/%f\n", partIdx, part->frame_base, part->max_frames, part->pan_r, part->level[0], part->level[1]);
      //       fflush(stdout);
      //    }
      // }
   }
   

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc  = (modDryWet  - voice->mod_drywet_cur)  * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur  = modDryWet;
      voice->mod_drywet_inc  = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(wave_multiplier_apdly8_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(wave_multiplier_apdly8_shared_t);
   ST_PLUGIN_VOICE_INFO_CAST(wave_multiplier_apdly8_info_t);

   unsigned int k = 0u;

   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      for(unsigned int ch = 0u; ch < 2u; ch++)
      {
         float l = _samplesIn[k];
         float out = 0.0f;

         for(unsigned int partIdx = 0u; partIdx < MAX_PARTS/*voice->num_parts*/; partIdx++)
         {
            wave_multiplier_apdly8_part_t *part = &voice->parts[partIdx];

            float freq;
            {
               float sinAngC = info->sintbl[(unsigned short)part->lfo_freq.angle];
               float angFrac = part->lfo_freq.angle - (float)((int)part->lfo_freq.angle);
               float sinAngN = info->sintbl[(unsigned short)(part->lfo_freq.angle+1)];
               float sinAng = sinAngC + (sinAngN - sinAngC) * angFrac;
               freq = part->freq_base + part->freq_rand * sinAng;
            }

            float lastOut = part->dly[ch].last_out;

            float flt = l + lastOut * voice->fb;
            for(unsigned int poleIdx = 0u; poleIdx < NUM_POLES; poleIdx++)
            {
               part->ap[ch][poleIdx].setC(freq);
               flt = part->ap[ch][poleIdx].process(flt);
            }
            part->dly[ch].pushRaw(flt);

            float off;
            {
               float sinAngC = info->sintbl[(unsigned short)part->lfo_delay.angle];
               float angFrac = part->lfo_delay.angle - (float)((int)part->lfo_delay.angle);
               float sinAngN = info->sintbl[(unsigned short)(part->lfo_delay.angle+1)];
               float sinAng = sinAngC + (sinAngN - sinAngC) * angFrac;
               off = part->delay_base + part->delay_rand * sinAng;
               off = Dstplugin_clamp(off, 0.0f, (float)(ST_DELAY_SIZE - 1u));
            }
            float dly = part->dly[ch].readLinear(off);

            out += dly * part->level[ch];

            if(1u == ch)
            {
               part->lfo_freq.angle += part->lfo_freq.speed;
               if(part->lfo_freq.angle >= 65536.0f)
                  part->lfo_freq.angle -= 65536.0f;
               else if(part->lfo_freq.angle <= 0.0f)
                  part->lfo_freq.angle += 65536.0f;

               part->lfo_delay.angle += part->lfo_delay.speed;
               if(part->lfo_delay.angle >= 65536.0f)
                  part->lfo_delay.angle -= 65536.0f;
               else if(part->lfo_delay.angle <= 0.0f)
                  part->lfo_delay.angle += 65536.0f;
            }
         }

         out = l + (out - l) * voice->mod_drywet_cur;

         _samplesOut[k++] = out;
      }

      // Next frame
      voice->mod_drywet_cur  += voice->mod_drywet_inc;
   }

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   wave_multiplier_apdly8_shared_t *ret = (wave_multiplier_apdly8_shared_t *)malloc(sizeof(wave_multiplier_apdly8_shared_t));
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
   wave_multiplier_apdly8_voice_t *ret = (wave_multiplier_apdly8_voice_t *)malloc(sizeof(wave_multiplier_apdly8_voice_t));
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
st_plugin_info_t *wave_multiplier_apdly8_init(void) {
   wave_multiplier_apdly8_info_t *ret = NULL;

   ret = (wave_multiplier_apdly8_info_t *)malloc(sizeof(wave_multiplier_apdly8_info_t));

   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp wave_multiplier_apdly8";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "wave_multiplier_apdly8";
      ret->base.short_name  = "wave_multiplier_apdly8";
      ret->base.flags       = ST_PLUGIN_FLAG_FX | ST_PLUGIN_FLAG_TRUE_STEREO_OUT;
      ret->base.category    = ST_PLUGIN_CAT_CHORUS;
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
      ret->base.process_replace    = &loc_process_replace;
      ret->base.plugin_exit        = &loc_plugin_exit;

      loc_init_sintbl(ret);
      ret->lfsr_state = 0x44894489u;////(unsigned int)(shared->params[PARAM_SEED] * 65536u);
   }

   return &ret->base;
}
} // extern "C"
