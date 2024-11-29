// ----
// ---- file   : fm_stack.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2023-2024 by Bastian Spiegel.
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : 4op FM stack
// ----
// ---- created: 21Aug2023
// ---- changed: 22Aug2023, 23Aug2023, 24Aug2023, 25Aug2023, 26Aug2023, 01Sep2023, 03Sep2023
// ----          06Sep2023, 07Sep2023, 08Sep2023, 09Sep2023, 10Sep2023, 11Sep2023, 12Sep2023
// ----          13Sep2023, 16Sep2023, 19Sep2023, 20Sep2023, 21Sep2023, 11Nov2023, 30Nov2023
// ----          15Dec2023, 11Jan2024, 21Jan2024, 07Feb2024, 28Apr2024, 14Oct2024
// ----
// ----
// ----

// defined before #include (see lores.cpp, hires.cpp, medres.cpp, ..):
//
//   FMSTACK_LORES              // low resolution tables, logmul
//   FMSTACK_HIRES              // high resolution tables
//   FMSTACK_MEDRES             // medium resolution tables, logmul
//   FMSTACK_MEDRESH            // medium resolution tables
//   FMSTACK_HIRES_AENV         // per-sample-frame amp envelope (defined) OR 1000Hz rate with per-sample-frame lerp (undefined)
//   FMSTACK_HIRES_PENV         // per-sample-frame pitch envelope (defined OR 1000Hz rate with per-sample-frame lerp (undefined)
//   FMSTACK_HIRES_LMUL         // linear-complexity mul
//   FMSTACK_ULORES             // experimental
//   FMSTACK_VLORES             // experimental
//   FMSTACK_VHIRES             // experimental
//   FMSTACK_UHIRES             // experimental
//   FMSTACK_PLUGIN_ID          // unique plugin id
//   FMSTACK_PLUGIN_NAME        // plugin name (UI)
//   FMSTACK_PLUGIN_SHORT_NAME  // plugin name (UI)
//   FMSTACK_VOICE_OP_S         // op struct name
//   FMSTACK_INIT               // init function name

#define MODFM defined

// -------------------------------------------------

// When defined, use exp2f approximation for pitch envelopes (Dexp2f_fast)
#define FMSTACK_FAST_EXP2F  defined

// When defined, use exp2f approximation for ratio calculation (Dexp2f_prec)
//   (note) usually good enough, very minor detuning with some patches
#if defined(FMSTACK_LORES) || defined(FMSTACK_LORESH) || defined(FMSTACK_MEDRES) || defined(FMSTACK_MEDRESH)
#define FMSTACK_FAST_EXP2F_PREC  defined
#endif

// When defined, use logf approximation (Dlogf_fast)
//  (note) x86_64(msvc) logf has precision issues compared to arm64(clang) implementation (resulting in noise)
#define FMSTACK_FAST_LOGF       defined
#define FMSTACK_FAST_LOGF_PREC  defined

// When defined, use log2f approximation (Dlog2f_fast)
#if !defined(FMSTACK_HIRES)
#define FMSTACK_FAST_LOG2F  defined
#endif

// When defined, use log2f approximation for ratio calculation (Dlog2f_prec)
//   (note) usually good enough, very minor detuning with some patches
#if defined(FMSTACK_LORES) || defined(FMSTACK_LORESH) || defined(FMSTACK_MEDRES) || defined(FMSTACK_MEDRESH)
#define FMSTACK_FAST_LOG2F_PREC  defined
#endif

// When defined, apply high-pass filter to remove DC offset from final output
#define FMSTACK_HPF  defined


#if 0
// #undef FMSTACK_FAST_EXPF
// #undef FMSTACK_FAST_EXP2F_PREC
// #undef FMSTACK_FAST_LOGF
// #undef FMSTACK_FAST_LOGF_PREC
// #undef FMSTACK_FAST_LOG2F
// #undef FMSTACK_FAST_LOG2F_PREC
#endif

// -------------------------------------------------

#include "../../../plugin.h"

#ifdef FMSTACK_HPF
#include "biquad.h"
#endif // FMSTACK_HPF

#include <math.h>
#include <string.h>  // memset, memcpy

typedef unsigned int sUI;
typedef signed   int sSI;
typedef signed   int sBool;
typedef float        sF32;

#if 1
// debug
#include <stdlib.h>
#include <stdio.h>
#endif

// -------------------------------------------------

#ifdef FMSTACK_HIRES_LMUL
static inline float loc_lm(float a, float b) {
   stplugin_fi_t ta, tb, tr; ta.f = a; tb.f = b;
   tr.u = (ta.u & 0x7FFFffffu) + (tb.u & 0x7FFFffffu) - 0x3F780000u;
   if(tr.s < 0)
      tr.s = 0;
   tr.u &= 0x7FFFffffu;
   tr.u |= ((ta.u & 0x80000000u) ^ (tb.u & 0x80000000u));
#if 0
   if(0x7F800000u == (tr.u & 0x7F800000u))
      tr.u = 0u;
#endif
   return tr.f;
}
#define Dmulf(a,b) loc_lm(a,b)
#else
#define Dmulf(a,b) Dmulf_prec(a,b)
#endif // FMSTACK_HIRES_LMUL
#define Dmulf_prec(a,b) ((a) * (b))

// -------------------------------------------------

extern const sF32 *get_env_shape_lut (sF32 _s);
extern const sF32 *get_vel_curve_lut (sF32 _s);

// -------------------------------------------------

struct FMSTACK_VOICE_S;
typedef struct FMSTACK_VOICE_S FMSTACK_VOICE_T;

// -------------------------------------------------

// (note) don't change.
#define NUM_OPS  4u

#ifdef FMSTACK_HIRES         // ----- HIRES -----
#define TRUE_SINE          defined
#define LOGWAV_TBLSZ       16384u
#define OVERSAMPLE_FACTOR  8.0f

#elif defined(FMSTACK_VHIRES) // ----- VHIRES -----
// **experimental**
#define TRUE_SINE          defined
#define LOGWAV_TBLSZ       16384u
#define OVERSAMPLE_FACTOR  16.0f

#elif defined(FMSTACK_UHIRES) // ----- UHIRES -----
// **experimental**
#define TRUE_SINE          defined
#define LOGWAV_TBLSZ       65536u
#define OVERSAMPLE_FACTOR  32.0f

#elif defined(FMSTACK_HIRES_LMUL) // ----- HIRES(LMUL) -----
#define TRUE_SINE          defined
#define LOGWAV_TBLSZ       16384u
#define OVERSAMPLE_FACTOR  8.0f
#define USE_LMUL           defined

#elif defined(FMSTACK_VLORES) // ----- VLORES -----
// **experimental**
#define LOGMUL             defined
#define LOGWAV_TBLSZ       128u
#define EXP_TBLSZ          1024u

#elif defined(FMSTACK_ULORES) // ----- ULORES -----
// **experimental**
#define LOGMUL             defined
#define LOGWAV_TBLSZ       64u
#define EXP_TBLSZ          512u

#elif defined(FMSTACK_LORES) // ----- LORES -----

#define LOGMUL             defined
#define LOGWAV_TBLSZ       256u
#define EXP_TBLSZ          2048u
#define OVERSAMPLE_FACTOR  2.0f

#elif defined(FMSTACK_LORESH) // ---- LORESH -----
// (note) no LOGMUL
#define LOGWAV_TBLSZ       256u
#define OVERSAMPLE_FACTOR  2.0f

#elif defined(FMSTACK_MEDRESH) // ---- MEDRESH ---
// (note) no LOGMUL
#define LOGWAV_TBLSZ       1024u
#define OVERSAMPLE_FACTOR  4.0f

#else                         // ----- MEDRES ----
// decent compromise between quality and grunginess
#define LOGMUL             defined
#define LOGWAV_TBLSZ       1024u
#define EXP_TBLSZ          4096u
#define OVERSAMPLE_FACTOR  4.0f

#endif // FMSTACK_HIRES

// -------------------------------------------------
#define LOGWAV_TBLMASK    (LOGWAV_TBLSZ - 1u)

#ifdef LOGMUL
#define EXP_TBLMASK       (EXP_TBLSZ - 1u)
#endif // LOGMUL

#ifdef TRUE_SINE
#define Dsinf(a) ((sF32)::sin((a) * ST_PLUGIN_2PI))
#endif // TRUE_SINE

// -------------------------------------------------

#ifndef FMSTACK_SAVE
#undef SAVE
#endif

#ifdef SAVE
#include <stdio.h>
static FILE *fh;
static void loc_save_file_open(const char *_name) {
   fh = fopen(_name, "wb");
}
static void loc_save_lut(const float *_data) {
   // reduce from 16384 to 2048 frames
   for(sUI i = 0u ; i < LOGWAV_TBLSZ; i += 8)
   {
      sF32 f = 0.0f;
      for(sUI j = 0u; j < 8u; j++)
         f += _data[i + j];
      f *= (1.0f / 8.0f);
      fwrite((void*)&f, sizeof(float), 1, fh);
   }
}
static void loc_save_file_close(void) {
   fclose(fh);
   fh = NULL;
}
extern void save_env_shapes (void);
#endif // SAVE

// -------------------------------------------------

// (note) MRPG1_VAR selects variation
// (note) MGRP2..8 modulate resulting parameter set

#define PARAM_MGRP1_VAR              0  // 0..1 => 1..<num_var>. def=0 (1). range may be increased via zone modmatrix (wrap around <num_var>)
#define PARAM_MGRP1_AMT              0  // alias for PARAM_MGRP1_VAR (base for mod groups 2..8)
#define PARAM_MGRP2/*_LEVEL*/        1  // 0..1 => /64 .. *4 range may be increased via zone modmatrix.
#define PARAM_MGRP3/*_MATRIX_AMT*/   2  // 0..1 => /64 .. *4. mod src amounts. def=0.5 (1). range may be increased via zone modmatrix.
#define PARAM_MGRP4/*_ATK_TIME*/     3  // 0..1 => /16 .. 1 .. *4. def=0.5 (*1)
#define PARAM_MGRP5/*_DCY_TIME*/     4
#define PARAM_MGRP6/*_RLS_TIME*/     5
#define PARAM_MGRP7/*_PITCH*/        6  // 0..1 => -4..+2 oct. range may be increased via zone modmatrix.
#define PARAM_MGRP8/*_WS_AB*/        7  // 0..1 => -2..2. def=0.5 (0)

#define PARAM_NUM_VARIATIONS        8  // 0.01..0.08 => 1..8. def=0.01 (1)
#define PARAM_VARIATION_EDIT_IDX    9  // 0.01..0.08 => 1..8. def=0.01 (1)
#define PARAM_VARIATION_EDIT_LOCK  10  // 0=apply PARAM_MGRPn_xxx mix, 1=force PARAM_VARIATION_EDIT_IDX
#define PARAM_VARIATION_EFF        11  // written by synth for editor: last seen effective (normalized) variation index (0..n)
#define PARAM_MODFM_MASK           12  // 0.00..0.15 => 1..15 (bit set=use modfm alg for op)
#define PARAM_RESVD_13             13
#define PARAM_RESVD_14             14
#define PARAM_RESVD_15             15

#define PARAM_MGRP1_OP_MASK        16  // 5 bit (0..0.31 => int 0..31). def=0.31 (0b11111)
#define PARAM_MGRP2_OP_MASK        17  //
#define PARAM_MGRP3_OP_MASK        18  //
#define PARAM_MGRP4_OP_MASK        19  //
#define PARAM_MGRP5_OP_MASK        20  //
#define PARAM_MGRP6_OP_MASK        21  //
#define PARAM_MGRP7_OP_MASK        22  //
#define PARAM_MGRP8_OP_MASK        23  //

#define PARAM_MGRP1_DEST           24  // not editable (always set to "variation")
#define PARAM_MGRP2_DEST           25  // see MGRP_DST_xxx
#define PARAM_MGRP3_DEST           26
#define PARAM_MGRP4_DEST           27
#define PARAM_MGRP5_DEST           28
#define PARAM_MGRP6_DEST           29
#define PARAM_MGRP7_DEST           30
#define PARAM_MGRP8_DEST           31

#define MGRP_DST_NONE               0
#define MGRP_DST_OP_LEVEL           1
#define MGRP_DST_OP_LEVEL_ENV_AMT   2
#define MGRP_DST_OP_PITCH           3
#define MGRP_DST_OP_PITCH_ENV_AMT   4
#define MGRP_DST_OP_PHASE           5
#define MGRP_DST_MATRIX_OP_DST_AMT  6
#define MGRP_DST_AENV_ATK           7
#define MGRP_DST_AENV_HLD           8
#define MGRP_DST_AENV_DCY           9
#define MGRP_DST_AENV_SUS          10
#define MGRP_DST_AENV_RLS          11
#define MGRP_DST_AENV_DCY_RLS      12
#define MGRP_DST_PENV_ATK          13
#define MGRP_DST_PENV_HLD          14
#define MGRP_DST_PENV_DCY          15
#define MGRP_DST_PENV_SUS          16
#define MGRP_DST_PENV_RLS          17
#define MGRP_DST_PENV_DCY_RLS      18
#define MGRP_DST_ALL_ATK           19
#define MGRP_DST_ALL_HLD           20
#define MGRP_DST_ALL_DCY           21
#define MGRP_DST_ALL_SUS           22
#define MGRP_DST_ALL_RLS           23
#define MGRP_DST_ALL_DCY_RLS       24
#define MGRP_DST_WS_A              25
#define MGRP_DST_WS_B              26
#define MGRP_DST_WS_AB_MIX         27
#define MGRP_DST_OUT               28
#define MGRP_DST_NUM               29

static const char *loc_mgrp_dst_names[MGRP_DST_NUM] = {
   "-",            //  0: MGRP_DST_NONE
   "Level",        //  1: MGRP_DST_OP_LEVEL
   "AEnv Amt",     //  2: MGRP_DST_OP_LEVEL_ENV_AMT
   "Pitch",        //  3: MGRP_DST_OP_PITCH
   "PEnv Amt",     //  4: MGRP_DST_OP_PITCH_ENV_AMT
   "Phase",        //  5: MGRP_DST_OP_PHASE
   "Mat Dst Amt",  //  6: MGRP_DST_MATRIX_OP_DST_AMT
   "AEnv Atk",     //  7: MGRP_DST_AENV_ATK
   "AEnv Hld",     //  8: MGRP_DST_AENV_HLD
   "AEnv Dcy",     //  9: MGRP_DST_AENV_DCY
   "AEnv Sus",     // 10: MGRP_DST_AENV_SUS
   "AEnv Rls",     // 11: MGRP_DST_AENV_RLS
   "AEnv Dcy+Rls", // 12: MGRP_DST_AENV_RLS
   "PEnv Atk",     // 13: MGRP_DST_PENV_ATK
   "PEnv Hld",     // 14: MGRP_DST_PENV_HLD
   "PEnv Dcy",     // 15: MGRP_DST_PENV_DCY
   "PEnv Sus",     // 16: MGRP_DST_PENV_SUS
   "PEnv Rls",     // 17: MGRP_DST_PENV_RLS
   "PEnv Dcy+Rls", // 18: MGRP_DST_PENV_RLS
   "All Atk",      // 19: MGRP_DST_ENV_ATK
   "All Hld",      // 20: MGRP_DST_ENV_HLD
   "All Dcy",      // 21: MGRP_DST_ENV_DCY
   "All Sus",      // 22: MGRP_DST_ENV_SUS
   "All Rls",      // 23: MGRP_DST_ENV_RLS
   "All Dcy+Rls",  // 24: MGRP_DST_ENV_RLS
   "WS A",         // 25: MGRP_DST_WS_A
   "WS B",         // 26: MGRP_DST_WS_B
   "WS AB Mix",    // 27: MGRP_DST_WS_AB_MIX
   "Out",          // 28: MGRP_DST_OUT
};

// 8 variations follow
#define PARAM_VAR_BASE             32

#define PARAM_MGRP_MATRIX_BASE     32
#define PARAM_MGRP_OP1_AMT          0  // 0..1 => -1..1. def=1
#define PARAM_MGRP_OP2_AMT          1
#define PARAM_MGRP_OP3_AMT          2
#define PARAM_MGRP_OP4_AMT          3
#define PARAM_MGRP_OP5_AMT          4
#define PARAM_MGRP_RESVD_5          5
#define PARAM_MGRP_RESVD_6          6
#define PARAM_MGRP_RESVD_7          7
// .. 5 ops * 8 modulation groups => 8*8=64 parameters
// (note) mgrp amount parameters are currently unused

#define PARAM_OP_BASE              96
#define PARAM_OP_PHASE              0  // 0..0.99 => 0..360°, 0.999=free running 1.0=random. def=0
#define PARAM_OP_COARSE             1  // 0..1 => 0..65536 (ratio). def=0.5 (0)
#define PARAM_OP_FINE               2  // 0..1 => -1..+1 octaves (unquantized). def=0.5 (0)
#define PARAM_OP_PITCH_KBD_CTR      3  // 0..1 => -48..48. def=0.5 (0(C-5))
#define PARAM_OP_PITCH_KBD_AMT      4  // 0..1 => -200%..200%. def=0.75 (100%)   0.5(0%)=fixed op ratio
#define PARAM_OP_PITCH_ENV_AMT      5  // 0..1 => -4..+4 octaves. def=0.5 (0)
#define PARAM_OP_PITCH_ENV_VEL_AMT  6  // 0..1 => -100%..100%. def=0.5 (0)
#define PARAM_OP_LEVEL              7  // 0..1 => 0..16 (max, including mod, =64). def=0 (op2..5) or 0.0625 (1)
#define PARAM_OP_LEVEL_VEL_AMT      8  // 0..1 => -100%..100%. def=0.5 (0)
#define PARAM_OP_LEVEL_ENV_AMT      9  // 0..1 => -100%..100%. def=1 (1)
#define PARAM_OP_LEVEL_ENV_VEL_AMT 10  // 0..1 => -100%..100%. def=0.5 (0)
#define PARAM_OP_VEL_CURVE         11  // 0..1 => -1..1 log/lin/exp
#define PARAM_OP_WS_A              12  // 0=sine (op1..4 only). 0.01,0.02,..
#define PARAM_OP_WS_B              13  // 0=sine (op1..4 only)
#define PARAM_OP_WS_MIX            14  // 0=A .. 0.5=A+B .. 1=B (op1..4 only)
#define PARAM_OP_VOICEBUS          15  // 0 (off).. 0.01(1) .. 0.16(16)..0.31 (31)  (op5 only)
#define PARAM_OP_NUM               16
// .. 5 ops => 5*16=80 params ..
//  (note) op5 is restricted to LEVEL*, VEL_CURVE, VOICEBUS parameters.

#define PARAM_AMP_ENV_BASE        176
#define PARAM_ENV_ATK               0  // 0..1 => 0..10000ms
#define PARAM_ENV_HLD               1  // 0..1 => 0..10000ms
#define PARAM_ENV_DCY               2  // 0..1 => 0..10000ms
#define PARAM_ENV_SUS               3  // 0..1
#define PARAM_ENV_RLS               4  // 0..1 => 0..10000ms
#define PARAM_ENV_ATK_SHAPE         5  // 0..1 => -1..1 (log/lin/exp)
#define PARAM_ENV_DCY_SHAPE         6  // 0..1 => -1..1 (log/lin/exp)
#define PARAM_ENV_RLS_SHAPE         7  // 0..1 => -1..1 (log/lin/exp)
#define NUM_ENV_PARAMS_PER_OP       8
// .. 5 ops => 5*8=40 params ..

#define PARAM_PITCH_ENV_BASE      216
// .. 4 ops => 4*8=32 params ..

// 5 columns, 5 rows
//   op5->op4 op5->op3 op5->op2 op5->op1 op5->out
//   op4->op4 op4->op3 op4->op2 op4->op1 op4->out
//   op3->op4 op3->op3 op3->op2 op3->op1 op3->out
//   op2->op4 op2->op3 op2->op2 op2->op1 op2->out
//   op1->op4 op1->op3 op1->op2 op1->op1 op1->out
//   (note)  displayed in descending op order, stored in ascending order
#define PARAM_MATRIX_BASE         248
#define PARAM_MATRIX_OP_TO_OP1      0  // 0..1 => 0..16
#define PARAM_MATRIX_OP_TO_OP2      1
#define PARAM_MATRIX_OP_TO_OP3      2
#define PARAM_MATRIX_OP_TO_OP4      3
#define PARAM_MATRIX_OP_TO_OUT      4
#define PARAM_MATRIX_OP_RESVD_5     5
#define PARAM_MATRIX_OP_RESVD_6     6
#define PARAM_MATRIX_OP_RESVD_7     7
#define PARAM_MATRIX_OP_NUM         8
// .. 5 ops => 5*5=25 params, aligned to 32 => 5*8=40 params

#define NUM_PARAMS_PER_VAR        (288 - PARAM_VAR_BASE/*32*/)  // => 256
#define NUM_PARAMS                (PARAM_VAR_BASE + NUM_PARAMS_PER_VAR*8)  // => 2080



