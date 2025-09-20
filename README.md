### WarpSpace


### WarpSpace is a Linux utility to move windows (active or inactive) between workspaces in Cinnamon (Mutter). Click a window, use an ncurses menu to pick a workspace, and it moves. It handles unlisted windows by falling back to the active window. Great for Linux Mint users managing workspaces.

### Prerequisites:
#### GCC
	sudo apt install build-essential

#### libx11-dev
	sudo apt install libx11-dev
#### libncurses-dev
	sudo apt install libncurses-dev
#### wmctrl
	sudo apt install wmctrl

#### xprop (optional for debugging)
	sudo apt install x11-utils
#### Verify with:
	gcc --version
	wmctrl --version
	xprop --version

#### Note: Best for Cinnamon/Mutter; other EWMH-compliant window managers may vary.

## Installation:
### Clone or download to ~/projects/clang/warpspace
	git clone <repository-url>
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
