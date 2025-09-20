#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

// Function to check if window is listed in wmctrl
int is_window_listed(Display *display, Window window) {
    char command[256];
    snprintf(command, sizeof(command), "wmctrl -l -x | grep 0x%08lx", (unsigned long)window);
    FILE *fp = popen(command, "r");
    if (fp) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), fp)) {
            pclose(fp);
            fprintf(stderr, "Window 0x%lx is listed in wmctrl: %s", window, buffer);
            return 1;
        }
        pclose(fp);
    }
    fprintf(stderr, "Window 0x%lx is not listed in wmctrl -l -x\n", window);
    return 0;
}

// Function to check if window is normal (movable)
int is_window_normal(Display *display, Window window) {
    Atom net_wm_window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop;

    XSync(display, False);
    if (XGetWindowProperty(display, window, net_wm_window_type,
                           0, 1024, False, XA_ATOM, &actual_type, &actual_format,
                           &nitems, &bytes_after, &prop) == Success && prop && nitems > 0) {
        Atom *types = (Atom *)prop;
        Atom normal = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
        for (unsigned long i = 0; i < nitems; i++) {
            if (types[i] == normal) {
                XFree(prop);
                fprintf(stderr, "Window 0x%lx is type NORMAL (movable)\n", window);
                return 1;
            }
        }
        XFree(prop);
    }
    fprintf(stderr, "Window 0x%lx is not type NORMAL or has no _NET_WM_WINDOW_TYPE\n", window);
    return 0;
}

// Function to check window properties
void check_window_properties(Display *display, Window window) {
    Atom net_wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom wm_class = XInternAtom(display, "WM_CLASS", False);
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop;

    // Check map state
    XWindowAttributes attr;
    if (XGetWindowAttributes(display, window, &attr)) {
        fprintf(stderr, "Window 0x%lx map state: %s\n", window,
                attr.map_state == IsViewable ? "Viewable" :
                attr.map_state == IsUnmapped ? "Unmapped" : "Unviewable");
    }

    // Check WM_CLASS
    XSync(display, False);
    if (XGetWindowProperty(display, window, wm_class,
                           0, 1024, False, XA_STRING, &actual_type, &actual_format,
                           &nitems, &bytes_after, &prop) == Success && prop && nitems > 0) {
        fprintf(stderr, "Window 0x%lx WM_CLASS: %s\n", window, prop);
        XFree(prop);
    } else {
        fprintf(stderr, "No WM_CLASS for window 0x%lx\n", window);
    }

    // Check _NET_WM_STATE
    if (XGetWindowProperty(display, window, net_wm_state,
                           0, 1024, False, XA_ATOM, &actual_type, &actual_format,
                           &nitems, &bytes_after, &prop) == Success && prop && nitems > 0) {
        Atom *states = (Atom *)prop;
        Atom sticky = XInternAtom(display, "_NET_WM_STATE_STICKY", False);
        for (unsigned long i = 0; i < nitems; i++) {
            if (states[i] == sticky) {
                fprintf(stderr, "Warning: Window 0x%lx is sticky (may resist moving)\n", window);
            }
        }
        XFree(prop);
    } else {
        fprintf(stderr, "No _NET_WM_STATE for window 0x%lx\n", window);
    }
}

// Function to get the current desktop of a window
int get_current_desktop(Display *display, Window window) {
    // Check if window is listed in wmctrl
    int listed = is_window_listed(display, window);
    if (listed) {
        char command[256];
        snprintf(command, sizeof(command), "wmctrl -l -x | grep 0x%08lx | awk '{print $2}'", (unsigned long)window);
        FILE *fp = popen(command, "r");
        if (fp) {
            char desktop_str[16];
            if (fgets(desktop_str, sizeof(desktop_str), fp)) {
                int desktop = atoi(desktop_str);
                fprintf(stderr, "wmctrl reports window 0x%lx on desktop %d\n", window, desktop);
                pclose(fp);
                return desktop;
            }
            pclose(fp);
            fprintf(stderr, "wmctrl failed to get desktop for window 0x%lx\n", window);
        }
    } else {
        fprintf(stderr, "Warning: Window 0x%lx may be unmanaged or a child window\n", window);
    }

    // Fallback to current display desktop
    int current_display_desktop = get_current_display_desktop(display);
    if (current_display_desktop >= 0) {
        fprintf(stderr, "Assuming window 0x%lx is on current display desktop %d\n", window, current_display_desktop);
        return current_display_desktop;
    }

    // Fallback to X11
    Atom net_wm_desktop = XInternAtom(display, "_NET_WM_DESKTOP", False);
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop;

    XSync(display, False);
    if (XGetWindowProperty(display, window, net_wm_desktop,
                           0, 1, False, XA_CARDINAL, &actual_type, &actual_format,
                           &nitems, &bytes_after, &prop) == Success && prop && nitems > 0) {
        int current_desktop = *(unsigned long *)prop;
        XFree(prop);
        fprintf(stderr, "X11 reports window 0x%lx on desktop %d\n", window, current_desktop);
        return current_desktop;
    }
    fprintf(stderr, "X11 failed to get current desktop for window 0x%lx (nitems=%lu)\n", window, nitems);
    return -1;
}

