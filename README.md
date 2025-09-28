# Warpspace

**Warpspace** is a lightweight command-line tool for Linux systems that allows users to move application windows between virtual desktops (workspaces) using a curses-based menu interface. It leverages `wmctrl` to list and manage top-level windows, making it compatible with EWMH-compliant window managers (e.g., Openbox, i3). The tool is designed to be generic, handling applications like LibreOffice, Firefox, or terminals without application-specific logic.

## What It Does

Warpspace enables users to:
1. **List Applications by Workspace**: Displays all top-level windows (applications) and their current workspaces using `wmctrl -l -x`.
2. **Select an Application**: Presents a curses-based menu showing each window’s title, `WM_CLASS`, and current workspace (e.g., “LibreOffice Calc (libreoffice.libreoffice-calc, Workspace 1)”).
3. **Choose a Destination Workspace**: Allows users to select a target workspace using a curses-based menu.
4. **Move the Application**: Moves the selected window to the chosen workspace using `wmctrl`, ensuring the current desktop doesn’t switch and avoiding sub-window issues (e.g., LibreOffice toolbars).

This approach avoids the complexities of sub-window handling by focusing on top-level windows, making it reliable for complex applications like LibreOffice.

## Features
- **Menu-Based Interface**: Uses a text-based curses menu for selecting applications and workspaces, navigable with arrow keys and Enter.
- **Generic Design**: Relies on `wmctrl` and X11 properties (`_NET_WM_STATE`, `_NET_WM_WINDOW_TYPE`) without hardcoding for specific applications.
- **Robust Window Movement**: Activates windows before moving, removes sticky properties, and verifies moves to ensure reliability.
- **Error Handling**: Retries failed moves, logs detailed errors to `stderr`, and avoids unintended workspace switching.

## Requirements
- Linux system with an EWMH-compliant window manager (e.g., Openbox, i3).
- `libX11` and `ncurses` development libraries for compilation.
- `wmctrl` installed for window management (`sudo apt install wmctrl` on Debian/Ubuntu or equivalent).

## Installation

1. **Clone the Repository** (if hosted on GitHub):
   ```bash
   git clone https://github.com/gilflorida2023/warpspace.git
   cd warpspace

clone <repository-url>
	cd warpspace
### Build:
	make clean && make

### Install (user or root): 
#### Installs to /usr/local/bin/warpspace or ~/projects/bin/warpspace depending on who initiated the install. 
	make install

### Add to PATH (if user install):
	echo 'export PATH=$PATH:~/projects/bin' >> ~/.bashrc
	source ~/.bashrc

## Usage:
### Run:
	warpspace

### Click a window (right-click to cancel).
### Use arrow keys to select a workspace in the ncurses menu, press Enter, or q to quit.
### WarpSpace moves the window:
#### Uses wmctrl -i -r <window_id> -t <desktop> for listed windows.
#### Falls back to wmctrl -r :ACTIVE: -t <desktop> for unlisted windows.
### Check the target workspace (e.g., Ctrl+Alt+Left/Right).
### Terminal shows logs like:
	Detected 4 desktops
	Selected window: 0x03600007
	Moved window 0x03600007 to desktop 0 using wmctrl ID
	Verification: Window 0x03600007 is on desktop 0

# Troubleshooting:
### Check window listing:
	wmctrl -l -x | grep <window_id>

### Test movement:
	wmctrl -i -r <window_id> -t 0
	wmctrl -r :ACTIVE: -t 0
### View properties:
	xprop -id <window_id>
### Check Mutter:
	dpkg -l | grep muffin
## License:
### MIT License
