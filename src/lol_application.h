#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define LOL_TYPE_APPLICATION (lol_application_get_type())

G_DECLARE_FINAL_TYPE (LolApplication, lol_application, LOL, APPLICATION, AdwApplication)

LolApplication *lol_application_new (const char        *application_id,
                                         GApplicationFlags  flags);

G_END_DECLS
