#include "gmodule.h"
#include <config.h>
#include <gtksourceview/gtksource.h>

#include <lol_window.h>

typedef enum
{
  ACTION_NONE,
  ACTION_WINDOW_CLOSE,
  ACTION_WINDOW_OPEN,
} UnsavedChangesAction;

struct _LolWindow
{
  AdwApplicationWindow parent_instance;

  /* Template widgets */
  AdwWindowTitle *title_widget;
  GSettings *settings;
  GtkSourceView *source_view;
  GtkSourceBuffer *source_buffer;
  AdwDialog *changes_dialog;

  GtkSourceStyleScheme *dark_scheme;
  GtkSourceStyleScheme *light_scheme;
  GtkSourceFile *source_file;

  gulong buffer_changed_handler_id;

  const char *file_hash;
  gboolean is_file_saved;

  UnsavedChangesAction unsaved_changes_action;

  // GFile* target_file;
};

G_DEFINE_FINAL_TYPE (LolWindow, lol_window, ADW_TYPE_APPLICATION_WINDOW)

void lol_window_open_file (LolWindow *self, GFile *file);
void lol_window_set_property (GObject *object, guint prop_id,
                              const GValue *value, GParamSpec *pspec);
void lol_window_get_property (GObject *object, guint prop_id, GValue *value,
                              GParamSpec *pspec);
void lol_window_save_source_file (LolWindow *self, GFile *target);
void lol_window_buffer_changed_cb (GtkTextBuffer *self, gpointer user_data);
const char *generate_hash (const char *string);
const char *generate_hash_from_buffer (GtkTextBuffer *buffer);
void lol_window_open_file_cb (GtkWidget *widget, const char *action_name,
                              GVariant *parameter);
void resume_unsaved_changes_action (LolWindow *self);
gboolean lol_window_close_requested (GtkWindow *self, gpointer user_data);

