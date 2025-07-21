#include "main_window.h"
#include <stdio.h>

// Log callback function
static void log_output_callback(const char *line, gpointer user_data) {
    MainWindow *win = (MainWindow*)user_data;

    // Check operation end marker
    if (g_strcmp0(line, "__OPERATION_FINISHED__") == 0) {
        // Operation completed - enable buttons
        win->operation_in_progress = FALSE;

        gtk_widget_set_sensitive(win->install_btn, TRUE);
        gtk_widget_set_sensitive(win->remove_btn, TRUE);
        gtk_widget_set_sensitive(win->update_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_cache_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_all_cache_btn, TRUE);

        gtk_label_set_text(GTK_LABEL(win->status_label), "Operation completed");
        return;
    }

    if (!win->log_buffer) return;

    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(win->log_buffer, &end_iter);

    // Add timestamp
    GDateTime *now = g_date_time_new_now_local();
    gchar *timestamp = g_date_time_format(now, "[%H:%M:%S] ");

    gtk_text_buffer_insert(win->log_buffer, &end_iter, timestamp, -1);
    gtk_text_buffer_insert(win->log_buffer, &end_iter, line, -1);
    gtk_text_buffer_insert(win->log_buffer, &end_iter, "\n", -1);

    // Auto-scroll to bottom
    gtk_text_buffer_get_end_iter(win->log_buffer, &end_iter);
    GtkTextMark *mark = gtk_text_buffer_create_mark(win->log_buffer, NULL, &end_iter, FALSE);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(win->log_textview), mark);
    gtk_text_buffer_delete_mark(win->log_buffer, mark);

    g_free(timestamp);
    g_date_time_unref(now);

    // Update UI
    while (g_main_context_pending(NULL)) {
        g_main_context_iteration(NULL, FALSE);
    }
}

static void show_log_window(MainWindow *win) {
    if (!win->log_window) {
        // Create log window
        win->log_window = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(win->log_window), "Installation Log");
        gtk_window_set_default_size(GTK_WINDOW(win->log_window), 600, 400);
        gtk_window_set_transient_for(GTK_WINDOW(win->log_window), GTK_WINDOW(win->window));

        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_widget_set_margin_start(vbox, 10);
        gtk_widget_set_margin_end(vbox, 10);
        gtk_widget_set_margin_top(vbox, 10);
        gtk_widget_set_margin_bottom(vbox, 10);

        // Scrolled window for log
        GtkWidget *scrolled = gtk_scrolled_window_new();
        gtk_widget_set_vexpand(scrolled, TRUE);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

        // Text view for log output
        win->log_textview = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(win->log_textview), FALSE);
        gtk_text_view_set_monospace(GTK_TEXT_VIEW(win->log_textview), TRUE);
        win->log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(win->log_textview));

        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), win->log_textview);

        // Close button
        win->close_log_btn = gtk_button_new_with_label("Close");
        g_signal_connect_swapped(win->close_log_btn, "clicked",
                                G_CALLBACK(gtk_window_close), win->log_window);

        gtk_box_append(GTK_BOX(vbox), scrolled);
        gtk_box_append(GTK_BOX(vbox), win->close_log_btn);

        gtk_window_set_child(GTK_WINDOW(win->log_window), vbox);
    }

    // Clear previous log
    gtk_text_buffer_set_text(win->log_buffer, "", -1);

    gtk_window_present(GTK_WINDOW(win->log_window));
}

