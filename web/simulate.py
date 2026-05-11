import paho.mqtt.client as mqtt
import json, time, math, random

BROKER = "broker.hivemq.com"
PORT   = 1883
TOPIC  = "iot/home/sensors"

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="simulator-001")

def on_connect(c, userdata, flags, reason_code, props):
    print(f"Connected to HiveMQ (rc={reason_code})")
    c.subscribe("iot/home/control/#")

def on_message(c, userdata, msg):
    print(f"  [control] {msg.topic} → {msg.payload.decode()}")

client.on_connect = on_connect
client.on_message = on_message

print("Connecting to broker.hivemq.com ...")
client.connect(BROKER, PORT, keepalive=30)
client.loop_start()

tick = 0
try:
    while True:
        tick += 1
        temp     = round(24 + 8 * math.sin(tick * 0.3) + random.uniform(-0.5, 0.5), 1)
        distance = random.choice([*([random.randint(15, 80)] * 8), random.randint(3, 9)])
        ldr      = random.randint(200, 900)
        payload  = json.dumps({
            "temp":      temp,
            "distance":  distance,
            "ldr":       ldr,
            "white_led": 1 if ldr > 500 else 0,
            "alarm_led": 1 if temp > 30 else 0,
        })
        client.publish(TOPIC, payload)
        print(f"[{tick:>3}] {payload}")
        time.sleep(4)
except KeyboardInterrupt:
    print("\nStopped.")
finally:
    client.loop_stop()
    client.disconnect()
