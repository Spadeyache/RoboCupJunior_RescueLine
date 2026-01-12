from media.sensor import Sensor
import time

print("Initializing sensor...")
sensor = Sensor()

try:
    sensor.reset()
    sensor.set_framesize(Sensor.VGA)  # Try QVGA or lower
    sensor.set_pixformat(Sensor.RGB565)
    time.sleep(1.0)

    print("Running sensor...")
    sensor.run()
    print("Sensor initialized.")

    print("Taking snapshot...")
    img = sensor.snapshot()
    print("Snapshot taken successfully.")

    img.save("/sdcard/test_image.jpg", quality=90)
    print("✅ Image saved to /sdcard/test_image.jpg")

except AttributeError as attr_err:
    print("❌ Attribute Error:", attr_err)

except Exception as e:
    print("❌ Error occurred:", e)

print("Done.")