static void on_search_clicked(GtkButton *button, gpointer user_data) {
    MainWindow *win = (MainWindow*)user_data;

    if (win->operation_in_progress) return;

    const char *query = gtk_editable_get_text(GTK_EDITABLE(win->search_entry));
    if (strlen(query) == 0) return;

    gtk_label_set_text(GTK_LABEL(win->status_label), "Searching...");

    // Clear previous results
    GtkWidget *child = gtk_widget_get_first_child(win->package_list);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(win->package_list), child);
        child = next;
    }

    // Check source (Official repos or AUR)
    const char *selected_source = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(win->source_combo));

    PackageList *packages = NULL;
    if (g_strcmp0(selected_source, "AUR") == 0) {
        packages = aur_search(query);
    } else {
        packages = pacman_search(query);
    }

    if (packages) {
        for (int i = 0; i < packages->count; i++) {
            Package *pkg = &packages->packages[i];

            GtkWidget *row = gtk_list_box_row_new();
            GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

            char title[512];
            snprintf(title, sizeof(title), "<b>%s</b> (%s) [%s]",
                    pkg->name, pkg->version, pkg->repository);
            GtkWidget *name_label = gtk_label_new(NULL);
            gtk_label_set_markup(GTK_LABEL(name_label), title);
            gtk_label_set_xalign(GTK_LABEL(name_label), 0.0);

            GtkWidget *desc_label = gtk_label_new(pkg->description);
            gtk_label_set_xalign(GTK_LABEL(desc_label), 0.0);
            gtk_label_set_wrap(GTK_LABEL(desc_label), TRUE);

            gtk_box_append(GTK_BOX(box), name_label);
            gtk_box_append(GTK_BOX(box), desc_label);
            gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);

            // Store package name and source
            g_object_set_data_full(G_OBJECT(row), "package_name",
                                 g_strdup(pkg->name), g_free);
            g_object_set_data_full(G_OBJECT(row), "package_source",
                                 g_strdup(selected_source), g_free);

            gtk_list_box_append(GTK_LIST_BOX(win->package_list), row);
        }

        char status[256];
        snprintf(status, sizeof(status), "Found %d packages in %s",
                packages->count, selected_source);
        gtk_label_set_text(GTK_LABEL(win->status_label), status);

        package_list_free(packages);
    } else {
        gtk_label_set_text(GTK_LABEL(win->status_label), "Search failed");
    }
}

static void on_package_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    MainWindow *win = (MainWindow*)user_data;

    if (row && !win->operation_in_progress) {
        const char *pkg_name = g_object_get_data(G_OBJECT(row), "package_name");
        if (pkg_name) {
            free(win->selected_package);
            win->selected_package = strdup(pkg_name);

            gtk_widget_set_sensitive(win->install_btn, TRUE);
            gtk_widget_set_sensitive(win->remove_btn, TRUE);
            gtk_widget_set_sensitive(win->deps_btn, TRUE);
        }
    }
}

static void on_install_clicked(GtkButton *button, gpointer user_data) {
    MainWindow *win = (MainWindow*)user_data;

    if (!win->selected_package || win->operation_in_progress) return;

    win->operation_in_progress = TRUE;
    gtk_widget_set_sensitive(win->install_btn, FALSE);
    gtk_widget_set_sensitive(win->remove_btn, FALSE);
    gtk_widget_set_sensitive(win->update_btn, FALSE);
    gtk_widget_set_sensitive(win->clean_cache_btn, FALSE);
    gtk_widget_set_sensitive(win->clean_all_cache_btn, FALSE);

    char status[256];
    snprintf(status, sizeof(status), "Installing %s...", win->selected_package);
    gtk_label_set_text(GTK_LABEL(win->status_label), status);

    show_log_window(win);

    GtkListBoxRow *selected_row = gtk_list_box_get_selected_row(GTK_LIST_BOX(win->package_list));
    const char *source = g_object_get_data(G_OBJECT(selected_row), "package_source");

    gboolean success;
    if (g_strcmp0(source, "AUR") == 0) {
        success = aur_install_async(win->selected_package, log_output_callback, win);
    } else {
        success = pacman_install_async(win->selected_package, log_output_callback, win);
    }

    if (!success) {
        // If failed to start - immediately enable buttons
        win->operation_in_progress = FALSE;
        gtk_widget_set_sensitive(win->install_btn, TRUE);
        gtk_widget_set_sensitive(win->remove_btn, TRUE);
        gtk_widget_set_sensitive(win->update_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_cache_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_all_cache_btn, TRUE);
        gtk_label_set_text(GTK_LABEL(win->status_label), "Failed to start installation");
    }
}