enum
{
  PROP_0,
  PROP_FILE,
  N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = { NULL };

static void
lol_window_init (LolWindow *self)
{
  // g_type_ensure (PTO_TYPE_TEXT_EDITOR);
  gtk_widget_init_template (GTK_WIDGET (self));

  self->settings = g_settings_new (APP_ID);

  self->is_file_saved = TRUE;
  self->unsaved_changes_action = ACTION_NONE;

  gint width = g_settings_get_int (self->settings, "window-width");
  gint height = g_settings_get_int (self->settings, "window-height");
  gboolean maximized
      = g_settings_get_boolean (self->settings, "window-is-maximized");
  gboolean fullscreened
      = g_settings_get_boolean (self->settings, "window-is-fullscreen");

  gtk_window_set_default_size (GTK_WINDOW (self), width, height);
  g_object_set (G_OBJECT (self), "maximized", maximized, NULL);
  g_object_set (G_OBJECT (self), "fullscreened", fullscreened, NULL);

  GtkSourceStyleSchemeManager *manager
      = gtk_source_style_scheme_manager_get_default ();
  self->dark_scheme
      = gtk_source_style_scheme_manager_get_scheme (manager, "Adwaita-dark");
  self->light_scheme
      = gtk_source_style_scheme_manager_get_scheme (manager, "Adwaita");

  gtk_source_buffer_set_style_scheme (self->source_buffer, self->dark_scheme);

  adw_window_title_set_title (self->title_widget, APP_DISPLAY_NAME);

  self->buffer_changed_handler_id
      = g_signal_connect (self->source_buffer, "changed",
                          G_CALLBACK (lol_window_buffer_changed_cb), self);

  g_signal_connect (self, "close-request",
                    G_CALLBACK (lol_window_close_requested), NULL);
}

gboolean
lol_window_close_requested (GtkWindow *self, gpointer user_data)
{
  LolWindow *window = LOL_WINDOW (self);
  if (!window->is_file_saved)
    {
      window->unsaved_changes_action = ACTION_WINDOW_CLOSE;
      adw_dialog_present (window->changes_dialog, GTK_WIDGET (window));
      return TRUE;
    }
  return FALSE;
}

static void
lol_window_finalize (LolWindow *self)
{
  int width, height;
  gtk_window_get_default_size (GTK_WINDOW (self), &width, &height);
  g_settings_set_int (self->settings, "window-width", width);
  g_settings_set_int (self->settings, "window-height", height);
  g_settings_set_boolean (self->settings, "window-is-maximized",
                          gtk_window_is_maximized (GTK_WINDOW (self)));
  g_settings_set_boolean (self->settings, "window-is-fullscreen",
                          gtk_window_is_fullscreen (GTK_WINDOW (self)));

  g_settings_sync ();

  g_object_unref (self->settings);

  G_OBJECT_CLASS (lol_window_parent_class)->finalize (G_OBJECT (self));
}

void
lol_window_save_as_finish (GObject *source_object, GAsyncResult *res,
                           gpointer data)
{

  GError *error = NULL;
  GtkFileDialog *dialog = NULL;

  g_return_if_fail (GTK_IS_FILE_DIALOG (source_object));
  g_return_if_fail (LOL_IS_WINDOW (data));

  LolWindow *self = LOL_WINDOW (data);

  dialog = GTK_FILE_DIALOG (source_object);

  GFile *file = gtk_file_dialog_save_finish (dialog, res, &error);

  if (error != NULL)
    {
      g_warning ("Could not save file: %s", error->message);
      g_error_free (error);
      if (file != NULL)
        g_object_unref (file);
      return;
    }

  lol_window_save_source_file (self, file);

  g_object_unref (dialog);

  resume_unsaved_changes_action (self);
}

void
lol_window_save_as_file_cb (GtkWidget *widget, const char *action_name,
                            GVariant *parameter)
{
  g_return_if_fail (LOL_IS_WINDOW (widget));
  LolWindow *self = LOL_WINDOW (widget);
  GFile *location = (GTK_SOURCE_IS_FILE (self->source_file))
                        ? gtk_source_file_get_location (self->source_file)
                        : NULL;
  GFile *folder = NULL;

  GtkFileDialog *dialog = gtk_file_dialog_new ();

  if (!G_IS_FILE (location))
    {
      folder = g_file_new_for_path (g_get_home_dir ());
    }
  else
    {
      folder = g_file_get_parent (location);
      g_object_unref (location);
    }

  gtk_file_dialog_set_initial_folder (dialog, folder);
  gtk_file_dialog_save (dialog, GTK_WINDOW (self), NULL,
                        lol_window_save_as_finish, self);
  // g_object_unref(folder);
}

void
lol_window_save_file_cb (GtkWidget *widget, const char *action_name,
                         GVariant *parameter)
{
  g_return_if_fail (LOL_IS_WINDOW (widget));
  LolWindow *self = LOL_WINDOW (widget);

  lol_window_save_source_file (self, NULL);
}

void
lol_window_save_file_finish (GObject *source_object, GAsyncResult *res,
                             gpointer data)
{

  GError *error = NULL;
  GtkSourceFileSaver *saver = NULL;
  gboolean success = FALSE;
  LolWindow *self = NULL;

  g_return_if_fail (GTK_SOURCE_IS_FILE_SAVER (source_object));
  g_return_if_fail (LOL_IS_WINDOW (data));
  self = LOL_WINDOW (data);

  saver = GTK_SOURCE_FILE_SAVER (source_object);
  success = gtk_source_file_saver_save_finish (saver, res, &error);

  if (error != NULL || !success)
    {
      g_warning ("Could not save file: %s", error->message);
      g_error_free (error);

      return;
    }

  adw_window_title_set_title (
      self->title_widget,
      g_strdup_printf ("%s - %s", APP_DISPLAY_NAME,
                       g_file_get_basename (
                           gtk_source_file_get_location (self->source_file))));

  resume_unsaved_changes_action (self);
  // g_object_unref(self->source_file);
  // self->source_file = gtk_source_file_saver_get_file(saver);

  // g_object_unref(saver);
}

void
resume_unsaved_changes_action (LolWindow *self)
{
  self->is_file_saved = TRUE;
  switch (self->unsaved_changes_action)
    {
    case ACTION_WINDOW_CLOSE:
      gtk_window_close (GTK_WINDOW (self));
      break;
    case ACTION_WINDOW_OPEN:
      lol_window_open_file_cb (GTK_WIDGET (self), "doc.open", NULL);
      break;
    default:
      break;
    }
  self->unsaved_changes_action = ACTION_NONE;
}

void
lol_window_save_source_file (LolWindow *self, GFile *target)
{

  GtkSourceFileSaver *saver = NULL;
  g_return_if_fail (GTK_SOURCE_IS_FILE (self->source_file));

  if (!G_IS_FILE (target))
    {
      target = gtk_source_file_get_location (self->source_file);
    }

  if (!G_IS_FILE (target))
    {
      lol_window_save_as_file_cb (GTK_WIDGET (self), "saveas", NULL);
      return;
    }
  saver = gtk_source_file_saver_new_with_target (self->source_buffer,
                                                 self->source_file, target);

  printf ("Saving file %s\n", g_file_get_path (target));

  gtk_source_file_saver_set_flags (saver, GTK_SOURCE_FILE_SAVER_FLAGS_NONE);
  gtk_source_file_saver_save_async (saver, G_PRIORITY_DEFAULT, NULL, NULL,
                                    NULL, NULL, lol_window_save_file_finish,
                                    self);
}

void
lol_window_open_finish (GObject *source_object, GAsyncResult *res,
                        gpointer data)
{
  GError *error = NULL;
  GtkFileDialog *dialog = NULL;

  g_return_if_fail (GTK_IS_FILE_DIALOG (source_object));
  g_return_if_fail (LOL_IS_WINDOW (data));

  dialog = GTK_FILE_DIALOG (source_object);

  GFile *file = gtk_file_dialog_open_finish (dialog, res, &error);

  if (error != NULL)
    {
      g_warning ("Could not open file: %s", error->message);
      g_error_free (error);
      if (file != NULL)
        g_object_unref (file);
      return;
    }

  g_object_set (G_OBJECT (data), "file", file, NULL);

  g_object_unref (dialog);
}

void
lol_window_open_file_cb (GtkWidget *widget, const char *action_name,
                         GVariant *parameter)
{
  g_return_if_fail (LOL_IS_WINDOW (widget));
  LolWindow *self = LOL_WINDOW (widget);

  if (!self->is_file_saved)
    {
      self->unsaved_changes_action = ACTION_WINDOW_OPEN;
      adw_dialog_present (self->changes_dialog, GTK_WIDGET (self));
      return;
    }

  GtkFileDialog *dialog = gtk_file_dialog_new ();
  gtk_file_dialog_open (dialog, GTK_WINDOW (self), NULL,
                        lol_window_open_finish, self);
}

static void
lol_window_unsaved_response_cb (AdwAlertDialog *dialog, gchar *response,
                                gpointer user_data)
{

  g_return_if_fail (ADW_IS_DIALOG (dialog));
  g_return_if_fail (LOL_IS_WINDOW (user_data));

  LolWindow *self = LOL_WINDOW (user_data);

  printf ("Response: '%s'\n", response);
  if (g_strcmp0 (response, "save") == 0)
    {
      lol_window_save_file_cb (GTK_WIDGET (user_data), "doc.save", NULL);
      // resume_unsaved_changes_action (self);
    }
  else if (g_strcmp0 (response, "discard") == 0)
    {
      resume_unsaved_changes_action (self);
    }
}

static void
lol_window_class_init (LolWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = (GObjectFinalizeFunc)lol_window_finalize;
  object_class->set_property = lol_window_set_property;
  object_class->get_property = lol_window_get_property;

  gtk_widget_class_set_template_from_resource (
      widget_class, DEFAULT_RESOURCE_PATH "/lol_window.ui");
  gtk_widget_class_bind_template_child (widget_class, LolWindow, title_widget);
  gtk_widget_class_bind_template_child (widget_class, LolWindow, source_view);
  gtk_widget_class_bind_template_child (widget_class, LolWindow,
                                        source_buffer);
  gtk_widget_class_bind_template_child (widget_class, LolWindow,
                                        changes_dialog);

  gtk_widget_class_bind_template_callback (widget_class,
                                           lol_window_unsaved_response_cb);

  properties[PROP_FILE]
      = g_param_spec_object ("file", "File", "The file to open", G_TYPE_FILE,
                             G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_properties (object_class, N_PROPERTIES, properties);

  gtk_widget_class_install_action (widget_class, "doc.save", NULL,
                                   lol_window_save_file_cb);
  gtk_widget_class_install_action (widget_class, "doc.saveas", NULL,
                                   lol_window_save_as_file_cb);
  gtk_widget_class_install_action (widget_class, "doc.open", NULL,
                                   lol_window_open_file_cb);

  GtkShortcut *save_shortcut = gtk_shortcut_new (
      GTK_SHORTCUT_TRIGGER (
          gtk_keyval_trigger_new (GDK_KEY_S, GDK_CONTROL_MASK)),
      GTK_SHORTCUT_ACTION (gtk_named_action_new ("doc.save")));
  GtkShortcut *saveas_shortcut = gtk_shortcut_new (
      GTK_SHORTCUT_TRIGGER (gtk_keyval_trigger_new (
          GDK_KEY_S, GDK_CONTROL_MASK | GDK_SHIFT_MASK)),
      GTK_SHORTCUT_ACTION (gtk_named_action_new ("doc.saveas")));
  GtkShortcut *open_shortcut = gtk_shortcut_new (
      GTK_SHORTCUT_TRIGGER (
          gtk_keyval_trigger_new (GDK_KEY_O, GDK_CONTROL_MASK)),
      GTK_SHORTCUT_ACTION (gtk_named_action_new ("doc.open")));

  gtk_widget_class_add_shortcut (widget_class, saveas_shortcut);
  gtk_widget_class_add_shortcut (widget_class, save_shortcut);
  gtk_widget_class_add_shortcut (widget_class, open_shortcut);
}

void
lol_window_set_property (GObject *object, guint prop_id, const GValue *value,
                         GParamSpec *pspec)
{
  LolWindow *self = LOL_WINDOW (object);
  switch (prop_id)
    {
    case PROP_FILE:
      {
        lol_window_open_file (self, g_value_get_object (value));
        break;
      }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
      break;
    }
}

void
lol_window_get_property (GObject *object, guint prop_id, GValue *value,
                         GParamSpec *pspec)
{

  LolWindow *self = LOL_WINDOW (object);
  switch (prop_id)
    {
    case PROP_FILE:
      g_value_set_object (value, NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
      break;
    }
}

static void
lol_window_load_file_finished (GObject *source_object, GAsyncResult *res,
                               gpointer data)
{
  g_return_if_fail (LOL_IS_WINDOW (data));
  g_return_if_fail (GTK_SOURCE_IS_FILE_LOADER (source_object));
  LolWindow *self = LOL_WINDOW (data);
  GtkSourceFileLoader *loader = GTK_SOURCE_FILE_LOADER (source_object);

  gboolean success = FALSE;

  success = gtk_source_file_loader_load_finish (loader, res, NULL);

  if (!success)
    {
      g_warning ("Could not load file");
      g_object_unref (self->source_file);
      return;
    }

  gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (self->source_buffer), TRUE);
  GFile *file = gtk_source_file_loader_get_location (loader);
  gtk_source_file_set_location (self->source_file, file);
  // g_object_set_data(G_OBJECT(self->source_buffer), "uri",
  //                   g_file_get_path(file));
  printf ("Loaded file %s\n",
          g_file_get_path (gtk_source_file_get_location (self->source_file)));
  g_object_unref (file);

  // g_signal_handler_unblock(self->source_buffer,
  //                          self->buffer_changed_handler_id);

  adw_window_title_set_title (
      self->title_widget,
      g_strdup_printf ("%s - %s", APP_DISPLAY_NAME,
                       g_file_get_basename (
                           gtk_source_file_get_location (self->source_file))));

  self->file_hash
      = generate_hash_from_buffer (GTK_TEXT_BUFFER (self->source_buffer));
  self->is_file_saved = TRUE;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FILE]);
}