static const char *loc_param_names[NUM_PARAMS] = {
   "MG1-Variation",  //   0: PARAM_MGRP1_VAR
   "MG2-Level",      //   1: PARAM_MGRP2_LEVEL
   "MG3-Matrix",     //   2: PARAM_MGRP3_MATRIX_AMT
   "MG4-Decay",      //   3: PARAM_MGRP4
   "MG5",            //   4: PARAM_MGRP5
   "MG6",            //   5: PARAM_MGRP6
   "MG7",            //   6: PARAM_MGRP7
   "MG8",            //   7: PARAM_MGRP8

   "Num Variations", //   8: PARAM_NUM_VARIATIONS
   "Var.Edit Idx",   //   9: PARAM_VARIATION_EDIT_IDX
   "Var.Edit Lock",  //  10: PARAM_VARIATION_EDIT_LOCK
   "<resvd 11>",     //  11: PARAM_RESVD_11
   "<resvd 12>",     //  12: PARAM_RESVD_12
   "<resvd 13>",     //  13: PARAM_RESVD_13
   "<resvd 14>",     //  14: PARAM_RESVD_14
   "<resvd 15>",     //  15: PARAM_RESVD_15

   "MG1 Op Mask",    //  16: PARAM_MGRP1_OP_MASK
   "MG2 Op Mask",    //  17: PARAM_MGRP2_OP_MASK
   "MG3 Op Mask",    //  18: PARAM_MGRP3_OP_MASK
   "MG4 Op Mask",    //  19: PARAM_MGRP4_OP_MASK
   "MG5 Op Mask",    //  20: PARAM_MGRP5_OP_MASK
   "MG6 Op Mask",    //  21: PARAM_MGRP6_OP_MASK
   "MG7 Op Mask",    //  22: PARAM_MGRP7_OP_MASK
   "MG8 Op Mask",    //  23: PARAM_MGRP8_OP_MASK

   "MG1 Dest",       //  24: PARAM_MGRP1_DEST
   "MG2 Dest",       //  25: PARAM_MGRP2_DEST
   "MG3 Dest",       //  26: PARAM_MGRP3_DEST
   "MG4 Dest",       //  27: PARAM_MGRP4_DEST
   "MG5 Dest",       //  28: PARAM_MGRP5_DEST
   "MG6 Dest",       //  29: PARAM_MGRP6_DEST
   "MG7 Dest",       //  30: PARAM_MGRP7_DEST
   "MG8 Dest",       //  31: PARAM_MGRP8_DEST

   "MG1 Op1 Amt",    //  32: PARAM_MGRP1_OP1_AMT
   "MG1 Op2 Amt",    //  33: PARAM_MGRP1_OP2_AMT
   "MG1 Op3 Amt",    //  34: PARAM_MGRP1_OP3_AMT
   "MG1 Op4 Amt",    //  35: PARAM_MGRP1_OP4_AMT
   "MG1 Op5 Amt",    //  36: PARAM_MGRP1_OP5_AMT
   "<resvd 29>",     //  37: PARAM_MGRP1_RESVD_5
   "<resvd 30>",     //  38: PARAM_MGRP1_RESVD_6
   "<resvd 31>",     //  39: PARAM_MGRP1_RESVD_7

   "MG2 Op1 Amt",    //  40: PARAM_MGRP2_OP1_AMT
   "MG2 Op2 Amt",    //  41: PARAM_MGRP2_OP2_AMT
   "MG2 Op3 Amt",    //  42: PARAM_MGRP2_OP3_AMT
   "MG2 Op4 Amt",    //  43: PARAM_MGRP2_OP4_AMT
   "MG2 Op5 Amt",    //  44: PARAM_MGRP2_OP5_AMT
   "<resvd 37>",     //  45: PARAM_MGRP2_RESVD_5
   "<resvd 38>",     //  46: PARAM_MGRP2_RESVD_6
   "<resvd 39>",     //  47: PARAM_MGRP2_RESVD_7

   "MG3 Op1 Amt",    //  48: PARAM_MGRP3_OP1_AMT
   "MG3 Op2 Amt",    //  49: PARAM_MGRP3_OP2_AMT
   "MG3 Op3 Amt",    //  50: PARAM_MGRP3_OP3_AMT
   "MG3 Op4 Amt",    //  51: PARAM_MGRP3_OP4_AMT
   "MG3 Op5 Amt",    //  52: PARAM_MGRP3_OP5_AMT
   "<resvd 45>",     //  53: PARAM_MGRP3_RESVD_5
   "<resvd 46>",     //  54: PARAM_MGRP3_RESVD_6
   "<resvd 47>",     //  55: PARAM_MGRP3_RESVD_7

   "MG4 Op1 Amt",    //  56: PARAM_MGRP4_OP1_AMT
   "MG4 Op2 Amt",    //  57: PARAM_MGRP4_OP2_AMT
   "MG4 Op3 Amt",    //  58: PARAM_MGRP4_OP3_AMT
   "MG4 Op4 Amt",    //  59: PARAM_MGRP4_OP4_AMT
   "MG4 Op5 Amt",    //  60: PARAM_MGRP4_OP5_AMT
   "<resvd 53>",     //  61: PARAM_MGRP4_RESVD_5
   "<resvd 54>",     //  62: PARAM_MGRP4_RESVD_6
   "<resvd 55>",     //  63: PARAM_MGRP4_RESVD_7

   "MG5 Op1 Amt",    //  64: PARAM_MGRP5_OP1_AMT
   "MG5 Op2 Amt",    //  65: PARAM_MGRP5_OP2_AMT
   "MG5 Op3 Amt",    //  66: PARAM_MGRP5_OP3_AMT
   "MG5 Op4 Amt",    //  67: PARAM_MGRP5_OP4_AMT
   "MG5 Op5 Amt",    //  68: PARAM_MGRP5_OP5_AMT
   "<resvd 61>",     //  69: PARAM_MGRP5_RESVD_5
   "<resvd 62>",     //  70: PARAM_MGRP5_RESVD_6
   "<resvd 63>",     //  71: PARAM_MGRP5_RESVD_7

   "MG6 Op1 Amt",    //  72: PARAM_MGRP6_OP1_AMT
   "MG6 Op2 Amt",    //  73: PARAM_MGRP6_OP2_AMT
   "MG6 Op3 Amt",    //  74: PARAM_MGRP6_OP3_AMT
   "MG6 Op4 Amt",    //  75: PARAM_MGRP6_OP4_AMT
   "MG6 Op5 Amt",    //  76: PARAM_MGRP6_OP5_AMT
   "<resvd 69>",     //  77: PARAM_MGRP6_RESVD_5
   "<resvd 70>",     //  78: PARAM_MGRP6_RESVD_6
   "<resvd 71>",     //  79: PARAM_MGRP6_RESVD_7

   "MG7 Op1 Amt",    //  80: PARAM_MGRP7_OP1_AMT
   "MG7 Op2 Amt",    //  81: PARAM_MGRP7_OP2_AMT
   "MG7 Op3 Amt",    //  82: PARAM_MGRP7_OP3_AMT
   "MG7 Op4 Amt",    //  83: PARAM_MGRP7_OP4_AMT
   "MG7 Op5 Amt",    //  84: PARAM_MGRP7_OP5_AMT
   "<resvd 77>",     //  85: PARAM_MGRP7_RESVD_5
   "<resvd 78>",     //  86: PARAM_MGRP7_RESVD_6
   "<resvd 79>",     //  87: PARAM_MGRP7_RESVD_7

   "MG8 Op1 Amt",    //  88: PARAM_MGRP8_OP1_AMT
   "MG8 Op2 Amt",    //  89: PARAM_MGRP8_OP2_AMT
   "MG8 Op3 Amt",    //  90: PARAM_MGRP8_OP3_AMT
   "MG8 Op4 Amt",    //  91: PARAM_MGRP8_OP4_AMT
   "MG8 Op5 Amt",    //  92: PARAM_MGRP8_OP5_AMT
   "<resvd 85>",     //  93: PARAM_MGRP8_RESVD_5
   "<resvd 86>",     //  94: PARAM_MGRP8_RESVD_6
   "<resvd 87>",     //  95: PARAM_MGRP8_RESVD_7

   "Op1 Phase",      //  96: PARAM_OP1_PHASE
   "Op1 Coarse",     //  97: PARAM_OP1_COARSE
   "Op1 Fine",       //  98: PARAM_OP1_FINE
   "Op1 P. Kbd Ctr", //  99: PARAM_OP1_PITCH_KBD_CTR
   "Op1 P. Kbd Amt", // 100: PARAM_OP1_PITCH_KBD_AMT
   "Op1 P. Env Amt", // 101: PARAM_OP1_PITCH_ENV_AMT
   "Op1 P. EnvVAmt", // 102: PARAM_OP1_PITCH_ENV_VEL_AMT
   "Op1 Level",      // 103: PARAM_OP1_LEVEL
   "Op1 L. Vel Amt", // 104: PARAM_OP1_LEVEL_VEL_AMT
   "Op1 L. Env Amt", // 105: PARAM_OP1_LEVEL_ENV_AMT
   "Op1 L. EnvVAmt", // 106: PARAM_OP1_LEVEL_ENV_VEL_AMT
   "Op1 Vel Curve",  // 107: PARAM_OP1_VEL_CURVE
   "Op1 WS A"        // 108: PARAM_OP1_WS_A
   "Op1 WS B",       // 109: PARAM_OP1_WS_B
   "Op1 WS AB Mix",  // 110: PARAM_OP1_WS_AB
   "Op1 VoiceBus",   // 111: - PARAM_OP1_VOICEBUS

   "Op2 Phase",      // 112: PARAM_OP2_PHASE
   "Op2 Coarse",     // 113: PARAM_OP2_COARSE
   "Op2 Fine",       // 114: PARAM_OP2_FINE
   "Op2 P. Kbd Ctr", // 115: PARAM_OP2_PITCH_KBD_CTR
   "Op2 P. Kbd Amt", // 116: PARAM_OP2_PITCH_KBD_AMT
   "Op2 P. Env Amt", // 117: PARAM_OP2_PITCH_ENV_AMT
   "Op2 P. EnvVAmt", // 118: PARAM_OP2_PITCH_ENV_VEL_AMT
   "Op2 Level",      // 119: PARAM_OP2_LEVEL
   "Op2 L. Vel Amt", // 120: PARAM_OP2_LEVEL_VEL_AMT
   "Op2 L. Env Amt", // 121: PARAM_OP2_LEVEL_ENV_AMT
   "Op2 L. EnvVAmt", // 122: PARAM_OP2_LEVEL_ENV_VEL_AMT
   "Op2 Vel Curve",  // 123: PARAM_OP2_VEL_CURVE
   "Op2 WS A"        // 124: PARAM_OP2_WS_A
   "Op2 WS B",       // 125: PARAM_OP2_WS_B
   "Op2 WS AB Mix",  // 126: PARAM_OP2_WS_AB
   "Op2 VoiceBus",   // 127: - PARAM_OP2_VOICEBUS

   "Op3 Phase",      // 128: PARAM_OP3_PHASE
   "Op3 Coarse",     // 129: PARAM_OP3_COARSE
   "Op3 Fine",       // 130: PARAM_OP3_FINE
   "Op3 P. Kbd Ctr", // 131: PARAM_OP3_PITCH_KBD_CTR
   "Op3 P. Kbd Amt", // 132: PARAM_OP3_PITCH_KBD_AMT
   "Op3 P. Env Amt", // 133: PARAM_OP3_PITCH_ENV_AMT
   "Op3 P. EnvVAmt", // 134: PARAM_OP3_PITCH_ENV_VEL_AMT
   "Op3 Level",      // 135: PARAM_OP3_LEVEL
   "Op3 L. Vel Amt", // 136: PARAM_OP3_LEVEL_VEL_AMT
   "Op3 L. Env Amt", // 137: PARAM_OP3_LEVEL_ENV_AMT
   "Op3 L. EnvVAmt", // 138: PARAM_OP3_LEVEL_ENV_VEL_AMT
   "Op3 Vel Curve",  // 139: PARAM_OP3_VEL_CURVE
   "Op3 WS A"        // 140: PARAM_OP3_WS_A
   "Op3 WS B",       // 141: PARAM_OP3_WS_B
   "Op3 WS AB Mix",  // 142: PARAM_OP3_WS_AB
   "Op3 VoiceBus",   // 143: - PARAM_OP3_VOICEBUS

   "Op4 Phase",      // 144: PARAM_OP4_PHASE
   "Op4 Coarse",     // 145: PARAM_OP4_COARSE
   "Op4 Fine",       // 146: PARAM_OP4_FINE
   "Op4 P. Kbd Ctr", // 147: PARAM_OP4_PITCH_KBD_CTR
   "Op4 P. Kbd Amt", // 148: PARAM_OP4_PITCH_KBD_AMT
   "Op4 P. Env Amt", // 149: PARAM_OP4_PITCH_ENV_AMT
   "Op4 P. EnvVAmt", // 150: PARAM_OP4_PITCH_ENV_VEL_AMT
   "Op4 Level",      // 151: PARAM_OP4_LEVEL
   "Op4 L. Vel Amt", // 152: PARAM_OP4_LEVEL_VEL_AMT
   "Op4 L. Env Amt", // 153: PARAM_OP4_LEVEL_ENV_AMT
   "Op4 L. EnvVAmt", // 154: PARAM_OP4_LEVEL_ENV_VEL_AMT
   "Op4 Vel Curve",  // 155: PARAM_OP4_VEL_CURVE
   "Op4 WS A"        // 156: PARAM_OP4_WS_A
   "Op4 WS B",       // 157: PARAM_OP4_WS_B
   "Op4 WS AB Mix",  // 158: PARAM_OP4_WS_AB
   "Op4 VoiceBus",   // 159: - PARAM_OP4_VOICEBUS

   "Op5 Phase",      // 160: - PARAM_OP5_PHASE
   "Op5 Coarse",     // 161: - PARAM_OP5_COARSE
   "Op5 Fine",       // 162: - PARAM_OP5_FINE
   "Op5 P. Kbd Ctr", // 163: - PARAM_OP5_PITCH_KBD_CTR
   "Op5 P. Kbd Amt", // 164: - PARAM_OP5_PITCH_KBD_AMT
   "Op5 P. Env Amt", // 165: - PARAM_OP5_PITCH_ENV_AMT
   "Op5 P. EnvVAmt", // 166: - PARAM_OP5_PITCH_ENV_VEL_AMT
   "Op5 Level",      // 167: PARAM_OP5_LEVEL
   "Op5 L. Vel Amt", // 168: PARAM_OP5_LEVEL_VEL_AMT
   "Op5 L. Env Amt", // 169: PARAM_OP5_LEVEL_ENV_AMT
   "Op5 L. EnvVAmt", // 170: PARAM_OP5_LEVEL_ENV_VEL_AMT
   "Op5 Vel Curve",  // 171: PARAM_OP5_VEL_CURVE
   "Op5 WS A"        // 172: - PARAM_OP5_WS_A
   "Op5 WS B",       // 173: - PARAM_OP5_WS_B
   "Op5 WS AB Mix",  // 174: - PARAM_OP5_WS_AB
   "Op5 VoiceBus",   // 175: PARAM_OP5_VOICEBUS

   "Op1 AEnv Attack", // 176: PARAM_OP1_AMP_ENV_ATK
   "Op1 AEnv Hold",   // 177: PARAM_OP1_AMP_ENV_HOLD
   "Op1 AEnv Decay",  // 178: PARAM_OP1_AMP_ENV_DCY
   "Op1 AEnv Sustain",// 179: PARAM_OP1_AMP_ENV_SUS
   "Op1 AEnv Release",// 180: PARAM_OP1_AMP_ENV_RLS
   "Op1 AEnv A Shape",// 181: PARAM_OP1_AMP_ENV_ATK_SHAPE
   "Op1 AEnv D Shape",// 182: PARAM_OP1_AMP_ENV_DCY_SHAPE
   "Op1 AEnv R Shape",// 183: PARAM_OP1_AMP_ENV_RLS_SHAPE

   "Op2 AEnv Attack", // 184: PARAM_OP2_AMP_ENV_ATK
   "Op2 AEnv Hold",   // 185: PARAM_OP2_AMP_ENV_HOLD
   "Op2 AEnv Decay",  // 186: PARAM_OP2_AMP_ENV_DCY
   "Op2 AEnv Sustain",// 187: PARAM_OP2_AMP_ENV_SUS
   "Op2 AEnv Release",// 188: PARAM_OP2_AMP_ENV_RLS
   "Op2 AEnv A Shape",// 189: PARAM_OP2_AMP_ENV_ATK_SHAPE
   "Op2 AEnv D Shape",// 190: PARAM_OP2_AMP_ENV_DCY_SHAPE
   "Op2 AEnv R Shape",// 191: PARAM_OP2_AMP_ENV_RLS_SHAPE

   "Op3 AEnv Attack", // 192: PARAM_OP3_AMP_ENV_ATK
   "Op3 AEnv Hold",   // 193: PARAM_OP3_AMP_ENV_HOLD
   "Op3 AEnv Decay",  // 194: PARAM_OP3_AMP_ENV_DCY
   "Op3 AEnv Sustain",// 195: PARAM_OP3_AMP_ENV_SUS
   "Op3 AEnv Release",// 196: PARAM_OP3_AMP_ENV_RLS
   "Op3 AEnv A Shape",// 197: PARAM_OP3_AMP_ENV_ATK_SHAPE
   "Op3 AEnv D Shape",// 198: PARAM_OP3_AMP_ENV_DCY_SHAPE
   "Op3 AEnv R Shape",// 199: PARAM_OP3_AMP_ENV_RLS_SHAPE

   "Op4 AEnv Attack", // 200: PARAM_OP4_AMP_ENV_ATK
   "Op4 AEnv Hold",   // 201: PARAM_OP4_AMP_ENV_HOLD
   "Op4 AEnv Decay",  // 202: PARAM_OP4_AMP_ENV_DCY
   "Op4 AEnv Sustain",// 203: PARAM_OP4_AMP_ENV_SUS
   "Op4 AEnv Release",// 204: PARAM_OP4_AMP_ENV_RLS
   "Op4 AEnv A Shape",// 205: PARAM_OP4_AMP_ENV_ATK_SHAPE
   "Op4 AEnv D Shape",// 206: PARAM_OP4_AMP_ENV_DCY_SHAPE
   "Op4 AEnv R Shape",// 207: PARAM_OP4_AMP_ENV_RLS_SHAPE

   "Op5 AEnv Attack", // 208: PARAM_OP5_AMP_ENV_ATK
   "Op5 AEnv Hold",   // 209: PARAM_OP5_AMP_ENV_HOLD
   "Op5 AEnv Decay",  // 210: PARAM_OP5_AMP_ENV_DCY
   "Op5 AEnv Sustain",// 211: PARAM_OP5_AMP_ENV_SUS
   "Op5 AEnv Release",// 212: PARAM_OP5_AMP_ENV_RLS
   "Op5 AEnv A Shape",// 213: PARAM_OP5_AMP_ENV_ATK_SHAPE
   "Op5 AEnv D Shape",// 214: PARAM_OP5_AMP_ENV_DCY_SHAPE
   "Op5 AEnv R Shape",// 215: PARAM_OP5_AMP_ENV_RLS_SHAPE

   "Op1 PEnv Attack", // 216: PARAM_OP1_PITCH_ENV_ATK
   "Op1 PEnv Hold",   // 217: PARAM_OP1_PITCH_ENV_HOLD
   "Op1 PEnv Decay",  // 218: PARAM_OP1_PITCH_ENV_DCY
   "Op1 PEnv Sustain",// 219: PARAM_OP1_PITCH_ENV_SUS
   "Op1 PEnv Release",// 220: PARAM_OP1_PITCH_ENV_RLS
   "Op1 PEnv A Shape",// 221: PARAM_OP1_PITCH_ENV_ATK_SHAPE
   "Op1 PEnv D Shape",// 222: PARAM_OP1_PITCH_ENV_DCY_SHAPE
   "Op1 PEnv R Shape",// 223: PARAM_OP1_PITCH_ENV_RLS_SHAPE

   "Op2 PEnv Attack", // 224: PARAM_OP2_PITCH_ENV_ATK
   "Op2 PEnv Hold",   // 225: PARAM_OP2_PITCH_ENV_HOLD
   "Op2 PEnv Decay",  // 226: PARAM_OP2_PITCH_ENV_DCY
   "Op2 PEnv Sustain",// 227: PARAM_OP2_PITCH_ENV_SUS
   "Op2 PEnv Release",// 228: PARAM_OP2_PITCH_ENV_RLS
   "Op2 PEnv A Shape",// 229: PARAM_OP2_PITCH_ENV_ATK_SHAPE
   "Op2 PEnv D Shape",// 230: PARAM_OP2_PITCH_ENV_DCY_SHAPE
   "Op2 PEnv R Shape",// 231: PARAM_OP2_PITCH_ENV_RLS_SHAPE

   "Op3 PEnv Attack", // 232: PARAM_OP3_PITCH_ENV_ATK
   "Op3 PEnv Hold",   // 233: PARAM_OP3_PITCH_ENV_HOLD
   "Op3 PEnv Decay",  // 234: PARAM_OP3_PITCH_ENV_DCY
   "Op3 PEnv Sustain",// 235: PARAM_OP3_PITCH_ENV_SUS
   "Op3 PEnv Release",// 236: PARAM_OP3_PITCH_ENV_RLS
   "Op3 PEnv A Shape",// 237: PARAM_OP3_PITCH_ENV_ATK_SHAPE
   "Op3 PEnv D Shape",// 238: PARAM_OP3_PITCH_ENV_DCY_SHAPE
   "Op3 PEnv R Shape",// 239: PARAM_OP3_PITCH_ENV_RLS_SHAPE

   "Op4 PEnv Attack", // 240: PARAM_OP4_PITCH_ENV_ATK
   "Op4 PEnv Hold",   // 241: PARAM_OP4_PITCH_ENV_HOLD
   "Op4 PEnv Decay",  // 242: PARAM_OP4_PITCH_ENV_DCY
   "Op4 PEnv Sustain",// 243: PARAM_OP4_PITCH_ENV_SUS
   "Op4 PEnv Release",// 244: PARAM_OP4_PITCH_ENV_RLS
   "Op4 PEnv A Shape",// 245: PARAM_OP4_PITCH_ENV_ATK_SHAPE
   "Op4 PEnv D Shape",// 246: PARAM_OP4_PITCH_ENV_DCY_SHAPE
   "Op4 PEnv R Shape",// 247: PARAM_OP4_PITCH_ENV_RLS_SHAPE

   "Op1 to Op1"       // 248: PARAM_OP1_TO_OP1
   "Op1 to Op2",      // 249: PARAM_OP1_TO_OP2
   "Op1 to Op3",      // 250: PARAM_OP1_TO_OP3
   "Op1 to Op4",      // 251: PARAM_OP1_TO_OP4
   "Op1 to Out",      // 252: PARAM_OP1_TO_OUT
   "<resvd_245>",     // 253: <resvd_245>
   "<resvd_246>",     // 254: <resvd_246>
   "<resvd_247>",     // 255: <resvd_247>

   "Op2 to Op1"       // 256: PARAM_OP2_TO_OP1
   "Op2 to Op2",      // 257: PARAM_OP2_TO_OP2
   "Op2 to Op3",      // 258: PARAM_OP2_TO_OP3
   "Op2 to Op4",      // 259: PARAM_OP2_TO_OP4
   "Op2 to Out",      // 260: PARAM_OP2_TO_OUT
   "<resvd_253>",     // 261: <resvd_253>
   "<resvd_254>",     // 262: <resvd_254>
   "<resvd_255>",     // 263: <resvd_255>

   "Op3 to Op1"       // 264: PARAM_OP3_TO_OP1
   "Op3 to Op2",      // 265: PARAM_OP3_TO_OP2
   "Op3 to Op3",      // 266: PARAM_OP3_TO_OP3
   "Op3 to Op4",      // 267: PARAM_OP3_TO_OP4
   "Op3 to Out",      // 268: PARAM_OP3_TO_OUT
   "<resvd_261>",     // 269: <resvd_261>
   "<resvd_262>",     // 270: <resvd_262>
   "<resvd_263>",     // 271: <resvd_263>

   "Op4 to Op1"       // 272: PARAM_OP4_TO_OP1
   "Op4 to Op2",      // 273: PARAM_OP4_TO_OP2
   "Op4 to Op3",      // 274: PARAM_OP4_TO_OP3
   "Op4 to Op4",      // 275: PARAM_OP4_TO_OP4
   "Op4 to Out",      // 276: PARAM_OP4_TO_OUT
   "<resvd_269>",     // 277: <resvd_269>
   "<resvd_270>",     // 278: <resvd_270>
   "<resvd_271>",     // 279: <resvd_271>

   "Op5 to Op1"       // 280: PARAM_OP5_TO_OP1
   "Op5 to Op2",      // 281: PARAM_OP5_TO_OP2
   "Op5 to Op3",      // 282: PARAM_OP5_TO_OP3
   "Op5 to Op4",      // 283: PARAM_OP5_TO_OP4
   "Op5 to Out",      // 284: PARAM_OP5_TO_OUT
   "<resvd_277>",     // 285: <resvd_277>
   "<resvd_278>",     // 286: <resvd_278>
   "<resvd_279>",     // 287: <resvd_279>

};