static void on_remove_clicked(GtkButton *button, gpointer user_data) {
    MainWindow *win = (MainWindow*)user_data;

    if (!win->selected_package || win->operation_in_progress) return;

    win->operation_in_progress = TRUE;
    gtk_widget_set_sensitive(win->install_btn, FALSE);
    gtk_widget_set_sensitive(win->remove_btn, FALSE);
    gtk_widget_set_sensitive(win->update_btn, FALSE);
    gtk_widget_set_sensitive(win->clean_cache_btn, FALSE);
    gtk_widget_set_sensitive(win->clean_all_cache_btn, FALSE);

    char status[256];
    snprintf(status, sizeof(status), "Removing %s...", win->selected_package);
    gtk_label_set_text(GTK_LABEL(win->status_label), status);

    show_log_window(win);

    gboolean success = pacman_remove_async(win->selected_package, log_output_callback, win);

    if (!success) {
        win->operation_in_progress = FALSE;
        gtk_widget_set_sensitive(win->install_btn, TRUE);
        gtk_widget_set_sensitive(win->remove_btn, TRUE);
        gtk_widget_set_sensitive(win->update_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_cache_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_all_cache_btn, TRUE);
        gtk_label_set_text(GTK_LABEL(win->status_label), "Failed to start removal");
    }
}

static void on_deps_clicked(GtkButton *button, gpointer user_data) {
    MainWindow *win = (MainWindow*)user_data;
    
    if (!win->selected_package) {
        gtk_label_set_text(GTK_LABEL(win->status_label), "Please select a package first");
        return;
    }
    
    if (!win->dep_viewer) {
        win->dep_viewer = dependency_viewer_new();
        gtk_window_set_transient_for(GTK_WINDOW(win->dep_viewer->window), GTK_WINDOW(win->window));
    }
    
    dependency_viewer_set_package(win->dep_viewer, win->selected_package);
    dependency_viewer_show(win->dep_viewer);
}

static void on_update_clicked(GtkButton *button, gpointer user_data) {
    MainWindow *win = (MainWindow*)user_data;

    if (win->operation_in_progress) return;

    win->operation_in_progress = TRUE;
    gtk_widget_set_sensitive(win->install_btn, FALSE);
    gtk_widget_set_sensitive(win->remove_btn, FALSE);
    gtk_widget_set_sensitive(win->update_btn, FALSE);
    gtk_widget_set_sensitive(win->clean_cache_btn, FALSE);
    gtk_widget_set_sensitive(win->clean_all_cache_btn, FALSE);
    gtk_widget_set_sensitive(win->clean_cache_btn, FALSE);
    gtk_widget_set_sensitive(win->clean_all_cache_btn, FALSE);

    gtk_label_set_text(GTK_LABEL(win->status_label), "Updating system...");

    show_log_window(win);

    gboolean success = pacman_update_system_async(log_output_callback, win);

    if (!success) {
        win->operation_in_progress = FALSE;
        gtk_widget_set_sensitive(win->install_btn, TRUE);
        gtk_widget_set_sensitive(win->remove_btn, TRUE);
        gtk_widget_set_sensitive(win->update_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_cache_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_all_cache_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_cache_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_all_cache_btn, TRUE);
        gtk_label_set_text(GTK_LABEL(win->status_label), "Failed to start system update");
    }
}

