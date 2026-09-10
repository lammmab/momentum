#ifndef CONFIG_C_API_H
#define CONFIG_C_API_H

#include <stdbool.h>
#include "config_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FIELD_STRING(name, path) \
    const char* mega_config_get_##name(void); \
    void        mega_config_set_##name(const char* value);
#define FIELD_BOOL(name, path) \
    bool mega_config_get_##name(void); \
    void mega_config_set_##name(bool value);
#include "config_fields.def"
#undef FIELD_STRING
#undef FIELD_BOOL

bool config_exists(void);
void config_parse(void);
void config_write(void);

#ifdef __cplusplus
}
#endif
#endif
