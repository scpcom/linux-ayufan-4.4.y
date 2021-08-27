How to install and use Pixiu CPA (Code Performance Analyzer).
- This documentation refer to Armada XP and Armada 370.
- The tool can be download from Marvell extranet ( Under My Products > Cellular & Handheld Solutions > Development Tools > General > Code Performance Analyzer)

Environment Setup:
Host:
- Download the tool, and extract the files to the PC.
- Make sure that the host has a network connection to the board.
- Copy file named <kernel dir>\tools\pixiu\Analyze.zip and unzip it, in the PC under CPA/ directory.

Host:
- Edit build-driver.sh, and set the compiler path (in CROSS_COMPILE_DEF) and the kernel linux path (in PXALINUX_DEF)
- Rub ./build-driver (set the appropriate parameters when prompted)

Board:
- Copy file named "mppd.linux.tgz" under <kernel dir>\tools\pixiu\mppd.linux.tgz to your target directory (e.g. /home/pixiu)
- Untar the file # tar xvf mppd.linux.tgz
- Run install.sh using the command # ./install.sh
- If the tool can't load the kernel driver on the board please follow the below instructions:
- Copy the files (you compiled in previous section) mpdc_cm.ko, mpdc_css.ko, mpdc_hs.ko, mpdc_tp.ko to your target root file-system \home\pixiu

Collect the Performance:
Board:
In the Pixiu directory (/home/pixiu) run mpdc_srv, using command #./mpdc_srv
If the launch is successful you will get the folloing message:
[root@linux/]#mpdc_svr
...
[MPDC] Server is listening to port 9000

 If the launch is fails, you have to run the following message:
#./load_mpdc.sh
#./mpdc_srv

PC:
Run the Pixiu tool, under CPA/Analyzer/cpa.exe
Make new project.
Click the "Play" icon to start profiling.
A connection to target device dialog box is displayed, chooce TCP/IP option and set the IP and Click Ok.
