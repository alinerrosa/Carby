# from guizero import App, Box, Text, TextBox, PushButton, ButtonGroup, MenuBar, info, yesno, warn
from time import time
import RPi.GPIO as GPIO
import busio
import digitalio
import board
import adafruit_mcp3xxx.mcp3008 as MCP
from adafruit_mcp3xxx.analog_in import AnalogIn

# Create the SPI bus
# spi = busio.SPI(clock=board.SCLK, MISO=board.MISO, MOSI=board.MOSI)
spi = busio.SPI(board.SCLK, board.MOSI, board.MISO)

# Create the cs (chip select)
cs = digitalio.DigitalInOut(board.D5)

# Creat the mcp object
mcp = MCP.MCP3008(spi, cs)

# Create analog inputs connected to the input pins on the MCP3008
channel_0 = AnalogIn(mcp, MCP.P0)

# Define application commands and features:


def _range(x, in_min, in_max, out_min, out_max):
    return int((x - in_min)*(out_max - out_min) / (in_max - in_min) + out_min)

# Test your module, then defin the value range - in this case between 0 and 60000


def evaluateSensorValue():
    sensorValue = range(channel_0.value, 0, 60000, 0, 1023)
    sensor_value.value = sensorValue
    # threshold
    if(sensorValue > 300):
        print("Status: DANGER")
        print("CO2: %g ppm", sensorValue)
    else:
        print("Status: OK")
        print("CO2: %g ppm", sensorValue)


# Update the sensor value
sensor_value.repeat(1000, evaluateSensorValue)
