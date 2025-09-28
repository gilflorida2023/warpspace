#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Structure to hold window information
struct WindowInfo {
    Window id;
    int desktop;
    char *wm_class;
    char *title;
};

// Function to get the number of desktops
int get_num_desktops(Display *display) {
    Atom net_number_of_desktops = XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False);
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop;

    if (XGetWindowProperty(display, DefaultRootWindow(display), net_number_of_desktops,
                           0, 1, False, XA_CARDINAL, &actual_type, &actual_format,
                           &nitems, &bytes_after, &prop) == Success && prop) {
        int num_desktops = *(unsigned long *)prop;
        XFree(prop);
        fprintf(stderr, "Detected %d desktops\n", num_desktops);
        return num_desktops;
    }
    fprintf(stderr, "Failed to get number of desktops\n");
    return 0;
}

// Function to get the current desktop of the display
int get_current_display_desktop(Display *display) {
    Atom net_current_desktop = XInternAtom(display, "_NET_CURRENT_DESKTOP", False);
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop;

    XSync(display, False);
    if (XGetWindowProperty(display, DefaultRootWindow(display), net_current_desktop,
                           0, 1, False, XA_CARDINAL, &actual_type, &actual_format,
                           &nitems, &bytes_after, &prop) == Success && prop && nitems > 0) {
        int current_desktop = *(unsigned long *)prop;
        XFree(prop);
        fprintf(stderr, "Current display desktop is %d\n", current_desktop);
        return current_desktop;
    }
    fprintf(stderr, "Failed to get current display desktop\n");
    return -1;
}

// Function to parse wmctrl -l -x output and get window list
int get_window_list(struct WindowInfo **windows, int *num_windows) {
    FILE *fp = popen("wmctrl -l -x", "r");
    if (!fp) {
        fprintf(stderr, "Failed to run wmctrl -l -x\n");
        return 0;
    }

    struct WindowInfo *temp_windows = NULL;
    int count = 0;
    char line[512];

    while (fgets(line, sizeof(line), fp)) {
        // Parse line: window_id desktop wm_class title
        unsigned long window_id;
        int desktop;
        char wm_class[256], title[256];
        if (sscanf(line, "0x%lx %d %255s %255[^\n]", &window_id, &desktop, wm_class, title) == 4) {
            temp_windows = realloc(temp_windows, (count + 1) * sizeof(struct WindowInfo));
            temp_windows[count].id = (Window)window_id;
            temp_windows[count].desktop = desktop;
            temp_windows[count].wm_class = strdup(wm_class);
            temp_windows[count].title = strdup(title);
            count++;
            fprintf(stderr, "Found window 0x%lx on desktop %d: %s (%s)\n",
                    window_id, desktop, wm_class, title);
        }
    }
    pclose(fp);

    *windows = temp_windows;
    *num_windows = count;
    return count > 0;
}

// Function to free window list
void free_window_list(struct WindowInfo *windows, int num_windows) {
    for (int i = 0; i < num_windows; i++) {
        free(windows[i].wm_class);
        free(windows[i].title);
    }
    free(windows);
}

// Function to select a window using curses
int select_window_curses(struct WindowInfo *windows, int num_windows) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    int highlight = 0;
    int choice = -1;

    while (1) {
        clear();
        mvprintw(0, 0, "Select an application (use arrow keys, Enter to confirm, q to quit):");
        for (int i = 0; i < num_windows; i++) {
            if (i == highlight)
                attron(A_REVERSE);
            char display_text[512];
            snprintf(display_text, sizeof(display_text), "%s (%s, Workspace %d)",
                     windows[i].title, windows[i].wm_class, windows[i].desktop + 1);
            mvprintw(i + 2, 2, "%.80s", display_text); // Truncate for display
            if (i == highlight)
                attroff(A_REVERSE);
        }
        refresh();

        int ch = getch();
        switch (ch) {
            case KEY_UP:
                if (highlight > 0) highlight--;
                break;
            case KEY_DOWN:
                if (highlight < num_windows - 1) highlight++;
                break;
            case 10: // Enter
                choice = highlight;
                break;
            case 'q':
                choice = -1;
                break;
        }
        if (choice != -1)
            break;
    }

    endwin();
    return choice;
}

