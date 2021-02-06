
#include "../../../plugin.h"

extern st_plugin_info_t *je_ringmod_init (unsigned int  _outputMode,
                                          const char   *_id,
                                          const char   *_name
                                          );

extern st_plugin_info_t *je_x_ringmod_init (unsigned int  _outputMode,
                                            const char   *_id,
                                            const char   *_name
                                            );

ST_PLUGIN_APICALL st_plugin_info_t *ST_PLUGIN_API st_plugin_init(unsigned int _pluginIdx) {
   switch(_pluginIdx)
   {
      case 0u:  // ring
         return je_ringmod_init(0u, "je ringmod 1 ring", "je ring modulator - 1 ring");

      case 1u:  // sum
         return je_ringmod_init(1u, "je ringmod 2 sum", "je ring modulator - 2 sum");

      case 2u:  // diff
         return je_ringmod_init(2u, "je ringmod 3 diff", "je ring modulator - 3 diff");

      case 3u:  // min
         return je_ringmod_init(3u, "je ringmod 4 min", "je ring modulator - 4 min");

      case 4u:  // max
         return je_ringmod_init(4u, "je ringmod 5 max", "je ring modulator - 5 max");

      case 5u:  // x ring
         return je_x_ringmod_init(0u, "x je ringmod 1 ring", "x je ring modulator - 1 ring");

      case 6u:  // x sum
         return je_x_ringmod_init(1u, "x je ringmod 2 sum", "x je ring modulator - 2 sum");

      case 7u:  // x diff
         return je_x_ringmod_init(2u, "x je ringmod 3 diff", "x je ring modulator - 3 diff");

      case 8u:  // x min
         return je_x_ringmod_init(3u, "x je ringmod 4 min", "x je ring modulator - 4 min");

      case 9u:  // x max
         return je_x_ringmod_init(4u, "x je ringmod 5 max", "x je ring modulator - 5 max");
   }
   return NULL;
}
