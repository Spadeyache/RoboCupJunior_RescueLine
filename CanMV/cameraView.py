from media.sensor import Sensor
from media.display import Display
from media.media import MediaManager
import time

# Initialize sensor
sensor = Sensor()
sensor.reset()
sensor.set_framesize(Sensor.QVGA)
sensor.set_pixformat(Sensor.RGB565)

# Initialize display
Display.init(Display.VIRT, width=320, height=240, to_ide=True)

# Initialize media manager
MediaManager.init()

sensor.run()

clock = time.clock()

while True:
    clock.tick()
    img = sensor.snapshot()
    Display.show_image(img)
    print("FPS:", clock.fps())
