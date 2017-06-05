
import serial
import time

ser = serial.Serial()
ser.baudrate = 115200
ser.port = '/dev/ttyUSB0'
ser.open()

x_before = 70
radius_before = 0 
while True:
	with open("value.txt","r") as cf:
		commfromcam = cf.readline().strip()

	try:
                div1 = commfromcam.split("::",1)
                temp = div1[1].replace("[","");
                temp = temp.replace("]","");
                x_y = temp.split(",",1)
        	radius = int(div1[0].strip())
       		x = int(x_y[0].strip())
        except IndexError:
                pass

	if ((abs(x_before - x) > 30) or (abs(radius_before - radius) > 25 )): #I'm lost range fault
			ser.write('echo "m0" > /dev/fmkm\n')
			ser.write('echo "d0" > /dev/fmkm\n')
			ser.write('echo "f1" > /dev/fmkm\n')  # range fault
	else:
                if radius > 70:
                        ser.write('echo "m2" > /dev/fmkm\n')
                elif radius < 43:
                        ser.write('echo "m1" > /dev/fmkm\n')
                
		elif radius < (200 - 15): # go foward (m1)
			if x > (300 + 50): #foward left
				ser.write('echo "d1" > /dev/fmkm\n')
			elif x < (300 - 50): #foward right
				ser.write('echo "d2" > /dev/fmkm\n')
			else:   #foward
				ser.write('echo "m0" > /dev/fmkm\n')
		elif radius > (200 + 15):	# go back
			if x > (300 + 50):			#back left
				ser.write('echo "d3" > /dev/fmkm\n')
			elif x < (300 - 50):		#back right
				ser.write('echo "d4" > /dev/fmkm\n')
			else:	#back
				ser.write('echo "m0" > /dev/fmkm\n')
		else:
			ser.write('echo "m0" > /dev/fmkm\n')
			ser.write('echo "d0" > /dev/fmkm\n')
	x_before = x
        radius_before = radius 

	time.sleep(0.4)




# ser=serial.Serial('/dev/ttyUSB0')
# print(ser.name)
# ser.write('echo "m1" > /dev/fmkm')
# ser.write('echo "m2" > /dev/fmkm')
# ser.write('echo "m2" > /dev/fmkm\n')
# ser.write('echo "m0" > /dev/fmkm\n')
