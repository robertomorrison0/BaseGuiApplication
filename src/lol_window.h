#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define LOL_TYPE_WINDOW (lol_window_get_type())

G_DECLARE_FINAL_TYPE (LolWindow, lol_window, LOL, WINDOW, AdwApplicationWindow)

G_END_DECLS
