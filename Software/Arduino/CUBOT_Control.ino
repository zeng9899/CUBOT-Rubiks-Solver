#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
const int DELAY = 500;

// 적외선 센서 2Y0A21 연결 핀
int irPin = A0;

// 완료 버튼 연결 핀
int buttonPin = 2;

// MG996R 실제 펄스 기준으로 계산한 정확한 값
int pulseMin = 102;   // ≈500µs
int pulseMax = 512;   // ≈2500µs

String solutionString = "";
String moves[100];
int moveCount = 0;
int currentMoveIndex = 0;

// 서보 각도를 드라이버 모듈에 맞게 펄스로 변환
int angleToPulse(int angle) { return map(angle, 0, 180, pulseMin, pulseMax); }

Adafruit_PWMServoDriver pca9685 = Adafruit_PWMServoDriver();

class Gripper {
  private:
    Adafruit_PWMServoDriver* pwm;
    uint8_t chGrip;
    uint8_t chRotate;
    int gripAngle;
    int rotateAngle;

  public:
    // 생성자(집는모터핀번호, 회전모터핀번호)
    Gripper(uint8_t gripCh, uint8_t rotateCh)
      : pwm(&pca9685), chGrip(gripCh), chRotate(rotateCh),
        gripAngle(90), rotateAngle(90) {}

    void begin() {
      moveGrip(90);
      moveRotate(90);
    }

    // 집게 여는 정도
    void moveGrip(int angle) {
      angle = constrain(angle, 0, 180);
      gripAngle = angle;
      pwm->setPWM(chGrip, 0, angleToPulse(angle));
    }

    // 그리퍼 회전 정도
    void moveRotate(int angle) {
      angle = constrain(angle, 0, 180);
      rotateAngle = angle;
      pwm->setPWM(chRotate, 0, angleToPulse(angle));
    }

    // 집게 열기
    void open() { moveGrip(90); }
    // 집게 닫기
    void close() { moveGrip(51); }
    // 그리퍼 시계방향 90도 회전
    void rotateCW() { moveRotate(180); }
    // 그리퍼 반시계방향 90도 회전
    void rotateCCW() { moveRotate(0); }
    // 그리퍼 제자리로 회전
    void rotateHome() { moveRotate(90); }
};

// ----- 함수 프로토타입 선언 -----
void CWSeq(Gripper& g);
void CCWSeq(Gripper& g);
void Seq180(Gripper& g);

// 그리퍼 4개 생성 (집는 모터, 회전 모터)
Gripper Rgripper(0, 2);
Gripper Lgripper(4, 6);
Gripper Fgripper(8, 10);
Gripper Bgripper(12, 14);

// 모든 그리퍼를 한번에 움직이는 클래스
class AllGripper {
  public:
    void begin() {
      Rgripper.begin();
      Lgripper.begin();
      Fgripper.begin();
      Bgripper.begin();
    }

    void open() {
      Rgripper.open();
      Lgripper.open();
      Fgripper.open();
      Bgripper.open();
    }

    void close() {
      Rgripper.close();
      Lgripper.close();
      Fgripper.close();
      Bgripper.close();
    }

    void rotateCW() {
      Rgripper.rotateCW();
      Lgripper.rotateCW();
      Fgripper.rotateCW();
      Bgripper.rotateCW();
    }

    void rotateCCW() {
      Rgripper.rotateCCW();
      Lgripper.rotateCCW();
      Fgripper.rotateCCW();
      Bgripper.rotateCCW();
    }

    void rotateHome() {
      Rgripper.rotateHome();
      Lgripper.rotateHome();
      Fgripper.rotateHome();
      Bgripper.rotateHome();
    }
};
// AllGripper 객체 생성
AllGripper Agripper;

