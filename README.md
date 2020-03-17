# CS179F-Project-in-OS
This is my senior design project with Douglas Tran and Jonathan Le. 

## Description
In this project we decided to extend the classic xv6 from MIT (https://pdos.csail.mit.edu/6.828/) by adding thread support for the xv6, that is, kernel-level threads, user level threads, and scheduler activations.

## Prerequisites
xv6 can already run on your local machine. 

## How to run? 
Once cloned this github link, type the following commands exactly on terminal:
1) "cd xv6" // move into the xv6 directory
2 "make clean && make qemu-nox" //clean the object and excess files, then run qemu to get into the xv6 directory
3) If error occurs such as a make error for "./sign.pl",
simply type "chmod 777 ./sign.pl" then repeat step 2 again. If a make error still occurs similar to this, try "chmod 777 *"   If an error still occurs, please let us know. 
This simple chmod command on terminal allows the user to read and execute the file.
4) Refer to the "Tests" section of the README to run the kernel and user level threads.

## Tests
Once compiled and inside the xv6, type "ls". Doing so ensures that a list of commands and functions from the Makefile show and you can choose which one to run. 

To test the kernel level threads, use "kernelthreads", and to test user level threads, use "uthreads".

## Credits
We first tried to implement everything ourselves but got stuck pretty often and did not know where to go, even with the professor and TA's help. We only had a bit under a quarter to implement everything. After a quick google search, we found a couple resources to get inspiration from for our project.

For kernel level threads, we used this class's assignment description (https://www.cs.bgu.ac.il/~os192/wiki.files/os192_assignment2.pdf), and for user-level threads we used this class's assignment description .
