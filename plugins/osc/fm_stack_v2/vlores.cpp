// ----
// ---- file   : vlores.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2023-2024 by Bastian Spiegel.
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : 4op FM stack (very LORES variant)
// ----
// ---- created: 21Aug2023
// ---- changed: 22Aug2023, 23Aug2023, 24Aug2023, 25Aug2023, 26Aug2023, 01Sep2023, 03Sep2023
// ----          06Sep2023, 07Sep2023, 14Oct2024
// ----
// ----
// ----

#include "../../../plugin.h"

#define FMSTACK_VLORES             defined
#define FMSTACK_PLUGIN_ID          "fm_stack_v2_vlores"
#define FMSTACK_PLUGIN_NAME        "osc fm stack v2 vlores"
#define FMSTACK_PLUGIN_SHORT_NAME  "osc fm stack v2 vlores"
#define FMSTACK_INFO_S             fm_stack_info_s_vlores
#define FMSTACK_INFO_T             fm_stack_info_t_vlores
#define FMSTACK_SHARED_S           fm_stack_shared_s_vlores
#define FMSTACK_SHARED_T           fm_stack_shared_t_vlores
#define FMSTACK_VOICE_OP_S         voice_op_s_vlores
#define FMSTACK_VOICE_OP_T         voice_op_t_vlores
#define FMSTACK_VOICE_S            fm_stack_voice_s_vlores
#define FMSTACK_VOICE_T            fm_stack_voice_t_vlores
#define FMSTACK_INIT               fm_stack_init_vlores
#include "fm_stack.cpp"