static float loc_param_resets[NUM_PARAMS] = {
   0.0f,             //   0: PARAM_MGRP1_VAR
   0.5f,             //   1: PARAM_MGRP2_LEVEL
   0.5f,             //   2: PARAM_MGRP3_MATRIX_AMT
   0.5f,             //   3: PARAM_MGRP4_ATK_TIME
   0.5f,             //   4: PARAM_MGRP5_DCY_TIME
   0.5f,             //   5: PARAM_MGRP6_RLS_TIME
   0.5f,             //   6: PARAM_MGRP7_PITCH
   0.5f,             //   7: PARAM_MGRP8_WS_AB

   0.01f,            //   8: PARAM_NUM_VARIATIONS
   0.01f,            //   9: PARAM_VARIATION_EDIT_IDX
   0.0f,             //  10: PARAM_VARIATION_EDIT_LOCK
   0.0f,             //  11: PARAM_MODFM_MASK
   0.0f,             //  12: PARAM_RESVD_12
   0.0f,             //  13: PARAM_RESVD_13
   0.0f,             //  14: PARAM_RESVD_14
   0.0f,             //  15: PARAM_RESVD_15

   0.31f,            //  16: PARAM_MGRP1_OP_MASK
   0.30f,            //  17: PARAM_MGRP2_OP_MASK   don't scale op1 level by default
   0.31f,            //  18: PARAM_MGRP3_OP_MASK
   0.31f,            //  19: PARAM_MGRP4_OP_MASK
   0.31f,            //  20: PARAM_MGRP5_OP_MASK
   0.31f,            //  21: PARAM_MGRP6_OP_MASK
   0.31f,            //  22: PARAM_MGRP7_OP_MASK
   0.31f,            //  23: PARAM_MGRP8_OP_MASK

   0.00f,            //  24: PARAM_MGRP1_DEST  (var)
   0.01f,            //  25: PARAM_MGRP2_DEST  (op level)
   0.06f,            //  26: PARAM_MGRP3_DEST  (matrix op dst amt)
   0.24f,            //  27: PARAM_MGRP4_DEST  (all dcy+rls)
   0.00f,            //  28: PARAM_MGRP5_DEST
   0.00f,            //  29: PARAM_MGRP6_DEST
   0.00f,            //  30: PARAM_MGRP7_DEST
   0.00f,            //  31: PARAM_MGRP8_DEST

   1.0f,             //  32: PARAM_MGRP1_OP1_AMT == PARAM_VAR_BASE
   1.0f,             //  33: PARAM_MGRP1_OP2_AMT
   1.0f,             //  34: PARAM_MGRP1_OP3_AMT
   1.0f,             //  35: PARAM_MGRP1_OP4_AMT
   1.0f,             //  36: PARAM_MGRP1_OP5_AMT
   0.0f,             //  37: PARAM_MGRP1_RESVD_5
   0.0f,             //  38: PARAM_MGRP1_RESVD_6
   0.0f,             //  39: PARAM_MGRP1_RESVD_7

   1.0f,             //  40: PARAM_MGRP2_OP1_AMT
   1.0f,             //  41: PARAM_MGRP2_OP2_AMT
   1.0f,             //  42: PARAM_MGRP2_OP3_AMT
   1.0f,             //  43: PARAM_MGRP2_OP4_AMT
   1.0f,             //  44: PARAM_MGRP2_OP5_AMT
   0.0f,             //  45: PARAM_MGRP2_RESVD_5
   0.0f,             //  46: PARAM_MGRP2_RESVD_6
   0.0f,             //  47: PARAM_MGRP2_RESVD_7

   1.0f,             //  48: PARAM_MGRP3_OP1_AMT
   1.0f,             //  49: PARAM_MGRP3_OP2_AMT
   1.0f,             //  50: PARAM_MGRP3_OP3_AMT
   1.0f,             //  51: PARAM_MGRP3_OP4_AMT
   1.0f,             //  52: PARAM_MGRP3_OP5_AMT
   0.0f,             //  53: PARAM_MGRP3_RESVD_5
   0.0f,             //  54: PARAM_MGRP3_RESVD_6
   0.0f,             //  55: PARAM_MGRP3_RESVD_7

   1.0f,             //  56: PARAM_MGRP4_OP1_AMT
   1.0f,             //  57: PARAM_MGRP4_OP2_AMT
   1.0f,             //  58: PARAM_MGRP4_OP3_AMT
   1.0f,             //  59: PARAM_MGRP4_OP4_AMT
   1.0f,             //  60: PARAM_MGRP4_OP5_AMT
   0.0f,             //  61: PARAM_MGRP4_RESVD_5
   0.0f,             //  62: PARAM_MGRP4_RESVD_6
   0.0f,             //  63: PARAM_MGRP4_RESVD_7

   1.0f,             //  64: PARAM_MGRP5_OP1_AMT
   1.0f,             //  65: PARAM_MGRP5_OP2_AMT
   1.0f,             //  66: PARAM_MGRP5_OP3_AMT
   1.0f,             //  67: PARAM_MGRP5_OP4_AMT
   1.0f,             //  68: PARAM_MGRP5_OP5_AMT
   0.0f,             //  69: PARAM_MGRP5_RESVD_5
   0.0f,             //  70: PARAM_MGRP5_RESVD_6
   0.0f,             //  71: PARAM_MGRP5_RESVD_7

   1.0f,             //  72: PARAM_MGRP6_OP1_AMT
   1.0f,             //  73: PARAM_MGRP6_OP2_AMT
   1.0f,             //  74: PARAM_MGRP6_OP3_AMT
   1.0f,             //  75: PARAM_MGRP6_OP4_AMT
   1.0f,             //  76: PARAM_MGRP6_OP5_AMT
   0.0f,             //  77: PARAM_MGRP6_RESVD_5
   0.0f,             //  78: PARAM_MGRP6_RESVD_6
   0.0f,             //  79: PARAM_MGRP6_RESVD_7

   1.0f,             //  80: PARAM_MGRP7_OP1_AMT
   1.0f,             //  81: PARAM_MGRP7_OP2_AMT
   1.0f,             //  82: PARAM_MGRP7_OP3_AMT
   1.0f,             //  83: PARAM_MGRP7_OP4_AMT
   1.0f,             //  84: PARAM_MGRP7_OP5_AMT
   0.0f,             //  85: PARAM_MGRP7_RESVD_5
   0.0f,             //  86: PARAM_MGRP7_RESVD_6
   0.0f,             //  87: PARAM_MGRP7_RESVD_7

   1.0f,             //  88: PARAM_MGRP8_OP1_AMT
   1.0f,             //  89: PARAM_MGRP8_OP2_AMT
   1.0f,             //  90: PARAM_MGRP8_OP3_AMT
   1.0f,             //  91: PARAM_MGRP8_OP4_AMT
   1.0f,             //  92: PARAM_MGRP8_OP5_AMT
   0.0f,             //  93: PARAM_MGRP8_RESVD_5
   0.0f,             //  94: PARAM_MGRP8_RESVD_6
   0.0f,             //  95: PARAM_MGRP8_RESVD_7

   0.0f,             //  96: PARAM_OP1_PHASE
   (1.0f/65536.0f),  //  97: PARAM_OP1_COARSE (ratio=1)
   0.5f,             //  98: PARAM_OP1_FINE
   0.5f,             //  99: PARAM_OP1_PITCH_KBD_CTR
   0.75f,            // 100: PARAM_OP1_PITCH_KBD_AMT
   0.5f,             // 101: PARAM_OP1_PITCH_ENV_AMT
   0.5f,             // 102: PARAM_OP1_PITCH_ENV_VEL_AMT
   0.0625f,          // 103: PARAM_OP1_LEVEL
   0.5f,             // 104: PARAM_OP1_LEVEL_VEL_AMT
   1.0f,             // 105: PARAM_OP1_LEVEL_ENV_AMT
   0.5f,             // 106: PARAM_OP1_LEVEL_ENV_VEL_AMT
   0.5f,             // 107: PARAM_OP1_VEL_CURVE
   2.43f,            // 108: PARAM_OP1_WS_A  (note: _should_ have been 0.223 but wave multiplier is 100, not 1000. fix in v3!)
   0.13f,            // 109: PARAM_OP1_WS_B
   0.0f,             // 110: PARAM_OP1_WS_AB
   0.0f,             // 111: - PARAM_OP1_VOICEBUS

   0.0f,             // 112: PARAM_OP2_PHASE
   (1.0f/65536.0f),  // 113: PARAM_OP2_COARSE
   0.5f,             // 114: PARAM_OP2_FINE
   0.5f,             // 115: PARAM_OP2_PITCH_KBD_CTR
   0.75f,            // 116: PARAM_OP2_PITCH_KBD_AMT
   0.5f,             // 117: PARAM_OP2_PITCH_ENV_AMT
   0.5f,             // 118: PARAM_OP2_PITCH_ENV_VEL_AMT
   0.0625f,          // 119: PARAM_OP2_LEVEL
   0.5f,             // 120: PARAM_OP2_LEVEL_VEL_AMT
   1.0f,             // 121: PARAM_OP2_LEVEL_ENV_AMT
   0.5f,             // 122: PARAM_OP2_LEVEL_ENV_VEL_AMT
   0.5f,             // 123: PARAM_OP2_VEL_CURVE
   2.43f,            // 124: PARAM_OP2_WS_A
   0.13f,            // 125: PARAM_OP2_WS_B
   0.0f,             // 126: PARAM_OP2_WS_AB
   0.0f,             // 127: - PARAM_OP2_VOICEBUS

   0.0f,             // 128: PARAM_OP3_PHASE
   (1.0f/65536.0f),  // 129: PARAM_OP3_COARSE
   0.5f,             // 130: PARAM_OP3_FINE
   0.5f,             // 131: PARAM_OP3_PITCH_KBD_CTR
   0.75f,            // 132: PARAM_OP3_PITCH_KBD_AMT
   0.5f,             // 133: PARAM_OP3_PITCH_ENV_AMT
   0.5f,             // 134: PARAM_OP3_PITCH_ENV_VEL_AMT
   0.0625f,          // 135: PARAM_OP3_LEVEL
   0.5f,             // 136: PARAM_OP3_LEVEL_VEL_AMT
   1.0f,             // 137: PARAM_OP3_LEVEL_ENV_AMT
   0.5f,             // 138: PARAM_OP3_LEVEL_ENV_VEL_AMT
   0.5f,             // 139: PARAM_OP3_VEL_CURVE
   2.43f,            // 140: PARAM_OP3_WS_A
   0.13f,            // 141: PARAM_OP3_WS_B
   0.0f,             // 142: PARAM_OP3_WS_AB
   0.0f,             // 143: - PARAM_OP3_VOICEBUS

   0.0f,             // 144: PARAM_OP4_PHASE
   (1.0f/65536.0f),  // 145: PARAM_OP4_COARSE
   0.5f,             // 146: PARAM_OP4_FINE
   0.5f,             // 147: PARAM_OP4_PITCH_KBD_CTR
   0.75f,            // 148: PARAM_OP4_PITCH_KBD_AMT
   0.5f,             // 149: PARAM_OP4_PITCH_ENV_AMT
   0.5f,             // 150: PARAM_OP4_PITCH_ENV_VEL_AMT
   0.0625f,          // 151: PARAM_OP4_LEVEL
   0.5f,             // 152: PARAM_OP4_LEVEL_VEL_AMT
   1.0f,             // 153: PARAM_OP4_LEVEL_ENV_AMT
   0.5f,             // 154: PARAM_OP4_LEVEL_ENV_VEL_AMT
   0.5f,             // 155: PARAM_OP4_VEL_CURVE
   2.43f,            // 156: PARAM_OP4_WS_A
   0.13f,            // 157: PARAM_OP4_WS_B
   0.0f,             // 158: PARAM_OP4_WS_AB
   0.0f,             // 159: - PARAM_OP4_VOICEBUS

   0.0f,             // 160: - PARAM_OP5_PHASE
   (1.0f/65536.0f),  // 161: - PARAM_OP5_COARSE
   0.5f,             // 162: - PARAM_OP5_FINE
   0.5f,             // 163: - PARAM_OP5_PITCH_KBD_CTR
   0.75f,            // 164: - PARAM_OP5_PITCH_KBD_AMT
   0.5f,             // 165: - PARAM_OP5_PITCH_ENV_AMT
   0.5f,             // 166: - PARAM_OP5_PITCH_ENV_VEL_AMT
   0.0625f,          // 167: PARAM_OP5_LEVEL
   0.5f,             // 168: PARAM_OP5_LEVEL_VEL_AMT
   1.0f,             // 169: PARAM_OP5_LEVEL_ENV_AMT
   0.5f,             // 170: PARAM_OP5_LEVEL_ENV_VEL_AMT
   0.5f,             // 171: PARAM_OP5_VEL_CURVE
   2.43f,            // 172: - PARAM_OP5_WS_A
   0.13f,            // 173: - PARAM_OP5_WS_B
   0.0f,             // 174: - PARAM_OP5_WS_AB
   0.0f,             // 175: PARAM_OP5_VOICEBUS

   0.002f,           // 176: PARAM_OP1_AMP_ENV_ATK
   0.0f,             // 177: PARAM_OP1_AMP_ENV_HOLD
   0.020f,           // 178: PARAM_OP1_AMP_ENV_DCY
   0.6889f,          // 179: PARAM_OP1_AMP_ENV_SUS
   0.1f,             // 180: PARAM_OP1_AMP_ENV_RLS
   0.5f,             // 181: PARAM_OP1_AMP_ENV_ATK_SHAPE
   0.5f,             // 182: PARAM_OP1_AMP_ENV_DCY_SHAPE
   0.5f,             // 183: PARAM_OP1_AMP_ENV_RLS_SHAPE

   0.002f,           // 184: PARAM_OP2_AMP_ENV_ATK
   0.0f,             // 185: PARAM_OP2_AMP_ENV_HOLD
   0.020f,           // 186: PARAM_OP2_AMP_ENV_DCY
   0.6889f,          // 187: PARAM_OP2_AMP_ENV_SUS
   0.1f,             // 188: PARAM_OP2_AMP_ENV_RLS
   0.5f,             // 189: PARAM_OP2_AMP_ENV_ATK_SHAPE
   0.5f,             // 190: PARAM_OP2_AMP_ENV_DCY_SHAPE
   0.5f,             // 191: PARAM_OP2_AMP_ENV_RLS_SHAPE

   0.002f,           // 192: PARAM_OP3_AMP_ENV_ATK
   0.0f,             // 193: PARAM_OP3_AMP_ENV_HOLD
   0.020f,           // 194: PARAM_OP3_AMP_ENV_DCY
   0.6889f,          // 195: PARAM_OP3_AMP_ENV_SUS
   0.1f,             // 196: PARAM_OP3_AMP_ENV_RLS
   0.5f,             // 197: PARAM_OP3_AMP_ENV_ATK_SHAPE
   0.5f,             // 198: PARAM_OP3_AMP_ENV_DCY_SHAPE
   0.5f,             // 199: PARAM_OP3_AMP_ENV_RLS_SHAPE

   0.002f,           // 200: PARAM_OP4_AMP_ENV_ATK
   0.0f,             // 201: PARAM_OP4_AMP_ENV_HOLD
   0.020f,           // 202: PARAM_OP4_AMP_ENV_DCY
   0.6889f,          // 203: PARAM_OP4_AMP_ENV_SUS
   0.1f,             // 204: PARAM_OP4_AMP_ENV_RLS
   0.5f,             // 205: PARAM_OP4_AMP_ENV_ATK_SHAPE
   0.5f,             // 206: PARAM_OP4_AMP_ENV_DCY_SHAPE
   0.5f,             // 207: PARAM_OP4_AMP_ENV_RLS_SHAPE

   0.002f,           // 208: PARAM_OP5_AMP_ENV_ATK
   0.0f,             // 209: PARAM_OP5_AMP_ENV_HOLD
   0.020f,           // 210: PARAM_OP5_AMP_ENV_DCY
   0.6889f,          // 211: PARAM_OP5_AMP_ENV_SUS
   0.1f,             // 212: PARAM_OP5_AMP_ENV_RLS
   0.5f,             // 213: PARAM_OP5_AMP_ENV_ATK_SHAPE
   0.5f,             // 214: PARAM_OP5_AMP_ENV_DCY_SHAPE
   0.5f,             // 215: PARAM_OP5_AMP_ENV_RLS_SHAPE

   0.002f,           // 216: PARAM_OP1_PITCH_ENV_ATK
   0.0f,             // 217: PARAM_OP1_PITCH_ENV_HOLD
   0.020f,           // 218: PARAM_OP1_PITCH_ENV_DCY
   0.0f,             // 219: PARAM_OP1_PITCH_ENV_SUS
   0.1f,             // 220: PARAM_OP1_PITCH_ENV_RLS
   0.5f,             // 221: PARAM_OP1_PITCH_ENV_ATK_SHAPE
   0.5f,             // 222: PARAM_OP1_PITCH_ENV_DCY_SHAPE
   0.5f,             // 223: PARAM_OP1_PITCH_ENV_RLS_SHAPE

   0.002f,           // 224: PARAM_OP2_PITCH_ENV_ATK
   0.0f,             // 225: PARAM_OP2_PITCH_ENV_HOLD
   0.020f,           // 226: PARAM_OP2_PITCH_ENV_DCY
   0.0f,             // 227: PARAM_OP2_PITCH_ENV_SUS
   0.1f,             // 228: PARAM_OP2_PITCH_ENV_RLS
   0.5f,             // 229: PARAM_OP2_PITCH_ENV_ATK_SHAPE
   0.5f,             // 230: PARAM_OP2_PITCH_ENV_DCY_SHAPE
   0.5f,             // 231: PARAM_OP2_PITCH_ENV_RLS_SHAPE

   0.002f,           // 232: PARAM_OP3_PITCH_ENV_ATK
   0.0f,             // 233: PARAM_OP3_PITCH_ENV_HOLD
   0.020f,           // 234: PARAM_OP3_PITCH_ENV_DCY
   0.0f,             // 235: PARAM_OP3_PITCH_ENV_SUS
   0.1f,             // 236: PARAM_OP3_PITCH_ENV_RLS
   0.5f,             // 237: PARAM_OP3_PITCH_ENV_ATK_SHAPE
   0.5f,             // 238: PARAM_OP3_PITCH_ENV_DCY_SHAPE
   0.5f,             // 239: PARAM_OP3_PITCH_ENV_RLS_SHAPE

   0.002f,           // 240: PARAM_OP4_PITCH_ENV_ATK
   0.0f,             // 241: PARAM_OP4_PITCH_ENV_HOLD
   0.020f,           // 242: PARAM_OP4_PITCH_ENV_DCY
   0.0f,             // 243: PARAM_OP4_PITCH_ENV_SUS
   0.1f,             // 244: PARAM_OP4_PITCH_ENV_RLS
   0.5f,             // 245: PARAM_OP4_PITCH_ENV_ATK_SHAPE
   0.5f,             // 246: PARAM_OP4_PITCH_ENV_DCY_SHAPE
   0.5f,             // 247: PARAM_OP4_PITCH_ENV_RLS_SHAPE

   0.0f,             // 248: PARAM_OP1_TO_OP1 == PARAM_MATRIX_BASE
   0.0f,             // 249: PARAM_OP1_TO_OP2
   0.0f,             // 250: PARAM_OP1_TO_OP3
   0.0f,             // 251: PARAM_OP1_TO_OP4
   0.0625f,          // 252: PARAM_OP1_TO_OUT
   0.0f,             // 253: <resvd_245>
   0.0f,             // 254: <resvd_246>
   0.0f,             // 255: <resvd_247>

   0.0625f,          // 256: PARAM_OP2_TO_OP1
   0.0f,             // 257: PARAM_OP2_TO_OP2
   0.0f,             // 258: PARAM_OP2_TO_OP3
   0.0f,             // 259: PARAM_OP2_TO_OP4
   0.0f,             // 260: PARAM_OP2_TO_OUT
   0.0f,             // 261: <resvd_253>
   0.0f,             // 262: <resvd_254>
   0.0f,             // 263: <resvd_255>

   0.0f,             // 264: PARAM_OP3_TO_OP1
   0.0f,             // 265: PARAM_OP3_TO_OP2
   0.0f,             // 266: PARAM_OP3_TO_OP3
   0.0f,             // 267: PARAM_OP3_TO_OP4
   0.0f,             // 268: PARAM_OP3_TO_OUT
   0.0f,             // 269: <resvd_261>
   0.0f,             // 270: <resvd_262>
   0.0f,             // 271: <resvd_263>

   0.0f,             // 272: PARAM_OP4_TO_OP1
   0.0f,             // 273: PARAM_OP4_TO_OP2
   0.0f,             // 274: PARAM_OP4_TO_OP3
   0.0f,             // 275: PARAM_OP4_TO_OP4
   0.0f,             // 276: PARAM_OP4_TO_OUT
   0.0f,             // 277: <resvd_269>
   0.0f,             // 278: <resvd_270>
   0.0f,             // 279: <resvd_271>

   0.0f,             // 280: PARAM_OP5_TO_OP1
   0.0f,             // 281: PARAM_OP5_TO_OP2
   0.0f,             // 282: PARAM_OP5_TO_OP3
   0.0f,             // 283: PARAM_OP5_TO_OP4
   0.0f,             // 284: PARAM_OP5_TO_OUT
   0.0f,             // 285: <resvd_277>
   0.0f,             // 286: <resvd_278>
   0.0f,             // 287: <resvd_279>
};

#define MOD_MGRP1_VAR            0
#define MOD_MGRP1_AMT            0  // alias for  MOD_MGRP1_VAR (base for mod groups 2..8)
#define MOD_MGRP2_LEVEL          1
#define MOD_MGRP3_MATRIX_AMT     2
#define MOD_MGRP4_ATK_TIME       3
#define MOD_MGRP5_DCY_TIME       4
#define MOD_MGRP6_RLS_TIME       5
#define MOD_MGRP7_PITCH          6
#define MOD_MGRP8_WS_AB          7
#define NUM_MODS                 8
static const char *loc_mod_names[NUM_MODS] = {
   "MG1-Variation",  // 0: MOD_MGRP1_VAR
   "MG2-Level",      // 1: MOD_MGRP2_LEVEL
   "MG3-Matrix",     // 2: MOD_MGRP3_MATRIX_AMT
   "MG4-Release",    // 3: MOD_MGRP4
   "MG5",            // 4: MOD_MGRP5
   "MG6",            // 5: MOD_MGRP6
   "MG7",            // 6: MOD_MGRP7
   "MG8",            // 7: MOD_MGRP8
};

static const char *loc_wave_names[] = {
   "  0: zero",      //   0
   "  1: sin",       //   1
   "  2: tx2",       //   2
   "  3: tx3",       //   3
   "  4: tx4",       //   4
   "  5: tx5",       //   5
   "  6: tx6",       //   6
   "  7: tx7",       //   7
   "  8: tx8",       //   8
   "  9: tri",       //   9
   " 10: tri90",     //  10
   " 11: tri180",    //  11
   " 12: tri270",    //  12
   " 13: p1",        //  13
   " 14: p2",        //  14
   " 15: p3",        //  15
   " 16: p4",        //  16
   " 17: p5",        //  17
   " 18: p6",        //  18
   " 19: p7",        //  19
   " 20: p8",        //  20
   " 21: p9",        //  21
   " 22: p10",       //  22
   " 23: p11",       //  23
   " 24: p12",       //  24
   " 25: p13",       //  25
   " 26: p14",       //  26
   " 27: p15",       //  27
   " 28: p16",       //  28
   " 29: p17",       //  29
   " 30: p18",       //  30
   " 31: p19",       //  31
   " 32: p20",       //  32
   " 33: ep1",       //  33
   " 34: ep2",       //  34
   " 35: ep3",       //  35
   " 36: ep4",       //  36
   " 37: ep5",       //  37
   " 38: ep6",       //  38
   " 39: ep7",       //  39
   " 40: ep8",       //  40
   " 41: ep9",       //  41
   " 42: ep10",      //  42
   " 43: ep11",      //  43
   " 44: ep12",      //  44
   " 45: ep13",      //  45
   " 46: ep14",      //  46
   " 47: ep15",      //  47
   " 48: ep16",      //  48
   " 49: ep17",      //  49
   " 50: ep18",      //  50
   " 51: ep19",      //  51
   " 52: ep20",      //  52
   " 53: sq1",       //  53
   " 54: sq2",       //  54
   " 55: sq3",       //  55
   " 56: sq4",       //  56
   " 57: sq5",       //  57
   " 58: sq6",       //  58
   " 59: sq7",       //  59
   " 60: sq8",       //  60
   " 61: sq9",       //  61
   " 62: sq10",      //  62
   " 63: gap1",      //  63
   " 64: gap2",      //  64
   " 65: gap3",      //  65
   " 66: gap4",      //  66
   " 67: gap5",      //  67
   " 68: gap6",      //  68
   " 69: gap7",      //  69
   " 70: gap8",      //  70
   " 71: gap9",      //  71
   " 72: gap10",     //  72
   " 73: dbl1",      //  73
   " 74: dbl2",      //  74
   " 75: dbl3",      //  75
   " 76: dbl4",      //  76
   " 77: dbl5",      //  77
   " 78: dbl6",      //  78
   " 79: dbl7",      //  79
   " 80: dbl8",      //  80
   " 81: dbl9",      //  81
   " 82: dbl10",     //  82
   " 83: sync1",     //  83
   " 84: sync2",     //  84
   " 85: sync3",     //  85
   " 86: sync4",     //  86
   " 87: sync5",     //  87
   " 88: sync6",     //  88
   " 89: sync7",     //  89
   " 90: sync8",     //  90
   " 91: sync9",     //  91
   " 92: sync10",    //  92
   " 93: sync11",    //  93
   " 94: sync12",    //  94
   " 95: sync13",    //  95
   " 96: sync14",    //  96
   " 97: sync15",    //  97
   " 98: sync16",    //  98
   " 99: sync17",    //  99
   "100: sync18",    // 100
   "101: sync19",    // 101
   "102: sync20",    // 102
   "103: drive1",    // 103
   "104: drive2",    // 104
   "105: drive3",    // 105
   "106: drive4",    // 106
   "107: drive5",    // 107
   "108: drive6",    // 108
   "109: drive7",    // 109
   "110: drive8",    // 110
   "111: drive9",    // 111
   "112: drive10",   // 112
   "113: drive11",   // 113
   "114: drive12",   // 114
   "115: drive13",   // 115
   "116: drive14",   // 116
   "117: drive15",   // 117
   "118: drive16",   // 118
   "119: drive17",   // 119
   "120: drive18",   // 120
   "121: drive19",   // 121
   "122: drive20",   // 122
   "123: sawup1",    // 123
   "124: sawup2",    // 124
   "125: sawup3",    // 125
   "126: sawup4",    // 126
   "127: sawup5",    // 127
   "128: sawup6",    // 128
   "129: sawup7",    // 129
   "130: sawup8",    // 130
   "131: sawup9",    // 131
   "132: sawup10",   // 132
   "133: sawdn1",    // 133
   "134: sawdn2",    // 134
   "135: sawdn3",    // 135
   "136: sawdn4",    // 136
   "137: sawdn5",    // 137
   "138: sawdn6",    // 138
   "139: sawdn7",    // 139
   "140: sawdn8",    // 140
   "141: sawdn9",    // 141
   "142: sawdn10",   // 142
   "143: br1",       // 143
   "144: br2",       // 144
   "145: br3",       // 145
   "146: br4",       // 146
   "147: br5",       // 147
   "148: br6",       // 148
   "149: br7",       // 149
   "150: br8",       // 150
   "151: br9",       // 151
   "152: br10",      // 152
   "153: br11",      // 153
   "154: br12",      // 154
   "155: br13",      // 155
   "156: br14",      // 156
   "157: br15",      // 157
   "158: br16",      // 158
   "159: br17",      // 159
   "160: br18",      // 160
   "161: br19",      // 161
   "162: br20",      // 162
   "163: sr1",       // 163
   "164: sr2",       // 164
   "165: sr3",       // 165
   "166: sr4",       // 166
   "167: sr5",       // 167
   "168: sr6",       // 168
   "169: sr7",       // 169
   "170: sr8",       // 170
   "171: sr9",       // 171
   "172: sr10",      // 172
   "173: sr11",      // 173
   "174: sr12",      // 174
   "175: sr13",      // 175
   "176: sr14",      // 176
   "177: sr15",      // 177
   "178: sr16",      // 178
   "179: sr17",      // 179
   "180: sr18",      // 180
   "181: sr19",      // 181
   "182: sr20",      // 182
   "183: sawsq1",    // 183
   "184: sawsq2",    // 184
   "185: sawsq3",    // 185
   "186: sawsq4",    // 186
   "187: sawsq5",    // 187
   "188: sawsq6",    // 188
   "189: sawsq7",    // 189
   "190: sawsq8",    // 190
   "191: sawsq9",    // 191
   "192: sawsq10",   // 192
   "193: sawsq11",   // 193
   "194: sawsq12",   // 194
   "195: sawsq13",   // 195
   "196: sawsq14",   // 196
   "197: sawsq15",   // 197
   "198: sawsq16",   // 198
   "199: sawsq17",   // 199
   "200: sawsq18",   // 200
   "201: sawsq19",   // 201
   "202: sawsq20",   // 202
   "203: bend1",     // 203
   "204: bend2",     // 204
   "205: bend3",     // 205
   "206: bend4",     // 206
   "207: bend5",     // 207
   "208: bend6",     // 208
   "209: bend7",     // 209
   "210: bend8",     // 210
   "211: bend9",     // 211
   "212: bend10",    // 212
   "213: bend11",    // 213
   "214: bend12",    // 214
   "215: bend13",    // 215
   "216: bend14",    // 216
   "217: bend15",    // 217
   "218: bend16",    // 218
   "219: bend17",    // 219
   "220: bend18",    // 220
   "221: bend19",    // 221
   "222: bend20",    // 222
   "223: warp1",     // 223
   "224: warp2",     // 224
   "225: warp3",     // 225
   "226: warp4",     // 226
   "227: warp5",     // 227
   "228: warp6",     // 228
   "229: warp7",     // 229
   "230: warp8",     // 230
   "231: warp9",     // 231
   "232: warp10",    // 232
   "233: warp11",    // 233
   "234: warp12",    // 234
   "235: warp13",    // 235
   "236: warp14",    // 236
   "237: warp15",    // 237
   "238: warp16",    // 238
   "239: warp17",    // 239
   "240: warp18",    // 240
   "241: warp19",    // 241
   "242: warp20",    // 242
   "243: phase1",    // 243
   "244: phase2",    // 244
   "245: phase3",    // 245
   "246: phase4",    // 246
   "247: phase5",    // 247
   "248: phase6",    // 248
   "249: phase7",    // 249
   "250: phase8",    // 250
   "251: phase9",    // 251
   "252: phase10",   // 252
   "253: phase11",   // 253
   "254: phase12",   // 254
   "255: phase13",   // 255
   "256: phase14",   // 256
   "257: phase15",   // 257
   "258: phase16",   // 258
   "259: phase17",   // 259
   "260: phase18",   // 260
   "261: phase19",   // 261
   "262: phase20",   // 262
};

// -------------------------------------------------

#define loop(X)  for(sUI i = 0u; i < (X); i++)
#define clamp(a,b,c) (((a)<(b))?(b):(((a)>(c))?(c):(a)))

static inline float mathLerpf(const float _a, const float _b, const float _t) { return _a + (_b - _a) * _t; }
static inline float mathClampf(const float a, const float b, const float c) { return (((a)<(b))?(b):(((a)>(c))?(c):(a))); }
static inline float mathMinf(const float a, const float b) { return (a<b)?a:b; }
static inline float mathMaxf(const float a, const float b) { return (a>b)?a:b; }
static inline float mathAbsMaxf(const float _x, const float _y) { return ( ( (_x<0.0f)?-_x:_x)>((_y<0.0f)?-_y:_y)?_x:_y ); }
static inline float mathAbsMinf(const float _x, const float _y) { return ( ((_x<0.0f)?-_x:_x)<((_y<0.0f)?-_y:_y)?_x:_y ); }

static inline float winLinear(const float *_s, const float _index) {
   const int idx = (int)_index;
   float r = _index - (float)idx;
   return mathLerpf(_s[idx], _s[idx+1], r);
}

static float ffrac_s(const float _f) { int i; if(_f >= 0.0f) { i = (int)_f; return _f - (float)i; } else { i = (int)-_f; return 1.0f - (-_f - (float)i); } }

#ifdef FMSTACK_FAST_EXP2F
static float fixup1 = 0.5f + 0.116f;  // 0.616 f
static float fixup2 = 0.5f - 0.116f;  // 0.384 f
static float fast_exp2f(const float _x) {
   int intX = (int)_x;
   float fracX = _x - (float)intX;
   if(_x < 0.0f)
      intX--;
   stplugin_fi_t r;
   r.s = (intX + 127) << 23;
   if(_x < 0.0f)
   {
      fracX = 1.0f + fracX;
   }
   fracX = fracX*fixup1 + fracX*fracX*fixup2;
   r.s |= int(fracX * 8388607) & 8388607;
   return r.f;
}
#define Dexp2f_fast(a) float(fast_exp2f(a))
#ifdef FMSTACK_FAST_EXP2F_PREC
#define Dexp2f_prec(a) fast_exp2f(a)
#else
// called once per block (1ms rate)
#define Dexp2f_prec(a) exp2f(a)
#endif // FMSTACK_FAST_EXP2F_PREC
#else
// math.h
#define Dexp2f_fast(a) exp2f(a)
#define Dexp2f_prec(a) exp2f(a)
#endif // FMSTACK_FAST_EXP2F

#if defined(FMSTACK_FAST_LOGF) || defined(FMSTACK_FAST_LOGF_PREC)
static sF32 APIC_mathLogf(const sF32 _x) {
   // portable approximation of logf()
   //   (note) based on public domain ln() code by Lingdong Huang <https://gist.github.com/LingDong-/7e4c4cae5cbbc44400a05fba65f06f23>
   union {
      float f;
      sUI   ui;
   } bx;
   bx.f = _x;
   const sUI ex = bx.ui >> 23;
   const sSI t = sSI(ex) - sSI(127);
   const sUI s = (t < 0) ? (-t) : t;
   bx.ui = 1065353216u | (bx.ui & 8388607u);
   return
#if 0
      /* less accurate */
      -1.49278f + (2.11263f + (-0.729104f + 0.10969f*bx.f)*bx.f)*bx.f
#else
      /* OR more accurate */
      -1.7417939f + (2.8212026f + (-1.4699568f + (0.44717955f - 0.056570851f * bx.f)*bx.f)*bx.f)*bx.f
#endif
      + 0.6931471806f * t;
}
#endif

#ifdef FMSTACK_FAST_LOGF
#define Dlogf APIC_mathLogf
#else
#define Dlogf logf
#endif // FMSTACK_FAST_LOGF

#ifdef FMSTACK_FAST_LOGF_PREC
#define Dlogf_prec APIC_mathLogf
#else
#define Dlogf_prec logf
#endif // FMSTACK_FAST_LOGF


#ifdef FMSTACK_FAST_LOG2F
// see <https://stackoverflow.com/questions/9411823/fast-log2float-x-implementation-c>
static float mFast_Log2(const float val) {
   union { float val; int x; } u = { val };
   float log_2 = (float)(((u.x >> 23) & 255) - 128);
   u.x   &= ~(255 << 23);
   u.x   += 127 << 23;
   // // log_2 += ((-0.3358287811f) * u.val + 2.0f) * u.val  -0.65871759316667f;
   log_2 += ((-0.34484843f) * u.val + 2.02466578f) * u.val  - 0.67487759f;
   return log_2;
}
#define Dlog2f_fast(a) mFast_Log2(a)

#ifdef FMSTACK_FAST_LOG2F_PREC
#define Dlog2f_prec(a) mFast_Log2(a)
#else
#define Dlog2f_prec(a) log2f(a)
#endif // FMSTACK_FAST_LOG2F_PREC

#else
#define Dlog2f_fast(a) log2f(a)
#define Dlog2f_prec(a) log2f(a)
#endif // FMSTACK_FAST_LOG2F


#ifdef LOGMUL

#if defined(_MSC_VER) || defined(__x86_64__)
// Intel i7 13700H
#ifdef FMSTACK_FAST_LOGF_PREC
// #define LOGMUL_MIN 1e-05f
#define LOGMUL_MIN 1e-03f
#else
#define LOGMUL_MIN 1e-03f
#endif // FMSTACK_FAST_LOGF_PREC
#else
// Apple M2pro
#define LOGMUL_MIN 1e-07f
#endif // _MSC_VER

