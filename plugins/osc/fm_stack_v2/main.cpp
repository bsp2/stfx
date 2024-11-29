// ----
// ---- file   : main.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2023-2024 by Bastian Spiegel.
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : 4op FM stack (LORES variant)
// ----
// ---- created: 21Aug2023
// ---- changed: 22Aug2023, 23Aug2023, 24Aug2023, 25Aug2023, 26Aug2023, 01Sep2023, 03Sep2023
// ----          06Sep2023, 19Sep2023, 14Oct2024
// ----
// ----
// ----

#include "../../../plugin.h"

typedef unsigned int sUI;
typedef signed   int sSI;
typedef signed   int sBool;
typedef float        sF32;

extern st_plugin_info_t *fm_stack_init_lores     (void);
extern st_plugin_info_t *fm_stack_init_loresh    (void);
extern st_plugin_info_t *fm_stack_init_medres    (void);
extern st_plugin_info_t *fm_stack_init_medres_h  (void);
extern st_plugin_info_t *fm_stack_init_medres_hpe(void);
extern st_plugin_info_t *fm_stack_init_hires     (void);
extern st_plugin_info_t *fm_stack_init_hires_le  (void);
extern st_plugin_info_t *fm_stack_init_hires_lm  (void);
extern st_plugin_info_t *fm_stack_init_hires     (void);

#if 0  // noisy, or quality gains do not justify increased CPU usage
extern st_plugin_info_t *fm_stack_init_ulores    (void);
extern st_plugin_info_t *fm_stack_init_vlores    (void);
extern st_plugin_info_t *fm_stack_init_vhires    (void);
extern st_plugin_info_t *fm_stack_init_uhires    (void);
#endif

static sF32 loc_env_shape_lut[512*2048];  // -1..1. lin(0)@off=256*2048
static sF32 loc_vel_curve_lut[32*32*256];

extern void calc_env_shapes (sF32 *_d);

static void loc_calc_vel_curve(sF32 _c, sUI _off) {
   // see "tks-examples/tksdl/mm_curves.tks"
   const sUI num = 256u*4u;
   sF32 t = 0.0f;
   const sF32 tStep = 1.0f / (num - 1u);
   const sF32 p1x = 0.0f;
   const sF32 p1y = 0.0f;
   const sF32 p2x = 0.5f + _c*0.5f;
   const sF32 p2y = 0.5f - _c*0.5f;
   const sF32 p3x = 1.0f;
   const sF32 p3y = 1.0f;
   for(sUI k = 0u; k < num; k++)
   {
      sF32 t12x = p1x + (p2x - p1x) * t;
      sF32 t12y = p1y + (p2y - p1y) * t;
      sF32 t23x = p2x + (p3x - p2x) * t;
      sF32 t23y = p2y + (p3y - p2y) * t;
      sF32 tx = t12x + (t23x - t12x) * t;
      sF32 ty = t12y + (t23y - t12y) * t;
      sUI i = sUI(tx * 255);
      // if(i > 255u)
      //    printf("xxx calc_mm_curve: oob k=%u c=%f i=%u\n", k, _c, i);
      // ty = powf(ty, _c*5.5f+1.0f);
      loc_vel_curve_lut[_off + i] = ty;
      // if( (_off == (563 * 256)) )
      //    printf("xxx calc_mm_curve: k=%u i=%u t=%f tx=%f ty=%f\n", k, i, t, tx, ty);
      t += tStep;
   }
   loc_vel_curve_lut[_off]        = 0.0f;
   loc_vel_curve_lut[_off + 255u] = 1.0f;
}

static void loc_init_vel_curve_lut(void) {
   sUI off = 0u;
   sF32 c =  1.0f;
   sF32 cStep = 2.0f / (32*32);
   for(sUI i = 0u; i < (32u*32u); i++)
   {
      loc_calc_vel_curve(c, off);
      off += 256u;
      c -= cStep;
   }
}

const sF32 *get_vel_curve_lut(sF32 _s) {
   // 0..1 => -1..1
   _s = _s * 2.0f - 1.0f;
   sUI velCurveIdx;
   if(_s >= 0.0f)
   {
      if(_s > 1.0f)
         _s = 1.0f;
      velCurveIdx = 512u + sUI(511u * _s);
   }
   else
   {
      if(_s < -1.0f)
         _s = -1.0f;
      velCurveIdx = 512u - sUI(512u * -_s);
   }
   return &loc_vel_curve_lut[velCurveIdx << 8];
}

const sF32 *get_env_shape_lut(sF32 _s) {
   // 0..1 => -1..1
   _s = _s * 2.0f - 1.0f;
   sUI shapeIdx;
   if(_s >= 0.0f)
   {
      if(_s > 1.0f)
         _s = 1.0f;
      shapeIdx = 256u + sUI(255u * _s);
   }
   else
   {
      if(_s < -1.0f)
         _s = -1.0f;
      shapeIdx = 256u - sUI(256u * -_s);
   }
   return &loc_env_shape_lut[shapeIdx << 11];
}

extern "C" {
ST_PLUGIN_APICALL st_plugin_info_t *ST_PLUGIN_API st_plugin_init(unsigned int _pluginIdx) {
   static sBool bInit = 1;

   if(bInit)
   {
      bInit = 0;
      loc_init_vel_curve_lut();
      calc_env_shapes(loc_env_shape_lut);
   }

   switch(_pluginIdx)
   {
      case 0u:
         return fm_stack_init_lores();

      case 1u:
         return fm_stack_init_loresh();

      case 2u:
         return fm_stack_init_medres();

      case 3u:
         return fm_stack_init_medres_h();

      case 4u:
         return fm_stack_init_medres_hpe();

      case 5u:
         return fm_stack_init_hires();

      case 6u:
         return fm_stack_init_hires_le();

      case 7u:
         return fm_stack_init_hires_lm();

#if 0  // noisy, or quality gains do not justify increased CPU usage
      case 8u:
         return fm_stack_init_ulores();

      case 9u:
         return fm_stack_init_vlores();

      case 10u:
         return fm_stack_init_vhires();

      case 11u:
         return fm_stack_init_uhires();
#endif
   }
   return NULL;
}
} // extern "C"
