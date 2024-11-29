// ----
// ---- file   : medres.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2023-2024 by Bastian Spiegel.
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : 4op FM stack (MEDRES variant)
// ----
// ---- created: 21Aug2023
// ---- changed: 22Aug2023, 23Aug2023, 24Aug2023, 25Aug2023, 26Aug2023, 01Sep2023, 03Sep2023
// ----          06Sep2023, 07Sep2023, 19Sep2023, 14Oct2024
// ----
// ----
// ----

#include "../../../plugin.h"

#define FMSTACK_MEDRES             defined
#define FMSTACK_PLUGIN_ID          "fm_stack_v2_medres"
#define FMSTACK_PLUGIN_NAME        "osc fm stack v2 medres"
#define FMSTACK_PLUGIN_SHORT_NAME  "osc fm stack v2 medres"
#define FMSTACK_INFO_S             fm_stack_info_medres_s
#define FMSTACK_INFO_T             fm_stack_info_medres_t
#define FMSTACK_SHARED_S           fm_stack_shared_medres_s
#define FMSTACK_SHARED_T           fm_stack_shared_medres_t
#define FMSTACK_VOICE_OP_S         fm_stack_voice_op_medres_s
#define FMSTACK_VOICE_S            fm_stack_voice_medres_s
#define FMSTACK_VOICE_T            fm_stack_voice_medres_t
#define FMSTACK_INIT               fm_stack_init_medres
#include "fm_stack.cpp"