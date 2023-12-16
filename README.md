# Peter's quick start

1. Need to hold bootsel and plug pico in
2. Then copy the flash_nuke.uf2 file onto the disk. This will eject the disk when copied
2. Then copy the Raspberry Pi Pico v1.21.0.uf2 file onto the disk. This will eject the disk when copied
3. Now run rshell -p /dev/cu.usbmodem21401 --buffer-size 512 (port may have changed run `rshell --list` to find it)
4. Type exit
4. run `ampy --port /dev/tty.usbmodem21401 put main.py`

# Tutorial

Thonny is the recommended way of working with a Pico and MicroPython as it has built-in support for running and uploading files. However, I wanted to use Visual Studio Code as my code editor and upload files via the command line instead. I found two CLI tools that I could use to achieve this: rshell and ampy.

rshell
rshell provides a shell that runs on the Pico. It can be downloaded using pip:

pip3 install rshell
To connect to the Pico:

rshell -p /dev/ttyXXXX --buffer-size 512
rshell -p /dev/tty.usbmodem213301 --buffer-size 512
Where ttyXXXXis the serial port that your Pico is connected to. You can find the serial port by running ls /dev/tty*
with the board disconnected, and then connected, and compare the results. If the output is hard to compare, redirect it to a text file (e.g. ls /dev/tty* >> connected.txt) and then use diff to compare the files. On my Mac, the Pico’s serial port is currently /dev/tty.usbmodem000000000001 but this might not be the same for you so make sure to check first!

The files for the Pico are stored in /pyboard. Once rshell has connected, you can see what files are on the Pico by running ls /pyboard . You can copy a local file to the Pico with cp main.py /pyboard. If you want the python file to run when the Pico is powered on, make sure the file is named main.py.

To exit the shell, type exit.

ampy
ampy is a CLI tool for manipulating files and running code on a MicroPython board, such as the Pico, over a serial connection. It’s the tool that I find myself using most and it can be also installed via pip:

ampy --port /dev/tty.usbmodem213301 ls

pip3 install adafruit-ampy
To list files on the board (see the discussion above on how to find out your serial port):

ampy --port /dev/ttyXXXX ls
To run a file on the board without uploading it:

ampy --port /dev/ttyXXXX run main.py
To upload a file:

ampy --port /dev/ttyXXXX put main.py
To remove a file:

ampy --port /dev/ttyXXXX rm main.py