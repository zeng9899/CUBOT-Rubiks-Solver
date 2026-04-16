import serial
import time
import threading
import os

import cv2
import numpy as np
from collections import Counter
from kociemba import solve

# =========================
# 설정
# =========================
COM_PORT = 'COM6'
BAUD_RATE = 9600
SAVE_FOLDER = "C:/cubecapture/"
os.makedirs(SAVE_FOLDER, exist_ok=True)

arduino = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
time.sleep(2)

print("SHOT 신호 대기중... (exit 입력 시 종료)")

# SHOT 저장 이름 순서
shot_names = ["U", "R", "F", "D", "L", "B"]
shot_index = 0   # 현재 몇 번째 SHOT인지 카운트

# Kociemba에서 요구하는 면 순서(고정)
KOCIEMBA_ORDER = ['U', 'R', 'F', 'D', 'L', 'B']


# ==================== 색상 분류 함수 ====================
def classify_color(h, s, v):
    if v >= 150 and s <= 90:
        return 'W'
    if 20 <= h <= 40 and s >= 120 and v >= 120:
        return 'Y'
    if 5 <= h <= 25 and s > 80 and v > 80:
        return 'O'
    if ((0 <= h <= 4) or (170 <= h <= 179)) and s > 80 and v > 80:
        return 'R'
    if 41 <= h <= 85 and s > 80 and v > 40:
        return 'G'
    if 86 <= h <= 130 and s > 80 and v > 60:
        return 'B'
    return '?'

