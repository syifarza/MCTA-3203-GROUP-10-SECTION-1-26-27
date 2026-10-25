import serial
import time
import math

# --- CONFIGURATION ---
SERIAL_PORT = "COM6"
BAUD_RATE = 9600
AUTHORIZED_ID = ("0013070376")

# ---------------------

def detect_circle(arduino):
    """
    Request MPU data from Arduino and detect circular motion
    """
    print("\n[!] Requesting samples from Arduino… Move in a circle NOW!")
    arduino.reset_input_buffer()
    arduino.write(b'R')  # trigger Arduino stream

    points = []
    start_time = time.time()
    timeout = 3.0

    while time.time() - start_time < timeout:
        raw = arduino.readline().decode(errors='ignore').strip()
        if not raw:
            continue

        parts = raw.split(',')
        if len(parts) >= 3:
            try:
                ax = int(parts[0])
                ay = int(parts[1])
                az = int(parts[2])
                points.append((ax, ay, az))
            except ValueError:
                continue

    print(f"Collected {len(points)} samples.")

    if len(points) < 8:
        print("Not enough motion data – try moving more continuously.")
        return False

    angles = [math.degrees(math.atan2(p[1], p[0])) for p in points]

    unwrapped = [angles[0]]
    for a in angles[1:]:
        prev = unwrapped[-1]
        diff = a - prev

        if diff > 180:
            diff -= 360
        elif diff < -180:
            diff += 360

        unwrapped.append(prev + diff)

    net_rotation = unwrapped[-1] - unwrapped[0]
    rotation_range = max(unwrapped) - min(unwrapped)

    print(f"Net rotation: {net_rotation:.1f}°, Range: {rotation_range:.1f}°")

    if abs(net_rotation) > 100 or rotation_range > 140:
        print("-> Circle gesture detected!")
        return True

    deltas = [unwrapped[i+1] - unwrapped[i] for i in range(len(unwrapped)-1)]
    large_moves = sum(1 for d in deltas if abs(d) > 10)

    if large_moves >= max(6, len(points)//4):
        print("-> Motion accepted as circle.")
        return True

    print("-> Motion insufficient.")
    return False


def main():
    arduino = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)

    print("System Ready. Waiting for RFID...")

    armed = False

    try:
        while True:
            card = input("\nTap card: ").strip()

            if card == AUTHORIZED_ID:
                armed = not armed

                if armed:
                    print("Card Accepted – Perform circular motion!")

                    if detect_circle(arduino):
                        print("UNLOCKED (LED Green)")
                        arduino.write(b'1')
                    else:
                        print("Wrong gesture!")
                        armed = False
                        arduino.write(b'0')

                else:
                    print("LOCKED (LED Red)")
                    arduino.write(b'0')

            else:
                print("WRONG CARD")
                arduino.write(b'X')

    except KeyboardInterrupt:
        print("\nExiting...")

    finally:
        arduino.write(b'0')
        arduino.close()


if __name__ == "__main__":
    main()