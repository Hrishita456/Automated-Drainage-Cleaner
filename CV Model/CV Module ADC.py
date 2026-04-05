import cv2
import serial
import time
import numpy as np

# ==========================
# CONFIG
# ==========================
SERIAL_PORT = "COM5"         # Windows example
# SERIAL_PORT = "/dev/ttyUSB0"   # Linux example
# SERIAL_PORT = "/dev/cu.usbmodem1101"  # macOS example
BAUD_RATE = 9600

CAMERA_INDEX = 0
MIN_CONTOUR_AREA = 1200
TRIGGER_COOLDOWN = 6.0       # seconds between consecutive clean cycles
SHOW_DEBUG = True

# Trigger zone as a fraction of frame size
ROI_X1_FRAC = 0.30
ROI_Y1_FRAC = 0.40
ROI_X2_FRAC = 0.75
ROI_Y2_FRAC = 0.90

# ==========================
# SERIAL SETUP
# ==========================
def init_serial():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)
        print(f"[INFO] Connected to Arduino on {SERIAL_PORT}")
        return ser
    except Exception as e:
        print(f"[WARN] Could not open serial port: {e}")
        return None

def send_command(ser, command):
    if ser is None:
        print(f"[MOCK SEND] {command}")
        return
    try:
        ser.write((command + "\n").encode("utf-8"))
        print(f"[SEND] {command}")
    except Exception as e:
        print(f"[ERROR] Failed to send command: {e}")

# ==========================
# HELPERS
# ==========================
def point_in_rect(cx, cy, rect):
    x1, y1, x2, y2 = rect
    return x1 <= cx <= x2 and y1 <= cy <= y2

def get_roi(frame):
    h, w = frame.shape[:2]
    x1 = int(w * ROI_X1_FRAC)
    y1 = int(h * ROI_Y1_FRAC)
    x2 = int(w * ROI_X2_FRAC)
    y2 = int(h * ROI_Y2_FRAC)
    return (x1, y1, x2, y2)

# ==========================
# MAIN
# ==========================
def main():
    ser = init_serial()

    cap = cv2.VideoCapture(CAMERA_INDEX)
    if not cap.isOpened():
        print("[ERROR] Could not open webcam")
        return

    # Background subtractor
    back_sub = cv2.createBackgroundSubtractorMOG2(
        history=300,
        varThreshold=25,
        detectShadows=True
    )

    last_trigger_time = 0

    print("[INFO] Press 'q' to quit")
    print("[INFO] Press 'a' for AUTO_ON, 'm' for AUTO_OFF, 's' for STOP_ALL")

    while True:
        ret, frame = cap.read()
        if not ret:
            print("[WARN] Frame not captured")
            break

        frame = cv2.resize(frame, (960, 540))
        display = frame.copy()

        roi = get_roi(frame)
        x1, y1, x2, y2 = roi

        # Background subtraction
        fg_mask = back_sub.apply(frame)

        # Remove shadows and clean mask
        _, fg_mask = cv2.threshold(fg_mask, 200, 255, cv2.THRESH_BINARY)

        kernel = np.ones((5, 5), np.uint8)
        fg_mask = cv2.morphologyEx(fg_mask, cv2.MORPH_OPEN, kernel)
        fg_mask = cv2.morphologyEx(fg_mask, cv2.MORPH_CLOSE, kernel)
        fg_mask = cv2.dilate(fg_mask, kernel, iterations=2)

        contours, _ = cv2.findContours(
            fg_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
        )

        waste_detected_in_roi = False

        for cnt in contours:
            area = cv2.contourArea(cnt)
            if area < MIN_CONTOUR_AREA:
                continue

            x, y, w, h = cv2.boundingRect(cnt)
            cx = x + w // 2
            cy = y + h // 2

            inside_roi = point_in_rect(cx, cy, roi)

            # Draw contour box
            color = (0, 255, 0) if inside_roi else (255, 0, 0)
            cv2.rectangle(display, (x, y), (x + w, y + h), color, 2)
            cv2.circle(display, (cx, cy), 4, (0, 0, 255), -1)

            label = f"waste? area={int(area)}"
            cv2.putText(
                display, label, (x, max(20, y - 10)),
                cv2.FONT_HERSHEY_SIMPLEX, 0.55, color, 2
            )

            if inside_roi:
                waste_detected_in_roi = True

        # Draw trigger zone
        cv2.rectangle(display, (x1, y1), (x2, y2), (0, 255, 255), 2)
        cv2.putText(
            display, "TRIGGER ZONE", (x1, max(25, y1 - 10)),
            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2
        )

        # Trigger logic
        current_time = time.time()
        cooldown_remaining = max(0, TRIGGER_COOLDOWN - (current_time - last_trigger_time))

        status_text = "IDLE"
        if waste_detected_in_roi:
            status_text = "WASTE DETECTED"

            if current_time - last_trigger_time >= TRIGGER_COOLDOWN:
                send_command(ser, "START_CLEAN")
                last_trigger_time = current_time
                status_text = "CLEANING TRIGGERED"

        cv2.putText(
            display,
            f"Status: {status_text}",
            (20, 30),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.8,
            (0, 255, 0),
            2
        )

        cv2.putText(
            display,
            f"Cooldown: {cooldown_remaining:.1f}s",
            (20, 65),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.7,
            (255, 255, 255),
            2
        )

        if SHOW_DEBUG:
            cv2.imshow("Drainage Cleaner - Camera", display)
            cv2.imshow("Foreground Mask", fg_mask)
        else:
            cv2.imshow("Drainage Cleaner - Camera", display)

        key = cv2.waitKey(1) & 0xFF

        if key == ord('q'):
            break
        elif key == ord('a'):
            send_command(ser, "AUTO_ON")
        elif key == ord('m'):
            send_command(ser, "AUTO_OFF")
        elif key == ord('s'):
            send_command(ser, "STOP_ALL")
        elif key == ord('c'):
            send_command(ser, "CONVEYOR_ON")
        elif key == ord('x'):
            send_command(ser, "CONVEYOR_OFF")
        elif key == ord('r'):
            send_command(ser, "RAKE_ON")
        elif key == ord('t'):
            send_command(ser, "RAKE_OFF")
        elif key == ord('f'):
            send_command(ser, "START_CLEAN")

    cap.release()
    cv2.destroyAllWindows()

    if ser is not None:
        ser.close()

if __name__ == "__main__":
    main()