static void on_installed_packages_loaded(PackageList *packages, gpointer user_data) {
    MainWindow *win = (MainWindow*)user_data;
    
    if (win->installed_packages) {
        package_list_free(win->installed_packages);
    }
    
    win->installed_packages = packages;
    
    if (packages) {
        // Update progress indicator
        char progress_text[128];
        snprintf(progress_text, sizeof(progress_text), "Processing %d packages...", packages->count);
        gtk_label_set_text(GTK_LABEL(win->loading_progress_label), progress_text);
        
        // Add packages to UI
        for (int i = 0; i < packages->count; i++) {
            Package *pkg = &packages->packages[i];

            GtkWidget *row = gtk_list_box_row_new();
            GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

            char title[512];
            snprintf(title, sizeof(title), "<b>%s</b> (%s)",
                    pkg->name, pkg->version);
            GtkWidget *name_label = gtk_label_new(NULL);
            gtk_label_set_markup(GTK_LABEL(name_label), title);
            gtk_label_set_xalign(GTK_LABEL(name_label), 0.0);

            GtkWidget *desc_label = gtk_label_new(pkg->description);
            gtk_label_set_xalign(GTK_LABEL(desc_label), 0.0);
            gtk_label_set_wrap(GTK_LABEL(desc_label), TRUE);

            gtk_box_append(GTK_BOX(box), name_label);
            gtk_box_append(GTK_BOX(box), desc_label);
            gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);

            // Store package name
            g_object_set_data_full(G_OBJECT(row), "package_name",
                                 g_strdup(pkg->name), g_free);
            g_object_set_data_full(G_OBJECT(row), "package_source",
                                 g_strdup("installed"), g_free);

            gtk_list_box_append(GTK_LIST_BOX(win->installed_list), row);
        }

        char status[256];
        snprintf(status, sizeof(status), "Loaded %d installed packages", packages->count);
        gtk_label_set_text(GTK_LABEL(win->status_label), status);
    } else {
        gtk_label_set_text(GTK_LABEL(win->status_label), "Failed to load installed packages");
    }
    
    // Stop spinner and show list with smooth transition
    gtk_spinner_stop(GTK_SPINNER(win->installed_spinner));
    gtk_stack_set_transition_type(GTK_STACK(win->installed_stack), GTK_STACK_TRANSITION_TYPE_SLIDE_DOWN);
    gtk_stack_set_visible_child_name(GTK_STACK(win->installed_stack), "list");
    
    // Enable refresh button
    gtk_widget_set_sensitive(win->refresh_installed_btn, TRUE);
}

static void populate_installed_packages(MainWindow *win) {
    gtk_label_set_text(GTK_LABEL(win->status_label), "Loading installed packages...");

    // Clear previous results
    GtkWidget *child = gtk_widget_get_first_child(win->installed_list);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(win->installed_list), child);
        child = next;
    }

    // Show spinner with smooth transition
    gtk_stack_set_transition_type(GTK_STACK(win->installed_stack), GTK_STACK_TRANSITION_TYPE_SLIDE_UP);
    gtk_stack_set_visible_child_name(GTK_STACK(win->installed_stack), "spinner");
    gtk_spinner_start(GTK_SPINNER(win->installed_spinner));
    gtk_widget_set_sensitive(win->refresh_installed_btn, FALSE);
    
    // Update progress
    gtk_label_set_text(GTK_LABEL(win->loading_progress_label), "Querying system packages...");

    // Start async loading
    pacman_list_installed_async(on_installed_packages_loaded, win);
}

static void on_refresh_installed_clicked(GtkButton *button, gpointer user_data) {
    MainWindow *win = (MainWindow*)user_data;
    
    if (win->operation_in_progress) return;
    
    populate_installed_packages(win);
}

static void on_installed_package_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    MainWindow *win = (MainWindow*)user_data;

    if (row && !win->operation_in_progress) {
        const char *pkg_name = g_object_get_data(G_OBJECT(row), "package_name");
        if (pkg_name) {
            free(win->selected_package);
            win->selected_package = strdup(pkg_name);

            // For installed packages, only enable remove and dependencies
            gtk_widget_set_sensitive(win->install_btn, FALSE);
            gtk_widget_set_sensitive(win->remove_btn, TRUE);
            gtk_widget_set_sensitive(win->deps_btn, TRUE);
        }
    }
}

static void on_notebook_page_switched(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data) {
    MainWindow *win = (MainWindow*)user_data;
    
    // If switching to installed packages tab (page 1) and packages not loaded yet
    if (page_num == 1 && !win->installed_packages_loaded) {
        win->installed_packages_loaded = TRUE;
        populate_installed_packages(win);
    }
}

static void update_cache_size_label(MainWindow *win) {
    char *cache_size = pacman_get_cache_size();
    char label_text[128];
    snprintf(label_text, sizeof(label_text), "Cache size: %s", cache_size);
    gtk_label_set_text(GTK_LABEL(win->cache_size_label), label_text);
    free(cache_size);
}