// Function to move a window to a specific desktop
void move_window_to_desktop(Display *display, Window window, int desktop) {
    // Check window properties
    check_window_properties(display, window);

    int listed = is_window_listed(display, window);
    int normal = is_window_normal(display, window);
    char command[256];
    int result;

    if (listed && normal) {
        // Try moving with window ID for listed, normal windows
        fprintf(stderr, "Attempting to move window 0x%lx to desktop %d using wmctrl ID\n", window, desktop);
        for (int i = 0; i < 3; i++) {
            snprintf(command, sizeof(command), "wmctrl -i -r 0x%lx -t %d", (unsigned long)window, desktop);
            result = system(command);
            if (result == 0) {
                fprintf(stderr, "Moved window 0x%lx to desktop %d using wmctrl ID (attempt %d)\n", window, desktop, i + 1);
                break;
            }
            fprintf(stderr, "Failed to move window 0x%lx to desktop %d using wmctrl ID (result=%d, attempt %d)\n",
                    window, desktop, result, i + 1);
            sleep(1);
        }
    } else {
        // Fall back to :ACTIVE: for unlisted or non-normal windows
        fprintf(stderr, "Window 0x%lx is unlisted or not normal, falling back to wmctrl :ACTIVE:\n", window);
        for (int i = 0; i < 3; i++) {
            snprintf(command, sizeof(command), "wmctrl -r :ACTIVE: -t %d", desktop);
            result = system(command);
            if (result == 0) {
                fprintf(stderr, "Moved active window to desktop %d using wmctrl :ACTIVE: (attempt %d)\n", desktop, i + 1);
                break;
            }
            fprintf(stderr, "Failed to move active window to desktop %d using wmctrl :ACTIVE: (result=%d, attempt %d)\n",
                    desktop, result, i + 1);
            sleep(1);
        }
    }

    // Verify desktop with wmctrl
    snprintf(command, sizeof(command), "wmctrl -l -x | grep 0x%08lx | awk '{print $2}'", (unsigned long)window);
    FILE *fp = popen(command, "r");
    if (fp) {
        char desktop_str[16];
        if (fgets(desktop_str, sizeof(desktop_str), fp)) {
            int current_desktop = atoi(desktop_str);
            if (current_desktop == desktop) {
                fprintf(stderr, "Verification: Window 0x%lx is on desktop %d\n", window, current_desktop);
            } else {
                fprintf(stderr, "Verification failed: Window 0x%lx is on desktop %d, not %d\n",
                        window, current_desktop, desktop);
            }
            pclose(fp);
            return;
        }
        pclose(fp);
        fprintf(stderr, "wmctrl failed to verify desktop for window 0x%lx\n", window);
    }

    // Fallback verification with X11
    Atom net_wm_desktop = XInternAtom(display, "_NET_WM_DESKTOP", False);
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop;

    XSync(display, False);
    if (XGetWindowProperty(display, window, net_wm_desktop,
                           0, 1, False, XA_CARDINAL, &actual_type, &actual_format,
                           &nitems, &bytes_after, &prop) == Success && prop && nitems > 0) {
        int current_desktop = *(unsigned long *)prop;
        XFree(prop);
        if (current_desktop == desktop) {
            fprintf(stderr, "X11 verification: Window 0x%lx is on desktop %d\n", window, current_desktop);
        } else {
            fprintf(stderr, "X11 verification failed: Window 0x%lx is on desktop %d, not %d\n",
                    window, current_desktop, desktop);
        }
    } else {
        fprintf(stderr, "X11 failed to verify desktop for window 0x%lx\n", window);
    }
}

// Function to display curses menu and get user selection
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

// Function to get the window under the cursor
Window get_window_under_cursor(Display *display) {
    Window root = DefaultRootWindow(display);
    Cursor cursor = XCreateFontCursor(display, XC_crosshair);

    if (XGrabPointer(display, root, False, ButtonPressMask, GrabModeAsync, GrabModeAsync,
                     root, cursor, CurrentTime) != GrabSuccess) {
        fprintf(stderr, "Failed to grab pointer\n");
        XFreeCursor(display, cursor);
        return None;
    }

    fprintf(stderr, "Click on a window to select it (right-click to cancel)...\n");

    XEvent event;
    Window selected_window = None;
    while (1) {
        XNextEvent(display, &event);
        if (event.type == ButtonPress) {
            if (event.xbutton.button == Button1) {
                Window root_return, child_return;
                int root_x, root_y, win_x, win_y;
                unsigned int mask;
                XQueryPointer(display, root, &root_return, &child_return,
                              &root_x, &root_y, &win_x, &win_y, &mask);
                selected_window = child_return ? child_return : root;
                fprintf(stderr, "Selected window: 0x%lx\n", selected_window);
            } else {
                fprintf(stderr, "Selection canceled\n");
            }
            break;
        }
    }

    XUngrabPointer(display, CurrentTime);
    XFreeCursor(display, cursor);
    XFlush(display);
    return selected_window;
}

int main() {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    int num_desktops = get_num_desktops(display);
    if (num_desktops <= 0) {
        fprintf(stderr, "Could not determine number of desktops. Is your window manager EWMH-compliant?\n");
        XCloseDisplay(display);
        return 1;
    }

    Window selected_window = get_window_under_cursor(display);
    if (selected_window == None) {
        fprintf(stderr, "No window selected or selection canceled\n");
        XCloseDisplay(display);
        return 1;
    }

    int current_desktop = get_current_desktop(display, selected_window);
    int new_desktop = select_desktop_curses(num_desktops, current_desktop);
    if (new_desktop < 0) {
        fprintf(stderr, "No desktop selected or selection canceled\n");
        XCloseDisplay(display);
        return 1;
    }

    move_window_to_desktop(display, selected_window, new_desktop);

    XCloseDisplay(display);
    return 0;
}
