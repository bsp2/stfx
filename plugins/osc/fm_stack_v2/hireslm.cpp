// ----
// ---- file   : hireslm.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2024 by Bastian Spiegel.
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : 4op FM stack (HIRES LMUL variant)
// ----
// ---- created: 21Aug2023
// ---- changed: 22Aug2023, 23Aug2023, 24Aug2023, 25Aug2023, 26Aug2023, 01Sep2023, 03Sep2023
// ----          06Sep2023, 16Sep2023, 19Sep2023, 28Apr2024, 14Oct2024
// ----
// ----
// ----

#include "../../../plugin.h"

// #define FMSTACK_SAVE               defined
#define FMSTACK_HIRES              defined
#define FMSTACK_HIRES_AENV         defined
#define FMSTACK_HIRES_PENV         defined
#define FMSTACK_HIRES_LMUL         defined
#define FMSTACK_PLUGIN_ID          "fm_stack_v2_hires_lm"
#define FMSTACK_PLUGIN_NAME        "osc fm stack v2 hires lm"
#define FMSTACK_PLUGIN_SHORT_NAME  "osc fm stack v2 hires lm"
#define FMSTACK_INFO_S             fm_stack_info_s_hires_lm
#define FMSTACK_INFO_T             fm_stack_info_t_hires_lm
#define FMSTACK_SHARED_S           fm_stack_shared_s_hires_lm
#define FMSTACK_SHARED_T           fm_stack_shared_t_hires_lm
#define FMSTACK_VOICE_OP_S         voice_op_s_hires_lm
#define FMSTACK_VOICE_OP_T         voice_op_t_hires_lm
#define FMSTACK_VOICE_S            fm_stack_voice_s_hires_lm
#define FMSTACK_VOICE_T            fm_stack_voice_t_hires_lm
#define FMSTACK_INIT               fm_stack_init_hires_lm
#include "fm_stack.cpp"
