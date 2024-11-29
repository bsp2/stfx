// ----
// ---- file   : wave_multiplier_delay4.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2021-2024 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : pseudo phase shifter
// ----
// ---- created: 13Oct2021
// ---- changed: 21Jan2024
// ----
// ----
// ----

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../plugin.h"

#define ST_DELAY_SIZE 1024
#define ST_DELAY_MASK 1023
#include "delay.h"

#define MAX_PARTS  4

#define PARAM_DRYWET      0
#define PARAM_PAN_RAND    1
#define PARAM_LEVEL_BASE  2
#define PARAM_LEVEL_RAND  3
#define PARAM_SPEED_BASE  4
#define PARAM_SPEED_RAND  5
#define PARAM_DELAY_BASE  6
#define PARAM_DELAY_RAND  7
#define NUM_PARAMS        8
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Pan Rand",
   "Level Base",
   "Level Rand",
   "Speed Base",
   "Speed Rand",
   "Delay Base",
   "Delay Rand",
};
static float loc_param_resets[NUM_PARAMS] = {
   0.4f,  // DRYWET
   0.6f,  // PAN_RAND
   0.6f,  // LEVEL_BASE
   0.4f,  // LEVEL_RAND
   0.1f,  // SPEED_BASE
   0.03f, // SPEED_RAND
   0.5f,  // DELAY_BASE
   0.09f, // DELAY_RAND
};

#define MOD_DRYWET      0
#define MOD_PAN_RAND    1
#define MOD_LEVEL_BASE  2
#define MOD_LEVEL_RAND  3
#define MOD_SPEED_BASE  4
#define MOD_SPEED_RAND  5
#define MOD_DELAY_BASE  6
#define MOD_DELAY_RAND  7
#define NUM_MODS        8
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Pan Rand",
   "Level Base",
   "Level Rand",
   "Speed Base",
   "Speed Rand",
   "Delay Base",
   "Delay Rand",
};

typedef struct wave_multiplier_delay4_info_s {
   st_plugin_info_t base;
   float sintbl[65536];
   unsigned int lfsr_state;
} wave_multiplier_delay4_info_t;

typedef struct wave_multiplier_delay4_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} wave_multiplier_delay4_shared_t;

typedef struct wave_multiplier_delay4_part_s {
   float angle;
   float speed;
   float          speed_r;
   float          level_r;
   float          pan_r;
   float          delay_r;
   float          level[2];
   float          frame_base;
   float          max_frames;
} wave_multiplier_delay4_part_t;

typedef struct wave_multiplier_delay4_voice_s {
   st_plugin_voice_t base;
   float   mods[NUM_MODS];
   float   sample_rate;
   float   mod_drywet_cur;
   float   mod_drywet_inc;
   // // unsigned int num_parts;
   wave_multiplier_delay4_part_t parts[MAX_PARTS];
   StDelay      dly[2];  // stereo
} wave_multiplier_delay4_voice_t;


