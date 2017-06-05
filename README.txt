# Follow Me car #   EC535 project 

### Instructions to run the code 

1. Kernel module issue

1)  Find the kl folder which contains kernel source code and its makefile in terminal of Raspberry Pi system, type make in terminal to get fmkm.ko file.

2)  Type command "mknod /dev/fmkm c 61 0" and "insmod fmkm.ko" to insert the module 

2. Object detection issue

1) Find the Object_Detection folder which contains object detection source code in terminal of Raspberry Pi system;

2) Type command "sudo modprobe bcm2835-v4l2" to get access to the camera;

3) Type command "g++ $(pkg-config --libs --cflags opencv) -o source Source.cpp" to compile the source code and get "source" executable file;

4) Type command "./source" to run the file.

3. Pyserial issue

1) Find the SCFD folder which contains pyserial code in terminal of Raspberry Pi system;

2) Type command "python gumstix_comm.py" to run the python file. 

4. turn on the switch of the car and test its intelligence ! 