// 안정적 잡기
void sgrip() {
  Lgripper.open();
  Rgripper.open();
  delay(DELAY);
  Lgripper.close();
  Rgripper.close();
  delay(DELAY);
  Fgripper.open();
  Bgripper.open();
  delay(DELAY);
  Fgripper.close();
  Bgripper.close();
  delay(DELAY);
}
// 정면 기울임
void F_Tilt() {
  // 앞뒤 그리퍼 열기
  Fgripper.open();
  Bgripper.open();
  delay(DELAY);
  // 좌우 그리퍼를 회전해 큐브를 기울임
  Rgripper.rotateCCW();
  Lgripper.rotateCW();
  delay(DELAY);
  // 앞뒤 그리퍼 닫기
  Fgripper.close();
  Bgripper.close();
  delay(DELAY);
  // RL 그리퍼 복귀 (열고 → 제자리 → 닫기)
  Rgripper.open();
  Lgripper.open();
  delay(DELAY);
  Rgripper.rotateHome();
  Lgripper.rotateHome();
  delay(DELAY);
  Rgripper.close();
  Lgripper.close();
  delay(DELAY);
}
// 후면 기울임
void B_Tilt() {
  // 앞뒤 그리퍼 열기
  Fgripper.open();
  Bgripper.open();
  delay(DELAY);
  // 좌우 그리퍼를 반대 방향으로 회전 (기울임 복원)
  Rgripper.rotateCW();
  Lgripper.rotateCCW();
  delay(DELAY);
  // 앞뒤 그리퍼 닫기
  Fgripper.close();
  Bgripper.close();
  delay(DELAY);
  //  RL 그리퍼 복귀 (열고 → 제자리 → 닫기)
  Rgripper.open();
  Lgripper.open();
  delay(DELAY);
  Rgripper.rotateHome();
  Lgripper.rotateHome();
  delay(DELAY);
  Rgripper.close();
  Lgripper.close();
  delay(DELAY);
}
// 좌측 기울임
void R_Tilt() {
  // 좌우 그리퍼 열기
  Rgripper.open();
  Lgripper.open();
  delay(DELAY);
  // 앞뒤 그리퍼를 회전해 큐브를 기울임
  Fgripper.rotateCW();
  Bgripper.rotateCCW();
  delay(DELAY);
  // 좌우 그리퍼 닫기
  Rgripper.close();
  Lgripper.close();
  delay(DELAY);
  // 앞뒤 그리퍼 복귀 (열고 → 제자리 → 닫기)
  Fgripper.open();
  Bgripper.open();
  delay(DELAY);
  Fgripper.rotateHome();
  Bgripper.rotateHome();
  delay(DELAY);
  Fgripper.close();
  Bgripper.close();
  delay(DELAY);
}
// 우측 기울임
void L_Tilt() {
  // 좌우 그리퍼 열기
  Rgripper.open();
  Lgripper.open();
  delay(DELAY);
  // 앞뒤 그리퍼를 회전해 큐브를 기울임
  Fgripper.rotateCCW();
  Bgripper.rotateCW();
  delay(DELAY);
  // 좌우 그리퍼 닫기
  Rgripper.close();
  Lgripper.close();
  delay(DELAY);
  // 앞뒤 그리퍼 복귀 (열고 → 제자리 → 닫기)
  Fgripper.open();
  Bgripper.open();
  delay(DELAY);
  Fgripper.rotateHome();
  Bgripper.rotateHome();
  delay(DELAY);
  Fgripper.close();
  Bgripper.close();
  delay(DELAY);
}
// 촬영을 위한 그리퍼 위치
void get_cap(){
  Rgripper.open();
  Lgripper.open();
  delay(DELAY);
  Rgripper.rotateCW();
  Lgripper.rotateCW();
  delay(DELAY);
  Rgripper.close();
  Lgripper.close();
  delay(DELAY);
  Fgripper.open();
  Bgripper.open();
  // 촬영시간
  delay(DELAY);
  Serial.println("SHOT");
  delay(3000);
  Fgripper.close();
  Bgripper.close();
  delay(DELAY);
  Rgripper.open();
  Lgripper.open();
  delay(DELAY);
  Rgripper.rotateHome();
  Lgripper.rotateHome();
  delay(DELAY);
  Rgripper.close();
  Lgripper.close();
  delay(DELAY);
}
// 인식 순서 U -> R -> F -> D -> L -> B
void recog(){
  // 첫 면을 U로 인식, 촬영
  get_cap();
  // R로 이동 후 촬영
  B_Tilt();
  L_Tilt();
  get_cap();
  // F로 이동 후 촬영
  R_Tilt();
  get_cap();
  // D로 이동 후 촬영
  B_Tilt();
  get_cap();
  // L로 이동 후 촬영
  F_Tilt();
  R_Tilt();
  get_cap();
  // B로 이동 후 촬영
  R_Tilt();
  get_cap();
  //정 위치로 정렬
  L_Tilt();
  L_Tilt();
  F_Tilt();
}
// 큐브 면회전
// 시계 방향 90도 회전
void CWSeq(Gripper& g) {
  g.rotateCW(); delay(DELAY);
  g.open(); delay(DELAY);
  g.rotateHome(); delay(DELAY);
  g.close(); delay(DELAY);
}
// 반시계 방향 90도 회전
void CCWSeq(Gripper& g) {
  g.rotateCCW(); delay(DELAY);
  g.open(); delay(DELAY);
  g.rotateHome(); delay(DELAY);
  g.close(); delay(DELAY);
}
// 180도 회전 회전
void Seq180(Gripper& g) {
  g.open(); delay(DELAY);
  g.rotateCCW(); delay(DELAY);
  g.close(); delay(DELAY);
  g.rotateCW(); delay(1000);
  g.open(); delay(DELAY);
  g.rotateHome(); delay(DELAY);
  g.close(); delay(DELAY);
}