void
lol_window_open_file (LolWindow *self, GFile *file)
{

  // g_signal_handler_block(self->source_buffer,
  // self->buffer_changed_handler_id);

  if (!G_IS_FILE (file))
    {
      self->source_file = gtk_source_file_new ();
      gtk_source_file_set_location (self->source_file, NULL);

      self->file_hash = generate_hash ("");
      self->is_file_saved = TRUE;

      // g_signal_handler_unblock(self->source_buffer,
      //                          self->buffer_changed_handler_id);
      return;
    }

  if (self->source_file != NULL)
    g_object_unref (self->source_file);

  self->source_file = gtk_source_file_new ();

  gtk_source_file_set_location (self->source_file, file);
  g_object_unref (file);

  GtkSourceFileLoader *loader
      = gtk_source_file_loader_new (self->source_buffer, self->source_file);

  gtk_source_file_loader_load_async (loader, G_PRIORITY_DEFAULT, NULL, NULL,
                                     NULL, NULL, lol_window_load_file_finished,
                                     self);
}

const char *
generate_hash (const char *string)
{
  gchar *hash = NULL;
  GChecksum *checksum = g_checksum_new (G_CHECKSUM_SHA256);

  if (string)
    {
      g_checksum_update (checksum, (const guchar *)string, strlen (string));
      hash = g_strdup (g_checksum_get_string (checksum));
    }

  g_checksum_free (checksum);
  return hash;
}

