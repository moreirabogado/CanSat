import time
import picamera
from datetime import datetime
import RPi.GPIO as gpio

senal = 24
flag = 0
gpio.setmode(gpio.BOARD)
gpio.setup(senal, gpio.IN)

time.sleep(120)
while True:
    print(gpio.input(senal))
    time.sleep(0.5)
    if gpio.input(senal):
        with picamera.PiCamera() as picam:
            current_date = datetime.now().strftime('%H:%M:%S')
            picam.start_preview()
            picam.start_recording("/home/pi/Desktop/" + current_date + ".h264")
            time.sleep(10)
            picam.stop_recording()
            picam.stop_preview()
            picam.close()
            flag = 1
    else:
      if flag:
          break