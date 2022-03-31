# Linux-Kiosk
This program is designed to run a program in a loop, you can specify an argument to the program and it will rerun the program even after
it's closed. This is a POC.

For example
./kiosk /usr/bin/firefox --kiosk (always use the absolute path for the program)

This will close all current instances of firefox and open firefox in kiosk mode.
If you try to close it with ALT-F4 for example, it will reopen again.

The way to stop the kiosk problem is to send a kill signal to kiosk. For example: 'killall kiosk'
or if run from a terminal a CTRL-C (SIGINT) could be sent.

How to get the program and use:

git clone git@github.com:DanRuckz/Linux-Kiosk.git --- 
g++ kiosk.cpp -o kiosk --- 
chmod +x kiosk (if needed) --- 
./kiosk <absolute_path_for_target_program> <args>
