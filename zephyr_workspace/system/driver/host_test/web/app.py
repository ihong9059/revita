"""
REVITA Tower Host Test - Web Simulation

Flask 웹서버로 USB CDC(ttyACMx)를 통해 Tower에 명령을 보내고 응답을 확인.
"""

import sys
import time
import threading
from flask import Flask, render_template, jsonify, request
import serial

app = Flask(__name__)

# Serial
SERIAL_PORT = "/dev/ttyACM4"
SERIAL_BAUD = 115200
ser = None
ser_lock = threading.Lock()

# Log
MAX_LOG = 200
comm_log = []


def log_entry(direction, text):
    entry = {
        "time": time.strftime("%H:%M:%S"),
        "dir": direction,
        "text": text.strip(),
    }
    comm_log.append(entry)
    if len(comm_log) > MAX_LOG:
        comm_log.pop(0)


def serial_connect():
    global ser
    try:
        ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=0.5)
        print(f"[OK] Connected to {SERIAL_PORT}")
        return True
    except Exception as e:
        print(f"[ERR] Serial open failed: {e}")
        ser = None
        return False


def send_command(cmd):
    with ser_lock:
        if ser is None or not ser.is_open:
            return None
        line = cmd + "\n"
        ser.write(line.encode())
        log_entry("TX", cmd)

        # 응답 대기
        time.sleep(0.2)
        resp = ""
        while ser.in_waiting > 0:
            resp += ser.read(ser.in_waiting).decode(errors="replace")
            time.sleep(0.05)

        if resp.strip():
            log_entry("RX", resp.strip())
        return resp.strip()


@app.route("/")
def index():
    return render_template("index.html", port=SERIAL_PORT)


@app.route("/api/connect", methods=["POST"])
def api_connect():
    global SERIAL_PORT
    port = request.json.get("port", SERIAL_PORT)
    SERIAL_PORT = port
    ok = serial_connect()
    return jsonify({"ok": ok, "port": SERIAL_PORT})


@app.route("/api/send", methods=["POST"])
def api_send():
    cmd = request.json.get("cmd", "")
    if not cmd:
        return jsonify({"ok": False, "error": "empty command"})
    resp = send_command(cmd)
    if resp is None:
        return jsonify({"ok": False, "error": "not connected"})
    return jsonify({"ok": True, "response": resp})


@app.route("/api/log")
def api_log():
    return jsonify(comm_log)


@app.route("/api/status")
def api_status():
    connected = ser is not None and ser.is_open
    return jsonify({"connected": connected, "port": SERIAL_PORT})


if __name__ == "__main__":
    if len(sys.argv) > 1:
        SERIAL_PORT = sys.argv[1]
    serial_connect()
    app.run(host="0.0.0.0", port=5000, debug=True)
