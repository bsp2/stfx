#include "../../../plugin.h"

extern st_plugin_info_t *sql_bootyshifter_init     (void);
extern st_plugin_info_t *sql_bootyshifter_pan_init (void);

ST_PLUGIN_APICALL st_plugin_info_t *ST_PLUGIN_API st_plugin_init(unsigned int _pluginIdx) {
   switch(_pluginIdx)
   {
      case 0u:
         return sql_bootyshifter_init();

      case 1u:
         return sql_bootyshifter_pan_init();
   }
   return NULL;
}