// 각 해법 18가지 = 6개 면 * 3가지 회전(시계방향90도, 반시계방향90도, 180도)
void R_CW()   { CWSeq(Rgripper); }
void R_CCW()  { CCWSeq(Rgripper); } 
void R_180()  { Seq180(Rgripper); } 

void L_CW()   { CWSeq(Lgripper); }
void L_CCW()  { CCWSeq(Lgripper); }
void L_180()  { Seq180(Lgripper); }

void F_CW()   { CWSeq(Fgripper); }
void F_CCW()  { CCWSeq(Fgripper); }
void F_180()  { Seq180(Fgripper); }

void B_CW()   { CWSeq(Bgripper); }
void B_CCW()  { CCWSeq(Bgripper); }
void B_180()  { Seq180(Bgripper); }

void U_CW()   { F_Tilt(); CWSeq(Fgripper); B_Tilt(); }
void U_CCW()  { F_Tilt(); CCWSeq(Fgripper); B_Tilt(); }
void U_180()  { F_Tilt(); Seq180(Fgripper); B_Tilt(); }

void D_CW()   { F_Tilt(); CWSeq(Bgripper); B_Tilt(); }
void D_CCW()  { F_Tilt(); CCWSeq(Bgripper); B_Tilt(); }
void D_180()  { F_Tilt(); Seq180(Bgripper); B_Tilt(); }

// 큐브 해법과 매핑
void executeMove(const String &mv) {
  if (mv == "R")              R_CW();
  else if (mv == "R'")        R_CCW();
  else if (mv == "R2")        R_180();

  else if (mv == "L")         L_CW();
  else if (mv == "L'")        L_CCW();
  else if (mv == "L2")        L_180();

  else if (mv == "F")         F_CW();
  else if (mv == "F'")        F_CCW();
  else if (mv == "F2")        F_180();

  else if (mv == "B")         B_CW();
  else if (mv == "B'")        B_CCW();
  else if (mv == "B2")        B_180();

  else if (mv == "U")         U_CW();
  else if (mv == "U'")        U_CCW();
  else if (mv == "U2")        U_180();

  else if (mv == "D")         D_CW();
  else if (mv == "D'")        D_CCW();
  else if (mv == "D2")        D_180();

  // 이미 풀린 상태
  else if (mv == "" || mv == " ") Serial.println("Already Solved");

  // 할당되지 않은 상태
  else {
      Serial.print("Unknown Move: ");
      Serial.println(mv);
  }
}