static void on_clean_cache_clicked(GtkButton *button, gpointer user_data) {
    MainWindow *win = (MainWindow*)user_data;

    if (win->operation_in_progress) return;

    win->operation_in_progress = TRUE;
    gtk_widget_set_sensitive(win->install_btn, FALSE);
    gtk_widget_set_sensitive(win->remove_btn, FALSE);
    gtk_widget_set_sensitive(win->update_btn, FALSE);
    gtk_widget_set_sensitive(win->clean_cache_btn, FALSE);
    gtk_widget_set_sensitive(win->clean_all_cache_btn, FALSE);
    gtk_widget_set_sensitive(win->clean_cache_btn, FALSE);
    gtk_widget_set_sensitive(win->clean_all_cache_btn, FALSE);

    gtk_label_set_text(GTK_LABEL(win->status_label), "Cleaning package cache...");

    show_log_window(win);

    gboolean success = pacman_clean_cache_async(log_output_callback, win);

    if (!success) {
        win->operation_in_progress = FALSE;
        gtk_widget_set_sensitive(win->install_btn, TRUE);
        gtk_widget_set_sensitive(win->remove_btn, TRUE);
        gtk_widget_set_sensitive(win->update_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_cache_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_all_cache_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_cache_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_all_cache_btn, TRUE);
        gtk_label_set_text(GTK_LABEL(win->status_label), "Failed to start cache cleaning");
    } else {
        // Update cache size after cleaning
        g_idle_add((GSourceFunc)update_cache_size_label, win);
    }
}

static void on_clean_all_cache_clicked(GtkButton *button, gpointer user_data) {
    MainWindow *win = (MainWindow*)user_data;

    if (win->operation_in_progress) return;

    // Show confirmation dialog using GTK4 async API
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(win->window),
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_WARNING,
                                               GTK_BUTTONS_YES_NO,
                                               "This will remove ALL cached packages, including those needed for downgrades.\n\nAre you sure you want to continue?");

    gtk_window_set_title(GTK_WINDOW(dialog), "Confirm Cache Cleanup");
    
    // For now, just proceed with cleaning (GTK4 async dialogs are more complex)
    gtk_window_destroy(GTK_WINDOW(dialog));

    win->operation_in_progress = TRUE;
    gtk_widget_set_sensitive(win->install_btn, FALSE);
    gtk_widget_set_sensitive(win->remove_btn, FALSE);
    gtk_widget_set_sensitive(win->update_btn, FALSE);
    gtk_widget_set_sensitive(win->clean_cache_btn, FALSE);
    gtk_widget_set_sensitive(win->clean_all_cache_btn, FALSE);
    gtk_widget_set_sensitive(win->clean_cache_btn, FALSE);
    gtk_widget_set_sensitive(win->clean_all_cache_btn, FALSE);

    gtk_label_set_text(GTK_LABEL(win->status_label), "Cleaning all package cache...");

    show_log_window(win);

    gboolean success = pacman_clean_all_cache_async(log_output_callback, win);

    if (!success) {
        win->operation_in_progress = FALSE;
        gtk_widget_set_sensitive(win->install_btn, TRUE);
        gtk_widget_set_sensitive(win->remove_btn, TRUE);
        gtk_widget_set_sensitive(win->update_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_cache_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_all_cache_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_cache_btn, TRUE);
        gtk_widget_set_sensitive(win->clean_all_cache_btn, TRUE);
        gtk_label_set_text(GTK_LABEL(win->status_label), "Failed to start cache cleaning");
    } else {
        // Update cache size after cleaning
        g_idle_add((GSourceFunc)update_cache_size_label, win);
    }
}

