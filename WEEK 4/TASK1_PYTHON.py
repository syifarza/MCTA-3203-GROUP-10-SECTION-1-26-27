import serial
import matplotlib.pyplot as plt
import numpy as np
from collections import deque
import time

# =====================================
# 1. SERIAL CONNECTION
# =====================================
PORT = "COM6"
BAUD_RATE = 9600

ser = serial.Serial(PORT, BAUD_RATE, timeout=1)
time.sleep(2)
ser.reset_input_buffer()

print("Arduino connected!")
print("Reading sensor data...")
print("-" * 65)

# =====================================
# 2. DATA STORAGE
# =====================================
MAX_POINTS = 200
WINDOW_SIZE = 60
DETECTION_COOLDOWN = 3

x_vals = deque(maxlen=MAX_POINTS)
y_vals = deque(maxlen=MAX_POINTS)

gesture_x = deque(maxlen=WINDOW_SIZE)
gesture_y = deque(maxlen=WINDOW_SIZE)

last_detection_time = 0
circle_detected = False

# =====================================
# 3. GRAPH SETUP
# =====================================
plt.ion()
fig, ax = plt.subplots(figsize=(9, 7))

line, = ax.plot(
    [], [], "b-", linewidth=1.5,
    label="Motion path"
)

point, = ax.plot(
    [], [], "ro", markersize=6,
    label="Latest position"
)

ax.set_title("Real-Time X-Y Motion")
ax.set_xlabel("Acceleration X (m/s²)")
ax.set_ylabel("Acceleration Y (m/s²)")
ax.grid(True)
ax.legend()
ax.set_aspect("equal", adjustable="datalim")

# Status stays at the TOP LEFT of the graph
status = ax.text(
    0.02, 0.98,
    "TRACKING MOTION...",
    transform=ax.transAxes,
    va="top",
    ha="left",
    fontsize=11,
    fontweight="bold",
    color="blue",
    bbox=dict(
        facecolor="white",
        edgecolor="gray",
        alpha=0.9
    )
)

# =====================================
# 4. CIRCULAR MOTION DETECTION
# =====================================
def detect_circle(xs, ys):
    if len(xs) < WINDOW_SIZE:
        return False

    x = np.asarray(xs, dtype=float)
    y = np.asarray(ys, dtype=float)

    cx = np.mean(x)
    cy = np.mean(y)

    dx = x - cx
    dy = y - cy

    radius = np.sqrt(dx**2 + dy**2)
    mean_radius = np.mean(radius)

    if mean_radius < 1e-6:
        return False

    radius_variation = np.std(radius) / mean_radius

    angles = np.unwrap(np.arctan2(dy, dx))
    angle_change = angles[-1] - angles[0]

    start_distance = np.hypot(
        x[-1] - x[0],
        y[-1] - y[0]
    )

    closure_ratio = start_distance / mean_radius

    return (
        abs(angle_change) >= 1.6 * np.pi
        and radius_variation < 0.50
        and closure_ratio < 0.60
    )

# =====================================
# 5. MAIN LOOP
# =====================================
try:
    while plt.fignum_exists(fig.number):

        raw = ser.readline().decode(
            "utf-8", errors="ignore"
        ).strip()

        if not raw:
            plt.pause(0.01)
            continue

        values = raw.split(",")

        if len(values) != 6:
            continue

        try:
            ax_val, ay_val, az_val, gx, gy, gz = [
                float(v) for v in values
            ]
        except ValueError:
            continue

        readings = [ax_val, ay_val, az_val, gx, gy, gz]

        if not np.all(np.isfinite(readings)):
            continue

        # ---------------------------------
        # PRINT ALL READINGS IN TERMINAL
        # ---------------------------------
        print(
            f"AX: {ax_val:7.3f} | "
            f"AY: {ay_val:7.3f} | "
            f"AZ: {az_val:7.3f} | "
            f"GX: {gx:7.3f} | "
            f"GY: {gy:7.3f} | "
            f"GZ: {gz:7.3f}"
        )

        # ---------------------------------
        # UPDATE X-Y GRAPH
        # ---------------------------------
        x_vals.append(ax_val)
        y_vals.append(ay_val)

        gesture_x.append(ax_val)
        gesture_y.append(ay_val)

        line.set_data(list(x_vals), list(y_vals))
        point.set_data([ax_val], [ay_val])

        ax.relim()
        ax.autoscale_view()
        ax.set_aspect("equal", adjustable="datalim")

        # ---------------------------------
        # CIRCULAR MOTION STATUS
        # ---------------------------------
        detected = detect_circle(gesture_x, gesture_y)
        current_time = time.time()

        if (
            detected
            and current_time - last_detection_time
            >= DETECTION_COOLDOWN
        ):
            circle_detected = True
            last_detection_time = current_time

        elif not detected:
            circle_detected = False

        if circle_detected:
            status.set_text("CIRCULAR MOTION DETECTED!")
            status.set_color("green")
        else:
            status.set_text("TRACKING MOTION...")
            status.set_color("blue")

        # Update graph only; detection messages stay on graph
        fig.canvas.draw_idle()
        plt.pause(0.001)

except KeyboardInterrupt:
    print("\nProgram stopped by user.")

except serial.SerialException as e:
    print("\nArduino connection error:", e)

finally:
    if ser is not None and ser.is_open:
        ser.close()

    plt.close("all")
    print("Serial connection closed.")