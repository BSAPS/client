# CCTV Monitoring System for Alleyway Safety

이면도로에서 발생할 수 있는 사고를 예방하기 위한 CCTV 기반 실시간 경고 시스템입니다.

## 프로젝트 개요

이 프로그램은 이면도로(골목길 등)에서 차량과 보행자의 충돌 사고를 방지하기 위해 만들어졌습니다.  
실시간 CCTV 영상을 분석하여 기준선을 통과하는 보행자를 인식하고, 차량에게 경고 메시지를 표시함으로써 안전 사고를 사전에 방지합니다.


## 주요 기능

 → 실시간 영상 스트리밍
     - CCTV에서 제공하는 RTSP 영상 스트림을 수신
     - Qt 기반 클라이언트에서 영상 실시간 표시
     
→ 도로선/기준선 설정
     - 사용자 마우스로 도로선 / 기준선 설정
     - 선의 좌표 정보를 TCP를 통해 서버에 전송
     
→ 객체 인식 및 경고 시스템
     - 영상에서 보행자와 차량을 인식
     - 보행자가 기준선을 넘을 경우, 서버가 Dot Matrix를 통해 차량에 경고 메시지 송출
     
→ 경고 상황 이미지 저장
     - 경고 발생 시 해당 프레임을 자동 캡처
     - 캡처 이미지를 서버 DB에 저장
     
→ 캡처 이미지 조회
     - 클라이언트(Qt)에서 날짜 및 시간대 선택
     - 서버로부터 해당 시점의 저장 이미지 수신 및 표시


## 시스템 구성도

     [CCTV] --(RTSP)--> [Qt 클라이언트] <--(TCP)--> [서버] --(제어)--> [Dot Matrix]
                                                    ↑
                                                 [DB 선 정보 저장 및 조회, 이미지 저장 및 조회]



## 기술 스택

| 구성 요소       | 기술 |
|----------------|------|
| 클라이언트 앱   | Qt (C++) |
| 영상 수신       | RTSP 프로토콜 |
| 통신 방식       | TCP Socket |
| 객체 인식       | YOLO 등 (※ 서버 측 구현 기준) |
| 메시지 디스플레이 | Dot Matrix |
| 이미지 저장/조회 | DB (서버 측 구현 기준) |


## 프로젝트 구조

     client/
     ├── main.cpp
     ├── MainWindow.cpp / .h
     ├── Login / SignUp / OTP 관련 UI
     ├── VideoStreamWidget.cpp / .h
     ├── icons/
     │   └── UI에 사용되는 아이콘 리소스
     └── README.md

## 빌드 방법

### 1. 프로젝트 클론 및 이동

```bash
git clone https://github.com/veda-team3-final-project/client
cd client
```

---

### 2. 빌드 방법 선택

#### 방법 A: 명령어로 빌드 (qmake + mingw)

```bash
cd /d C:\client  # 또는 프로젝트 루트 경로로 이동
qmake CCTVMonitoring.pro
mingw32-make
```

- 빌드 완료 후 `release\CCTVMonitoring.exe` 또는 `debug\CCTVMonitoring.exe` 실행

---

#### 방법 B: Qt Creator에서 빌드

1. Qt Creator 실행  
2. `CCTVMonitoring.pro` 열기  
3. 상단의 Build 버튼 클릭 (또는 Ctrl + R)  
4. 실행 파일이 `release\` 또는 `debug\` 폴더에 생성됨



## 사용법

1. 서버 실행
2. 클라이언트(Qt) 실행
3. 로그인 후 실시간 CCTV 영상 확인
4. 도로선과 기준선을 설정
5. 경고 발생 시 서버가 캡처 및 저장
6. 캡처 이미지 조회 탭에서 원하는 날짜/시간 선택하여 확인


## 개발/테스트 환경

- Windows 10 / 11
- Qt 5.x / 6.x
- Visual Studio or Qt Creator


## 참고사항

- 서버 코드와 Dot Matrix 제어는 별도 제공
- 이 프로젝트는 실시간 영상처리와 IoT 제어 시스템의 융합 예시입니다
- 개인정보 및 보안 관련 고려 필요 (실제 운영 시)


## 제작 의도

이면도로에서 종종 발생하는 보행자-차량 사고를 "사전 인식 → 경고 → 기록" 의 흐름으로 예방할 수 있도록 기획된 프로젝트입니다.
