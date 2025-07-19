#include "pacman_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static AURHelper current_aur_helper = AUR_HELPER_NONE;

typedef struct {
    LogCallback callback;
    gpointer user_data;
    int pipe_fd;
    GIOChannel *channel;
    guint watch_id;
    pid_t child_pid;
} AsyncOperation;

AURHelper detect_aur_helper(void) {
    if (system("which yay > /dev/null 2>&1") == 0) {
        return AUR_HELPER_YAY;
    } else if (system("which paru > /dev/null 2>&1") == 0) {
        return AUR_HELPER_PARU;
    }
    return AUR_HELPER_NONE;
}

void set_aur_helper(AURHelper helper) {
    current_aur_helper = helper;
}

static char* run_command(const char *cmd) {
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;

    char *output = malloc(4096);
    char *result = malloc(1);
    result[0] = '\0';

    while (fgets(output, 4096, fp)) {
        result = realloc(result, strlen(result) + strlen(output) + 1);
        strcat(result, output);
    }

    pclose(fp);
    free(output);
    return result;
}

static gboolean read_pipe_data(GIOChannel *channel, GIOCondition condition, gpointer user_data) {
    AsyncOperation *op = (AsyncOperation*)user_data;

    if (condition & G_IO_IN) {
        gchar *line;
        gsize length;
        GError *error = NULL;

        GIOStatus status = g_io_channel_read_line(channel, &line, &length, NULL, &error);

        if (status == G_IO_STATUS_NORMAL) {
            if (length > 0 && line[length-1] == '\n') {
                line[length-1] = '\0';
            }

            if (op->callback) {
                op->callback(line, op->user_data);
            }

            g_free(line);
            return TRUE;
        }
    }

    if (condition & (G_IO_HUP | G_IO_ERR)) {
        // Pipe closed - check child process status
        int status;
        waitpid(op->child_pid, &status, 0);

        // Send operation completion signal
        if (op->callback) {
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                op->callback("=== Operation completed successfully ===", op->user_data);
            } else {
                op->callback("=== Operation failed ===", op->user_data);
            }

            // Special operation end marker
            op->callback("__OPERATION_FINISHED__", op->user_data);
        }

        g_io_channel_unref(channel);
        close(op->pipe_fd);
        g_free(op);
        return FALSE;
    }

    return TRUE;
}

static gboolean run_command_async(const char *cmd, LogCallback callback, gpointer user_data) {
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) == -1) {
        return FALSE;
    }

    pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        return FALSE;
    }

    if (pid == 0) {
        // Child process
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        execl("/bin/sh", "sh", "-c", cmd, NULL);
        exit(1);
    } else {
        // Parent process
        close(pipefd[1]);

        AsyncOperation *op = g_malloc(sizeof(AsyncOperation));
        op->callback = callback;
        op->user_data = user_data;
        op->pipe_fd = pipefd[0];
        op->child_pid = pid;

        op->channel = g_io_channel_unix_new(pipefd[0]);
        g_io_channel_set_flags(op->channel, G_IO_FLAG_NONBLOCK, NULL);

        op->watch_id = g_io_add_watch(op->channel,
                                      G_IO_IN | G_IO_HUP | G_IO_ERR,
                                      read_pipe_data, op);

        return TRUE;
    }
}

PackageList* pacman_search(const char *query) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "pacman -Ss %s", query);

    char *output = run_command(cmd);
    if (!output) return NULL;

    PackageList *list = malloc(sizeof(PackageList));
    list->packages = malloc(sizeof(Package) * 100);
    list->count = 0;

    char *line = strtok(output, "\n");
    while (line && list->count < 100) {
        if (strstr(line, "/")) {
            Package *pkg = &list->packages[list->count];

            char *slash = strchr(line, '/');
            char *space = strchr(slash, ' ');

            pkg->repository = strndup(line, slash - line);
            pkg->name = strndup(slash + 1, space - slash - 1);
            pkg->version = strdup(space + 1);

            line = strtok(NULL, "\n");
            if (line && line[0] == ' ') {
                pkg->description = strdup(line + 4);
            }

            pkg->installed = FALSE;
            list->count++;
        }
        line = strtok(NULL, "\n");
    }

    free(output);
    return list;
}

