#include <gtk-4.0/gtk/gtk.h>
#include "ui/main_window.h"
#include <gio/gio.h>

static void setup_theme(void) {
    GSettings *interface_settings;
    gchar *color_scheme;
    GtkSettings *gtk_settings;

    // Try to get GNOME settings
    interface_settings = g_settings_new("org.gnome.desktop.interface");

    if (interface_settings) {
        color_scheme = g_settings_get_string(interface_settings, "color-scheme");
        gtk_settings = gtk_settings_get_default();

        if (g_strcmp0(color_scheme, "prefer-dark") == 0) {
            g_object_set(gtk_settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
        } else {
            g_object_set(gtk_settings, "gtk-application-prefer-dark-theme", FALSE, NULL);
        }

        g_free(color_scheme);
        g_object_unref(interface_settings);
    }

    // Fallback - check environment variable
    const char *gtk_theme = g_getenv("GTK_THEME");
    if (gtk_theme && g_str_has_suffix(gtk_theme, ":dark")) {
        GtkSettings *settings = gtk_settings_get_default();
        g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
    }
}

static void on_activate(GtkApplication *app, gpointer user_data) {
    setup_theme();

    MainWindow *main_win = main_window_new();

    // Set application for window
    gtk_window_set_application(GTK_WINDOW(main_win->window), app);

    // Show window
    main_window_show(main_win);

    // Store window pointer in app data for cleanup
    g_object_set_data_full(G_OBJECT(app), "main_window", main_win,
                          (GDestroyNotify)main_window_free);
}

static void on_shutdown(GtkApplication *app, gpointer user_data) {
    g_print("Application shutting down...\n");
}

int main(int argc, char *argv[]) {
    GtkApplication *app;
    int status;

    // Create application
    app = gtk_application_new("org.archlinux.pacman-gui", G_APPLICATION_FLAGS_NONE);

    // Connect signals
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    g_signal_connect(app, "shutdown", G_CALLBACK(on_shutdown), NULL);

    // Run application
    status = g_application_run(G_APPLICATION(app), argc, argv);

    // Cleanup
    g_object_unref(app);

    return status;
}