#ifndef GPIOCONTROLLER_H
#define GPIOCONTROLLER_H

#include <QObject>              // Qt 객체 기본 클래스
#include <QTimer>               // 타이머 클래스
#include <QMap>                 // 맵 컨테이너

#ifdef RASPBERRY_PI_BUILD
#include <wiringPi.h>           // WiringPi 라이브러리 (라즈베리파이에서만)
#endif

// GPIO 핀 정의
enum GPIOPin {
    LCD_PIN_1 = 18,             // LCD 1 제어 핀 (GPIO 18)
    LCD_PIN_2 = 19,             // LCD 2 제어 핀 (GPIO 19)
    LCD_PIN_3 = 20,             // LCD 3 제어 핀 (GPIO 20)
    LCD_PIN_4 = 21,             // LCD 4 제어 핀 (GPIO 21)
    LED_STATUS = 16,            // 상태 LED 핀 (GPIO 16)
    BUZZER_PIN = 12             // 부저 핀 (GPIO 12)
};

// 라즈베리파이 GPIO 제어 클래스
class GPIOController : public QObject
{
    Q_OBJECT

public:
    explicit GPIOController(QObject *parent = nullptr);  // 생성자
    ~GPIOController();                                   // 소멸자

    // GPIO 초기화 및 정리
    bool initializeGPIO();                               // GPIO 초기화
    void cleanupGPIO();                                  // GPIO 정리

    // LCD 제어 함수들
    void setLCDState(int lcdId, bool state);             // LCD 상태 설정
    bool getLCDState(int lcdId) const;                   // LCD 상태 조회
    void toggleLCD(int lcdId);                           // LCD 토글

    // LED 및 부저 제어
    void setStatusLED(bool state);                       // 상태 LED 제어
    void blinkStatusLED(int duration = 1000);            // 상태 LED 깜빡임
    void soundBuzzer(int duration = 500);                // 부저 소리

    // GPIO 상태 확인
    bool isGPIOInitialized() const;                      // GPIO 초기화 상태 확인

signals:
    void lcdStateChanged(int lcdId, bool state);         // LCD 상태 변경 시그널
    void gpioError(const QString &error);                // GPIO 에러 시그널

private slots:
    void onStatusLEDTimer();                             // 상태 LED 타이머 슬롯
    void onBuzzerTimer();                                // 부저 타이머 슬롯

private:
    // 내부 함수들
    int getLCDPin(int lcdId) const;                      // LCD ID에 해당하는 핀 번호 반환
    void setPinMode(int pin, int mode);                  // 핀 모드 설정
    void digitalWrite(int pin, int value);               // 디지털 출력
    int digitalRead(int pin);                            // 디지털 입력

    // 멤버 변수들
    bool m_gpioInitialized;                              // GPIO 초기화 상태
    QMap<int, bool> m_lcdStates;                         // LCD 상태 저장
    QTimer *m_statusLEDTimer;                            // 상태 LED 타이머
    QTimer *m_buzzerTimer;                               // 부저 타이머
    bool m_statusLEDState;                               // 현재 상태 LED 상태
};

#endif // GPIOCONTROLLER_H