PackageList* aur_search(const char *query) {
    if (current_aur_helper == AUR_HELPER_NONE) {
        current_aur_helper = detect_aur_helper();
    }

    char cmd[512];
    if (current_aur_helper == AUR_HELPER_YAY) {
        snprintf(cmd, sizeof(cmd), "yay -Ss %s", query);
    } else if (current_aur_helper == AUR_HELPER_PARU) {
        snprintf(cmd, sizeof(cmd), "paru -Ss %s", query);
    } else {
        return NULL; // No AUR helper available
    }

    char *output = run_command(cmd);
    if (!output) return NULL;

    PackageList *list = malloc(sizeof(PackageList));
    list->packages = malloc(sizeof(Package) * 100);
    list->count = 0;

    char *line = strtok(output, "\n");
    while (line && list->count < 100) {
        if (strstr(line, "/")) {
            Package *pkg = &list->packages[list->count];

            char *slash = strchr(line, '/');
            char *space = strchr(slash, ' ');

            pkg->repository = strndup(line, slash - line);
            pkg->name = strndup(slash + 1, space - slash - 1);
            pkg->version = strdup(space + 1);

            line = strtok(NULL, "\n");
            if (line && line[0] == ' ') {
                pkg->description = strdup(line + 4);
            }

            pkg->installed = FALSE;
            list->count++;
        }
        line = strtok(NULL, "\n");
    }

    free(output);
    return list;
}

gboolean pacman_install_async(const char *package_name, LogCallback callback, gpointer user_data) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "pkexec pacman -S --noconfirm %s", package_name);

    return run_command_async(cmd, callback, user_data);
}

gboolean aur_install_async(const char *package_name, LogCallback callback, gpointer user_data) {
    if (current_aur_helper == AUR_HELPER_NONE) {
        current_aur_helper = detect_aur_helper();
    }

    char cmd[512];
    if (current_aur_helper == AUR_HELPER_YAY) {
        snprintf(cmd, sizeof(cmd), "yay -S --noconfirm %s", package_name);
    } else if (current_aur_helper == AUR_HELPER_PARU) {
        snprintf(cmd, sizeof(cmd), "paru -S --noconfirm %s", package_name);
    } else {
        return FALSE;
    }

    return run_command_async(cmd, callback, user_data);
}

gboolean pacman_remove_async(const char *package_name, LogCallback callback, gpointer user_data) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "pkexec pacman -R --noconfirm %s", package_name);

    return run_command_async(cmd, callback, user_data);
}

gboolean pacman_update_system_async(LogCallback callback, gpointer user_data) {
    return run_command_async("pkexec pacman -Syu --noconfirm", callback, user_data);
}

PackageList* pacman_list_updates(void) {
    char *output = run_command("pacman -Qu");
    if (!output) return NULL;

    PackageList *list = malloc(sizeof(PackageList));
    list->packages = malloc(sizeof(Package) * 100);
    list->count = 0;

    char *line = strtok(output, "\n");
    while (line && list->count < 100) {
        char *space = strchr(line, ' ');
        if (space) {
            Package *pkg = &list->packages[list->count];
            pkg->name = strndup(line, space - line);
            pkg->version = strdup(space + 1);
            pkg->repository = strdup("local");
            pkg->description = strdup("Update available");
            pkg->installed = TRUE;
            list->count++;
        }
        line = strtok(NULL, "\n");
    }

    free(output);
    return list;
}

void package_list_free(PackageList *list) {
    if (!list) return;

    for (int i = 0; i < list->count; i++) {
        free(list->packages[i].name);
        free(list->packages[i].version);
        free(list->packages[i].description);
        free(list->packages[i].repository);
    }

    free(list->packages);
    free(list);
}