static sF32 loc_level_to_lut_log(const sF32 _f) {
   sF32 out = _f + 1.0f;

   if(out > 2.0f) out = 2.0f;
   else if(out < LOGMUL_MIN) out = LOGMUL_MIN;

   out = Dlogf_prec(out);

   return out;
}
#endif // LOGMUL

static float loc_sine_tbl_f[16384];

static float loc_lut_logzero       [LOGWAV_TBLSZ];
static float loc_lut_logsin        [LOGWAV_TBLSZ];
static float loc_lut_logsin_2      [LOGWAV_TBLSZ];
static float loc_lut_logsin_3      [LOGWAV_TBLSZ];
static float loc_lut_logsin_4      [LOGWAV_TBLSZ];
static float loc_lut_logsin_5      [LOGWAV_TBLSZ];
static float loc_lut_logsin_6      [LOGWAV_TBLSZ];
static float loc_lut_logsin_7      [LOGWAV_TBLSZ];
static float loc_lut_logsin_8      [LOGWAV_TBLSZ];
static float loc_lut_logtri        [LOGWAV_TBLSZ];
static float loc_lut_logtri90      [LOGWAV_TBLSZ];
static float loc_lut_logtri180     [LOGWAV_TBLSZ];
static float loc_lut_logtri270     [LOGWAV_TBLSZ];
static float loc_lut_logsinp1      [LOGWAV_TBLSZ];
static float loc_lut_logsinp2      [LOGWAV_TBLSZ];
static float loc_lut_logsinp3      [LOGWAV_TBLSZ];
static float loc_lut_logsinp4      [LOGWAV_TBLSZ];
static float loc_lut_logsinp5      [LOGWAV_TBLSZ];
static float loc_lut_logsinp6      [LOGWAV_TBLSZ];
static float loc_lut_logsinp7      [LOGWAV_TBLSZ];
static float loc_lut_logsinp8      [LOGWAV_TBLSZ];
static float loc_lut_logsinp9      [LOGWAV_TBLSZ];
static float loc_lut_logsinp10     [LOGWAV_TBLSZ];
static float loc_lut_logsinp11     [LOGWAV_TBLSZ];
static float loc_lut_logsinp12     [LOGWAV_TBLSZ];
static float loc_lut_logsinp13     [LOGWAV_TBLSZ];
static float loc_lut_logsinp14     [LOGWAV_TBLSZ];
static float loc_lut_logsinp15     [LOGWAV_TBLSZ];
static float loc_lut_logsinp16     [LOGWAV_TBLSZ];
static float loc_lut_logsinp17     [LOGWAV_TBLSZ];
static float loc_lut_logsinp18     [LOGWAV_TBLSZ];
static float loc_lut_logsinp19     [LOGWAV_TBLSZ];
static float loc_lut_logsinp20     [LOGWAV_TBLSZ];
static float loc_lut_logsinpe1     [LOGWAV_TBLSZ];
static float loc_lut_logsinpe2     [LOGWAV_TBLSZ];
static float loc_lut_logsinpe3     [LOGWAV_TBLSZ];
static float loc_lut_logsinpe4     [LOGWAV_TBLSZ];
static float loc_lut_logsinpe5     [LOGWAV_TBLSZ];
static float loc_lut_logsinpe6     [LOGWAV_TBLSZ];
static float loc_lut_logsinpe7     [LOGWAV_TBLSZ];
static float loc_lut_logsinpe8     [LOGWAV_TBLSZ];
static float loc_lut_logsinpe9     [LOGWAV_TBLSZ];
static float loc_lut_logsinpe10    [LOGWAV_TBLSZ];
static float loc_lut_logsinpe11    [LOGWAV_TBLSZ];
static float loc_lut_logsinpe12    [LOGWAV_TBLSZ];
static float loc_lut_logsinpe13    [LOGWAV_TBLSZ];
static float loc_lut_logsinpe14    [LOGWAV_TBLSZ];
static float loc_lut_logsinpe15    [LOGWAV_TBLSZ];
static float loc_lut_logsinpe16    [LOGWAV_TBLSZ];
static float loc_lut_logsinpe17    [LOGWAV_TBLSZ];
static float loc_lut_logsinpe18    [LOGWAV_TBLSZ];
static float loc_lut_logsinpe19    [LOGWAV_TBLSZ];
static float loc_lut_logsinpe20    [LOGWAV_TBLSZ];
static float loc_lut_logsinsq1     [LOGWAV_TBLSZ];
static float loc_lut_logsinsq2     [LOGWAV_TBLSZ];
static float loc_lut_logsinsq3     [LOGWAV_TBLSZ];
static float loc_lut_logsinsq4     [LOGWAV_TBLSZ];
static float loc_lut_logsinsq5     [LOGWAV_TBLSZ];
static float loc_lut_logsinsq6     [LOGWAV_TBLSZ];
static float loc_lut_logsinsq7     [LOGWAV_TBLSZ];
static float loc_lut_logsinsq8     [LOGWAV_TBLSZ];
static float loc_lut_logsinsq9     [LOGWAV_TBLSZ];
static float loc_lut_logsinsq10    [LOGWAV_TBLSZ];
static float loc_lut_logsinsg1     [LOGWAV_TBLSZ];
static float loc_lut_logsinsg2     [LOGWAV_TBLSZ];
static float loc_lut_logsinsg3     [LOGWAV_TBLSZ];
static float loc_lut_logsinsg4     [LOGWAV_TBLSZ];
static float loc_lut_logsinsg5     [LOGWAV_TBLSZ];
static float loc_lut_logsinsg6     [LOGWAV_TBLSZ];
static float loc_lut_logsinsg7     [LOGWAV_TBLSZ];
static float loc_lut_logsinsg8     [LOGWAV_TBLSZ];
static float loc_lut_logsinsg9     [LOGWAV_TBLSZ];
static float loc_lut_logsinsg10    [LOGWAV_TBLSZ];
static float loc_lut_logsinsd1     [LOGWAV_TBLSZ];
static float loc_lut_logsinsd2     [LOGWAV_TBLSZ];
static float loc_lut_logsinsd3     [LOGWAV_TBLSZ];
static float loc_lut_logsinsd4     [LOGWAV_TBLSZ];
static float loc_lut_logsinsd5     [LOGWAV_TBLSZ];
static float loc_lut_logsinsd6     [LOGWAV_TBLSZ];
static float loc_lut_logsinsd7     [LOGWAV_TBLSZ];
static float loc_lut_logsinsd8     [LOGWAV_TBLSZ];
static float loc_lut_logsinsd9     [LOGWAV_TBLSZ];
static float loc_lut_logsinsd10    [LOGWAV_TBLSZ];
static float loc_lut_logsinsy1     [LOGWAV_TBLSZ];
static float loc_lut_logsinsy2     [LOGWAV_TBLSZ];
static float loc_lut_logsinsy3     [LOGWAV_TBLSZ];
static float loc_lut_logsinsy4     [LOGWAV_TBLSZ];
static float loc_lut_logsinsy5     [LOGWAV_TBLSZ];
static float loc_lut_logsinsy6     [LOGWAV_TBLSZ];
static float loc_lut_logsinsy7     [LOGWAV_TBLSZ];
static float loc_lut_logsinsy8     [LOGWAV_TBLSZ];
static float loc_lut_logsinsy9     [LOGWAV_TBLSZ];
static float loc_lut_logsinsy10    [LOGWAV_TBLSZ];
static float loc_lut_logsinsy11    [LOGWAV_TBLSZ];
static float loc_lut_logsinsy12    [LOGWAV_TBLSZ];
static float loc_lut_logsinsy13    [LOGWAV_TBLSZ];
static float loc_lut_logsinsy14    [LOGWAV_TBLSZ];
static float loc_lut_logsinsy15    [LOGWAV_TBLSZ];
static float loc_lut_logsinsy16    [LOGWAV_TBLSZ];
static float loc_lut_logsinsy17    [LOGWAV_TBLSZ];
static float loc_lut_logsinsy18    [LOGWAV_TBLSZ];
static float loc_lut_logsinsy19    [LOGWAV_TBLSZ];
static float loc_lut_logsinsy20    [LOGWAV_TBLSZ];
static float loc_lut_logsindr1     [LOGWAV_TBLSZ];
static float loc_lut_logsindr2     [LOGWAV_TBLSZ];
static float loc_lut_logsindr3     [LOGWAV_TBLSZ];
static float loc_lut_logsindr4     [LOGWAV_TBLSZ];
static float loc_lut_logsindr5     [LOGWAV_TBLSZ];
static float loc_lut_logsindr6     [LOGWAV_TBLSZ];
static float loc_lut_logsindr7     [LOGWAV_TBLSZ];
static float loc_lut_logsindr8     [LOGWAV_TBLSZ];
static float loc_lut_logsindr9     [LOGWAV_TBLSZ];
static float loc_lut_logsindr10    [LOGWAV_TBLSZ];
static float loc_lut_logsindr11    [LOGWAV_TBLSZ];
static float loc_lut_logsindr12    [LOGWAV_TBLSZ];
static float loc_lut_logsindr13    [LOGWAV_TBLSZ];
static float loc_lut_logsindr14    [LOGWAV_TBLSZ];
static float loc_lut_logsindr15    [LOGWAV_TBLSZ];
static float loc_lut_logsindr16    [LOGWAV_TBLSZ];
static float loc_lut_logsindr17    [LOGWAV_TBLSZ];
static float loc_lut_logsindr18    [LOGWAV_TBLSZ];
static float loc_lut_logsindr19    [LOGWAV_TBLSZ];
static float loc_lut_logsindr20    [LOGWAV_TBLSZ];
static float loc_lut_logsawup1     [LOGWAV_TBLSZ];
static float loc_lut_logsawup2     [LOGWAV_TBLSZ];
static float loc_lut_logsawup3     [LOGWAV_TBLSZ];
static float loc_lut_logsawup4     [LOGWAV_TBLSZ];
static float loc_lut_logsawup5     [LOGWAV_TBLSZ];
static float loc_lut_logsawup6     [LOGWAV_TBLSZ];
static float loc_lut_logsawup7     [LOGWAV_TBLSZ];
static float loc_lut_logsawup8     [LOGWAV_TBLSZ];
static float loc_lut_logsawup9     [LOGWAV_TBLSZ];
static float loc_lut_logsawup10    [LOGWAV_TBLSZ];
static float loc_lut_logsawdown1   [LOGWAV_TBLSZ];
static float loc_lut_logsawdown2   [LOGWAV_TBLSZ];
static float loc_lut_logsawdown3   [LOGWAV_TBLSZ];
static float loc_lut_logsawdown4   [LOGWAV_TBLSZ];
static float loc_lut_logsawdown5   [LOGWAV_TBLSZ];
static float loc_lut_logsawdown6   [LOGWAV_TBLSZ];
static float loc_lut_logsawdown7   [LOGWAV_TBLSZ];
static float loc_lut_logsawdown8   [LOGWAV_TBLSZ];
static float loc_lut_logsawdown9   [LOGWAV_TBLSZ];
static float loc_lut_logsawdown10  [LOGWAV_TBLSZ];
static float loc_lut_logsinbr1     [LOGWAV_TBLSZ];
static float loc_lut_logsinbr2     [LOGWAV_TBLSZ];
static float loc_lut_logsinbr3     [LOGWAV_TBLSZ];
static float loc_lut_logsinbr4     [LOGWAV_TBLSZ];
static float loc_lut_logsinbr5     [LOGWAV_TBLSZ];
static float loc_lut_logsinbr6     [LOGWAV_TBLSZ];
static float loc_lut_logsinbr7     [LOGWAV_TBLSZ];
static float loc_lut_logsinbr8     [LOGWAV_TBLSZ];
static float loc_lut_logsinbr9     [LOGWAV_TBLSZ];
static float loc_lut_logsinbr10    [LOGWAV_TBLSZ];
static float loc_lut_logsinbr11    [LOGWAV_TBLSZ];
static float loc_lut_logsinbr12    [LOGWAV_TBLSZ];
static float loc_lut_logsinbr13    [LOGWAV_TBLSZ];
static float loc_lut_logsinbr14    [LOGWAV_TBLSZ];
static float loc_lut_logsinbr15    [LOGWAV_TBLSZ];
static float loc_lut_logsinbr16    [LOGWAV_TBLSZ];
static float loc_lut_logsinbr17    [LOGWAV_TBLSZ];
static float loc_lut_logsinbr18    [LOGWAV_TBLSZ];
static float loc_lut_logsinbr19    [LOGWAV_TBLSZ];
static float loc_lut_logsinbr20    [LOGWAV_TBLSZ];
static float loc_lut_logsinsr1     [LOGWAV_TBLSZ];
static float loc_lut_logsinsr2     [LOGWAV_TBLSZ];
static float loc_lut_logsinsr3     [LOGWAV_TBLSZ];
static float loc_lut_logsinsr4     [LOGWAV_TBLSZ];
static float loc_lut_logsinsr5     [LOGWAV_TBLSZ];
static float loc_lut_logsinsr6     [LOGWAV_TBLSZ];
static float loc_lut_logsinsr7     [LOGWAV_TBLSZ];
static float loc_lut_logsinsr8     [LOGWAV_TBLSZ];
static float loc_lut_logsinsr9     [LOGWAV_TBLSZ];
static float loc_lut_logsinsr10    [LOGWAV_TBLSZ];
static float loc_lut_logsinsr11    [LOGWAV_TBLSZ];
static float loc_lut_logsinsr12    [LOGWAV_TBLSZ];
static float loc_lut_logsinsr13    [LOGWAV_TBLSZ];
static float loc_lut_logsinsr14    [LOGWAV_TBLSZ];
static float loc_lut_logsinsr15    [LOGWAV_TBLSZ];
static float loc_lut_logsinsr16    [LOGWAV_TBLSZ];
static float loc_lut_logsinsr17    [LOGWAV_TBLSZ];
static float loc_lut_logsinsr18    [LOGWAV_TBLSZ];
static float loc_lut_logsinsr19    [LOGWAV_TBLSZ];
static float loc_lut_logsinsr20    [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq1  [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq2  [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq3  [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq4  [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq5  [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq6  [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq7  [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq8  [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq9  [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq10 [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq11 [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq12 [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq13 [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq14 [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq15 [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq16 [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq17 [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq18 [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq19 [LOGWAV_TBLSZ];
static float loc_lut_logsinsawsq20 [LOGWAV_TBLSZ];
static float loc_lut_logsinbd1     [LOGWAV_TBLSZ];
static float loc_lut_logsinbd2     [LOGWAV_TBLSZ];
static float loc_lut_logsinbd3     [LOGWAV_TBLSZ];
static float loc_lut_logsinbd4     [LOGWAV_TBLSZ];
static float loc_lut_logsinbd5     [LOGWAV_TBLSZ];
static float loc_lut_logsinbd6     [LOGWAV_TBLSZ];
static float loc_lut_logsinbd7     [LOGWAV_TBLSZ];
static float loc_lut_logsinbd8     [LOGWAV_TBLSZ];
static float loc_lut_logsinbd9     [LOGWAV_TBLSZ];
static float loc_lut_logsinbd10    [LOGWAV_TBLSZ];
static float loc_lut_logsinbd11    [LOGWAV_TBLSZ];
static float loc_lut_logsinbd12    [LOGWAV_TBLSZ];
static float loc_lut_logsinbd13    [LOGWAV_TBLSZ];
static float loc_lut_logsinbd14    [LOGWAV_TBLSZ];
static float loc_lut_logsinbd15    [LOGWAV_TBLSZ];
static float loc_lut_logsinbd16    [LOGWAV_TBLSZ];
static float loc_lut_logsinbd17    [LOGWAV_TBLSZ];
static float loc_lut_logsinbd18    [LOGWAV_TBLSZ];
static float loc_lut_logsinbd19    [LOGWAV_TBLSZ];
static float loc_lut_logsinbd20    [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp1   [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp2   [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp3   [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp4   [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp5   [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp6   [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp7   [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp8   [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp9   [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp10  [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp11  [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp12  [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp13  [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp14  [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp15  [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp16  [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp17  [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp18  [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp19  [LOGWAV_TBLSZ];
static float loc_lut_logsinwarp20  [LOGWAV_TBLSZ];
static float loc_lut_logsinph1     [LOGWAV_TBLSZ];
static float loc_lut_logsinph2     [LOGWAV_TBLSZ];
static float loc_lut_logsinph3     [LOGWAV_TBLSZ];
static float loc_lut_logsinph4     [LOGWAV_TBLSZ];
static float loc_lut_logsinph5     [LOGWAV_TBLSZ];
static float loc_lut_logsinph6     [LOGWAV_TBLSZ];
static float loc_lut_logsinph7     [LOGWAV_TBLSZ];
static float loc_lut_logsinph8     [LOGWAV_TBLSZ];
static float loc_lut_logsinph9     [LOGWAV_TBLSZ];
static float loc_lut_logsinph10    [LOGWAV_TBLSZ];
static float loc_lut_logsinph11    [LOGWAV_TBLSZ];
static float loc_lut_logsinph12    [LOGWAV_TBLSZ];
static float loc_lut_logsinph13    [LOGWAV_TBLSZ];
static float loc_lut_logsinph14    [LOGWAV_TBLSZ];
static float loc_lut_logsinph15    [LOGWAV_TBLSZ];
static float loc_lut_logsinph16    [LOGWAV_TBLSZ];
static float loc_lut_logsinph17    [LOGWAV_TBLSZ];
static float loc_lut_logsinph18    [LOGWAV_TBLSZ];
static float loc_lut_logsinph19    [LOGWAV_TBLSZ];
static float loc_lut_logsinph20    [LOGWAV_TBLSZ];

#define NUM_WAVES 263
static float *loc_waves[NUM_WAVES] = {
   loc_lut_logzero,         //   0
   loc_lut_logsin,          //   1
   loc_lut_logsin_2,        //   2
   loc_lut_logsin_3,        //   3
   loc_lut_logsin_4,        //   4
   loc_lut_logsin_5,        //   5
   loc_lut_logsin_6,        //   6
   loc_lut_logsin_7,        //   7
   loc_lut_logsin_8,        //   8
   loc_lut_logtri,          //   9
   loc_lut_logtri90,        //  10
   loc_lut_logtri180,       //  11
   loc_lut_logtri270,       //  12
   loc_lut_logsinp1,        //  13
   loc_lut_logsinp2,        //  14
   loc_lut_logsinp3,        //  15
   loc_lut_logsinp4,        //  16
   loc_lut_logsinp5,        //  17
   loc_lut_logsinp6,        //  18
   loc_lut_logsinp7,        //  19
   loc_lut_logsinp8,        //  20
   loc_lut_logsinp9,        //  21
   loc_lut_logsinp10,       //  22
   loc_lut_logsinp11,       //  23
   loc_lut_logsinp12,       //  24
   loc_lut_logsinp13,       //  25
   loc_lut_logsinp14,       //  26
   loc_lut_logsinp15,       //  27
   loc_lut_logsinp16,       //  28
   loc_lut_logsinp17,       //  29
   loc_lut_logsinp18,       //  30
   loc_lut_logsinp19,       //  31
   loc_lut_logsinp20,       //  32
   loc_lut_logsinpe1,       //  33
   loc_lut_logsinpe2,       //  34
   loc_lut_logsinpe3,       //  35
   loc_lut_logsinpe4,       //  36
   loc_lut_logsinpe5,       //  37
   loc_lut_logsinpe6,       //  38
   loc_lut_logsinpe7,       //  39
   loc_lut_logsinpe8,       //  40
   loc_lut_logsinpe9,       //  41
   loc_lut_logsinpe10,      //  42
   loc_lut_logsinpe11,      //  43
   loc_lut_logsinpe12,      //  44
   loc_lut_logsinpe13,      //  45
   loc_lut_logsinpe14,      //  46
   loc_lut_logsinpe15,      //  47
   loc_lut_logsinpe16,      //  48
   loc_lut_logsinpe17,      //  49
   loc_lut_logsinpe18,      //  50
   loc_lut_logsinpe19,      //  51
   loc_lut_logsinpe20,      //  52
   loc_lut_logsinsq1,       //  53
   loc_lut_logsinsq2,       //  54
   loc_lut_logsinsq3,       //  55
   loc_lut_logsinsq4,       //  56
   loc_lut_logsinsq5,       //  57
   loc_lut_logsinsq6,       //  58
   loc_lut_logsinsq7,       //  59
   loc_lut_logsinsq8,       //  60
   loc_lut_logsinsq9,       //  61
   loc_lut_logsinsq10,      //  62
   loc_lut_logsinsg1,       //  63
   loc_lut_logsinsg2,       //  64
   loc_lut_logsinsg3,       //  65
   loc_lut_logsinsg4,       //  66
   loc_lut_logsinsg5,       //  67
   loc_lut_logsinsg6,       //  68
   loc_lut_logsinsg7,       //  69
   loc_lut_logsinsg8,       //  70
   loc_lut_logsinsg9,       //  71
   loc_lut_logsinsg10,      //  72
   loc_lut_logsinsd1,       //  73
   loc_lut_logsinsd2,       //  74
   loc_lut_logsinsd3,       //  75
   loc_lut_logsinsd4,       //  76
   loc_lut_logsinsd5,       //  77
   loc_lut_logsinsd6,       //  78
   loc_lut_logsinsd7,       //  79
   loc_lut_logsinsd8,       //  80
   loc_lut_logsinsd9,       //  81
   loc_lut_logsinsd10,      //  82
   loc_lut_logsinsy1,       //  83
   loc_lut_logsinsy2,       //  84
   loc_lut_logsinsy3,       //  85
   loc_lut_logsinsy4,       //  86
   loc_lut_logsinsy5,       //  87
   loc_lut_logsinsy6,       //  88
   loc_lut_logsinsy7,       //  89
   loc_lut_logsinsy8,       //  90
   loc_lut_logsinsy9,       //  91
   loc_lut_logsinsy10,      //  92
   loc_lut_logsinsy11,      //  93
   loc_lut_logsinsy12,      //  94
   loc_lut_logsinsy13,      //  95
   loc_lut_logsinsy14,      //  96
   loc_lut_logsinsy15,      //  97
   loc_lut_logsinsy16,      //  98
   loc_lut_logsinsy17,      //  99
   loc_lut_logsinsy18,      // 100
   loc_lut_logsinsy19,      // 101
   loc_lut_logsinsy20,      // 102
   loc_lut_logsindr1,       // 103
   loc_lut_logsindr2,       // 104
   loc_lut_logsindr3,       // 105
   loc_lut_logsindr4,       // 106
   loc_lut_logsindr5,       // 107
   loc_lut_logsindr6,       // 108
   loc_lut_logsindr7,       // 109
   loc_lut_logsindr8,       // 110
   loc_lut_logsindr9,       // 111
   loc_lut_logsindr10,      // 112
   loc_lut_logsindr11,      // 113
   loc_lut_logsindr12,      // 114
   loc_lut_logsindr13,      // 115
   loc_lut_logsindr14,      // 116
   loc_lut_logsindr15,      // 117
   loc_lut_logsindr16,      // 118
   loc_lut_logsindr17,      // 119
   loc_lut_logsindr18,      // 120
   loc_lut_logsindr19,      // 121
   loc_lut_logsindr20,      // 122
   loc_lut_logsawup1,       // 123
   loc_lut_logsawup2,       // 124
   loc_lut_logsawup3,       // 125
   loc_lut_logsawup4,       // 126
   loc_lut_logsawup5,       // 127
   loc_lut_logsawup6,       // 128
   loc_lut_logsawup7,       // 129
   loc_lut_logsawup8,       // 130
   loc_lut_logsawup9,       // 131
   loc_lut_logsawup10,      // 132
   loc_lut_logsawdown1,     // 133
   loc_lut_logsawdown2,     // 134
   loc_lut_logsawdown3,     // 135
   loc_lut_logsawdown4,     // 136
   loc_lut_logsawdown5,     // 137
   loc_lut_logsawdown6,     // 138
   loc_lut_logsawdown7,     // 139
   loc_lut_logsawdown8,     // 140
   loc_lut_logsawdown9,     // 141
   loc_lut_logsawdown10,    // 142
   loc_lut_logsinbr1,       // 143
   loc_lut_logsinbr2,       // 144
   loc_lut_logsinbr3,       // 145
   loc_lut_logsinbr4,       // 146
   loc_lut_logsinbr5,       // 147
   loc_lut_logsinbr6,       // 148
   loc_lut_logsinbr7,       // 149
   loc_lut_logsinbr8,       // 150
   loc_lut_logsinbr9,       // 151
   loc_lut_logsinbr10,      // 152
   loc_lut_logsinbr11,      // 153
   loc_lut_logsinbr12,      // 154
   loc_lut_logsinbr13,      // 155
   loc_lut_logsinbr14,      // 156
   loc_lut_logsinbr15,      // 157
   loc_lut_logsinbr16,      // 158
   loc_lut_logsinbr17,      // 159
   loc_lut_logsinbr18,      // 160
   loc_lut_logsinbr19,      // 161
   loc_lut_logsinbr20,      // 162
   loc_lut_logsinsr1,       // 163
   loc_lut_logsinsr2,       // 164
   loc_lut_logsinsr3,       // 165
   loc_lut_logsinsr4,       // 166
   loc_lut_logsinsr5,       // 167
   loc_lut_logsinsr6,       // 168
   loc_lut_logsinsr7,       // 169
   loc_lut_logsinsr8,       // 170
   loc_lut_logsinsr9,       // 171
   loc_lut_logsinsr10,      // 172
   loc_lut_logsinsr11,      // 173
   loc_lut_logsinsr12,      // 174
   loc_lut_logsinsr13,      // 175
   loc_lut_logsinsr14,      // 176
   loc_lut_logsinsr15,      // 177
   loc_lut_logsinsr16,      // 178
   loc_lut_logsinsr17,      // 179
   loc_lut_logsinsr18,      // 180
   loc_lut_logsinsr19,      // 181
   loc_lut_logsinsr20,      // 182
   loc_lut_logsinsawsq1,    // 183
   loc_lut_logsinsawsq2,    // 184
   loc_lut_logsinsawsq3,    // 185
   loc_lut_logsinsawsq4,    // 186
   loc_lut_logsinsawsq5,    // 187
   loc_lut_logsinsawsq6,    // 188
   loc_lut_logsinsawsq7,    // 189
   loc_lut_logsinsawsq8,    // 190
   loc_lut_logsinsawsq9,    // 191
   loc_lut_logsinsawsq10,   // 192
   loc_lut_logsinsawsq11,   // 193
   loc_lut_logsinsawsq12,   // 194
   loc_lut_logsinsawsq13,   // 195
   loc_lut_logsinsawsq14,   // 196
   loc_lut_logsinsawsq15,   // 197
   loc_lut_logsinsawsq16,   // 198
   loc_lut_logsinsawsq17,   // 199
   loc_lut_logsinsawsq18,   // 200
   loc_lut_logsinsawsq19,   // 201
   loc_lut_logsinsawsq20,   // 202
   loc_lut_logsinbd1,       // 203
   loc_lut_logsinbd2,       // 204
   loc_lut_logsinbd3,       // 205
   loc_lut_logsinbd4,       // 206
   loc_lut_logsinbd5,       // 207
   loc_lut_logsinbd6,       // 208
   loc_lut_logsinbd7,       // 209
   loc_lut_logsinbd8,       // 210
   loc_lut_logsinbd9,       // 211
   loc_lut_logsinbd10,      // 212
   loc_lut_logsinbd11,      // 213
   loc_lut_logsinbd12,      // 214
   loc_lut_logsinbd13,      // 215
   loc_lut_logsinbd14,      // 216
   loc_lut_logsinbd15,      // 217
   loc_lut_logsinbd16,      // 218
   loc_lut_logsinbd17,      // 219
   loc_lut_logsinbd18,      // 220
   loc_lut_logsinbd19,      // 221
   loc_lut_logsinbd20,      // 222
   loc_lut_logsinwarp1,     // 223
   loc_lut_logsinwarp2,     // 224
   loc_lut_logsinwarp3,     // 225
   loc_lut_logsinwarp4,     // 226
   loc_lut_logsinwarp5,     // 227
   loc_lut_logsinwarp6,     // 228
   loc_lut_logsinwarp7,     // 229
   loc_lut_logsinwarp8,     // 230
   loc_lut_logsinwarp9,     // 231
   loc_lut_logsinwarp10,    // 232
   loc_lut_logsinwarp11,    // 233
   loc_lut_logsinwarp12,    // 234
   loc_lut_logsinwarp13,    // 235
   loc_lut_logsinwarp14,    // 236
   loc_lut_logsinwarp15,    // 237
   loc_lut_logsinwarp16,    // 238
   loc_lut_logsinwarp17,    // 239
   loc_lut_logsinwarp18,    // 240
   loc_lut_logsinwarp19,    // 241
   loc_lut_logsinwarp20,    // 242
   loc_lut_logsinph1,       // 243
   loc_lut_logsinph2,       // 244
   loc_lut_logsinph3,       // 245
   loc_lut_logsinph4,       // 246
   loc_lut_logsinph5,       // 247
   loc_lut_logsinph6,       // 248
   loc_lut_logsinph7,       // 249
   loc_lut_logsinph8,       // 250
   loc_lut_logsinph9,       // 251
   loc_lut_logsinph10,      // 252
   loc_lut_logsinph11,      // 253
   loc_lut_logsinph12,      // 254
   loc_lut_logsinph13,      // 255
   loc_lut_logsinph14,      // 256
   loc_lut_logsinph15,      // 257
   loc_lut_logsinph16,      // 258
   loc_lut_logsinph17,      // 259
   loc_lut_logsinph18,      // 260
   loc_lut_logsinph19,      // 261
   loc_lut_logsinph20,      // 262
};

#ifdef LOGMUL
static float loc_lut_exp[EXP_TBLSZ];
#endif // LOGMUL

static float mathPowerf(float _x, float _y) {
   // used for LUT pre-calculation
   float r;
   if(_y != 0.0f)
   {
      if(_x < 0.0f)
      {
         r = (float)( -expf(_y*Dlogf(-_x)) );
      }
      else if(_x > 0.0f)
      {
         r = (float)( expf(_y*Dlogf(_x)) );
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

#if 0
static float mathLogLinExpf(const float _f, const float _c) {
   // c: <0: log
   //     0: lin
   //    >0: exp
   stplugin_fi_t uSign;
   uSign.f = _f;
   stplugin_fi_t u;
   u.f = _f;
   u.ui &= 0x7fffFFFFu;
   u.f = powf(u.f, powf(2.0f, _c));
   u.ui |= uSign.ui & 0x80000000u;
   return u.f;
}
#endif

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

static sF32 loc_lut_modfm_exp[16384];

static void loc_calc_lut_modfm_exp(void) {
   sF32 x = 0.0f;
   sF32 *d = loc_lut_modfm_exp;
   for(sUI i = 0u; i < 16384; i++)
   {
      d[i] = expf(x * 128.0f - 64.0f);
      x += 6.10389e-05f;
   }
}

typedef struct env_params_s {
   sUI  atk_num_frames;
   sUI  hld_num_frames;
   sUI  dcy_num_frames;
   sF32 sus_level;
   sUI  rls_num_frames;

   void update(const sF32 _sampleRate,
               const sF32 _atkMS,
               const sF32 _hldMS,
               const sF32 _dcyMS,
               const sF32 _susLevel,
               const sF32 _rlsMS
               ) {
      const sF32 sr = _sampleRate * (1.0f / 1000.0f);
      atk_num_frames = sUI(sr * _atkMS);
      hld_num_frames = sUI(sr * _hldMS);
      dcy_num_frames = sUI(sr * _dcyMS);
      sus_level      = _susLevel;
      rls_num_frames = sUI(sr * _rlsMS);
   }

} env_params_t;

typedef struct env_s {
   static const sUI SEG_ATK = 0u;
   static const sUI SEG_HLD = 1u;
   static const sUI SEG_DCY = 2u;
   static const sUI SEG_SUS = 3u;
   static const sUI SEG_RLS = 4u;
   static const sUI SEG_END = 5u;

   sUI seg_idx;
   sUI seg_frame_idx;

   sF32 last_seg_level;
   sF32 last_out;

   const float *lut_atk_shape;  // 256 entries (0..1)
   const float *lut_dcy_shape;
   const float *lut_rls_shape;

   void noteOn(void) {
      seg_idx        = SEG_ATK;
      seg_frame_idx  = 0u;
      last_seg_level = 0.0f;
      lut_atk_shape  = NULL;
      lut_dcy_shape  = NULL;
      lut_rls_shape  = NULL;
   }

   void noteOff(void) {
      seg_idx        = SEG_RLS;
      seg_frame_idx  = 0u;
      last_seg_level = last_out;
   }

   static sF32 ApplyShape(const float *_lut, const sF32 _t) {
      const sF32 idxF = _t * 2047.0f;
      const sUI idx = sUI(idxF);
      if(idx >= 2047u)
      {
         return _t * _lut[2047];
      }
      else
      {
         const sF32 amtB = idxF - idx;
         sF32 s = _lut[idx];
         s = s + (_lut[idx + 1u] - s) * amtB;
         return s;
      }
   }

   sF32 step(const sUI _numFrames, const env_params_t *_params) {
      sF32 r = 0.0f;
      sF32 t;

      if(SEG_END == seg_idx)
         return 0.0f;

      if(SEG_ATK == seg_idx)
      {
         if(_params->atk_num_frames > 0u)
         {
            r = float(seg_frame_idx) / float(_params->atk_num_frames);
            r = ApplyShape(lut_atk_shape, 1.0f - r);

            seg_frame_idx += _numFrames;
            if(seg_frame_idx >= _params->atk_num_frames)
            {
               last_seg_level = 1.0f;
               seg_idx        = SEG_HLD;
               seg_frame_idx  = 0u;
            }
         }
         else
         {
            last_seg_level = 1.0f;
            seg_idx        = SEG_HLD;
         }
      }

      if(SEG_HLD == seg_idx)
      {
         r = last_seg_level;

         if(_params->hld_num_frames > 0u)
         {
            seg_frame_idx += _numFrames;
            if(seg_frame_idx >= _params->hld_num_frames)
            {
               seg_idx        = SEG_DCY;
               seg_frame_idx  = 0u;
            }
         }
         else
         {
            seg_idx = SEG_DCY;
         }
      }

      if(SEG_DCY == seg_idx)
      {
         if(_params->dcy_num_frames > 0u)
         {
            t = float(seg_frame_idx) / float(_params->dcy_num_frames);
            const sF32 s = 1.0f - ApplyShape(lut_dcy_shape, t);
            r = last_seg_level + (_params->sus_level - last_seg_level) * s;

            seg_frame_idx += _numFrames;
            if(seg_frame_idx >= _params->dcy_num_frames)
            {
               last_seg_level = _params->sus_level;
               seg_idx        = SEG_SUS;
               seg_frame_idx  = 0u;
            }
         }
         else
         {
            seg_idx = SEG_DCY;
            r = last_seg_level = _params->sus_level;
         }
      }

      if(SEG_SUS == seg_idx)
      {
         r = last_seg_level = _params->sus_level;
      }

      if(SEG_RLS == seg_idx)
      {
         if(_params->rls_num_frames > 0u)
         {
            t = float(seg_frame_idx) / float(_params->rls_num_frames);
            const sF32 s = ApplyShape(lut_rls_shape, t);
            r = last_seg_level * s;

            seg_frame_idx += _numFrames;
            if(seg_frame_idx >= _params->rls_num_frames)
            {
               last_seg_level = r;
               seg_idx        = SEG_END;
               seg_frame_idx  = 0u;
            }
         }
         else
         {
            seg_idx = SEG_END;
         }
      }

      last_out = r;
      return r;
   }

} env_t;

typedef struct FMSTACK_INFO_S {
   st_plugin_info_t base;
} FMSTACK_INFO_T;

typedef struct FMSTACK_SHARED_S {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} FMSTACK_SHARED_T;

typedef struct FMSTACK_VOICE_OP_S {

   sBool b_active;  // updated in loc_prepare() / prepareBlock()

   float mod_speed_cur;
   float mod_speed_inc;

   float mod_level_cur;
   float mod_level_inc;

#ifdef LOGMUL
   float mod_loglevel_cur;
   float mod_loglevel_inc;
#endif // LOGMUL

   float mod_wav_mix_cur;
   float mod_wav_mix_inc;

   float          pha_phase;
   unsigned short pha_lfsr_state;
   float          pha_rand;

   const float *lut_logwav_a;
   const float *lut_logwav_b;

   const float *vel_curve_lut;  // 256 entries

   sSI mod_voicebus_idx;  // -1=none

   float last_out;

   float level_scl;  // MGRP2_LEVEL modulation
   float effective_fmmatrix_src_amts[NUM_OPS + 1];   // see PARAM_MATRIX_BASE. updated in prepare_block()
   float effective_op_to_out;

#ifdef FMSTACK_HIRES_AENV
   float aenv_amt;
   float aenv_vel_amt_scl;
#endif // FMSTACK_HIRES_AENV

#ifdef FMSTACK_HIRES_PENV
    float penv_amt;
    float penv_vel_scl;
#endif // FMSTACK_HIRES_PENV

   env_t aenv;
   env_params_t aenv_params;

   env_t penv;
   env_params_t penv_params;

   struct {
      sF32 level_scl;
      sF32 level_env_amt_scl;
      sF32 pitch_oct;
      sF32 pitch_env_amt_scl;
      sF32 phase_off;
      sF32 ws_a_off;
      sF32 ws_b_off;
      sF32 ws_ab_mix_off;
      sF32 matrix_dst_amt_scl;
      sF32 aenv_atk_scl;
      sF32 aenv_hld_scl;
      sF32 aenv_dcy_scl;
      sF32 aenv_sus_scl;
      sF32 aenv_rls_scl;
      sF32 penv_atk_scl;
      sF32 penv_hld_scl;
      sF32 penv_dcy_scl;
      sF32 penv_sus_scl;
      sF32 penv_rls_scl;
      sF32 out_scl;

      void reset(void) {
         level_scl          = 1.0f;
         level_env_amt_scl  = 1.0f;
         pitch_oct          = 0.0f;
         pitch_env_amt_scl  = 1.0f;
         phase_off          = 0.0f;
         ws_a_off           = 0.0f;
         ws_b_off           = 0.0f;
         ws_ab_mix_off      = 0.0f;
         matrix_dst_amt_scl = 1.0f;
         aenv_atk_scl       = 1.0f;
         aenv_hld_scl       = 1.0f;
         aenv_dcy_scl       = 1.0f;
         aenv_sus_scl       = 1.0f;
         aenv_rls_scl       = 1.0f;
         penv_atk_scl       = 1.0f;
         penv_hld_scl       = 1.0f;
         penv_dcy_scl       = 1.0f;
         penv_sus_scl       = 1.0f;
         penv_rls_scl       = 1.0f;
         out_scl            = 1.0f;
      }
   } mgrp_mod;

   void noteOn (const FMSTACK_SHARED_T *_shared,
                const FMSTACK_VOICE_T *_voice,
                const sUI   _paramOff,
                const sUI   _paramOffOpEnv,
                const sBool _bGlide
                );

   void noteOff (void);

   void prepareBlock (const FMSTACK_SHARED_T *_shared,
                      const FMSTACK_VOICE_T *_voice,
                      const sUI  _paramOff,
                      const sUI  _paramOffOpEnv,
                      const sUI  _numFramesOrig,
                      const sUI  _numFramesOS,
                      const sF32 _noteSpeedOS,
                      const sF32 _note
                      );

#ifdef FMSTACK_HIRES_AENV
   sF32 calcHiresAEnvLvl (void);
#endif // FMSTACK_HIRES_AENV

#ifdef FMSTACK_HIRES_PENV
   sF32 calcHiresPEnvLvl (void);
#endif // FMSTACK_HIRES_PENV

   void calcOut (const FMSTACK_SHARED_T *_shared,
                 FMSTACK_VOICE_T *_voice
#ifdef MODFM
                 , sUI _modFMOpBit
#endif // MODFM
                 );

   void calcOutOp5 (const FMSTACK_SHARED_T *_shared,
                    FMSTACK_VOICE_T *_voice,
                    const sUI _k,
                    const sF32 _inSmp
                    );

} FMSTACK_VOICE_OP_T;

typedef struct FMSTACK_VOICE_S {
   st_plugin_voice_t base;

   sBool b_glide;  // 0=no glide, 1=glide, 2=glide+reset phase/env

   float velocity;
   sUI   velocity_255;
   float sample_rate;
   float note_cur;
   float note_inc;
   float mods[NUM_MODS];

   FMSTACK_VOICE_OP_T ops[NUM_OPS + 1];

   sUI   var_param_off_a;
   sUI   var_param_off_b;
   float var_b_amt;

#ifdef FMSTACK_HPF
   StBiquad hpf;  // DC offset removal
#endif // FMSTACK_HPF

   float getVarParam(const FMSTACK_SHARED_T *_shared, const sUI _paramIdx) const {
      if(_paramIdx >= PARAM_VAR_BASE)
      {
         const sUI paramIdxA = var_param_off_a + (_paramIdx - PARAM_VAR_BASE);
         const sUI paramIdxB = var_param_off_b + (_paramIdx - PARAM_VAR_BASE);
         return mathLerpf(_shared->params[paramIdxA], _shared->params[paramIdxB], var_b_amt);
      }
      return 0.0f;
   }

   static float RatioToOct(const sF32 _ratio) {
      // ratio 0..65536 => oct -16..+16
      if(_ratio > 0.0f)
      {
         return Dlog2f_prec(_ratio);
      }
      else
      {
         return -16.0f;
      }
   }

   static float OctToRatio(const sF32 _oct) {
      return Dexp2f_prec(_oct);
   }

   float getVarParamCoarse(const FMSTACK_SHARED_T *_shared, sUI _paramIdx) const {
      // linear vs exponential power-of-two (octave) interpolation:
      //                                                           --linear--                      --exponential--
      // e.g.  ratioA = 0.5(oct-1), ratioB = 4(oct+2), amt=0.5  => oct=1.16992500144,linRto=2.25   oct=0.5,linRto=1.41421356237
      // e.g.  ratioA = 0.5(oct-1), ratioB = 2(oct+1), amt=0.5  => oct=0.32192809488,linRto=1.25   oct=0.0,linRto=1.00000000000
      // e.g.  ratioA = 1.0(oct+0), ratioB = 8(oct+3), amt=0.5  => oct=2.16992500145,linRto=4.5    oct=1.5,linRto=2.82842712473
      if(_paramIdx >= PARAM_VAR_BASE)
      {
         const sUI paramIdxA = var_param_off_a + (_paramIdx - PARAM_VAR_BASE);
         const sUI paramIdxB = var_param_off_b + (_paramIdx - PARAM_VAR_BASE);
         const sF32 coarseA = RatioToOct(_shared->params[paramIdxA] * 65536.0f);
         const sF32 coarseB = RatioToOct(_shared->params[paramIdxB] * 65536.0f);
         const sF32 lerpOct = mathLerpf(coarseA, coarseB, var_b_amt);
         if(lerpOct < -15.9f)  // 0Hz
            return 0.0f;
         else
            return OctToRatio(lerpOct);
      }
      return 0.0f;
   }

} FMSTACK_VOICE_T;

static sF32 loc_calc_vel_scl(sF32 _vel, sF32 _amt) {
   if(_amt < 0.0f)
   {
      _amt = -_amt;
      _vel = 1.0f - _vel;
   }
   return (1.0f - _amt) + (_vel * _amt);
}

void FMSTACK_VOICE_OP_T::noteOn(const FMSTACK_SHARED_T *_shared,
                        const FMSTACK_VOICE_T *_voice,
                        const sUI   _paramOff,
                        const sUI   _paramOffEnv,
                        const sBool _bGlide
                        ) {
   if(_bGlide < 1)
   {
      // start phase randomization
      {
         pha_lfsr_state ^= pha_lfsr_state >> 7;
         pha_lfsr_state ^= pha_lfsr_state << 9;
         pha_lfsr_state ^= pha_lfsr_state >> 13;
         const short s = short(pha_lfsr_state & 65520);
         pha_rand = (s >> 4) / 2048.0f;
      }
   }

   if(0 == _bGlide)
   {
      pha_phase = 0.0f;
      last_out = 0.0f;
   }

   aenv.noteOn();
   penv.noteOn();
}

void FMSTACK_VOICE_OP_T::noteOff(void) {
   aenv.noteOff();
   penv.noteOff();
}

void FMSTACK_VOICE_OP_T::prepareBlock(const FMSTACK_SHARED_T *_shared,
                              const FMSTACK_VOICE_T *_voice,
                              const sUI  _paramOff,
                              const sUI  _paramOffEnv,
                              const sUI  _numFramesOrig,
                              const sUI  _numFramesOS,
                              const sF32 _noteSpeedOS,
                              const sF32 _note
                              ) {
   sF32 modRatio = 1.0f;

   vel_curve_lut = get_vel_curve_lut(_voice->getVarParam(_shared, _paramOff + PARAM_OP_VEL_CURVE));

   const sF32 coarse = _voice->getVarParamCoarse(_shared, _paramOff + PARAM_OP_COARSE);
   modRatio *= coarse;

   sF32 fine = _voice->getVarParam(_shared, _paramOff + PARAM_OP_FINE) * 2.0f - 1.0f;
   fine += mgrp_mod.pitch_oct * 3.0f;
   if(fine > 0.0f)
      modRatio *= (1.0f + fine);
   else if(fine < 0.0f)
      modRatio /= (-fine + 1.0f);

   const sF32 pitchKbdCtr = _voice->getVarParam(_shared, _paramOff + PARAM_OP_PITCH_KBD_CTR) * 96.0f - 48.0f + (5*12)/*C-5*/;
   const sF32 pitchKbdAmt = _voice->getVarParam(_shared, _paramOff + PARAM_OP_PITCH_KBD_AMT) * 4.0f - 2.0f;
   const sF32 pitchNoteCur = (_note - pitchKbdCtr); // delta
   const sF32 pitchNote = pitchNoteCur * pitchKbdAmt;
   if(pitchNote > pitchNoteCur)
   {
      modRatio *= Dexp2f_prec((pitchNote - pitchNoteCur) * (1.0f / 12.0f));
   }
   else if(pitchNote < pitchNoteCur)
   {
      modRatio /= Dexp2f_prec((pitchNoteCur - pitchNote) * (1.0f / 12.0f));
   }

   sUI wsA = sUI((_voice->getVarParam(_shared, _paramOff + PARAM_OP_WS_A) + mgrp_mod.ws_a_off) * 100.0f + 0.5f);
   if(wsA >= NUM_WAVES)
      wsA = NUM_WAVES - 1u;
   lut_logwav_a = loc_waves[wsA];


   sUI wsB = sUI((_voice->getVarParam(_shared, _paramOff + PARAM_OP_WS_B) + mgrp_mod.ws_b_off) * 100.0f + 0.5f);
   if(wsB >= NUM_WAVES)
      wsB = NUM_WAVES - 1u;
   lut_logwav_b = loc_waves[wsB];

   // (note) intentionally not clipped to 0..1 to allow for waves outside the a..b blend range
   const sF32 modWavMix = _voice->getVarParam(_shared, _paramOff + PARAM_OP_WS_MIX) + mgrp_mod.ws_ab_mix_off;

   level_scl *= loc_calc_vel_scl(vel_curve_lut[_voice->velocity_255],
                                 _voice->getVarParam(_shared, _paramOff + PARAM_OP_LEVEL_VEL_AMT) * 2.0f - 1.0f
                                 );


   if(NULL == aenv.lut_atk_shape)
   {
      // amp env shape
      aenv.lut_atk_shape = get_env_shape_lut(_voice->getVarParam(_shared, _paramOffEnv + PARAM_ENV_ATK_SHAPE));
      aenv.lut_dcy_shape = get_env_shape_lut(_voice->getVarParam(_shared, _paramOffEnv + PARAM_ENV_DCY_SHAPE));
      aenv.lut_rls_shape = get_env_shape_lut(_voice->getVarParam(_shared, _paramOffEnv + PARAM_ENV_RLS_SHAPE));

      // pitch env shape
      const sUI paramOffPEnv = _paramOffEnv + (PARAM_PITCH_ENV_BASE - PARAM_AMP_ENV_BASE);
      penv.lut_atk_shape = get_env_shape_lut(_voice->getVarParam(_shared, paramOffPEnv + PARAM_ENV_ATK_SHAPE));
      penv.lut_dcy_shape = get_env_shape_lut(_voice->getVarParam(_shared, paramOffPEnv + PARAM_ENV_DCY_SHAPE));
      penv.lut_rls_shape = get_env_shape_lut(_voice->getVarParam(_shared, paramOffPEnv + PARAM_ENV_RLS_SHAPE));
   }

   // Amp envelope
   {
      const float atk = _voice->getVarParam(_shared, _paramOffEnv + PARAM_ENV_ATK) * 10000.0f * mgrp_mod.aenv_atk_scl;
      const float hld = _voice->getVarParam(_shared, _paramOffEnv + PARAM_ENV_HLD) * 10000.0f * mgrp_mod.aenv_hld_scl;
      const float dcy = _voice->getVarParam(_shared, _paramOffEnv + PARAM_ENV_DCY) * 10000.0f * mgrp_mod.aenv_dcy_scl;
      const float sus = _voice->getVarParam(_shared, _paramOffEnv + PARAM_ENV_SUS)            * mgrp_mod.aenv_sus_scl;
      const float rls = _voice->getVarParam(_shared, _paramOffEnv + PARAM_ENV_RLS) * 10000.0f * mgrp_mod.aenv_rls_scl;
      aenv_params.update(_voice->sample_rate, atk, hld, dcy, sus, rls);

      sF32 envAmt = _voice->getVarParam(_shared, _paramOff + PARAM_OP_LEVEL_ENV_AMT) * 2.0f - 1.0f;
      envAmt *= mgrp_mod.level_env_amt_scl;
      const sF32 envVelAmt = _voice->getVarParam(_shared, _paramOff + PARAM_OP_LEVEL_ENV_VEL_AMT) * 2.0f - 1.0f;
      const sF32 envVelAmtScl = loc_calc_vel_scl(vel_curve_lut[_voice->velocity_255], envVelAmt);

#ifndef FMSTACK_HIRES_AENV
      sF32 envLvl = aenv.step(_numFramesOrig, &aenv_params);
      envLvl *= envVelAmtScl;
      if(envAmt >= 0.0f)
         envLvl = 1.0f + (envLvl - 1.0f) * envAmt;
      else
         envLvl = 1.0f + ((1.0f - envLvl) - 1.0f) * -envAmt;
      level_scl *= envLvl;
#else
      aenv_amt = envAmt;
      aenv_vel_amt_scl = envVelAmtScl;
#endif // FMSTACK_HIRES_AENV
   }

   // Pitch envelope
   {
      const sUI paramOffPEnv = _paramOffEnv + (PARAM_PITCH_ENV_BASE - PARAM_AMP_ENV_BASE);
      const float atk = _voice->getVarParam(_shared, paramOffPEnv + PARAM_ENV_ATK) * 10000.0f * mgrp_mod.penv_atk_scl;
      const float hld = _voice->getVarParam(_shared, paramOffPEnv + PARAM_ENV_HLD) * 10000.0f * mgrp_mod.penv_hld_scl;
      const float dcy = _voice->getVarParam(_shared, paramOffPEnv + PARAM_ENV_DCY) * 10000.0f * mgrp_mod.penv_dcy_scl;
      const float sus = _voice->getVarParam(_shared, paramOffPEnv + PARAM_ENV_SUS)            * mgrp_mod.penv_sus_scl;
      const float rls = _voice->getVarParam(_shared, paramOffPEnv + PARAM_ENV_RLS) * 10000.0f * mgrp_mod.penv_rls_scl;
      penv_params.update(_voice->sample_rate, atk, hld, dcy, sus, rls);

      sF32 envAmt = _voice->getVarParam(_shared, _paramOff + PARAM_OP_PITCH_ENV_AMT) * 8.0f - 4.0f;
      envAmt *= mgrp_mod.pitch_env_amt_scl;
      const sF32 envVelAmt = _voice->getVarParam(_shared, _paramOff + PARAM_OP_PITCH_ENV_VEL_AMT) * 2.0f - 1.0f;
      const sF32 velScl = loc_calc_vel_scl(vel_curve_lut[_voice->velocity_255], envVelAmt);

#ifndef FMSTACK_HIRES_PENV
      sF32 envLvl = penv.step(_numFramesOrig, &penv_params);
      envLvl *= velScl;
      envLvl *= envAmt;
      if(envLvl != 0.0f)
      {
         modRatio *= Dexp2f_fast(envLvl);
      }
#else
      penv_amt     = envAmt;
      penv_vel_scl = velScl;
#endif // FMSTACK_HIRES_PENV
   }

   // phaSpeed
   const sF32 modSpeed = _noteSpeedOS * modRatio;

   sF32 modLevel = _voice->getVarParam(_shared, _paramOff + PARAM_OP_LEVEL) * 16.0f * level_scl;

#ifdef LOGMUL
   if(modLevel > 64.0f)
      modLevel = 64.0f;
   else if(modLevel < LOGMUL_MIN)
      modLevel = LOGMUL_MIN;
   sF32 modLogLevel = Dlogf_prec(modLevel);
#else
   if(modLevel > 64.0f)
      modLevel = 64.0f;
#endif // LOGMUL

   mod_voicebus_idx = sSI(_voice->getVarParam(_shared, _paramOff + PARAM_OP_VOICEBUS) * 100.0f + 0.5f);
   if(mod_voicebus_idx > 0)
      mod_voicebus_idx = (mod_voicebus_idx - 1) & 31;
   else
      mod_voicebus_idx = -1;

   if(_numFramesOS > 0u)
   {
      // lerp
      const sF32 recBlockSize = (1.0f / _numFramesOS);
      mod_speed_inc     = (modSpeed     - mod_speed_cur)    * recBlockSize;
      mod_level_inc     = (modLevel     - mod_level_cur)    * recBlockSize;
#ifdef LOGMUL
      mod_loglevel_inc  = (modLogLevel  - mod_loglevel_cur) * recBlockSize;
#endif // LOGMUL
      mod_wav_mix_inc   = (modWavMix    - mod_wav_mix_cur)  * recBlockSize;
   }
   else
   {
      mod_speed_cur    = modSpeed;
      mod_speed_inc    = 0.0f;
      mod_level_cur    = modLevel;
      mod_level_inc    = 0.0f;
#ifdef LOGMUL
      mod_loglevel_cur = modLogLevel;
      mod_loglevel_inc = 0.0f;
#endif // LOGMUL
      mod_wav_mix_cur  = modWavMix;
      mod_wav_mix_inc  = 0.0f;

      if(0 == _voice->b_glide)
      {
         // Modulate / Calc start phase
         pha_phase = _voice->getVarParam(_shared, _paramOff + PARAM_OP_PHASE);
         if(pha_phase > 0.99f)
         {
            // printf("xxx voice=%p pha_phase=%f\n", _voice, pha_phase);
            if(pha_phase > 0.999f)
               pha_phase = pha_rand; // <rand> (1.0)
            // else <free> [11Jan2024] (0.999)
         }
         else
         {
            pha_phase = (pha_phase / 0.99f);
            pha_phase += mgrp_mod.phase_off;
            if(pha_phase < 0.0f)
               pha_phase += 1.0f;
            else if(pha_phase > 1.0f)
               pha_phase -= 1.0f;
         }
      }
   }
}

#ifdef FMSTACK_HIRES_AENV
sF32 FMSTACK_VOICE_OP_T::calcHiresAEnvLvl(void) {
   sF32 envLvl = aenv.step(1u/*numFrames*/, &aenv_params);
   envLvl *= aenv_vel_amt_scl;
   if(aenv_amt >= 0.0f)
      envLvl = 1.0f + (envLvl - 1.0f) * aenv_amt;
   else
      envLvl = 1.0f + ((1.0f - envLvl) - 1.0f) * -aenv_amt;
   return envLvl;
}
#endif // FMSTACK_HIRES_AENV

#ifdef FMSTACK_HIRES_PENV
sF32 FMSTACK_VOICE_OP_T::calcHiresPEnvLvl(void) {
   sF32 envLvl = penv.step(1u/*numFrames*/, &penv_params);
   envLvl *= penv_vel_scl;
   envLvl *= penv_amt;
   if(envLvl != 0.0f)
   {
      return Dexp2f_fast(envLvl);
   }
   else
   {
      return 1.0f;
   }
}
#endif // FMSTACK_HIRES_PENV

void FMSTACK_VOICE_OP_T::calcOut(const FMSTACK_SHARED_T *_shared,
                         FMSTACK_VOICE_T *_voice
#ifdef MODFM
                         , sUI _modFMOpBit
#endif // MODFM
                         ) {
   // pha freq=$v_ratio3
   // lut logsin
   // + $v_loglevel3
   // + 16.118095651
   // * 1/20.9701259149
   // lut exp
   // - $v_level3

#ifdef FMSTACK_HIRES_AENV
   const sF32 aenvLvl = calcHiresAEnvLvl();
#endif // FMSTACK_HIRES_AENV

#ifdef FMSTACK_HIRES_PENV
   const sF32 penvLvl = calcHiresPEnvLvl();  // ratio (1..f)
#endif // FMSTACK_HIRES_PENV

   sF32 out = 0.0f;
#ifdef OVERSAMPLE_FACTOR
   sF32 outOS = 0.0f;
   for(sUI osi = 0u; osi < OVERSAMPLE_FACTOR; osi++)
#endif // OVERSAMPLE_FACTOR
   {

      // Sum matrix phase modulation
      sF32 phaseMod = 0.0f;
#ifdef MODFM
      sBool bModFM = (0u != (sUI(_shared->params[PARAM_MODFM_MASK] * 100.0f + 0.5f) & _modFMOpBit));
      sF32 modFMAmt = 0.0f;
      if(bModFM)
      {
         for(sUI opModIdx = 0u; opModIdx < (NUM_OPS + 1u); opModIdx++)
         {
            const FMSTACK_VOICE_OP_T *opMod = &_voice->ops[opModIdx];
            const sF32 amt = effective_fmmatrix_src_amts[opModIdx];
            modFMAmt += amt;
            phaseMod += Dmulf(opMod->last_out, amt);
         }
      }
      else
      {
         // Regular PM
         for(sUI opModIdx = 0u; opModIdx < (NUM_OPS + 1u); opModIdx++)
         {
            const FMSTACK_VOICE_OP_T *opMod = &_voice->ops[opModIdx];
            // (todo) use log/exp lut ?
            phaseMod += Dmulf(opMod->last_out, effective_fmmatrix_src_amts[opModIdx]);
         }
      }
#else
      // Regular PM
      for(sUI opModIdx = 0u; opModIdx < (NUM_OPS + 1u); opModIdx++)
      {
         const FMSTACK_VOICE_OP_T *opMod = &_voice->ops[opModIdx];
         // (todo) use log/exp lut ?
         phaseMod += Dmulf(opMod->last_out, effective_fmmatrix_src_amts[opModIdx]);
      }
#endif // MODFM

      // Calc effective phase
#ifdef MODFM
      sF32 curPhase;
      sF32 modFM;
      if(bModFM)
      {
         curPhase = ffrac_s(pha_phase + 0.25f/*cos*/);

         // Exponential waveshaper
         modFM = loc_lut_modfm_exp[sUI(phaseMod * (16384.0f / 128.0f) + 8192) & 16383u];
      }
      else
      {
         // Regular PM
         curPhase = ffrac_s(pha_phase + phaseMod);
      }
#else
      const sF32 curPhase = ffrac_s(pha_phase + phaseMod);
#endif // MODFM

      // Accumulate phase
#ifndef FMSTACK_HIRES_PENV
      pha_phase = ffrac_s(pha_phase + mod_speed_cur);
#else
      pha_phase = ffrac_s(pha_phase + Dmulf_prec(mod_speed_cur, penvLvl));
#endif // FMSTACK_HIRES_PENV

      // Calc amplified sine value
      const sF32 a = lut_logwav_a[((sUI)(curPhase * LOGWAV_TBLMASK)) & LOGWAV_TBLMASK];
      const sF32 b = lut_logwav_b[((sUI)(curPhase * LOGWAV_TBLMASK)) & LOGWAV_TBLMASK];
      out = a + (b - a) * mod_wav_mix_cur;

#ifdef MODFM
      if(bModFM)
      {
         // Ring-modulate and normalize
         out = Dmulf(out, modFM * loc_lut_modfm_exp[sUI(-modFMAmt * (16384.0f / 128.0f) + 8192) & 16383u]);
      }
#endif // MODFM

#ifdef FMSTACK_HIRES_AENV
      out = Dmulf(out, aenvLvl);
#endif // FMSTACK_HIRES_AENV

#ifdef LOGMUL
      out += mod_loglevel_cur;
      out += 16.1181f;
      out *= 0.0476869f;
      // if(out > 0.99999f)
      //    printf("xxx out=%f\n", out);
      out  = loc_lut_exp[((sUI)(out * EXP_TBLMASK)) & EXP_TBLMASK];
      out -= mod_level_cur;
#else
      out = Dmulf(out, mod_level_cur);
#endif // LOGMUL

#ifdef OVERSAMPLE_FACTOR
      outOS += out;
#endif // OVERSAMPLE_FACTOR

      // Modulate
      mod_speed_cur    += mod_speed_inc;
#ifdef LOGMUL
      mod_loglevel_cur += mod_loglevel_inc;
#endif // LOGMUL
      mod_level_cur    += mod_level_inc;
      mod_wav_mix_cur  += mod_wav_mix_inc;
   }
#ifdef OVERSAMPLE_FACTOR
   // Apply lowpass filter before downsampling
   //   (note) normalized Fc = F/Fs = 0.442947 / sqrt(oversample_factor² - 1)
   last_out = outOS * (1.0f / OVERSAMPLE_FACTOR);
#else
   last_out = out;
#endif // OVERSAMPLE_FACTOR

}


static void loc_calc_lut_logconst    (float *_d, float _c);
static void loc_calc_lut_logsin      (float *_d);
static void loc_calc_lut_logsin_2    (float *_d);
static void loc_calc_lut_logsin_3    (float *_d);
static void loc_calc_lut_logsin_4    (float *_d);
static void loc_calc_lut_logsin_5    (float *_d);
static void loc_calc_lut_logsin_6    (float *_d);
static void loc_calc_lut_logsin_7    (float *_d);
static void loc_calc_lut_logsin_8    (float *_d);
static void loc_calc_lut_logtri      (float *_d, sF32 _phase);
static void loc_calc_lut_logsinp     (float *_d, sF32 _pow);
static void loc_calc_lut_logsinpe    (float *_d, sF32 _pow);
static void loc_calc_lut_logsinpq    (float *_d, sF32 _pow);
static void loc_calc_lut_logsinpd    (float *_d, sF32 _pow);
static void loc_calc_lut_logsinsy    (float *_d, sF32 _mul);
static void loc_calc_lut_logsindr    (float *_d, sF32 _drive);
static void loc_calc_lut_logsinsw    (float *_d, sF32 _bend, sBool _bFlip);
static void loc_calc_lut_logsinbr    (float *_d, sF32 _bits);
static void loc_calc_lut_logsinsr    (float *_d, sF32 _f);
static void loc_calc_lut_logsinsawsq (float *_d, sF32 _f);
static void loc_calc_lut_logsinbd    (float *_d, sF32 _exp);
static void loc_calc_lut_logsinwarp  (float *_d, sF32 _exp);

#ifdef LOGMUL
static void loc_calc_lut_exp (float *_d);
#endif // LOGMUL

static void loc_prepare(st_plugin_voice_t *_voice, const sUI _numFrames, const sF32 _noteSpeed, const sF32 _note);


void FMSTACK_VOICE_OP_T::calcOutOp5(const FMSTACK_SHARED_T *_shared,
                            FMSTACK_VOICE_T *_voice,
                            const sUI _k,
                            const sF32 _inSmp
                            ) {
   if(mod_voicebus_idx >= 0)
   {
      const float *samplesBus = _voice->base.voice_bus_buffers[mod_voicebus_idx] + _voice->base.voice_bus_read_offset;
      const float busL = samplesBus[_k];
      const float busR = samplesBus[_k + 1u];
      last_out = (busL + busR) * 0.5f * mod_level_cur;
   }
   else
      last_out = _inSmp * mod_level_cur;

#ifdef FMSTACK_HIRES_AENV
   const sF32 envLvl = calcHiresAEnvLvl();
   last_out = Dmulf(last_out, envLvl);
#endif // FMSTACK_HIRES_AENV

#ifdef OVERSAMPLE_FACTOR
   mod_level_cur += mod_level_inc * OVERSAMPLE_FACTOR;
#else
   mod_level_cur += mod_level_inc;
#endif // OVERSAMPLE_FACTOR

}

static void loc_calc_lut_logconst(float *_d, float _c) {
   float out = _c;

#ifdef LOGMUL
   out = loc_level_to_lut_log(out);
#endif // LOGMUL

   loop(LOGWAV_TBLSZ)
   {
      _d[i] = out;

   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logconst() */

static void loc_calc_lut_logsin(float *_d, float _phase) {
   // tx81z waveform 1
   float x = 0.0f;
   loop(LOGWAV_TBLSZ)
   {
      float ph = ffrac_s(x);
      ph += _phase;
      if(ph >= 1.0f)
         ph -= 1.0f;
#ifndef TRUE_SINE
      float out = loc_sine_tbl_f[(unsigned short)(16384 * ph)&16383u];
#else
      float out = Dsinf(ph);
#endif // TRUE_SINE

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsin() */

static void loc_calc_lut_logsin_2(float *_d) {
   // tx81z waveform 2
   //      .
   //      .
   //     . .
   //   ..   ..   ..
   //          . .
   //           .
   //           .
   float x = 0.0f;
   loop(LOGWAV_TBLSZ)
   {
      float out;
      sUI ph = sUI(16384 * ffrac_s(x)) & 16383u;
#ifndef TRUE_SINE
      if(ph < 4096u)
         out = 1.0f - loc_sine_tbl_f[ph + 4096u];
      else if(ph < 8192u)
         out = 1.0f - loc_sine_tbl_f[ph - 4096u];
      else if(ph < 12288u)
         out = -(1.0f - loc_sine_tbl_f[ph - 8192u + 4096u]);
      else
         out = -(1.0f - loc_sine_tbl_f[ph - 12288u]);
#else
      sF32 phF = ffrac_s(x);
      if(ph < 4096u)
         out = 1.0f - Dsinf(phF + 0.25f);
      else if(ph < 8192u)
         out = 1.0f - Dsinf(phF - 0.25f);
      else if(ph < 12288u)
         out = -(1.0f - Dsinf(phF - 0.5f + 0.25f));
      else
         out = -(1.0f - Dsinf(phF - 0.75f));
#endif // TRUE_SINE

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsin_2() */

void loc_calc_lut_logsin_3(float *_d) {
   // tx81z waveform 3
   //     ..
   //    .  .
   //   .    .
   //   .    .......
   //
   //
   //
   float x = 0.0f;
   loop(LOGWAV_TBLSZ)
   {
      float out;
      sUI ph = sUI(16384 * ffrac_s(x)) & 16383u;
#ifndef TRUE_SINE
      if(ph < 8192u)
         out = loc_sine_tbl_f[ph];
      else
         out = 0.0f;
#else
      sF32 phF = ffrac_s(x);
      if(ph < 8192u)
         out = Dsinf(phF);
      else
         out = 0.0f;
#endif // TRUE_SINE

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsin_3() */

static void loc_calc_lut_logsin_4(float *_d) {
   // tx81z waveform 4
   //      .
   //      .
   //     . .
   //   ..   .......
   //
   //
   //
   float x = 0.0f;
   loop(LOGWAV_TBLSZ)
   {
      float out;
      sUI ph = sUI(16384 * ffrac_s(x)) & 16383u;
#ifndef TRUE_SINE
      if(ph < 4096u)
         out = 1.0f - loc_sine_tbl_f[ph + 4096u];
      else if(ph < 8192u)
         out = 1.0f - loc_sine_tbl_f[ph - 4096u];
      else
         out = 0.0f;
#else
      sF32 phF = ffrac_s(x);
      if(ph < 4096u)
         out = 1.0f - Dsinf(phF + 0.25f);
      else if(ph < 8192u)
         out = 1.0f - Dsinf(phF - 0.25f);
      else
         out = 0.0f;
#endif // TRUE_SINE

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsin_4() */

static void loc_calc_lut_logsin_5(float *_d) {
   // tx81z waveform 5
   //    .
   //   . .
   //   . .
   //   . . ........
   //     . .
   //     . .
   //      .
   float x = 0.0f;
   loop(LOGWAV_TBLSZ)
   {
      float out;
      sUI ph = sUI(16384 * ffrac_s(x)) & 16383u;
#ifndef TRUE_SINE
      if(ph < 8192u)
         out = loc_sine_tbl_f[ph << 1];
      else
         out = 0.0f;
#else
      sF32 phF = ffrac_s(x);
      if(ph < 8192u)
         out = Dsinf(phF * 2.0);
      else
         out = 0.0f;
#endif // TRUE_SINE

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsin_5() */

static void loc_calc_lut_logsin_6(float *_d) {
   // tx81z waveform 6
   //      .
   //      .
   //     . .
   //   ..   ..   ..............
   //          . .
   //           .
   //           .
   float x = 0.0f;
   loop(LOGWAV_TBLSZ)
   {
      float out;
      sUI ph = sUI(16384 * ffrac_s(x)) & 16383u;
      if(ph < 8192u)
      {
         ph = ph << 1;
#ifndef TRUE_SINE
         if(ph < 4096u)
            out = 1.0f - loc_sine_tbl_f[ph + 4096u];
         else if(ph < 8192u)
            out = 1.0f - loc_sine_tbl_f[ph - 4096u];
         else if(ph < 12288u)
            out = -(1.0f - loc_sine_tbl_f[ph - 8192u + 4096u]);
         else
            out = -(1.0f - loc_sine_tbl_f[ph - 12288u]);
#else
         sF32 phF = ffrac_s(x * 2.0f);
         if(ph < 4096u)
            out = 1.0f - Dsinf(phF + 0.25f);
         else if(ph < 8192u)
            out = 1.0f - Dsinf(phF - 0.25f);
         else if(ph < 12288u)
            out = -(1.0f - Dsinf(phF - 0.5f + 0.25f));
         else
            out = -(1.0f - Dsinf(phF - 0.75f));
#endif // TRUE_SINE
      }
      else
         out = 0.0f;

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsin_6() */

static void loc_calc_lut_logsin_7(float *_d) {
   // tx81z waveform 7
   //    .  .
   //   . .. .
   //   . .. .
   //   . .. .......
   //
   //
   //
   float x = 0.0f;
   loop(LOGWAV_TBLSZ)
   {
      float out;
      sUI ph = sUI(16384 * ffrac_s(x)) & 16383u;
      if(ph < 8192u)
      {
#ifndef TRUE_SINE
         ph = (ph << 1) & 8191u;
         out = loc_sine_tbl_f[ph];
#else
         sF32 phF = ffrac_s(x * 2.0f);
         if(phF >= 0.5f)
            phF -= 0.5f;
         out = Dsinf(phF);
#endif // TRUE_SINE
      }
      else
         out = 0.0f;

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsin_7() */

static void loc_calc_lut_logsin_8(float *_d) {
   // tx81z waveform 8
   //      .      .
   //      .      .
   //     . .    . .
   //   ..   ....   ..............
   //
   //
   //
   float x = 0.0f;
   loop(LOGWAV_TBLSZ)
   {
      float out;
      sUI ph = sUI(16384 * ffrac_s(x)) & 16383u;
      if(ph < 8192u)
      {
         ph = (ph << 1) & 8191u;
#ifndef TRUE_SINE
         if(ph < 4096u)
            out = 1.0f - loc_sine_tbl_f[ph + 4096u];
         else
            out = 1.0f - loc_sine_tbl_f[ph - 4096u];
#else
         sF32 phF = ffrac_s(x * 2.0f);
         if(phF >= 0.5f)
            phF -= 0.5f;
         if(ph < 4096u)
            out = 1.0f - Dsinf(phF + 0.25f);
         else
            out = 1.0f - Dsinf(phF - 0.25f);
#endif // TRUE_SINE
      }
      else
         out = 0.0f;

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsin_8() */

static void loc_calc_lut_logtri(float *_d, const sF32 _phase) {
   float x = _phase;
   loop(LOGWAV_TBLSZ)
   {
      float out;
      if(x >= 0.5f)
         out = 1.0f - ((x-0.5f) * 4.0f);
      else
         out = -1.0f + (x * 4.0f);

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
      if(x >= 1.0f)
         x -= 1.0f;
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logtri() */

static void loc_calc_lut_logsinp(float *_d, sF32 _pow) {
   float x = 0.0f;
   loop(LOGWAV_TBLSZ)
   {
      float ph = ffrac_s(x);
#ifndef TRUE_SINE
      float out = loc_sine_tbl_f[(unsigned short)(16384 * ph)&16383u];
#else
      float out = Dsinf(ph);
#endif // TRUE_SINE
      out = powf(out, _pow);

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsinp() */

static void loc_calc_lut_logsinpe(float *_d, float _pow) {
   float x = 0.0f;
   loop(LOGWAV_TBLSZ)
   {
      float ph = ffrac_s(x);
#ifndef TRUE_SINE
      float out = loc_sine_tbl_f[(unsigned short)(16384 * ph)&16383u];
#else
      float out = Dsinf(ph);
#endif // TRUE_SINE
      if(out >= 0.0f)
         out = powf(out, _pow);
      else
         out = 0.0f;

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsinpe() */

static void loc_calc_lut_logsinpq(float *_d, float _pow) {
   float x = 0.0f;
   loop(LOGWAV_TBLSZ)
   {
      float ph = ffrac_s(x);
#ifndef TRUE_SINE
      float out = loc_sine_tbl_f[(unsigned short)(16384 * ph)&16383u];
#else
      float out = Dsinf(ph);
#endif // TRUE_SINE
      if(out >= 0.0f)
         out = powf(out, _pow);
      else
         out = -powf(-out, _pow);

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsinpq() */

static void loc_calc_lut_logsinpd(float *_d, float _pow) {
   float x = 0.0f;
   loop(LOGWAV_TBLSZ)
   {
      float ph = ffrac_s(x);
#ifndef TRUE_SINE
      float out = loc_sine_tbl_f[(unsigned short)(16384 * ph)&16383u];
#else
      float out = Dsinf(ph);
#endif // TRUE_SINE
      if(out >= 0.0f)
         out = powf(out, _pow);
      else
         out = powf(-out, _pow);

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsinpd() */

static void loc_calc_lut_logsinsy(float *_d, float _mul) {
   float x = 0.0f;
   loop(LOGWAV_TBLSZ)
   {
      float ph = ffrac_s(x);
      float scl = loc_sine_tbl_f[(unsigned short)(8192 * ph)&16383u];
#ifndef TRUE_SINE
      float out = loc_sine_tbl_f[(unsigned short)(16384 * ph * _mul)&16383u];
      out *= scl;
#else
      float out = Dsinf(ph);
      out *= scl;
#endif // TRUE_SINE

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsinsy() */

static void loc_calc_lut_logsindr(float *_d, float _drive) {
   float x = 0.0f;
   loop(LOGWAV_TBLSZ)
   {
      float ph = ffrac_s(x);
#ifndef TRUE_SINE
      float out = loc_sine_tbl_f[(unsigned short)(16384 * ph)&16383u];
#else
      float out = Dsinf(ph);
#endif // TRUE_SINE
      out = ::tanhf(out * _drive);

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsindr() */

static void loc_calc_lut_logsinsw(float *_d, sF32 _bend, sBool _bFlip) {
   float x = 0.0f;
   sF32 xexp = 0.05f + (1.0f - _bend) * 0.9f;
   loop(LOGWAV_TBLSZ)
   {
      float ph = ffrac_s(x);
      ph += 0.5f;
      if(ph >= 1.0f)
         ph -= 1.0f;

      ph = (ph - 0.5f) * 2.0f;
      ph = mathPowerf(ph, xexp);
      ph = ph * 0.5f + 0.5f;

      if(_bFlip)
         ph = 1.0f - ph;

#ifndef TRUE_SINE
      float out = loc_sine_tbl_f[(unsigned short)(16384 * ph)&16383u];
#else
      float out = Dsinf(ph);
#endif // TRUE_SINE

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsinsw() */

static void loc_calc_lut_logsinbr(float *_d, sF32 _bits) {
   float x = 0.0f;
   float scl = powf(2.0f, _bits);
   loop(LOGWAV_TBLSZ)
   {
      float ph = ffrac_s(x);

#ifndef TRUE_SINE
      float out = loc_sine_tbl_f[(unsigned short)(16384 * ph)&16383u];
#else
      float out = Dsinf(ph);
#endif // TRUE_SINE

      out = int(out * scl + 0.5f) / scl;

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsinbr() */

static void loc_calc_lut_logsinsr(float *_d, sF32 _f) {
   float x = 0.0f;
   float fExp = powf(2.0f, _f);
   _f = 16384.0f / fExp;
   float fRev = fExp / 16384.0f;
   loop(LOGWAV_TBLSZ)
   {
      float ph = ffrac_s(x);
      ph = int(ph * _f) * fRev;

#ifndef TRUE_SINE
      float out = loc_sine_tbl_f[(unsigned short)(16384 * ph)&16383u];
#else
      float out = Dsinf(ph);
#endif // TRUE_SINE

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsinsr() */

static void loc_calc_lut_logsinsawsq(float *_d, sF32 _f) {
   float x = 0.0f;
   _f = 1.0f / _f;
   loop(LOGWAV_TBLSZ)
   {
      float ph = ffrac_s(x);
      ph = int(ph / _f) * _f;

#ifndef TRUE_SINE
      float out = loc_sine_tbl_f[(unsigned short)(16384 * ph)&16383u];
#else
      float out = Dsinf(ph);
#endif // TRUE_SINE

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsinsawsq() */

static void loc_calc_lut_logsinbd(float *_d, sF32 _exp) {
   float x = 0.0f;
   loop(LOGWAV_TBLSZ)
   {
      float ph = ffrac_s(x);
      // printf("x=%f ph=%f exp=%f => %f\n", x, ph, _exp, powf(ph, _exp));
      ph = powf(ph, _exp);

#ifndef TRUE_SINE
      float out = loc_sine_tbl_f[(unsigned short)(16384 * ph)&16383u];
#else
      float out = Dsinf(ph);
#endif // TRUE_SINE

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsinbd() */

#ifdef LOGMUL
static void loc_calc_lut_exp(float *_d) {
   float x = 0.0f;
   loop(EXP_TBLSZ)
   {
      float out = 2.71828f;
      float t = x;
      t *= 20.9701f;
      t -= 16.1181f;
      out = mathPowerf(out, t);
      _d[i] = out;

      // Next x
      x += (1.0f / EXP_TBLMASK);
   } // loop EXP_TBLSZ
} /* end loc_calc_lut_exp() */
#endif // LOGMUL

static void loc_calc_lut_logsinwarp(float *_d, sF32 _exp) {
   float x = 0.0f;
   loop(LOGWAV_TBLSZ)
   {
      float ph = ffrac_s(x);

      ph = 2.0f * ph - 1.0f;

      if(ph < 0.0f)
      {
         ph = 1.0f + ph;
         ph = powf(ph, _exp);
         ph = ph - 1.0f;
      }
      else
      {
         ph = 1.0f - ph;
         ph = powf(ph, _exp);
         ph = 1.0f - ph;
      }

      ph = ph * 0.5f + 0.5f;

#ifndef TRUE_SINE
      float out = loc_sine_tbl_f[(unsigned short)(16384 * ph)&16383u];
#else
      float out = Dsinf(ph);
#endif // TRUE_SINE

#ifdef LOGMUL
      out = loc_level_to_lut_log(out);
#endif // LOGMUL

      _d[i] = out;

      // Next x
      x += (1.0f / LOGWAV_TBLMASK);
   } // loop LOGWAV_TBLSZ
} /* end loc_calc_lut_logsinwarp() */

static void loc_prepare(st_plugin_voice_t *_voice,
                        const sUI  _numFramesOrig,
                        const sUI  _numFramesOS,
                        const sF32 _noteSpeedOS,
                        const sF32 _note
                        ) {
   ST_PLUGIN_VOICE_CAST(FMSTACK_VOICE_T);
   ST_PLUGIN_VOICE_SHARED_CAST(FMSTACK_SHARED_T);

   // Calc variation A/B and lerp amt
   if(shared->params[PARAM_VARIATION_EDIT_LOCK] >= 0.5f)
   {
      sUI varIdx = sUI(shared->params[PARAM_VARIATION_EDIT_IDX] * 100.0f) - 1u;
      varIdx &= 7u;
      voice->var_param_off_a = PARAM_VAR_BASE + (varIdx * NUM_PARAMS_PER_VAR);
      voice->var_param_off_b = voice->var_param_off_a;
      voice->var_b_amt = 0.0f;
   }
   else
   {
      sF32 varIdx = shared->params[PARAM_MGRP1_VAR] + voice->mods[MOD_MGRP1_VAR];
      if(varIdx < 0.0f)
         varIdx += 1.0f;
      if(varIdx < 0.0f)
         varIdx += 1.0f;
      sUI numVar = sUI(shared->params[PARAM_NUM_VARIATIONS] * 100.0f);
      if(numVar < 1u)
         numVar = 1u;
      else if(numVar > 8u)
         numVar = 8u;
      varIdx *= numVar;
      shared->params[PARAM_VARIATION_EFF] = varIdx;
      sUI varIdxA = sUI(varIdx) % numVar;
      sUI varIdxB = (varIdxA + 1u) % numVar;
      voice->var_param_off_a = PARAM_VAR_BASE + (varIdxA * NUM_PARAMS_PER_VAR);
      voice->var_param_off_b = PARAM_VAR_BASE + (varIdxB * NUM_PARAMS_PER_VAR);
      voice->var_b_amt = ffrac_s(varIdx);
   }

   // Calc effective modulation group amounts
   {
      // Reset ops
      for(sUI opIdx = 0u ; opIdx < (NUM_OPS + 1u); opIdx++)
      {
         FMSTACK_VOICE_OP_T *op = &voice->ops[opIdx];
         op->mgrp_mod.reset();
      }

      // Iterate modulation group destinations
      //  (note) first modulation group implicitely targets 'variation' (skip)
      for(sUI mgrpIdx = 1u; mgrpIdx < 8u; mgrpIdx++)
      {
         sUI mgrpDst = sUI(shared->params[PARAM_MGRP1_DEST + mgrpIdx] * 100.0f + 0.5f);

         if(MGRP_DST_NONE != mgrpDst)
         {
            sF32 amt = (shared->params[PARAM_MGRP1_AMT + mgrpIdx] * 2.0f - 1.0f) + voice->mods[MOD_MGRP1_AMT + mgrpIdx];
#if 0
            if(amt > 1.0f)
               amt = 1.0f;
            else if(amt < -1.0f)
               amt = -1.0f;
#endif
            sF32 amtAdd = amt;
            sF32 amtScl = loc_bipolar_to_scale(amt, 64.0f, 4.0f);

            sUI opMask = sUI(shared->params[PARAM_MGRP1_OP_MASK + mgrpIdx] * 100.0f);

            for(sUI opIdx = 0u; opIdx < (NUM_OPS + 1u); opIdx++)
            {
               if(opMask & (1u << opIdx))
               {
                  FMSTACK_VOICE_OP_T *op = &voice->ops[opIdx];

                  switch(mgrpDst)
                  {
                     default:
                     case MGRP_DST_NONE:
                        break;

                     case MGRP_DST_OP_LEVEL:           //  1
                        op->mgrp_mod.level_scl *= amtScl;
                        break;

                     case MGRP_DST_OP_LEVEL_ENV_AMT:   //  2
                        op->mgrp_mod.level_env_amt_scl *= amtScl;
                        break;

                     case MGRP_DST_OP_PITCH:           //  3
                        op->mgrp_mod.pitch_oct += amtAdd;
                        break;

                     case MGRP_DST_OP_PITCH_ENV_AMT:   //  4
                        op->mgrp_mod.pitch_env_amt_scl *= amtScl;
                        break;

                     case MGRP_DST_OP_PHASE:           //  5
                        op->mgrp_mod.phase_off += amtAdd;
                        break;

                     case MGRP_DST_MATRIX_OP_DST_AMT:  //  6
                        op->mgrp_mod.matrix_dst_amt_scl *= amtScl;
                        break;

                     case MGRP_DST_AENV_ATK:           //  7
                        op->mgrp_mod.aenv_atk_scl *= amtScl;
                        break;

                     case MGRP_DST_AENV_HLD:           //  8
                        op->mgrp_mod.aenv_hld_scl *= amtScl;
                        break;

                     case MGRP_DST_AENV_DCY:           //  9
                        op->mgrp_mod.aenv_dcy_scl *= amtScl;
                        break;

                     case MGRP_DST_AENV_SUS:           // 10
                        op->mgrp_mod.aenv_sus_scl *= amtScl;
                        break;

                     case MGRP_DST_AENV_RLS:           // 11
                        op->mgrp_mod.aenv_rls_scl *= amtScl;
                        break;

                     case MGRP_DST_AENV_DCY_RLS:       // 12
                        op->mgrp_mod.aenv_dcy_scl *= amtScl;
                        op->mgrp_mod.aenv_rls_scl *= amtScl;
                        break;

                     case MGRP_DST_PENV_ATK:           // 13
                        op->mgrp_mod.penv_atk_scl *= amtScl;
                        break;

                     case MGRP_DST_PENV_HLD:           // 14
                        op->mgrp_mod.penv_hld_scl *= amtScl;
                        break;

                     case MGRP_DST_PENV_DCY:           // 15
                        op->mgrp_mod.penv_dcy_scl *= amtScl;
                        break;

                     case MGRP_DST_PENV_SUS:           // 16
                        op->mgrp_mod.penv_sus_scl *= amtScl;
                        break;

                     case MGRP_DST_PENV_RLS:           // 17
                        op->mgrp_mod.penv_rls_scl *= amtScl;
                        break;

                     case MGRP_DST_PENV_DCY_RLS:       // 18
                        op->mgrp_mod.penv_dcy_scl *= amtScl;
                        op->mgrp_mod.penv_rls_scl *= amtScl;
                        break;

                     case MGRP_DST_ALL_ATK:            // 19
                        op->mgrp_mod.aenv_atk_scl *= amtScl;
                        op->mgrp_mod.penv_atk_scl *= amtScl;
                        break;

                     case MGRP_DST_ALL_HLD:            // 20
                        op->mgrp_mod.aenv_hld_scl *= amtScl;
                        op->mgrp_mod.penv_hld_scl *= amtScl;
                        break;

                     case MGRP_DST_ALL_DCY:            // 21
                        op->mgrp_mod.aenv_dcy_scl *= amtScl;
                        op->mgrp_mod.penv_dcy_scl *= amtScl;
                        break;

                     case MGRP_DST_ALL_SUS:            // 22
                        op->mgrp_mod.aenv_sus_scl *= amtScl;
                        op->mgrp_mod.penv_sus_scl *= amtScl;
                        break;

                     case MGRP_DST_ALL_RLS:            // 23
                        op->mgrp_mod.aenv_rls_scl *= amtScl;
                        op->mgrp_mod.penv_rls_scl *= amtScl;
                        break;

                     case MGRP_DST_ALL_DCY_RLS:        // 24
                        op->mgrp_mod.aenv_dcy_scl *= amtScl;
                        op->mgrp_mod.aenv_rls_scl *= amtScl;
                        op->mgrp_mod.penv_dcy_scl *= amtScl;
                        op->mgrp_mod.penv_rls_scl *= amtScl;
                        break;

                     case MGRP_DST_WS_A:               // 25
                        op->mgrp_mod.ws_a_off += amtAdd * 0.1f;
                        break;

                     case MGRP_DST_WS_B:               // 26
                        op->mgrp_mod.ws_b_off += amtAdd * 0.1f;
                        break;

                     case MGRP_DST_WS_AB_MIX:          // 27
                        op->mgrp_mod.ws_ab_mix_off += amtAdd;
                        break;

                     case MGRP_DST_OUT:                // 28
                        op->mgrp_mod.out_scl *= amtScl;
                        break;
                  }
               } // if opMask

            } // loop ops
         } // if (MGRP_DST_NONE != mgrpDst)
      } // loop mgrp
   } // calc mgrp_mod

   // Calc effective FM matrix source amounts and op level scaling
   {
      for(sUI opIdx = 0u; opIdx < (NUM_OPS + 1u); opIdx++)
      {
         FMSTACK_VOICE_OP_T *op = &voice->ops[opIdx];
         op->b_active = (voice->getVarParam(shared, PARAM_OP_BASE + (opIdx << 4)/*PARAM_OP_NUM*/ + PARAM_OP_LEVEL) > 0.0f);
         if(op->b_active)
         {
            for(sUI opDstIdx = 0u; opDstIdx < NUM_OPS; opDstIdx++)
            {
               FMSTACK_VOICE_OP_T *opDst = &voice->ops[opDstIdx];
               sF32 *d = opDst->effective_fmmatrix_src_amts;

               d[opIdx] = voice->getVarParam(shared, PARAM_MATRIX_BASE + ( (opIdx << 3) + opDstIdx )) * 16.0f * opDst->mgrp_mod.matrix_dst_amt_scl;
            }

            op->level_scl = op->mgrp_mod.level_scl;
         }
      }

#if 0
      for(sUI opIdx = 0u; opIdx < NUM_OPS; opIdx++)
      {
         FMSTACK_VOICE_OP_T *op = &voice->ops[opIdx];
         printf("xxx [%u] eff_src_amounts[] = { %f; %f; %f; %f; %f }\n", opIdx,
                op->effective_fmmatrix_src_amts[0],
                op->effective_fmmatrix_src_amts[1],
                op->effective_fmmatrix_src_amts[2],
                op->effective_fmmatrix_src_amts[3],
                op->effective_fmmatrix_src_amts[4]
                );
      }
#endif

   }

   // calc effective op to out
   {
      sUI paramIdx = PARAM_MATRIX_BASE + PARAM_MATRIX_OP_TO_OUT;
      for(sUI opIdx = 0u; opIdx < (NUM_OPS + 1u); opIdx++)
      {
         FMSTACK_VOICE_OP_T *op = &voice->ops[opIdx];
         op->effective_op_to_out = voice->getVarParam(shared, paramIdx) * 16.0f * op->mgrp_mod.out_scl;
         paramIdx += PARAM_MATRIX_OP_NUM;
      }
   }

   {
      sUI paramOff = PARAM_OP_BASE;
      sUI paramOffEnv = PARAM_AMP_ENV_BASE;
      for(sUI opIdx = 0u; opIdx < (NUM_OPS + 1u); opIdx++)
      {
         FMSTACK_VOICE_OP_T *op = &voice->ops[opIdx];
         if(op->b_active)
            op->prepareBlock(shared, voice, paramOff, paramOffEnv, _numFramesOrig, _numFramesOS, _noteSpeedOS, _note);
         paramOff += PARAM_OP_NUM/*16*/;
         paramOffEnv += NUM_ENV_PARAMS_PER_OP/*8*/;
      }
   }

} /* end prepare */

static const char *ST_PLUGIN_API loc_get_param_name(st_plugin_info_t *_info,
                                                    unsigned int      _paramIdx
                                                    ) {
   (void)_info;
   return loc_param_names[_paramIdx];
}

static unsigned int loc_copy_chars(char *_d, const unsigned int _dSize, const char *_s) {
   unsigned int r = 0u;
   if(NULL != _d)
   {
      // Write min(dSize,len(s)) characters to 'd'
      char *d = _d;
      if(NULL != _s)
      {
         const char *s = _s;
         for(;;)
         {
            if(r < _dSize)
            {
               char c = s[r++];
               *d++ = c;
               if(0 == c)
                  break;
            }
            else
            {
               if(r > 0u)
                  d[-1] = 0;  // force asciiz
               break;
            }
         }
      }
      else
      {
         if(_dSize > 0u)
         {
            _d[0] = 0;
            r = 1u;
         }
      }
   }
   else
   {
      // Return total length of source string
      if(NULL != _s)
      {
         const char *s = _s;
         for(;;)
         {
            char c = s[r++];
            if(0 == c)
               break;
         }
      }
   }
   return r;
}

static unsigned int ST_PLUGIN_API loc_query_dynamic_param_name(st_plugin_shared_t *_shared,
                                                               const unsigned int  _paramIdx,
                                                               char               *_retBuf,
                                                               const unsigned int  _retBufSize
                                                               ) {
   ST_PLUGIN_SHARED_CAST(FMSTACK_SHARED_T);
   if(0u == _paramIdx)
   {
      return loc_copy_chars(_retBuf, _retBufSize, "Variation");
   }
   else if(_paramIdx < 8u)
   {
      sUI mgrpDst = sUI(shared->params[PARAM_MGRP1_DEST + _paramIdx] * 100.0f + 0.5f);
      if(mgrpDst < MGRP_DST_NUM)
      {
         return loc_copy_chars(_retBuf, _retBufSize, loc_mgrp_dst_names[mgrpDst]);
      }
      else
      {
         return loc_copy_chars(_retBuf, _retBufSize, "?");
      }
   }
   else
   {
      return loc_copy_chars(_retBuf, _retBufSize, loc_param_names[_paramIdx]);
   }
}

static unsigned int loc_copy_floats(float              *_d,
                                    const unsigned int  _dSize,
                                    const float        *_s,
                                    const unsigned int  _sNum
                                    ) {
   unsigned int r = 0u;
   if(NULL != _d)
   {
      // Write min(dSize,sNum) characters to 'd'
      if(NULL != _s)
      {
         unsigned int num = (_sNum > _dSize) ? _dSize : _sNum;
         memcpy((void*)_d, (const void*)_s, num * sizeof(float));
         r = num;
      }
   }
   else
   {
      // Query total number of presets
      r = _sNum;
   }
   return r;
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
   ST_PLUGIN_SHARED_CAST(FMSTACK_SHARED_T);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(FMSTACK_SHARED_T);
   shared->params[_paramIdx] = _value;
}

static const char *ST_PLUGIN_API loc_get_mod_name(st_plugin_info_t *_info,
                                                  unsigned int      _modIdx
                                                  ) {
   (void)_info;
   return loc_mod_names[_modIdx];
}

static unsigned int ST_PLUGIN_API loc_query_dynamic_mod_name(st_plugin_shared_t *_shared,
                                                             const unsigned int  _modIdx,
                                                             char               *_retBuf,
                                                             const unsigned int  _retBufSize
                                                             ) {
   ST_PLUGIN_SHARED_CAST(FMSTACK_SHARED_T);
   if(0u == _modIdx)
   {
      return loc_copy_chars(_retBuf, _retBufSize, "Variation");
   }
   else
   {
      sUI mgrpDst = sUI(shared->params[PARAM_MGRP1_DEST + _modIdx] * 100.0f + 0.5f);
      if(mgrpDst < MGRP_DST_NUM)
      {
         return loc_copy_chars(_retBuf, _retBufSize, loc_mgrp_dst_names[mgrpDst]);
      }
      else
      {
         return loc_copy_chars(_retBuf, _retBufSize, "?");
      }
   }
}

static unsigned int ST_PLUGIN_API loc_query_dynamic_param_preset_values(st_plugin_shared_t *_shared,
                                                                        const unsigned int  _paramIdx,
                                                                        float              *_retValues,
                                                                        const unsigned int  _retValuesSize
                                                                        ) {
   // (note) preset values are also encoded in SamplePluginFMStack* UI script classes
   ST_PLUGIN_SHARED_CAST(FMSTACK_SHARED_T);
   sUI r = 0u;
   if(_paramIdx > 0u)  // 0 is "Variation"
   {
      sUI mgrpDst = sUI(shared->params[PARAM_MGRP1_DEST + _paramIdx] * 100.0f + 0.5f);
      // printf("xxx fm_stack:loc_query_dynamic_param_preset_values: paramIdx=%u => mgrpDst=%u\n", _paramIdx, mgrpDst);
      if(MGRP_DST_WS_A == mgrpDst || MGRP_DST_WS_B == mgrpDst)
      {
         if(NULL != _retValues)
         {
            r = (_retValuesSize < NUM_WAVES) ? _retValuesSize : NUM_WAVES;
            for(sUI i = 0u; i < r; i++)
               _retValues[i] = i / 100.0f;
         }
         else
         {
            // Query 'retValues' size
            r = NUM_WAVES;
         }
      }
   }
   return r;
}

static unsigned int ST_PLUGIN_API loc_query_dynamic_param_preset_name(st_plugin_shared_t *_shared,
                                                                      const unsigned int  _paramIdx,
                                                                      const unsigned int  _presetIdx,
                                                                      char               *_retBuf,
                                                                      const unsigned int  _retBufSize
                                                                      ) {
   ST_PLUGIN_SHARED_CAST(FMSTACK_SHARED_T);
   sUI r = 0u;
   if(_paramIdx > 0u)  // 0 is "Variation"
   {
      sUI mgrpDst = sUI(shared->params[PARAM_MGRP1_DEST + _paramIdx] * 100.0f + 0.5f);
      if(MGRP_DST_WS_A == mgrpDst || MGRP_DST_WS_B == mgrpDst)
      {
         if(_presetIdx < NUM_WAVES)
            r = loc_copy_chars(_retBuf, _retBufSize, loc_wave_names[_presetIdx]);
      }
   }
   return r;
}

static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(FMSTACK_VOICE_T);
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(FMSTACK_VOICE_T);
   ST_PLUGIN_VOICE_SHARED_CAST(FMSTACK_SHARED_T);
   voice->b_glide = _bGlide;
   (void)_note;

   if(_bGlide < 1)
   {
      // No gliding (bGlide=0) or glide with 'smp' reset (bGlide=-1)
      voice->velocity = _vel;
      voice->velocity_255 = sUI(_vel * 255u) & 255u;
   }

   if(0 == _bGlide)
   {
      // No gliding (reset voice modulation)
      memset((void*)voice->mods, 0, sizeof(voice->mods));
   }

   if(_bGlide < 1)
   {
      // Send noteOn() to operators
      sUI paramOff = 0u;
      sUI paramOffEnv = PARAM_AMP_ENV_BASE;
      for(sUI opIdx = 0u; opIdx < (NUM_OPS + 1u); opIdx++)
      {
         FMSTACK_VOICE_OP_T *op = &voice->ops[opIdx];
         op->noteOn(shared, voice, paramOff, paramOffEnv, _bGlide);
         paramOff += PARAM_OP_NUM/*16*/;
         paramOffEnv += NUM_ENV_PARAMS_PER_OP/*8*/;
      }
   }
}

static void ST_PLUGIN_API loc_note_off(st_plugin_voice_t  *_voice,
                                       unsigned char       _note,
                                       float               _vel
                                       ) {
   ST_PLUGIN_VOICE_CAST(FMSTACK_VOICE_T);
   ST_PLUGIN_VOICE_SHARED_CAST(FMSTACK_SHARED_T);
   (void)_note;
   (void)_vel;

   for(sUI opIdx = 0u; opIdx < (NUM_OPS + 1u); opIdx++)
   {
      FMSTACK_VOICE_OP_T *op = &voice->ops[opIdx];
      op->noteOff();
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(FMSTACK_VOICE_T);
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
   ST_PLUGIN_VOICE_CAST(FMSTACK_VOICE_T);
   ST_PLUGIN_VOICE_SHARED_CAST(FMSTACK_SHARED_T);
   (void)_vol;
   (void)_pan;

#ifdef OVERSAMPLE_FACTOR
   float noteSpeedOS = _freqHz / (voice->sample_rate * OVERSAMPLE_FACTOR);
   sUI numFramesOS = _numFrames * sUI(OVERSAMPLE_FACTOR);
#else
   float noteSpeedOS = _freqHz / voice->sample_rate;
   sUI numFramesOS = _numFrames;
#endif // OVERSAMPLE_FACTOR

   if(numFramesOS > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / numFramesOS);
      voice->note_inc = (_note - voice->note_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->note_cur = _note;
      voice->note_inc = 0.0f;
   }

   loc_prepare(&voice->base, _numFrames, numFramesOS, noteSpeedOS, _note);
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut,
                                              unsigned int        _numFrames
                                              ) {
   (void)_bMonoIn;
   (void)_samplesIn;

   ST_PLUGIN_VOICE_CAST(FMSTACK_VOICE_T);
   ST_PLUGIN_VOICE_SHARED_CAST(FMSTACK_SHARED_T);

   // Mono output (replicate left to right channel)
   sUI k = 0u;
   for(sUI i = 0u; i < _numFrames; i++)
   {
      // Calc Op outputs
      {
         // Op 1..4
#ifdef MODFM
         sUI modFMOpBit = 1u;
#endif // MODFM
         for(sUI opIdx = 0u; opIdx < NUM_OPS; opIdx++)
         {
            FMSTACK_VOICE_OP_T *op = &voice->ops[opIdx];
            if(op->b_active)
               op->calcOut(shared, voice
#ifdef MODFM
                           , modFMOpBit
#endif // MODFM
                           );
#ifdef MODFM
            modFMOpBit = modFMOpBit << 1;
#endif // MODFM
         }

         // Op 5
         {
            FMSTACK_VOICE_OP_T *op = &voice->ops[4];
            if(op->b_active)
               op->calcOutOp5(shared, voice, k, _samplesIn[k]);
         }
      }

      // Sum Op outputs
      sF32 out = 0.0f;
      for(sUI opIdx = 0u; opIdx < (NUM_OPS + 1u); opIdx++)
      {
         FMSTACK_VOICE_OP_T *op = &voice->ops[opIdx];
         out += op->last_out * op->effective_op_to_out;
      }

#ifdef FMSTACK_HPF
      out = voice->hpf.filterNoStep(out);
#endif // FMSTACK_HPF

      _samplesOut[k]      = out;
      _samplesOut[k + 1u] = out;

      // Next frame
      k += 2u;
   } /* loop numFrames */
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   FMSTACK_SHARED_T *ret = (FMSTACK_SHARED_T *)malloc(sizeof(FMSTACK_SHARED_T));
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
   FMSTACK_VOICE_T *voice = (FMSTACK_VOICE_T *)malloc(sizeof(FMSTACK_VOICE_T));
   if(NULL != voice)
   {
      memset((void*)voice, 0, sizeof(*voice));
      voice->base.info = _info;

      for(sUI opIdx = 0u; opIdx < NUM_OPS; opIdx++)
      {
         FMSTACK_VOICE_OP_T *op = &voice->ops[opIdx];
         sUI t = sUI(size_t(op)) * (_voiceIdx + 1u);
         op->pha_lfsr_state = (unsigned short)((t >> 4) ^ (t >> 16));
      }

#ifdef FMSTACK_HPF
      voice->hpf.reset();
      voice->hpf.calcParamsNoStep(StBiquad::HPF,
                                  0.0f/*gainDB*/,
                                  0.001f/*freq*/,
                                  0.05f/*q*/
                                  );
#endif // FMSTACK_HPF
   }
   return &voice->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(FMSTACK_VOICE_T);

   free((void*)_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {
   free((void*)_info);
}

static void loc_calc_sine_tbl(void) {
#define QCOS_BITS 16
#define QCOS_ONE  (1 << QCOS_BITS)
#define QCOS_MASK (QCOS_ONE - 1)
   float *qcos = loc_sine_tbl_f + 4096; // quarter cos tbl

   // calc quarter cos tbl
   int k = 0;
   for(int a = 0; a < (QCOS_ONE/4); a += (QCOS_ONE/16384)/*4*/)  // 4096 elements
   {
      int x = a - ((int)(QCOS_ONE * 0.25 + 0.5)) + ((a + ((int)(QCOS_ONE * 0.25 + 0.5)))&~QCOS_MASK);
      x = ((x * ((x<0?-x:x) - ((int)(QCOS_ONE * 0.5 + 0.5)))) >> QCOS_BITS) << 4;
      x += (((((int)(QCOS_ONE * 0.225 + 0.5)) * x) >> QCOS_BITS) * ((x<0?-x:x) - QCOS_ONE)) >> QCOS_BITS;
      qcos[k++] = x / float(QCOS_ONE);
   }

   // 0°..90° (rev qtbl)
   int j = 4096;
   k = 0;
   loop(4096)
      loc_sine_tbl_f[k++] = qcos[--j];

   // 90..180° = cos tbl
   k += 4096;

   // 180°..360° (y-flip first half of tbl)
   j = 0;
   loop(8192)
      loc_sine_tbl_f[k++] = -loc_sine_tbl_f[j++];
}

st_plugin_info_t *FMSTACK_INIT(void) {
   FMSTACK_INFO_T *ret = (FMSTACK_INFO_T *)malloc(sizeof(FMSTACK_INFO_T));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      // printf("[trc] fmstack id=\"%s\" sizeof(struct)=%lu\n", FMSTACK_PLUGIN_ID, sizeof(FMSTACK_VOICE_OP_T));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = FMSTACK_PLUGIN_ID;  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = FMSTACK_PLUGIN_NAME;
      ret->base.short_name  = FMSTACK_PLUGIN_SHORT_NAME;
      ret->base.flags       = ST_PLUGIN_FLAG_OSC;
      ret->base.category    = ST_PLUGIN_CAT_UNKNOWN;
      ret->base.num_params  = NUM_PARAMS;
      ret->base.num_mods    = NUM_MODS;

      ret->base.shared_new                        = &loc_shared_new;
      ret->base.shared_delete                     = &loc_shared_delete;
      ret->base.voice_new                         = &loc_voice_new;
      ret->base.voice_delete                      = &loc_voice_delete;
      ret->base.get_param_name                    = &loc_get_param_name;
      ret->base.query_dynamic_param_name          = &loc_query_dynamic_param_name;
      ret->base.query_dynamic_param_preset_values = &loc_query_dynamic_param_preset_values;
      ret->base.query_dynamic_param_preset_name   = &loc_query_dynamic_param_preset_name;
      ret->base.get_param_reset                   = &loc_get_param_reset;
      ret->base.get_param_value                   = &loc_get_param_value;
      ret->base.set_param_value                   = &loc_set_param_value;
      ret->base.get_mod_name                      = &loc_get_mod_name;
      ret->base.query_dynamic_mod_name            = &loc_query_dynamic_mod_name;
      ret->base.set_sample_rate                   = &loc_set_sample_rate;
      ret->base.note_on                           = &loc_note_on;
      ret->base.note_off                          = &loc_note_off;
      ret->base.set_mod_value                     = &loc_set_mod_value;
      ret->base.prepare_block                     = &loc_prepare_block;
      ret->base.process_replace                   = &loc_process_replace;
      ret->base.plugin_exit                       = &loc_plugin_exit;

      loc_calc_sine_tbl();

      loc_calc_lut_logconst(loc_lut_logzero, 0.0f);
      loc_calc_lut_logsin  (loc_lut_logsin, 0.0f);
      loc_calc_lut_logsin_2(loc_lut_logsin_2);
      loc_calc_lut_logsin_3(loc_lut_logsin_3);
      loc_calc_lut_logsin_4(loc_lut_logsin_4);
      loc_calc_lut_logsin_5(loc_lut_logsin_5);
      loc_calc_lut_logsin_6(loc_lut_logsin_6);
      loc_calc_lut_logsin_7(loc_lut_logsin_7);
      loc_calc_lut_logsin_8(loc_lut_logsin_8);
      loc_calc_lut_logtri  (loc_lut_logtri,    0.25f);
      loc_calc_lut_logtri  (loc_lut_logtri90,  0.50f);
      loc_calc_lut_logtri  (loc_lut_logtri180, 0.75f);
      loc_calc_lut_logtri  (loc_lut_logtri270, 0.00f);
      // odd power (bipolar)
      loc_calc_lut_logsinp (loc_lut_logsinp1,    3.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp2,    5.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp3,    7.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp4,    9.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp5,   11.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp6,   13.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp7,   15.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp8,   17.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp9,   19.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp10,  25.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp11,  31.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp12,  43.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp13,  67.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp14,  89.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp15, 143.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp16, 199.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp17, 273.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp18, 421.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp19, 645.0f);
      loc_calc_lut_logsinp (loc_lut_logsinp20, 999.0f);
      // even power (unipolar)
      loc_calc_lut_logsinpe (loc_lut_logsinpe1,    1.600f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe2,    2.000f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe3,    3.000f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe4,    4.000f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe5,    5.200f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe6,    6.000f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe7,    7.200f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe8,    8.000f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe9,    9.200f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe10,  10.000f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe11,  12.000f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe12,  16.000f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe13,  24.000f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe14,  36.000f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe15,  56.000f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe16,  90.000f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe17, 140.000f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe18, 240.000f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe19, 440.000f);
      loc_calc_lut_logsinpe (loc_lut_logsinpe20, 990.000f);
      // square-ish:
      loc_calc_lut_logsinpq(loc_lut_logsinsq1,  0.040f);
      loc_calc_lut_logsinpq(loc_lut_logsinsq2,  0.072f);
      loc_calc_lut_logsinpq(loc_lut_logsinsq3,  0.120f);
      loc_calc_lut_logsinpq(loc_lut_logsinsq4,  0.200f);
      loc_calc_lut_logsinpq(loc_lut_logsinsq5,  0.360f);
      loc_calc_lut_logsinpq(loc_lut_logsinsq6,  0.440f);
      loc_calc_lut_logsinpq(loc_lut_logsinsq7,  0.520f);
      loc_calc_lut_logsinpq(loc_lut_logsinsq8,  0.680f);
      loc_calc_lut_logsinpq(loc_lut_logsinsq9,  0.760f);
      loc_calc_lut_logsinpq(loc_lut_logsinsq10, 0.840f);
      // square-gap:
      loc_calc_lut_logsinpe(loc_lut_logsinsg1,  0.07f);
      loc_calc_lut_logsinpe(loc_lut_logsinsg2,  0.18f);
      loc_calc_lut_logsinpe(loc_lut_logsinsg3,  0.25f);
      loc_calc_lut_logsinpe(loc_lut_logsinsg4,  0.37f);
      loc_calc_lut_logsinpe(loc_lut_logsinsg5,  0.49f);
      loc_calc_lut_logsinpe(loc_lut_logsinsg6,  0.57f);
      loc_calc_lut_logsinpe(loc_lut_logsinsg7,  0.69f);
      loc_calc_lut_logsinpe(loc_lut_logsinsg8,  0.77f);
      loc_calc_lut_logsinpe(loc_lut_logsinsg9,  0.89f);
      loc_calc_lut_logsinpe(loc_lut_logsinsg10, 0.99f);
      // double-square:
      loc_calc_lut_logsinpd(loc_lut_logsinsd1,  0.08f);
      loc_calc_lut_logsinpd(loc_lut_logsinsd2,  0.16f);
      loc_calc_lut_logsinpd(loc_lut_logsinsd3,  0.24f);
      loc_calc_lut_logsinpd(loc_lut_logsinsd4,  0.336f);
      loc_calc_lut_logsinpd(loc_lut_logsinsd5,  0.448f);
      loc_calc_lut_logsinpd(loc_lut_logsinsd6,  0.560f);
      loc_calc_lut_logsinpd(loc_lut_logsinsd7,  0.672f);
      loc_calc_lut_logsinpd(loc_lut_logsinsd8,  0.752f);
      loc_calc_lut_logsinpd(loc_lut_logsinsd9,  0.848f);
      loc_calc_lut_logsinpd(loc_lut_logsinsd10, 0.992f);
      // sine-sync:
      loc_calc_lut_logsinsy(loc_lut_logsinsy1,  1.0f + 7.0f * ( 0.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy2,  1.0f + 7.0f * ( 1.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy3,  1.0f + 7.0f * ( 2.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy4,  1.0f + 7.0f * ( 3.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy5,  1.0f + 7.0f * ( 4.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy6,  1.0f + 7.0f * ( 5.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy7,  1.0f + 7.0f * ( 6.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy8,  1.0f + 7.0f * ( 7.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy9,  1.0f + 7.0f * ( 8.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy10, 1.0f + 7.0f * ( 9.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy11, 1.0f + 7.0f * (10.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy12, 1.0f + 7.0f * (11.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy13, 1.0f + 7.0f * (12.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy14, 1.0f + 7.0f * (13.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy15, 1.0f + 7.0f * (14.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy16, 1.0f + 7.0f * (15.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy17, 1.0f + 7.0f * (16.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy18, 1.0f + 7.0f * (17.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy19, 1.0f + 7.0f * (18.0f / 19.0f));
      loc_calc_lut_logsinsy(loc_lut_logsinsy20, 1.0f + 7.0f * (19.0f / 19.0f));
      // sine-drive:
      loc_calc_lut_logsindr(loc_lut_logsindr1,  1.0f + 9.0f * ( 0.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr2,  1.0f + 9.0f * ( 1.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr3,  1.0f + 9.0f * ( 2.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr4,  1.0f + 9.0f * ( 3.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr5,  1.0f + 9.0f * ( 4.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr6,  1.0f + 9.0f * ( 5.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr7,  1.0f + 9.0f * ( 6.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr8,  1.0f + 9.0f * ( 7.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr9,  1.0f + 9.0f * ( 8.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr10, 1.0f + 9.0f * ( 9.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr11, 10.0f + 30.0f * ( 0.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr12, 10.0f + 30.0f * ( 1.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr13, 10.0f + 30.0f * ( 2.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr14, 10.0f + 30.0f * ( 3.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr15, 10.0f + 30.0f * ( 4.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr16, 10.0f + 30.0f * ( 5.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr17, 10.0f + 30.0f * ( 6.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr18, 10.0f + 30.0f * ( 7.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr19, 10.0f + 30.0f * ( 8.0f / 9.0f));
      loc_calc_lut_logsindr(loc_lut_logsindr20, 10.0f + 30.0f * ( 9.0f / 9.0f));
      // sine-saw up:
      loc_calc_lut_logsinsw(loc_lut_logsawup1,  (0.0f / 9.0f), 0/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawup2,  (1.0f / 9.0f), 0/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawup3,  (2.0f / 9.0f), 0/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawup4,  (3.0f / 9.0f), 0/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawup5,  (4.0f / 9.0f), 0/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawup6,  (5.0f / 9.0f), 0/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawup7,  (6.0f / 9.0f), 0/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawup8,  (7.0f / 9.0f), 0/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawup9,  (8.0f / 9.0f), 0/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawup10, (9.0f / 9.0f), 0/*bFlip*/);
      // sine-saw down:
      loc_calc_lut_logsinsw(loc_lut_logsawdown1,  (0.0f / 9.0f), 1/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawdown2,  (1.0f / 9.0f), 1/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawdown3,  (2.0f / 9.0f), 1/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawdown4,  (3.0f / 9.0f), 1/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawdown5,  (4.0f / 9.0f), 1/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawdown6,  (5.0f / 9.0f), 1/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawdown7,  (6.0f / 9.0f), 1/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawdown8,  (7.0f / 9.0f), 1/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawdown9,  (8.0f / 9.0f), 1/*bFlip*/);
      loc_calc_lut_logsinsw(loc_lut_logsawdown10, (9.0f / 9.0f), 1/*bFlip*/);
      // sine-bitreduction:
      loc_calc_lut_logsinbr(loc_lut_logsinbr1,   8.00f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr2,   7.00f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr3,   6.50f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr4,   6.00f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr5,   5.75f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr6,   5.50f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr7,   5.25f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr8,   5.00f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr9,   4.75f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr10,  4.50f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr11,  4.25f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr12,  4.00f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr13,  3.80f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr14,  3.60f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr15,  3.40f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr16,  3.20f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr17,  3.00f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr18,  2.75f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr19,  2.50f  );
      loc_calc_lut_logsinbr(loc_lut_logsinbr20,  2.25f  );
      // sine-sampleratereduction:
      loc_calc_lut_logsinsr(loc_lut_logsinsr1,   1.0f +  1.00f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr2,   1.0f +  3.00f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr3,   1.0f +  5.00f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr4,   1.0f +  6.00f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr5,   1.0f +  6.50f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr6,   1.0f +  7.00f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr7,   1.0f +  7.50f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr8,   1.0f +  8.00f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr9,   1.0f +  8.25f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr10,  1.0f +  8.50f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr11,  1.0f +  8.75f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr12,  1.0f +  9.00f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr13,  1.0f +  9.25f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr14,  1.0f +  9.50f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr15,  1.0f +  9.75f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr16,  1.0f + 10.00f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr17,  1.0f + 10.25f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr18,  1.0f + 10.50f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr19,  1.0f + 10.75f);
      loc_calc_lut_logsinsr(loc_lut_logsinsr20,  1.0f + 11.00f);
      // sine-saw-square:
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq1,   1.10f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq2,   1.20f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq3,   1.30f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq4,   1.40f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq5,   1.50f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq6,   1.60f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq7,   1.70f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq8,   1.80f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq9,   1.90f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq10,  2.00f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq11,  2.20f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq12,  2.40f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq13,  2.60f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq14,  2.80f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq15,  3.00f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq16,  3.20f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq17,  3.40f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq18,  3.60f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq19,  3.80f  );
      loc_calc_lut_logsinsawsq(loc_lut_logsinsawsq20,  4.00f  );
      // sine-bend:
      loc_calc_lut_logsinbd(loc_lut_logsinbd1,  1.0f + 7.0f * ( 0.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd2,  1.0f + 7.0f * ( 1.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd3,  1.0f + 7.0f * ( 2.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd4,  1.0f + 7.0f * ( 3.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd5,  1.0f + 7.0f * ( 4.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd6,  1.0f + 7.0f * ( 5.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd7,  1.0f + 7.0f * ( 6.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd8,  1.0f + 7.0f * ( 7.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd9,  1.0f + 7.0f * ( 8.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd10, 1.0f + 7.0f * ( 9.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd11, 1.0f + 7.0f * (10.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd12, 1.0f + 7.0f * (11.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd13, 1.0f + 7.0f * (12.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd14, 1.0f + 7.0f * (13.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd15, 1.0f + 7.0f * (14.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd16, 1.0f + 7.0f * (15.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd17, 1.0f + 7.0f * (16.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd18, 1.0f + 7.0f * (17.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd19, 1.0f + 7.0f * (18.0f / 19.0f));
      loc_calc_lut_logsinbd(loc_lut_logsinbd20, 1.0f + 7.0f * (19.0f / 19.0f));
      // sine-warp:
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp1,  1.0f + 7.0f * ( 0.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp2,  1.0f + 7.0f * ( 1.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp3,  1.0f + 7.0f * ( 2.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp4,  1.0f + 7.0f * ( 3.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp5,  1.0f + 7.0f * ( 4.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp6,  1.0f + 7.0f * ( 5.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp7,  1.0f + 7.0f * ( 6.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp8,  1.0f + 7.0f * ( 7.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp9,  1.0f + 7.0f * ( 8.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp10, 1.0f + 7.0f * ( 9.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp11, 1.0f + 7.0f * (10.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp12, 1.0f + 7.0f * (11.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp13, 1.0f + 7.0f * (12.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp14, 1.0f + 7.0f * (13.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp15, 1.0f + 7.0f * (14.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp16, 1.0f + 7.0f * (15.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp17, 1.0f + 7.0f * (16.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp18, 1.0f + 7.0f * (17.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp19, 1.0f + 7.0f * (18.0f / 19.0f));
      loc_calc_lut_logsinwarp(loc_lut_logsinwarp20, 1.0f + 7.0f * (19.0f / 19.0f));
      // sine-phase:
      loc_calc_lut_logsin(loc_lut_logsinph1,  0.0f + ( 0.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph2,  0.0f + ( 1.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph3,  0.0f + ( 2.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph4,  0.0f + ( 3.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph5,  0.0f + ( 4.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph6,  0.0f + ( 5.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph7,  0.0f + ( 6.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph8,  0.0f + ( 7.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph9,  0.0f + ( 8.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph10, 0.0f + ( 9.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph11, 0.0f + (10.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph12, 0.0f + (11.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph13, 0.0f + (12.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph14, 0.0f + (13.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph15, 0.0f + (14.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph16, 0.0f + (15.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph17, 0.0f + (16.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph18, 0.0f + (17.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph19, 0.0f + (18.0f / 20.0f));
      loc_calc_lut_logsin(loc_lut_logsinph20, 0.0f + (19.0f / 20.0f));


#ifdef SAVE
      loc_save_file_open("fmstack_lut.dat");
      printf("xxx save \"fmstack_lut.dat\"\n");
      /*   0 */ loc_save_lut(loc_lut_logzero);
      /*   1 */ loc_save_lut(loc_lut_logsin);
      /*   2 */ loc_save_lut(loc_lut_logsin_2);
      /*   3 */ loc_save_lut(loc_lut_logsin_3);
      /*   4 */ loc_save_lut(loc_lut_logsin_4);
      /*   5 */ loc_save_lut(loc_lut_logsin_5);
      /*   6 */ loc_save_lut(loc_lut_logsin_6);
      /*   7 */ loc_save_lut(loc_lut_logsin_7);
      /*   8 */ loc_save_lut(loc_lut_logsin_8);
      /*   9 */ loc_save_lut(loc_lut_logtri);
      /*  10 */ loc_save_lut(loc_lut_logtri90);
      /*  11 */ loc_save_lut(loc_lut_logtri180);
      /*  12 */ loc_save_lut(loc_lut_logtri270);
      // odd power (bipolar)
      /*  13 */ loc_save_lut (loc_lut_logsinp1);
      /*  14 */ loc_save_lut (loc_lut_logsinp2);
      /*  15 */ loc_save_lut (loc_lut_logsinp3);
      /*  16 */ loc_save_lut (loc_lut_logsinp4);
      /*  17 */ loc_save_lut (loc_lut_logsinp5);
      /*  18 */ loc_save_lut (loc_lut_logsinp6);
      /*  19 */ loc_save_lut (loc_lut_logsinp7);
      /*  20 */ loc_save_lut (loc_lut_logsinp8);
      /*  21 */ loc_save_lut (loc_lut_logsinp9);
      /*  22 */ loc_save_lut (loc_lut_logsinp10);
      /*  23 */ loc_save_lut (loc_lut_logsinp11);
      /*  24 */ loc_save_lut (loc_lut_logsinp12);
      /*  25 */ loc_save_lut (loc_lut_logsinp13);
      /*  26 */ loc_save_lut (loc_lut_logsinp14);
      /*  27 */ loc_save_lut (loc_lut_logsinp15);
      /*  28 */ loc_save_lut (loc_lut_logsinp16);
      /*  29 */ loc_save_lut (loc_lut_logsinp17);
      /*  30 */ loc_save_lut (loc_lut_logsinp18);
      /*  31 */ loc_save_lut (loc_lut_logsinp19);
      /*  32 */ loc_save_lut (loc_lut_logsinp20);
      // even power (unipolar)
      /*  33 */ loc_save_lut (loc_lut_logsinpe1);
      /*  34 */ loc_save_lut (loc_lut_logsinpe2);
      /*  35 */ loc_save_lut (loc_lut_logsinpe3);
      /*  36 */ loc_save_lut (loc_lut_logsinpe4);
      /*  37 */ loc_save_lut (loc_lut_logsinpe5);
      /*  38 */ loc_save_lut (loc_lut_logsinpe6);
      /*  39 */ loc_save_lut (loc_lut_logsinpe7);
      /*  40 */ loc_save_lut (loc_lut_logsinpe8);
      /*  41 */ loc_save_lut (loc_lut_logsinpe9);
      /*  42 */ loc_save_lut (loc_lut_logsinpe10);
      /*  43 */ loc_save_lut (loc_lut_logsinpe11);
      /*  44 */ loc_save_lut (loc_lut_logsinpe12);
      /*  45 */ loc_save_lut (loc_lut_logsinpe13);
      /*  46 */ loc_save_lut (loc_lut_logsinpe14);
      /*  47 */ loc_save_lut (loc_lut_logsinpe15);
      /*  48 */ loc_save_lut (loc_lut_logsinpe16);
      /*  49 */ loc_save_lut (loc_lut_logsinpe17);
      /*  50 */ loc_save_lut (loc_lut_logsinpe18);
      /*  51 */ loc_save_lut (loc_lut_logsinpe19);
      /*  52 */ loc_save_lut (loc_lut_logsinpe20);
      // square-ish:
      /*  53 */ loc_save_lut(loc_lut_logsinsq1);
      /*  54 */ loc_save_lut(loc_lut_logsinsq2);
      /*  55 */ loc_save_lut(loc_lut_logsinsq3);
      /*  56 */ loc_save_lut(loc_lut_logsinsq4);
      /*  57 */ loc_save_lut(loc_lut_logsinsq5);
      /*  58 */ loc_save_lut(loc_lut_logsinsq6);
      /*  59 */ loc_save_lut(loc_lut_logsinsq7);
      /*  60 */ loc_save_lut(loc_lut_logsinsq8);
      /*  61 */ loc_save_lut(loc_lut_logsinsq9);
      /*  62 */ loc_save_lut(loc_lut_logsinsq10);
      // square-gap:
      /*  63 */ loc_save_lut(loc_lut_logsinsg1);
      /*  64 */ loc_save_lut(loc_lut_logsinsg2);
      /*  65 */ loc_save_lut(loc_lut_logsinsg3);
      /*  66 */ loc_save_lut(loc_lut_logsinsg4);
      /*  67 */ loc_save_lut(loc_lut_logsinsg5);
      /*  68 */ loc_save_lut(loc_lut_logsinsg6);
      /*  69 */ loc_save_lut(loc_lut_logsinsg7);
      /*  70 */ loc_save_lut(loc_lut_logsinsg8);
      /*  71 */ loc_save_lut(loc_lut_logsinsg9);
      /*  72 */ loc_save_lut(loc_lut_logsinsg10);
      // double-square:
      loc_save_lut(loc_lut_logsinsd1);
      loc_save_lut(loc_lut_logsinsd2);
      loc_save_lut(loc_lut_logsinsd3);
      loc_save_lut(loc_lut_logsinsd4);
      loc_save_lut(loc_lut_logsinsd5);
      loc_save_lut(loc_lut_logsinsd6);
      loc_save_lut(loc_lut_logsinsd7);
      loc_save_lut(loc_lut_logsinsd8);
      loc_save_lut(loc_lut_logsinsd9);
      loc_save_lut(loc_lut_logsinsd10);
      // sine-sync:
      loc_save_lut(loc_lut_logsinsy1);
      loc_save_lut(loc_lut_logsinsy2);
      loc_save_lut(loc_lut_logsinsy3);
      loc_save_lut(loc_lut_logsinsy4);
      loc_save_lut(loc_lut_logsinsy5);
      loc_save_lut(loc_lut_logsinsy6);
      loc_save_lut(loc_lut_logsinsy7);
      loc_save_lut(loc_lut_logsinsy8);
      loc_save_lut(loc_lut_logsinsy9);
      loc_save_lut(loc_lut_logsinsy10);
      loc_save_lut(loc_lut_logsinsy11);
      loc_save_lut(loc_lut_logsinsy12);
      loc_save_lut(loc_lut_logsinsy13);
      loc_save_lut(loc_lut_logsinsy14);
      loc_save_lut(loc_lut_logsinsy15);
      loc_save_lut(loc_lut_logsinsy16);
      loc_save_lut(loc_lut_logsinsy17);
      loc_save_lut(loc_lut_logsinsy18);
      loc_save_lut(loc_lut_logsinsy19);
      loc_save_lut(loc_lut_logsinsy20);
      // sine-drive:
      loc_save_lut(loc_lut_logsindr1);
      loc_save_lut(loc_lut_logsindr2);
      loc_save_lut(loc_lut_logsindr3);
      loc_save_lut(loc_lut_logsindr4);
      loc_save_lut(loc_lut_logsindr5);
      loc_save_lut(loc_lut_logsindr6);
      loc_save_lut(loc_lut_logsindr7);
      loc_save_lut(loc_lut_logsindr8);
      loc_save_lut(loc_lut_logsindr9);
      loc_save_lut(loc_lut_logsindr10);
      loc_save_lut(loc_lut_logsindr11);
      loc_save_lut(loc_lut_logsindr12);
      loc_save_lut(loc_lut_logsindr13);
      loc_save_lut(loc_lut_logsindr14);
      loc_save_lut(loc_lut_logsindr15);
      loc_save_lut(loc_lut_logsindr16);
      loc_save_lut(loc_lut_logsindr17);
      loc_save_lut(loc_lut_logsindr18);
      loc_save_lut(loc_lut_logsindr19);
      loc_save_lut(loc_lut_logsindr20);
      // sine-saw up:
      loc_save_lut(loc_lut_logsawup1);
      loc_save_lut(loc_lut_logsawup2);
      loc_save_lut(loc_lut_logsawup3);
      loc_save_lut(loc_lut_logsawup4);
      loc_save_lut(loc_lut_logsawup5);
      loc_save_lut(loc_lut_logsawup6);
      loc_save_lut(loc_lut_logsawup7);
      loc_save_lut(loc_lut_logsawup8);
      loc_save_lut(loc_lut_logsawup9);
      loc_save_lut(loc_lut_logsawup10);
      // sine-saw down:
      loc_save_lut(loc_lut_logsawdown1);
      loc_save_lut(loc_lut_logsawdown2);
      loc_save_lut(loc_lut_logsawdown3);
      loc_save_lut(loc_lut_logsawdown4);
      loc_save_lut(loc_lut_logsawdown5);
      loc_save_lut(loc_lut_logsawdown6);
      loc_save_lut(loc_lut_logsawdown7);
      loc_save_lut(loc_lut_logsawdown8);
      loc_save_lut(loc_lut_logsawdown9);
      loc_save_lut(loc_lut_logsawdown10);
      // sine-bitreduction:
      loc_save_lut(loc_lut_logsinbr1);
      loc_save_lut(loc_lut_logsinbr2);
      loc_save_lut(loc_lut_logsinbr3);
      loc_save_lut(loc_lut_logsinbr4);
      loc_save_lut(loc_lut_logsinbr5);
      loc_save_lut(loc_lut_logsinbr6);
      loc_save_lut(loc_lut_logsinbr7);
      loc_save_lut(loc_lut_logsinbr8);
      loc_save_lut(loc_lut_logsinbr9);
      loc_save_lut(loc_lut_logsinbr10);
      loc_save_lut(loc_lut_logsinbr11);
      loc_save_lut(loc_lut_logsinbr12);
      loc_save_lut(loc_lut_logsinbr13);
      loc_save_lut(loc_lut_logsinbr14);
      loc_save_lut(loc_lut_logsinbr15);
      loc_save_lut(loc_lut_logsinbr16);
      loc_save_lut(loc_lut_logsinbr17);
      loc_save_lut(loc_lut_logsinbr18);
      loc_save_lut(loc_lut_logsinbr19);
      loc_save_lut(loc_lut_logsinbr20);
      // sine-sampleratereduction:
      loc_save_lut(loc_lut_logsinsr1);
      loc_save_lut(loc_lut_logsinsr2);
      loc_save_lut(loc_lut_logsinsr3);
      loc_save_lut(loc_lut_logsinsr4);
      loc_save_lut(loc_lut_logsinsr5);
      loc_save_lut(loc_lut_logsinsr6);
      loc_save_lut(loc_lut_logsinsr7);
      loc_save_lut(loc_lut_logsinsr8);
      loc_save_lut(loc_lut_logsinsr9);
      loc_save_lut(loc_lut_logsinsr10);
      loc_save_lut(loc_lut_logsinsr11);
      loc_save_lut(loc_lut_logsinsr12);
      loc_save_lut(loc_lut_logsinsr13);
      loc_save_lut(loc_lut_logsinsr14);
      loc_save_lut(loc_lut_logsinsr15);
      loc_save_lut(loc_lut_logsinsr16);
      loc_save_lut(loc_lut_logsinsr17);
      loc_save_lut(loc_lut_logsinsr18);
      loc_save_lut(loc_lut_logsinsr19);
      loc_save_lut(loc_lut_logsinsr20);
      // sine-saw-square:
      loc_save_lut(loc_lut_logsinsawsq1);
      loc_save_lut(loc_lut_logsinsawsq2);
      loc_save_lut(loc_lut_logsinsawsq3);
      loc_save_lut(loc_lut_logsinsawsq4);
      loc_save_lut(loc_lut_logsinsawsq5);
      loc_save_lut(loc_lut_logsinsawsq6);
      loc_save_lut(loc_lut_logsinsawsq7);
      loc_save_lut(loc_lut_logsinsawsq8);
      loc_save_lut(loc_lut_logsinsawsq9);
      loc_save_lut(loc_lut_logsinsawsq10);
      loc_save_lut(loc_lut_logsinsawsq11);
      loc_save_lut(loc_lut_logsinsawsq12);
      loc_save_lut(loc_lut_logsinsawsq13);
      loc_save_lut(loc_lut_logsinsawsq14);
      loc_save_lut(loc_lut_logsinsawsq15);
      loc_save_lut(loc_lut_logsinsawsq16);
      loc_save_lut(loc_lut_logsinsawsq17);
      loc_save_lut(loc_lut_logsinsawsq18);
      loc_save_lut(loc_lut_logsinsawsq19);
      loc_save_lut(loc_lut_logsinsawsq20);
      // sine-bend:
      loc_save_lut(loc_lut_logsinbd1);
      loc_save_lut(loc_lut_logsinbd2);
      loc_save_lut(loc_lut_logsinbd3);
      loc_save_lut(loc_lut_logsinbd4);
      loc_save_lut(loc_lut_logsinbd5);
      loc_save_lut(loc_lut_logsinbd6);
      loc_save_lut(loc_lut_logsinbd7);
      loc_save_lut(loc_lut_logsinbd8);
      loc_save_lut(loc_lut_logsinbd9);
      loc_save_lut(loc_lut_logsinbd10);
      loc_save_lut(loc_lut_logsinbd11);
      loc_save_lut(loc_lut_logsinbd12);
      loc_save_lut(loc_lut_logsinbd13);
      loc_save_lut(loc_lut_logsinbd14);
      loc_save_lut(loc_lut_logsinbd15);
      loc_save_lut(loc_lut_logsinbd16);
      loc_save_lut(loc_lut_logsinbd17);
      loc_save_lut(loc_lut_logsinbd18);
      loc_save_lut(loc_lut_logsinbd19);
      loc_save_lut(loc_lut_logsinbd20);
      // sine-warp:
      loc_save_lut(loc_lut_logsinwarp1);
      loc_save_lut(loc_lut_logsinwarp2);
      loc_save_lut(loc_lut_logsinwarp3);
      loc_save_lut(loc_lut_logsinwarp4);
      loc_save_lut(loc_lut_logsinwarp5);
      loc_save_lut(loc_lut_logsinwarp6);
      loc_save_lut(loc_lut_logsinwarp7);
      loc_save_lut(loc_lut_logsinwarp8);
      loc_save_lut(loc_lut_logsinwarp9);
      loc_save_lut(loc_lut_logsinwarp10);
      loc_save_lut(loc_lut_logsinwarp11);
      loc_save_lut(loc_lut_logsinwarp12);
      loc_save_lut(loc_lut_logsinwarp13);
      loc_save_lut(loc_lut_logsinwarp14);
      loc_save_lut(loc_lut_logsinwarp15);
      loc_save_lut(loc_lut_logsinwarp16);
      loc_save_lut(loc_lut_logsinwarp17);
      loc_save_lut(loc_lut_logsinwarp18);
      loc_save_lut(loc_lut_logsinwarp19);
      loc_save_lut(loc_lut_logsinwarp20);
      // sine-phase:
      loc_save_lut(loc_lut_logsinph1);
      loc_save_lut(loc_lut_logsinph2);
      loc_save_lut(loc_lut_logsinph3);
      loc_save_lut(loc_lut_logsinph4);
      loc_save_lut(loc_lut_logsinph5);
      loc_save_lut(loc_lut_logsinph6);
      loc_save_lut(loc_lut_logsinph7);
      loc_save_lut(loc_lut_logsinph8);
      loc_save_lut(loc_lut_logsinph9);
      loc_save_lut(loc_lut_logsinph10);
      loc_save_lut(loc_lut_logsinph11);
      loc_save_lut(loc_lut_logsinph12);
      loc_save_lut(loc_lut_logsinph13);
      loc_save_lut(loc_lut_logsinph14);
      loc_save_lut(loc_lut_logsinph15);
      loc_save_lut(loc_lut_logsinph16);
      loc_save_lut(loc_lut_logsinph17);
      loc_save_lut(loc_lut_logsinph18);
      loc_save_lut(loc_lut_logsinph19);
      loc_save_lut(loc_lut_logsinph20);

      loc_save_file_close();

      save_env_shapes();
#endif // SAVE

#ifdef LOGMUL
      loc_calc_lut_exp(loc_lut_exp);
#endif // LOGMUL

      loc_calc_lut_modfm_exp();

      // Replicate variation param names and resets
      {
         const char**sNames = &loc_param_names[PARAM_VAR_BASE];
         const char**dNames = sNames + NUM_PARAMS_PER_VAR;

         float *sResets = &loc_param_resets[PARAM_VAR_BASE];
         float *dResets = sResets + NUM_PARAMS_PER_VAR;

         for(sUI i = 1u; i < 8u; i++)
         {
            for(sUI varParamIdx = 0u; varParamIdx < NUM_PARAMS_PER_VAR; varParamIdx++)
            {
               dNames [varParamIdx] = sNames [varParamIdx];
               dResets[varParamIdx] = sResets[varParamIdx];
            }

            dNames  += NUM_PARAMS_PER_VAR;
            dResets += NUM_PARAMS_PER_VAR;
         }
      }
   }

   return &ret->base;
}