static void loc_init_sintbl(wave_multiplier_delay4_info_t *_info) {
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
   ST_PLUGIN_VOICE_CAST(wave_multiplier_delay4_voice_t);
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
   ST_PLUGIN_SHARED_CAST(wave_multiplier_delay4_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(wave_multiplier_delay4_shared_t);
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
   ST_PLUGIN_VOICE_CAST(wave_multiplier_delay4_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(wave_multiplier_delay4_shared_t);
   ST_PLUGIN_VOICE_INFO_CAST(wave_multiplier_delay4_info_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
      voice->dly[0].reset();
      voice->dly[1].reset();

      for(unsigned int partIdx = 0u; partIdx < MAX_PARTS; partIdx++)
      {
         wave_multiplier_delay4_part_t *part = &voice->parts[partIdx];

         part->angle = loc_lfsr_randf(info->lfsr_state, 65536.0f/*max*/);
         part->speed_r = loc_lfsr_randf(info->lfsr_state, 2.0f) - 1.0f;
         part->level_r = loc_lfsr_randf(info->lfsr_state, 2.0f) - 1.0f;
         part->pan_r   = loc_lfsr_randf(info->lfsr_state, 2.0f) - 1.0f;
         part->delay_r = loc_lfsr_randf(info->lfsr_state, 2.0f) - 1.0f;
      }
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(wave_multiplier_delay4_voice_t);
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
   ST_PLUGIN_VOICE_CAST(wave_multiplier_delay4_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(wave_multiplier_delay4_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   // // voice->num_parts = (unsigned int) (MAX_PARTS * (shared->params[PARAM_VOICES] + voice->mods[MOD_VOICES]));
   // // voice->num_parts = Dstplugin_clamp(voice->num_parts, 1, MAX_PARTS);

   float maxMs = (float(ST_DELAY_SIZE) / 88.200f);    // maxMs @ 88.2kHz = ~11.6099773243ms
   float maxFrames = maxMs * (voice->sample_rate / 1000.0f);

   // static int xxx = 0;
   // xxx++;

   for(unsigned int partIdx = 0u; partIdx < MAX_PARTS; partIdx++)
   {
      wave_multiplier_delay4_part_t *part = &voice->parts[partIdx];

      float pan = (shared->params[PARAM_PAN_RAND] + voice->mods[MOD_PAN_RAND]) * part->pan_r;
      pan = Dstplugin_clamp(pan, -1.0f, 1.0f);
      pan *= partIdx / float(MAX_PARTS);
      float vol = shared->params[PARAM_LEVEL_BASE] + voice->mods[MOD_LEVEL_BASE] + part->level_r * (shared->params[PARAM_LEVEL_RAND] + voice->mods[MOD_LEVEL_RAND]);
      vol = Dstplugin_clamp(vol, 0.0f, 1.0f) * (1.0f - partIdx/float(MAX_PARTS));
      float ap = ((1.0f - pan) * 0.5f);
      part->level[0/*l*/] = vol * ap;
      part->level[1/*r*/] = vol * (1.0f - ap);

      part->speed = (8.0f * (shared->params[PARAM_SPEED_BASE] + voice->mods[MOD_SPEED_BASE] + (shared->params[PARAM_SPEED_RAND] + voice->mods[MOD_SPEED_RAND]) * part->speed_r));

      part->frame_base = (shared->params[PARAM_DELAY_BASE] + voice->mods[MOD_DELAY_BASE]) * maxFrames;
      part->max_frames = maxFrames * (shared->params[PARAM_DELAY_RAND] + voice->mods[MOD_DELAY_RAND]) * part->delay_r;

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
   ST_PLUGIN_VOICE_CAST(wave_multiplier_delay4_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(wave_multiplier_delay4_shared_t);
   ST_PLUGIN_VOICE_INFO_CAST(wave_multiplier_delay4_info_t);

   unsigned int k = 0u;

   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      for(unsigned int ch = 0u; ch < 2u; ch++)
      {
         float l = _samplesIn[k];
         float out = 0.0f;

         for(unsigned int partIdx = 0u; partIdx < MAX_PARTS/*voice->num_parts*/; partIdx++)
         {
            wave_multiplier_delay4_part_t *part = &voice->parts[partIdx];

            // float off = part->frame_base + part->max_frames * info->sintbl[(unsigned short)part->angle];
            float sinAngC = info->sintbl[(unsigned short)part->angle];
            float angFrac = part->angle - (float)((int)part->angle);
            float sinAngN = info->sintbl[(unsigned short)(part->angle+1)];
            float sinAng = sinAngC + (sinAngN - sinAngC) * angFrac;
            float off = part->frame_base + part->max_frames * sinAng;

            off = Dstplugin_clamp(off, 0.0f, (float)(ST_DELAY_SIZE - 1u));
            float dly = voice->dly[ch].readLinear(off);
            out += dly * part->level[ch];

            if(1u == ch)
            {
               part->angle += part->speed;
               if(part->angle >= 65536.0f)
                  part->angle -= 65536.0f;
               else if(part->angle <= 0.0f)
                  part->angle += 65536.0f;
            }
         }

         out = l + (out - l) * voice->mod_drywet_cur;

         voice->dly[ch].push(l, 0.0f/*fb*/);

         _samplesOut[k++] = out;
      }

      // Next frame
      voice->mod_drywet_cur  += voice->mod_drywet_inc;
   }

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   wave_multiplier_delay4_shared_t *ret = (wave_multiplier_delay4_shared_t *)malloc(sizeof(wave_multiplier_delay4_shared_t));
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
   wave_multiplier_delay4_voice_t *ret = (wave_multiplier_delay4_voice_t *)malloc(sizeof(wave_multiplier_delay4_voice_t));
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
st_plugin_info_t *wave_multiplier_delay4_init(void) {
   wave_multiplier_delay4_info_t *ret = NULL;

   ret = (wave_multiplier_delay4_info_t *)malloc(sizeof(wave_multiplier_delay4_info_t));

   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp wave_multiplier_delay4";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "wave_multiplier_delay4";
      ret->base.short_name  = "wave_multiplier_delay4";
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