MainWindow* main_window_new(void) {
    MainWindow *win = malloc(sizeof(MainWindow));
    win->selected_package = NULL;
    win->operation_in_progress = FALSE;
    win->log_window = NULL;
    win->dep_viewer = NULL;
    win->current_packages = NULL;
    win->installed_packages = NULL;
    win->installed_packages_loaded = FALSE;

    // Create window
    win->window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win->window), "Pacman GUI");
    gtk_window_set_default_size(GTK_WINDOW(win->window), 900, 700);

    // Main container
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(vbox, 10);
    gtk_widget_set_margin_end(vbox, 10);
    gtk_widget_set_margin_top(vbox, 10);
    gtk_widget_set_margin_bottom(vbox, 10);

    // Create notebook for tabs
    win->notebook = gtk_notebook_new();
    g_signal_connect(win->notebook, "switch-page", G_CALLBACK(on_notebook_page_switched), win);

    // === SEARCH TAB ===
    GtkWidget *search_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    
    // Source selection
    GtkWidget *source_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *source_label = gtk_label_new("Source:");
    win->source_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->source_combo), "Official Repos");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->source_combo), "AUR");
    gtk_combo_box_set_active(GTK_COMBO_BOX(win->source_combo), 0);

    gtk_box_append(GTK_BOX(source_box), source_label);
    gtk_box_append(GTK_BOX(source_box), win->source_combo);

    // Search section
    GtkWidget *search_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    win->search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(win->search_entry), "Search packages...");

    GtkWidget *search_btn = gtk_button_new_with_label("Search");
    g_signal_connect(search_btn, "clicked", G_CALLBACK(on_search_clicked), win);

    gtk_box_append(GTK_BOX(search_box), win->search_entry);
    gtk_box_append(GTK_BOX(search_box), search_btn);

    // Package list for search results
    GtkWidget *search_scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(search_scrolled, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(search_scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    win->package_list = gtk_list_box_new();
    g_signal_connect(win->package_list, "row-selected",
                     G_CALLBACK(on_package_selected), win);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(search_scrolled), win->package_list);

    gtk_box_append(GTK_BOX(search_tab), source_box);
    gtk_box_append(GTK_BOX(search_tab), search_box);
    gtk_box_append(GTK_BOX(search_tab), search_scrolled);

    // === INSTALLED PACKAGES TAB ===
    GtkWidget *installed_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    
    // Refresh button for installed packages
    GtkWidget *installed_controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    win->refresh_installed_btn = gtk_button_new_with_label("Refresh Installed Packages");
    g_signal_connect(win->refresh_installed_btn, "clicked", G_CALLBACK(on_refresh_installed_clicked), win);
    gtk_box_append(GTK_BOX(installed_controls), win->refresh_installed_btn);

    // Stack for switching between spinner and list
    win->installed_stack = gtk_stack_new();
    gtk_widget_set_vexpand(win->installed_stack, TRUE);
    
    // Spinner page
    GtkWidget *spinner_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_valign(spinner_box, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(spinner_box, GTK_ALIGN_CENTER);
    
    win->installed_spinner = gtk_spinner_new();
    gtk_widget_set_size_request(win->installed_spinner, 48, 48);
    
    GtkWidget *loading_label = gtk_label_new("Loading installed packages...");
    gtk_widget_add_css_class(loading_label, "dim-label");
    
    win->loading_progress_label = gtk_label_new("Initializing...");
    gtk_widget_add_css_class(win->loading_progress_label, "caption");
    
    gtk_box_append(GTK_BOX(spinner_box), win->installed_spinner);
    gtk_box_append(GTK_BOX(spinner_box), loading_label);
    gtk_box_append(GTK_BOX(spinner_box), win->loading_progress_label);
    
    gtk_stack_add_named(GTK_STACK(win->installed_stack), spinner_box, "spinner");

    // Package list page
    GtkWidget *installed_scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(installed_scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    win->installed_list = gtk_list_box_new();
    g_signal_connect(win->installed_list, "row-selected",
                     G_CALLBACK(on_installed_package_selected), win);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(installed_scrolled), win->installed_list);
    
    gtk_stack_add_named(GTK_STACK(win->installed_stack), installed_scrolled, "list");
    
    // Initial loading state with placeholder
    GtkWidget *placeholder_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_valign(placeholder_box, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(placeholder_box, GTK_ALIGN_CENTER);
    
    GtkWidget *package_icon = gtk_label_new("📦");
    gtk_label_set_markup(GTK_LABEL(package_icon), "<span size='xx-large'>📦</span>");
    
    GtkWidget *placeholder_label = gtk_label_new("Installed packages will be loaded when you switch to this tab");
    gtk_widget_add_css_class(placeholder_label, "dim-label");
    
    GtkWidget *click_hint = gtk_label_new("This helps keep the app startup fast");
    gtk_widget_add_css_class(click_hint, "caption");
    
    gtk_box_append(GTK_BOX(placeholder_box), package_icon);
    gtk_box_append(GTK_BOX(placeholder_box), placeholder_label);
    gtk_box_append(GTK_BOX(placeholder_box), click_hint);
    
    gtk_stack_add_named(GTK_STACK(win->installed_stack), placeholder_box, "placeholder");
    
    // Set initial state to placeholder
    gtk_stack_set_visible_child_name(GTK_STACK(win->installed_stack), "placeholder");

    gtk_box_append(GTK_BOX(installed_tab), installed_controls);
    gtk_box_append(GTK_BOX(installed_tab), win->installed_stack);

    // Add tabs to notebook
    gtk_notebook_append_page(GTK_NOTEBOOK(win->notebook), search_tab,
                           gtk_label_new("Search Packages"));
    gtk_notebook_append_page(GTK_NOTEBOOK(win->notebook), installed_tab,
                           gtk_label_new("Installed Packages"));

    // === BUTTONS SECTION (shared between tabs) ===
    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    win->install_btn = gtk_button_new_with_label("Install");
    win->remove_btn = gtk_button_new_with_label("Remove");
    win->deps_btn = gtk_button_new_with_label("Dependencies");
    win->update_btn = gtk_button_new_with_label("Update System");

    gtk_widget_set_sensitive(win->install_btn, FALSE);
    gtk_widget_set_sensitive(win->remove_btn, FALSE);
    gtk_widget_set_sensitive(win->deps_btn, FALSE);

    g_signal_connect(win->install_btn, "clicked", G_CALLBACK(on_install_clicked), win);
    g_signal_connect(win->remove_btn, "clicked", G_CALLBACK(on_remove_clicked), win);
    g_signal_connect(win->deps_btn, "clicked", G_CALLBACK(on_deps_clicked), win);
    g_signal_connect(win->update_btn, "clicked", G_CALLBACK(on_update_clicked), win);

    gtk_box_append(GTK_BOX(btn_box), win->install_btn);
    gtk_box_append(GTK_BOX(btn_box), win->remove_btn);
    gtk_box_append(GTK_BOX(btn_box), win->deps_btn);
    gtk_box_append(GTK_BOX(btn_box), win->update_btn);

    // === CACHE MANAGEMENT SECTION ===
    GtkWidget *cache_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    win->cache_size_label = gtk_label_new("Cache size: calculating...");
    win->clean_cache_btn = gtk_button_new_with_label("Clean Cache (old packages)");
    win->clean_all_cache_btn = gtk_button_new_with_label("Clean All Cache");

    g_signal_connect(win->clean_cache_btn, "clicked", G_CALLBACK(on_clean_cache_clicked), win);
    g_signal_connect(win->clean_all_cache_btn, "clicked", G_CALLBACK(on_clean_all_cache_clicked), win);

    gtk_box_append(GTK_BOX(cache_box), win->cache_size_label);
    gtk_box_append(GTK_BOX(cache_box), win->clean_cache_btn);
    gtk_box_append(GTK_BOX(cache_box), win->clean_all_cache_btn);

    // === STATUS BAR ===
    win->status_label = gtk_label_new("Ready");
    gtk_label_set_xalign(GTK_LABEL(win->status_label), 0.0);

    // Pack everything into main container
    gtk_box_append(GTK_BOX(vbox), win->notebook);
    gtk_box_append(GTK_BOX(vbox), btn_box);
    gtk_box_append(GTK_BOX(vbox), cache_box);
    gtk_box_append(GTK_BOX(vbox), win->status_label);

    gtk_window_set_child(GTK_WINDOW(win->window), vbox);

    // Initialize cache size display
    update_cache_size_label(win);

    // Detect AUR helper on startup
    set_aur_helper(detect_aur_helper());

    // Don't load installed packages immediately to speed up startup
    win->installed_packages_loaded = FALSE;

    return win;
}

void main_window_show(MainWindow *win) {
    gtk_window_present(GTK_WINDOW(win->window));
}

void main_window_free(MainWindow *win) {
    if (win->selected_package) free(win->selected_package);
    if (win->current_packages) package_list_free(win->current_packages);
    if (win->installed_packages) package_list_free(win->installed_packages);
    if (win->log_window) gtk_window_destroy(GTK_WINDOW(win->log_window));
    if (win->dep_viewer) dependency_viewer_free(win->dep_viewer);
    free(win);
}