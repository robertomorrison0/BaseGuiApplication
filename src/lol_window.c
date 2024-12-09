#include "gio/gio.h"
#include <config.h>
#include <gtksourceview/gtksource.h>

#include <lol_window.h>

struct _LolWindow {
  AdwApplicationWindow parent_instance;

  /* Template widgets */
  AdwWindowTitle *title_widget;
  GSettings *settings;
  GtkSourceView *source_view;
  GtkSourceBuffer *source_buffer;

  GtkSourceStyleScheme *dark_scheme;
  GtkSourceStyleScheme *light_scheme;
  GtkSourceFile *source_file;
};

G_DEFINE_FINAL_TYPE(LolWindow, lol_window, ADW_TYPE_APPLICATION_WINDOW)

void lol_window_open_file(LolWindow *self, GFile*file);
void lol_window_set_property(GObject *object, guint prop_id,
                             const GValue *value, GParamSpec *pspec);
void lol_window_get_property(GObject *object, guint prop_id, GValue *value,
                             GParamSpec *pspec);

enum {
  PROP_0,
  PROP_FILE,
  N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL};

static void lol_window_init(LolWindow *self) {
  // g_type_ensure (PTO_TYPE_TEXT_EDITOR);
  gtk_widget_init_template(GTK_WIDGET(self));

  self->settings = g_settings_new(APP_ID);

  gint width = g_settings_get_int(self->settings, "window-width");
  gint height = g_settings_get_int(self->settings, "window-height");
  gboolean maximized =
      g_settings_get_boolean(self->settings, "window-is-maximized");
  gboolean fullscreened =
      g_settings_get_boolean(self->settings, "window-is-fullscreen");

  gtk_window_set_default_size(GTK_WINDOW(self), width, height);
  g_object_set(G_OBJECT(self), "maximized", maximized, NULL);
  g_object_set(G_OBJECT(self), "fullscreened", fullscreened, NULL);

  GtkSourceStyleSchemeManager *manager =
      gtk_source_style_scheme_manager_get_default();
  self->dark_scheme =
      gtk_source_style_scheme_manager_get_scheme(manager, "Adwaita-dark");
  self->light_scheme =
      gtk_source_style_scheme_manager_get_scheme(manager, "Adwaita");

  gtk_source_buffer_set_style_scheme(self->source_buffer, self->dark_scheme);

  adw_window_title_set_title(self->title_widget, APP_DISPLAY_NAME);
}

static void lol_window_finalize(LolWindow *self) {
  int width, height;
  gtk_window_get_default_size(GTK_WINDOW(self), &width, &height);
  g_settings_set_int(self->settings, "window-width", width);
  g_settings_set_int(self->settings, "window-height", height);
  g_settings_set_boolean(self->settings, "window-is-maximized",
                         gtk_window_is_maximized(GTK_WINDOW(self)));
  g_settings_set_boolean(self->settings, "window-is-fullscreen",
                         gtk_window_is_fullscreen(GTK_WINDOW(self)));

  g_settings_sync();

  g_object_unref(self->settings);

  G_OBJECT_CLASS(lol_window_parent_class)->finalize(G_OBJECT(self));
}

static void lol_window_class_init(LolWindowClass *klass) {
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->finalize = (GObjectFinalizeFunc)lol_window_finalize;
  object_class->set_property = lol_window_set_property;
  object_class->get_property = lol_window_get_property;

  gtk_widget_class_set_template_from_resource(
      widget_class, DEFAULT_RESOURCE_PATH "/lol_window.ui");
  gtk_widget_class_bind_template_child(widget_class, LolWindow, title_widget);
  gtk_widget_class_bind_template_child(widget_class, LolWindow, source_view);
  gtk_widget_class_bind_template_child(widget_class, LolWindow, source_buffer);

  properties[PROP_FILE] =
      g_param_spec_object("file", "File", "The file to open", G_TYPE_FILE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

void lol_window_set_property(GObject *object, guint prop_id,
                             const GValue *value, GParamSpec *pspec) {
  LolWindow *self = LOL_WINDOW(object);
  switch (prop_id) {
  case PROP_FILE: {
    lol_window_open_file(self, g_value_get_object(value));
    break;
  }
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(self, prop_id, pspec);
    break;
  }
}

void lol_window_get_property(GObject *object, guint prop_id, GValue *value,
                             GParamSpec *pspec) {

  LolWindow *self = LOL_WINDOW(object);
  switch (prop_id) {
  case PROP_FILE:
    g_value_set_object(value, NULL);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(self, prop_id, pspec);
    break;
  }
}

static void lol_window_load_file_finished(GtkSourceFileLoader *loader,
                                          GAsyncResult *res, LolWindow *self) {
  gboolean success = FALSE;
  success = gtk_source_file_loader_load_finish(loader, res, NULL);

  if (!success) {
    g_warning("Could not load file");
    g_object_unref(self->source_file);
    return;
  }

  gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(self->source_buffer), TRUE);
  GFile *file = gtk_source_file_loader_get_location(loader);
  g_object_set_data(G_OBJECT(self->source_buffer), "uri",
                    g_file_get_path(file));
  g_object_unref(file);

  g_object_unref(loader);

  g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_FILE]);
}

void lol_window_open_file(LolWindow *self, GFile* file) {
  g_return_if_fail(G_IS_FILE(file));

  self->source_file = gtk_source_file_new();

  gtk_source_file_set_location(self->source_file, file);
  g_object_unref(file);

  GtkSourceFileLoader *loader = gtk_source_file_loader_new(
      GTK_SOURCE_BUFFER(self->source_buffer), self->source_file);

  gtk_source_file_loader_load_async(
      loader, G_PRIORITY_DEFAULT, NULL, NULL, NULL, NULL,
      (GAsyncReadyCallback)lol_window_load_file_finished, self);
}