// 해법 받기
void receiveSolution() {
  if (Serial.available()) {
    solutionString = Serial.readStringUntil('\n');  
    solutionString.trim();
    Serial.print("Received solution: ");
    Serial.println(solutionString);

    // 문자열을 공백으로 분리
    moveCount = 0;
    int start = 0;
    for (int i = 0; i < solutionString.length(); i++) {
      if (solutionString[i] == ' ') {
        moves[moveCount++] = solutionString.substring(start, i);
        start = i + 1;
      }
    }
    moves[moveCount++] = solutionString.substring(start);

    currentMoveIndex = 0;

    Serial.print("Parsed moves: ");
    for (int i = 0; i < moveCount; i++) {
      Serial.print(moves[i]);
      Serial.print(" ");
    }
    Serial.println();
  }
}

// 장치 상태
enum State {
  TEST,           // 0. TEST용 메뉴얼 상태
  WAITING,        // 1. 대기중
  VERIFYING,      // 2. 검증
  SCANNING,       // 3. 인식
  SOLVING,        // 4. 알고리즘 처리
  EXECUTING,      // 5. 큐브 정렬 시작
  COMPLETED,      // 6. 완성 후 수령 대기
};
volatile State currentState = WAITING; // 장치 초기상태 설정 TEST or WAITING

void setup() {
  Serial.begin(9600);             // 시리얼 모니터 연결
  Wire.begin();                   // I2C 통신 시작
  pca9685.begin();                // PCA9685 드라이버 초기화
  pca9685.setPWMFreq(50);         // 서보모터 PWM 주파수 설정 (표준 50Hz)

  pinMode(buttonPin, INPUT_PULLUP);   // 내부 풀업 사용 → 외부 저항 필요 없음

  Agripper.begin();               // 모든 그리퍼 open, 90도 정렬
  delay(2000);                    // 서보 안정화 대기 (2초)
  Serial.println("System Ready");
  delay(DELAY);
}