// Function to select a desktop using curses
int select_desktop_curses(int num_desktops, int current_desktop) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    int highlight = (current_desktop >= 0 && current_desktop < num_desktops) ? current_desktop : 0;
    int choice = -1;

    while (1) {
        clear();
        mvprintw(0, 0, "Select a workspace (use arrow keys, Enter to confirm, q to quit):");
        for (int i = 0; i < num_desktops; i++) {
            if (i == highlight)
                attron(A_REVERSE);
            mvprintw(i + 2, 2, "Workspace %d", i + 1);
            if (i == highlight)
                attroff(A_REVERSE);
        }
        refresh();

        int ch = getch();
        switch (ch) {
            case KEY_UP:
                if (highlight > 0) highlight--;
                break;
            case KEY_DOWN:
                if (highlight < num_desktops - 1) highlight++;
                break;
            case 10: // Enter
                choice = highlight;
                break;
            case 'q':
                choice = -1;
                break;
        }
        if (choice != -1)
            break;
    }

    endwin();
    return choice;
}

// Function to move a window to a specific desktop
int move_window_to_desktop(Display *display, Window window, int desktop) {
    char command[256];
    int result;

    // Activate the window to ensure focus
    snprintf(command, sizeof(command), "wmctrl -i -a 0x%lx", (unsigned long)window);
    result = system(command);
    if (result == 0) {
        fprintf(stderr, "Activated window 0x%lx\n", window);
    } else {
        fprintf(stderr, "Failed to activate window 0x%lx (result=%d)\n", window, result);
    }

    // Remove sticky property
    Atom net_wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom sticky = XInternAtom(display, "_NET_WM_STATE_STICKY", False);
    XClientMessageEvent ev;
    ev.type = ClientMessage;
    ev.window = window;
    ev.message_type = net_wm_state;
    ev.format = 32;
    ev.data.l[0] = 0; // Remove sticky
    ev.data.l[1] = sticky;
    XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask | SubstructureRedirectMask, (XEvent *)&ev);
    XSync(display, False);

    // Try moving with window ID
    fprintf(stderr, "Attempting to move window 0x%lx to desktop %d\n", window, desktop);
    for (int i = 0; i < 3; i++) {
        snprintf(command, sizeof(command), "wmctrl -i -r 0x%lx -t %d", (unsigned long)window, desktop);
        result = system(command);
        if (result == 0) {
            fprintf(stderr, "Moved window 0x%lx to desktop %d (attempt %d)\n", window, desktop, i + 1);
            // Verify the move
            snprintf(command, sizeof(command), "wmctrl -l -x | grep 0x%08lx | awk '{print $2}'", (unsigned long)window);
            FILE *fp = popen(command, "r");
            if (fp) {
                char desktop_str[16];
                if (fgets(desktop_str, sizeof(desktop_str), fp)) {
                    int current_desktop = atoi(desktop_str);
                    pclose(fp);
                    if (current_desktop == desktop) {
                        fprintf(stderr, "Verification: Window 0x%lx is on desktop %d\n", window, current_desktop);
                        return 1;
                    }
                    fprintf(stderr, "Verification failed: Window 0x%lx is on desktop %d, not %d\n",
                            window, current_desktop, desktop);
                } else {
                    fprintf(stderr, "wmctrl failed to verify desktop for window 0x%lx\n", window);
                }
                pclose(fp);
            }
            return 0;
        }
        fprintf(stderr, "Failed to move window 0x%lx to desktop %d (result=%d, attempt %d)\n",
                window, desktop, result, i + 1);
        sleep(1);
    }

    fprintf(stderr, "Failed to move window 0x%lx to desktop %d after all attempts\n", window, desktop);
    return 0;
}

int main() {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    // Get number of desktops
    int num_desktops = get_num_desktops(display);
    if (num_desktops <= 0) {
        fprintf(stderr, "Could not determine number of desktops. Is your window manager EWMH-compliant?\n");
        XCloseDisplay(display);
        return 1;
    }

    // Get window list
    struct WindowInfo *windows = NULL;
    int num_windows = 0;
    if (!get_window_list(&windows, &num_windows) || num_windows == 0) {
        fprintf(stderr, "No windows found or failed to get window list\n");
        XCloseDisplay(display);
        return 1;
    }

    // Select a window
    int selected_window_idx = select_window_curses(windows, num_windows);
    if (selected_window_idx < 0) {
        fprintf(stderr, "No window selected or selection canceled\n");
        free_window_list(windows, num_windows);
        XCloseDisplay(display);
        return 1;
    }

    Window selected_window = windows[selected_window_idx].id;
    int current_desktop = windows[selected_window_idx].desktop;

    // Select destination desktop
    int new_desktop = select_desktop_curses(num_desktops, current_desktop);
    if (new_desktop < 0) {
        fprintf(stderr, "No desktop selected or selection canceled\n");
        free_window_list(windows, num_windows);
        XCloseDisplay(display);
        return 1;
    }

    // Move the selected window
    move_window_to_desktop(display, selected_window, new_desktop);

    // Clean up
    free_window_list(windows, num_windows);
    XCloseDisplay(display);
    return 0;
}
