# CS179F-Project-in-OS
This is my senior design project with Jonathan Le and Douglas Tran. 

## Description
In this project we decided to extend the classic xv6 from MIT (https://pdos.csail.mit.edu/6.828/) by adding thread support for the xv6, that is, kernel-level threads, user level threads, and scheduler activations.

## Prerequisites
-xv6 can already run on your local machine.
-a working copy of virtualbox and vagrant

## How to run?
Initially, we attempted using Travis CI to run a simple demo of this, however the .travis.yml file would only let us initiate xv6 but not run any commands through its shell. We followed through with running it either on the sledge server, or more efficiently, a VM (instructions from CS153 at https://www.cs.ucr.edu/~nael/cs153/labs.html).
1) From your local terminal, install virtualbox and vagrant with `sudo apt-get install virtualbox` and `sudo apt-get install vagrant`.
2) Install a copy of ubuntu with `vagrant box add ubuntu/xenial64`.
3) Move to any existing working directory in your terminal.
4) Create your Vagrantfile with `vagrant init ubuntu/xenial64`. A Vagrantfile will be created in your current path.
5) Run `vagrant up` and then `vagrant ssh` to launch the VM.
6) Install a required toolchain in the VM with `sudo apt-get update` and then `sudo apt-get install -y build-essential gdb git gcc-multilib`.
7) Install QEMU by running the following commands in this order(make sure you are on root directory ~): `git clone http://web.mit.edu/ccutler/www/qemu.git -b 6.828-2.3.0`, `sudo apt-get install -y libsdl1.2-dev libtool-bin libglib2.0-dev libz-dev libpixman-1-dev`, `cd qemu`, `./configure --disable-kvm --target-list="i386-softmmu x86_64-softmmu"`, `make`, `sudo make install`.
8) Clone this project's repo link into the VM at root ~.
9) Load local gdbinit once with `echo "add-auto-load-safe-path $HOME/xv6/.gdbinit" > ~/.gdbinit`.
10) cd into the repo directory.

Once cloned this github link, type the following commands exactly on terminal:
1) `cd xv6` // move into the xv6 directory
2) `make clean && make qemu-nox` //clean the object and excess files, then run qemu to get into the xv6 directory
3) If error occurs such as a make error for `./sign.pl`,
simply type `chmod 777 ./sign.pl` then repeat step 2 again. If a make error still occurs similar to this, try `chmod 777 *`. If an error still occurs, please let us know. 
This simple chmod command on terminal allows the user to read and execute the file.
4) Refer to the "Tests" section of the README to run the kernel and user level threads.

## Tests
Once compiled and inside the xv6, type `ls`. Doing so ensures that a list of commands and functions from the Makefile show and you can choose which one to run. 

To test the kernel level threads, use `kernelthreads`, and to test user level threads, use `uthread`.

## Credits
We first tried to implement everything ourselves but got stuck pretty often and did not know where to go, even with the professor and TA's help. We only had a bit under a quarter to implement everything. After a quick google search, we found a couple resources to get inspiration from for our project.

For kernel level threads, we used this class's assignment description (https://www.cs.bgu.ac.il/~os192/wiki.files/os192_assignment2.pdf).

For user level threads, we worked off of MIT's provided uthread.c and uthread_switch.S files, provided as one of their class assignments (https://pdos.csail.mit.edu/6.828/2018/homework/xv6-uthread.html). 
Further modifications for synchronization required the use of a header file in which the uthread.c elements were put into there instead.
