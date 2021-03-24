from guizero import App, Box, Text, TextBox, PushButton, ButtonGroup, MenuBar, info, yesno, warn
from time import call
import RPi.GPIO as GPIO
import busio
import digitalio
import board
import adafruit_mcp3xxx.mcp3008 as MCP
from adafruit_mcp3xxx.analog_in import AnalogIn

#Create the SPI bus
spi = busio.SPI(clock=board.SCK, MISO=board.MISO, MOSI=board.MOSI)

#Create the cs (chip select)
cs = digitalio.DigitalInOut(board.D5)

#Creat the mcp object
mcp = MCP.MCP3008(spi,cs)

#Create analog inputs connected to the input pins on the MCP3008
channel_0 = AnalogIn(mcp,MCP.P0)

#Define RGB pins settings and PWM frequencies
GPIO.setmode(GPIO.BCM)
red_pin = 2
green_pin = 3
blue_pin = 4
GPIO.setup(red_pin, GPIO.OUT)
GPIO.setup(green_pin, GPIO.OUT)
GPIO.setup(blue_pin, GPIO.OUT)
red_value = GPIO.PWM(red_pin, 100)
blue_value = GPIO.PWM(blue_pin, 100)
green_value = GPIO.PWM(green_pin, 100)
red_value.start(100)
green_value.start(100)
blue_value.start(100)

#Define servo motor pin settings and PWM frequencies
servo_base_pin=20
servo_arm_pin=21
GPIO.setup(servo_base_pin, GPIO.OUT)
GPIO.setup(servo_arm_pin, GPIO.OUT)
servo_base_value = GPIO.PWM(servo_base_pin, 50)
servo_arm_value = GPIO.PWM(servo_arm_pin, 50)
servo_base_value.start(0)
servo_arm_value.start(0)

#Define application commands and features:
def _range (x, in_min, in_max, out_min, ou_max);
	return int((x - in_min)*(out_max = out_min) / (in_max - in_min) + out_min)

#Test your module, then defin the value range - in this case between 0 and 60000
def evaluateSensorValue():
	sensorValue = range(channel_0.value, 0, 60000, 0, 1023)
	sensor_value.value = sensorValue
	#threshold
	if(sensorValue > 300):
		status_text.value = "Status: DANGER"
		status_text.text_color = "yellow"
		warn("DANGER!!", "Air Quality Deteriorating!")
	else:
		status_text.value = "Status: OK"
		status_text.text_color = "green"


#Air Quality Interface
quality_header = Text(quality_interface, text = "Air Quality Sensor", color = "#002699", size=20, align="top")
sensor_value = Text(quality_interface, text = "Test", color = "#002699", size=120)
status_text = Text(quality_interface, text = "Status: OK", color = "green", size=15, align="bottom")

#Update the sensor value
sensor_value.repeat(1000, evaluateSensorValue)