bool stable = true;
String input;
void loop() {
  switch (currentState) {
    // 0. 메뉴얼 모드
    case TEST:
      Serial.println("STATE: TEST - INPUT YOUR COMMAND.");
      while (Serial.available() == 0) {
        delay(DELAY);
      }
      input = Serial.readStringUntil('\n');
      input.trim();   // 공백/줄바꿈 제거
      if (input == "c") {
        Serial.println("RECOG C");
        Agripper.close();
      } else if (input == "o") {
        Serial.println("RECOG O");
        Agripper.open();
      } else if (input == "f") {
        Serial.println("RECOG F");
        F_CW();
        delay(DELAY);
        F_CCW();
        delay(DELAY);
        F_180();
      } else if (input == "b") {
        Serial.println("RECOG B");
        B_CW();
        delay(DELAY);
        B_CCW();
        delay(DELAY);
        B_180();
      } else if (input == "r") {
        Serial.println("RECOG R");
        R_CW();
        delay(DELAY);
        R_CCW();
        delay(DELAY);
        R_180();
      } else if (input == "l") {
        Serial.println("RECOG L");
        L_CW();
        delay(DELAY);
        L_CCW();
        delay(DELAY);
        L_180();
      } else if (input == "u") {
        Serial.println("RECOG u");
        U_CW();
        delay(DELAY);
        U_CCW();
        delay(DELAY);
        U_180();
      } else if (input == "d") {
        Serial.println("RECOG D");
        D_CW();
        delay(DELAY);
        D_CCW();
        delay(DELAY);
        D_180();
      } else if (input == "ft") {
        Serial.println("RECOG F_Tilt");
        F_Tilt();
      } else if (input == "bt") {
        Serial.println("RECOG B_Tilt");
        B_Tilt();
      } else if (input == "rt") {
        Serial.println("RECOG R_Tilt");
        R_Tilt();
      } else if (input == "lt") {
        Serial.println("RECOG L_Tilt");
        L_Tilt();
      } else if (input == "g") {
        Serial.println("RECOG G");
        sgrip();
        sgrip();
      } else if (input == "cap") {
        Serial.println("RECOG cap");
        get_cap();
      } else if (input == "recog") {
        Serial.println("RECOG recog");
        recog();
      } else if (input == "s") {
        Serial.println("SHOT");
      }  else {
        Serial.println("Unknown command");
      }
      break;

    // 1. 대기 상태
    case WAITING:
      Serial.println("STATE: WAITING");
      Rgripper.rotateCCW();
      Lgripper.rotateCW();
      delay(DELAY);                   // 서보 안정화 대기
      // 적외선 센서변화를 대기하는 단계
      // 아날로그 값 읽기
      while(true){
        int d = analogRead(irPin);
        Serial.println(d);
        if (d > 400 ) {
          break;
        }
        delay(1000);
      }
      // 값이 변화하면 현재 상태를 VERIFYING으로 변환
      Serial.println("Something detected.");
      currentState = VERIFYING;
      
      break;

    // 2. 검증 상태
    case VERIFYING:
      Serial.println("STATE: VERIFYING");
      // 일정 시간 동안 센서값이 안정적으로 유지되는지 검사
      // 3초간 검사
      for (int i = 0; i < 3; i++) {
        int d = analogRead(irPin);
        Serial.println(d);
        if (d < 400) {
          stable = false;
          break;
        }
        delay(1000);
      }

      // 거리가 안정정이지 않다면 다시 대기상태로
      if(!stable) {
        Serial.println("Verification failed. Returning to WAITING...");
        currentState = WAITING;
      } else {
        // 거리가 안정적이라면 큐브를 잡음
        Serial.println("Verification complete.");
        Rgripper.close();
        Lgripper.close();
        delay(DELAY);
        Rgripper.rotateHome();
        Lgripper.rotateHome();
        delay(DELAY);
        Fgripper.close();
        Bgripper.close();
        delay(DELAY);
        Rgripper.open();
        Lgripper.open();
        delay(DELAY);
        Rgripper.close();
        Lgripper.close();
        delay(DELAY);
        currentState = SCANNING;
      }
      break;

    // 3. 인식 상태 %%%테스트 단계
    case SCANNING:
      Serial.println("STATE: SCANNING");
      sgrip(); // 안정적으로 큐브 잡기
      // 큐브 회전 + 색상 카메라/센서로 면 인식
      // 큐브 회전 함수
      Serial.println("Scanning cube colors...");
      recog();
      currentState = SOLVING;
      break;

    // 4. 알고리즘 처리 상태
    case SOLVING:
      Serial.println("STATE: SOLVING");
      // TODO: 알고리즘 풀이완료 까지 대기 후 전달 받으면 다음 단계 이행
      while (Serial.available() == 0) {
        delay(DELAY);
      }
      Serial.println("Algorithm has complite");
      currentState = EXECUTING;
    
      break;
    // 5. 큐브 정렬 실행 상태
    case EXECUTING:
      Serial.println("STATE: EXECUTING");
    
      // TODO: 계산된 무브 시퀀스대로 모터 제어
      // 받은 해법 순서대로 미리 만든 함수 실행
      receiveSolution();
      if (currentMoveIndex < moveCount) {
        String mv = moves[currentMoveIndex];
        Serial.print("Executing move: ");
        Serial.println(mv);

        executeMove(mv);       // 네가 만들어둔 매핑 함수 실행

        currentMoveIndex++;
        delay(300);            // 각 동작 사이 딜레이 (모터 시간)
      } 
      else {
        Serial.println("All moves completed.");
        currentState = COMPLETED;
      }
    
      break;
    // 6. 완료 상태 (버튼 대기)
    case COMPLETED:
      Serial.println("STATE: COMPLETED - Waiting for user to pick up.");
      Agripper.close();
    
      // 버튼 누름을 무한히 대기
      while (true) {
        // 버튼이 눌리면(LOW) → 초기 상태로 복귀
        int btn = digitalRead(buttonPin);
        // 버튼 눌림
        if (btn == LOW) {      
          Serial.println("Button pressed!");
          break;
        }
        delay(DELAY);
      }
      Serial.println("Gripper open in 3");
      delay(DELAY);
      Serial.println("Gripper open in 2");
      delay(DELAY);
      Serial.println("Gripper open in 1");
      delay(DELAY);
      // 큐브 놓기
      Agripper.open();
      delay(DELAY);

      Serial.println("Returning to WAITING state.");
      delay(DELAY);
      currentState = WAITING;
    
      break;
    default:
      Serial.println("ERROR");
      break;
  }
}

