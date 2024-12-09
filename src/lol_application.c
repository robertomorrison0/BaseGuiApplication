
/* TODO: test */
#include <config.h>

#include <gtksourceview/gtksource.h>
#include <lol_application.h>
#include <lol_window.h>

struct _LolApplication
{
  AdwApplication parent_instance;

  GFile *file;
};

G_DEFINE_TYPE (LolApplication, lol_application, ADW_TYPE_APPLICATION)

gint lol_application_handle_local_options (GApplication *app,
                                           GVariantDict *options);
gint lol_application_command_line (GApplication *application,
                                   GApplicationCommandLine *command_line);

LolApplication *
lol_application_new (const char *application_id, GApplicationFlags flags)
{
  g_return_val_if_fail (application_id, NULL);

  return g_object_new (
      LOL_TYPE_APPLICATION, "application-id", application_id, "flags",
      flags | G_APPLICATION_HANDLES_COMMAND_LINE | G_APPLICATION_HANDLES_OPEN,
      NULL);
}

static void
lol_application_activate (GApplication *app)
{
  GtkWindow *window;

  g_assert (LOL_IS_APPLICATION (app));
  LolApplication *self = LOL_APPLICATION (app);

  window = gtk_application_get_active_window (GTK_APPLICATION (app));

  if (window == NULL)
    window = g_object_new (LOL_TYPE_WINDOW, "application", app, "file",
                           self->file, NULL);

  gtk_window_present (window);
}

static void
lol_application_startup (GApplication *app)
{
  g_assert (LOL_IS_APPLICATION (app));

  G_APPLICATION_CLASS (lol_application_parent_class)->startup (app);
  gtk_window_set_default_icon_name (PACKAGE_ICON_NAME);
}

static void
lol_application_class_init (LolApplicationClass *klass)
{
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  app_class->activate = lol_application_activate;
  app_class->startup = lol_application_startup;
  app_class->command_line = lol_application_command_line;
  app_class->handle_local_options = lol_application_handle_local_options;
}

static void
lol_application_about_action (GSimpleAction *action, GVariant *parameter,
                              gpointer user_data)
{
  static const char *developers[] = { DEVELOPER, NULL };
  LolApplication *self = user_data;
  GtkWindow *window = NULL;

  g_assert (LOL_IS_APPLICATION (self));

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  adw_show_about_dialog (
      GTK_WIDGET (window), "application-name", APP_DISPLAY_NAME,
      "application-icon", APP_ID, "developer-name", DEVELOPER_NAME, "version",
      PACKAGE_VERSION, "developers", developers, "copyright",
      g_strdup_printf ("© 2023 %s", DEVELOPER_NAME), NULL);
}

static void
lol_application_quit_action (GSimpleAction *action, GVariant *parameter,
                             gpointer user_data)
{
  LolApplication *self = user_data;

  g_assert (LOL_IS_APPLICATION (self));

  g_application_quit (G_APPLICATION (self));
}

static const GActionEntry app_actions[] = {
  { "quit", lol_application_quit_action },
  { "about", lol_application_about_action },
};

static void
lol_application_init (LolApplication *self)
{
  g_action_map_add_action_entries (G_ACTION_MAP (self), app_actions,
                                   G_N_ELEMENTS (app_actions), self);
  gtk_application_set_accels_for_action (
      GTK_APPLICATION (self), "app.quit",
      (const char *[]){ "<primary>q", NULL });

  static const GOptionEntry options[]
      = { { "version", '\0', 0, G_OPTION_ARG_NONE, NULL,
            ("Show the version of the program."), NULL },
          { "new-window", 'w', 0, G_OPTION_ARG_NONE, NULL,
            ("Always open a new window for browsing specified URIs"), NULL },
          { "quit", 'q', 0, G_OPTION_ARG_NONE, NULL, ("Quit Nautilus."),
            NULL },
          { "select", 's', 0, G_OPTION_ARG_NONE, NULL,
            ("Select specified URI in parent folder."), NULL },
          { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, NULL, NULL,
            ("[URI…]") },

          { NULL } };
  g_application_add_main_option_entries (G_APPLICATION (self), options);
}

gint
lol_application_handle_local_options (GApplication *app, GVariantDict *options)
{
  gchar *cwd;

  cwd = g_get_current_dir ();
  g_variant_dict_insert (options, "cwd", "s", cwd);
  g_free (cwd);

  return -1;
}

gint
lol_application_handle_file_args (LolApplication *self, GVariantDict *options)
{
  gint idx, len;
  g_autofree const gchar **remaining = NULL;

  g_variant_dict_lookup (options, G_OPTION_REMAINING, "^a&s", &remaining);

  if (remaining)
    {
      for (idx = 0; remaining[idx] != NULL; idx++)
        {
          gchar *cwd;

          g_variant_dict_lookup (options, "cwd", "s", &cwd);

          if (cwd == NULL)
            {
              self->file = g_file_new_for_commandline_arg (remaining[idx]);
            }
          else
            {
              self->file = g_file_new_for_commandline_arg_and_cwd (
                  remaining[idx], cwd);
              g_free (cwd);
            }
          break;
        }
    }

  if (self->file == NULL)
    {
      printf ("No file given. Opening empty file\n");
    }

  lol_application_activate (G_APPLICATION (self));

  return EXIT_SUCCESS;
}

gint
lol_application_command_line (GApplication *application,
                              GApplicationCommandLine *command_line)
{
  LolApplication *self = LOL_APPLICATION (application);
  GVariantDict *options;

  options = g_application_command_line_get_options_dict (command_line);
  if (g_variant_dict_contains (options, "version"))
    {
      g_application_command_line_print (command_line,
                                        APP_NAME " (" APP_DISPLAY_NAME
                                                 ") " PACKAGE_VERSION "\n");
      return EXIT_SUCCESS;
    }

  return lol_application_handle_file_args (self, options);
}
