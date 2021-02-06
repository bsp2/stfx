#include "../../../plugin.h"

extern st_plugin_info_t *lrt_alma_init    (void);
extern st_plugin_info_t *lrt_laika_init   (void);
extern st_plugin_info_t *lrt_valerie_init (void);
extern st_plugin_info_t *lrt_vampyr_init  (void);

ST_PLUGIN_APICALL st_plugin_info_t *ST_PLUGIN_API st_plugin_init(unsigned int _pluginIdx) {
   switch(_pluginIdx)
   {
      case 0u:
         return lrt_alma_init();    // Ladder

      case 1u:
         return lrt_laika_init();   // Diode Ladder

      case 2u:
         return lrt_valerie_init(); // Korg-like MS20 ZDF

      case 3u:
         return lrt_vampyr_init();  // Korg-like Type 35 (MS20) dual lp/hp filter
   }
   return NULL;
}