# ==================== 한 면 1:1 이미지 인식 ====================
def detect_face_colors(image_path, debug=False):
    img = cv2.imread(image_path)
    if img is None:
        raise FileNotFoundError(f"이미지 읽기 실패: {image_path}")

    h0, w0, _ = img.shape
    if debug:
        print(f"\n=== 이미지 처리: {image_path} === 크기: {w0} x {h0}")

    side = min(h0, w0)
    cx = w0 // 2
    cy = h0 // 2
    x1 = cx - side // 2
    y1 = cy - side // 2
    x2 = x1 + side
    y2 = y1 + side

    face = img[y1:y2, x1:x2]
    face_size = 300
    face = cv2.resize(face, (face_size, face_size))
    face_vis = face.copy()

    cell = face_size // 3
    if debug:
        for i in range(1, 3):
            cv2.line(face_vis, (0, i * cell), (face_size, i * cell), (0, 255, 0), 1)
            cv2.line(face_vis, (i * cell, 0), (i * cell, face_size), (0, 255, 0), 1)

    face_hsv = cv2.cvtColor(face, cv2.COLOR_BGR2HSV)
    inner_ratio = 0.5
    colors = []

    if debug:
        print("각 셀 HSV 중앙값 및 분류:")

    for row in range(3):
        row_colors = []
        for col in range(3):
            x_cell1 = col * cell
            y_cell1 = row * cell

            inner = int(cell * inner_ratio)
            xx1 = x_cell1 + (cell - inner) // 2
            yy1 = y_cell1 + (cell - inner) // 2
            xx2 = xx1 + inner
            yy2 = yy1 + inner

            roi = face_hsv[yy1:yy2, xx1:xx2]

            h = int(np.median(roi[:, :, 0]))
            s = int(np.median(roi[:, :, 1]))
            v = int(np.median(roi[:, :, 2]))

            label = classify_color(h, s, v)
            row_colors.append(label)

            if debug:
                print(f"({row},{col}) HSV=({h},{s},{v}) -> {label}")
                cv2.rectangle(face_vis, (xx1, yy1), (xx2, yy2), (0, 255, 255), 1)
                cx = (xx1 + xx2) // 2
                cy = (yy1 + yy2) // 2
                cv2.circle(face_vis, (cx, cy), 3, (0, 0, 255), -1)
                cv2.putText(face_vis, label, (x_cell1 + cell // 2 - 8, y_cell1 + cell // 2 + 8), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2, cv2.LINE_AA)

        colors.append(row_colors)

    if debug:
        print("3x3 색상 매트릭스:", colors)
        cv2.imshow("Cube face (with grid & ROI)", face_vis)
        print("아무 키나 눌러 현재 창 닫기...")
        cv2.waitKey(0)
        cv2.destroyAllWindows()

    return colors

# ==================== 중심 색 기반 매핑 + 문자열 생성 ====================
def build_kociemba_string(face_images, debug=False):
    order_process = ['U', 'R', 'F', 'D', 'L', 'B']
    face_colors = {}
    color_to_face = {}

    for face_name in order_process:
        if debug:
            print(f"\n===== 면 인식 {face_name} =====")

        path = face_images[face_name]
        colors = detect_face_colors(path, debug=debug)
        center_color = colors[1][1]

        if center_color == '?':
            raise ValueError(f"{face_name} 면 중심 색 '?' 인식, 사진/조명 확인 필요")

        color_to_face[center_color] = face_name
        face_colors[face_name] = colors

    state_chars = []
    for face_name in KOCIEMBA_ORDER:
        colors = face_colors[face_name]
        for row in range(3):
            for col in range(3):
                c = colors[row][col]
                face_char = color_to_face[c]
                state_chars.append(face_char)

    cube_string = ''.join(state_chars)
    return cube_string, face_colors, color_to_face

# ==================== 6면 촬영 후 Kociemba 호출 ====================
# True = 테스트 모드(HSV 출력 / 그림 표시 / 체크 / solve)
# False = 자동 모드(조용히, 결과만 출력)
TEST_MODE = 1
def solve_cube_after_shot():
    """
    6면 사진을 기반으로 Kociemba 문자열 생성 후 해법 계산
    반환: solution (문자열)
    """
    face_images = {f: os.path.join(SAVE_FOLDER, f"{f}.jpg") for f in 'URFDLB'}

    # 디버그 모드
    cube_string, face_colors, mapping = build_kociemba_string(face_images, debug=TEST_MODE)
    print("\nKociemba 입력 문자열:", cube_string)

    # 해법 계산
    solution = solve(cube_string)
    print("✅ 계산된 해법:", solution)

    # 아두이노 전송은 여기서 하지 않고 read_from_arduino()에서 처리
    return solution


# =========================
# 자동 촬영 함수
# =========================
def take_photo(save_name):
    #os.system("adb shell am start -a android.media.action.STILL_IMAGE_CAMERA")
    #time.sleep(1.2)
    res = os.popen("adb shell wm size").read().strip()
    w, h = map(int, res.split(":")[-1].strip().split("x"))
    tap_x = int(w / 2)
    tap_y = int(h * 0.88)
    os.system(f"adb shell input tap {tap_x} {tap_y}")
    time.sleep(2)
    latest = os.popen("adb shell ls -t /sdcard/DCIM/Camera").read().strip().split("\n")[0]
    os.system(f"adb pull /sdcard/DCIM/Camera/{latest} {SAVE_FOLDER}{save_name}.jpg")

# =========================
# 아두이노 SHOT 수신 스레드
# =========================
def read_from_arduino():
    global shot_index, last_solution
    while True:
        if arduino.in_waiting > 0:
            line = arduino.readline().decode().strip()
            print(f"[아두이노] {line}")
            # ------------------------
            # 사진 촬영
            # ------------------------
            if line == "SHOT" and shot_index < 6:
                name = shot_names[shot_index]
                take_photo(name)
                shot_index += 1

                # 모든 사진 촬영 완료 시 해법 계산
                if shot_index >= 6:
                    last_solution = solve_cube_after_shot()
                    shot_index = 0
                    
                    arduino.reset_input_buffer()
                    arduino.write(b"SOLUTION READY\n")   

            # ------------------------
            # 아두이노가 EXECUTING 상태일 때 해법 전송
            # ------------------------
            elif line == "STATE: EXECUTING":
                if last_solution:
                    arduino.reset_input_buffer()
                    arduino.write((last_solution + "\n").encode())
                    print("PC -> 아두이노로 해법 전송 완료")


thread = threading.Thread(target=read_from_arduino, daemon=True)
thread.start()

# =========================
# 사용자 입력
# =========================
while True:
    cmd = input(">>> ")
    if cmd.lower() == "exit":
        break
    if cmd.lower() == "reset":
        shot_index = 0
        continue
    arduino.write((cmd + "\n").encode())

arduino.close()