const char *
generate_hash_from_buffer (GtkTextBuffer *buffer)
{
  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_get_end_iter (buffer, &end);
  const char *content = g_strdup (
      gtk_text_buffer_get_text (GTK_TEXT_BUFFER (buffer), &start, &end, TRUE));
  return generate_hash (content);
}

gboolean
lol_content_changed (LolWindow *self)
{
  const char *hash
      = generate_hash_from_buffer (GTK_TEXT_BUFFER (self->source_buffer));

  printf ("Hash: %s\n", hash);
  printf ("Old hash: %s\n\n", self->file_hash);

  return strcmp (hash, self->file_hash) != 0;
}

void
lol_window_buffer_changed_cb (GtkTextBuffer *buffer, gpointer user_data)
{
  g_return_if_fail (LOL_IS_WINDOW (user_data));
  LolWindow *self = LOL_WINDOW (user_data);

  if (lol_content_changed (self))
    {
      adw_window_title_set_title (
          self->title_widget,
          g_strdup_printf ("%s - %s*", APP_DISPLAY_NAME,
                           g_file_get_basename (gtk_source_file_get_location (
                               self->source_file))));
      self->is_file_saved = FALSE;
    }
  else
    {
      adw_window_title_set_title (
          self->title_widget,
          g_strdup_printf ("%s - %s", APP_DISPLAY_NAME,
                           g_file_get_basename (gtk_source_file_get_location (
                               self->source_file))));
      self->is_file_saved = TRUE;
    }
}
