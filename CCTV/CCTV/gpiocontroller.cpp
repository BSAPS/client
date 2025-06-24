#include "gpiocontroller.h"
#include <QDebug>               // 디버그 출력

GPIOController::GPIOController(QObject *parent)
    : QObject(parent)
    , m_gpioInitialized(false)
    , m_statusLEDTimer(nullptr)
    , m_buzzerTimer(nullptr)
    , m_statusLEDState(false)
{
    // LCD 상태 초기화 (모두 OFF)
    for (int i = 1; i <= 4; ++i) {
        m_lcdStates[i] = false;
    }
    
    // 타이머 초기화
    m_statusLEDTimer = new QTimer(this);
    m_statusLEDTimer->setSingleShot(true);
    connect(m_statusLEDTimer, &QTimer::timeout, this, &GPIOController::onStatusLEDTimer);
    
    m_buzzerTimer = new QTimer(this);
    m_buzzerTimer->setSingleShot(true);
    connect(m_buzzerTimer, &QTimer::timeout, this, &GPIOController::onBuzzerTimer);
    
    // GPIO 초기화 시도
    initializeGPIO();
}

GPIOController::~GPIOController()
{
    cleanupGPIO();                      // GPIO 정리
}

bool GPIOController::initializeGPIO()
{
#ifdef RASPBERRY_PI_BUILD
    // WiringPi 초기화
    if (wiringPiSetupGpio() == -1) {    // GPIO 번호 사용 모드로 초기화
        qCritical() << "Failed to initialize WiringPi";
        emit gpioError("Failed to initialize GPIO");
        return false;
    }
    
    // LCD 제어 핀들을 출력 모드로 설정
    setPinMode(LCD_PIN_1, OUTPUT);
    setPinMode(LCD_PIN_2, OUTPUT);
    setPinMode(LCD_PIN_3, OUTPUT);
    setPinMode(LCD_PIN_4, OUTPUT);
    
    // 상태 LED와 부저 핀을 출력 모드로 설정
    setPinMode(LED_STATUS, OUTPUT);
    setPinMode(BUZZER_PIN, OUTPUT);
    
    // 모든 핀을 LOW로 초기화
    digitalWrite(LCD_PIN_1, LOW);
    digitalWrite(LCD_PIN_2, LOW);
    digitalWrite(LCD_PIN_3, LOW);
    digitalWrite(LCD_PIN_4, LOW);
    digitalWrite(LED_STATUS, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    
    m_gpioInitialized = true;
    qDebug() << "GPIO initialized successfully";
    
    // 초기화 완료 신호 (LED 깜빡임)
    blinkStatusLED(2000);
    
    return true;
#else
    // 라즈베리파이가 아닌 환경에서는 시뮬레이션 모드
    qDebug() << "GPIO simulation mode (not on Raspberry Pi)";
    m_gpioInitialized = true;           // 시뮬레이션에서는 항상 성공
    return true;
#endif
}

void GPIOController::cleanupGPIO()
{
    if (!m_gpioInitialized) {
        return;
    }
    
#ifdef RASPBERRY_PI_BUILD
    // 모든 핀을 LOW로 설정하여 안전하게 정리
    digitalWrite(LCD_PIN_1, LOW);
    digitalWrite(LCD_PIN_2, LOW);
    digitalWrite(LCD_PIN_3, LOW);
    digitalWrite(LCD_PIN_4, LOW);
    digitalWrite(LED_STATUS, LOW);
    digitalWrite(BUZZER_PIN, LOW);
#endif
    
    m_gpioInitialized = false;
    qDebug() << "GPIO cleaned up";
}

void GPIOController::setLCDState(int lcdId, bool state)
{
    if (!m_gpioInitialized) {
        emit gpioError("GPIO not initialized");
        return;
    }
    
    if (lcdId < 1 || lcdId > 4) {
        qWarning() << "Invalid LCD ID:" << lcdId;
        return;
    }
    
    int pin = getLCDPin(lcdId);         // LCD ID에 해당하는 핀 번호 가져오기
    
#ifdef RASPBERRY_PI_BUILD
    digitalWrite(pin, state ? HIGH : LOW);  // 실제 GPIO 제어
#else
    qDebug() << QString("LCD %1 %2 (Simulated)").arg(lcdId).arg(state ? "ON" : "OFF");
#endif
    
    m_lcdStates[lcdId] = state;         // 상태 저장
    emit lcdStateChanged(lcdId, state); // 상태 변경 시그널 발생
    
    qDebug() << QString("LCD %1 turned %2").arg(lcdId).arg(state ? "ON" : "OFF");
}

bool GPIOController::getLCDState(int lcdId) const
{
    return m_lcdStates.value(lcdId, false); // LCD 상태 반환
}

void GPIOController::toggleLCD(int lcdId)
{
    bool currentState = getLCDState(lcdId); // 현재 상태 가져오기
    setLCDState(lcdId, !currentState);      // 상태 반전
}

void GPIOController::setStatusLED(bool state)
{
    if (!m_gpioInitialized) {
        return;
    }
    
#ifdef RASPBERRY_PI_BUILD
    digitalWrite(LED_STATUS, state ? HIGH : LOW);   // 실제 LED 제어
#else
    qDebug() << QString("Status LED %1 (Simulated)").arg(state ? "ON" : "OFF");
#endif
    
    m_statusLEDState = state;           // 상태 저장
}

void GPIOController::blinkStatusLED(int duration)
{
    if (!m_gpioInitialized) {
        return;
    }
    
    setStatusLED(true);                 // LED 켜기
    m_statusLEDTimer->start(duration);  // 지정된 시간 후 끄기
}

void GPIOController::soundBuzzer(int duration)
{
    if (!m_gpioInitialized) {
        return;
    }
    
#ifdef RASPBERRY_PI_BUILD
    digitalWrite(BUZZER_PIN, HIGH);     // 부저 켜기
#else
    qDebug() << QString("Buzzer ON for %1ms (Simulated)").arg(duration);
#endif
    
    m_buzzerTimer->start(duration);     // 지정된 시간 후 끄기
}

bool GPIOController::isGPIOInitialized() const
{
    return m_gpioInitialized;           // GPIO 초기화 상태 반환
}

void GPIOController::onStatusLEDTimer()
{
    setStatusLED(false);                // 상태 LED 끄기
}

void GPIOController::onBuzzerTimer()
{
#ifdef RASPBERRY_PI_BUILD
    digitalWrite(BUZZER_PIN, LOW);      // 부저 끄기
#else
    qDebug() << "Buzzer OFF (Simulated)";
#endif
}

int GPIOController::getLCDPin(int lcdId) const
{
    switch (lcdId) {                    // LCD ID에 따른 핀 번호 반환
    case 1: return LCD_PIN_1;
    case 2: return LCD_PIN_2;
    case 3: return LCD_PIN_3;
    case 4: return LCD_PIN_4;
    default: return -1;
    }
}

void GPIOController::setPinMode(int pin, int mode)
{
#ifdef RASPBERRY_PI_BUILD
    pinMode(pin, mode);                 // WiringPi 핀 모드 설정
#else
    Q_UNUSED(pin)
    Q_UNUSED(mode)
    // 시뮬레이션에서는 아무것도 하지 않음
#endif
}

void GPIOController::digitalWrite(int pin, int value)
{
#ifdef RASPBERRY_PI_BUILD
    ::digitalWrite(pin, value);         // WiringPi 디지털 출력
#else
    Q_UNUSED(pin)
    Q_UNUSED(value)
    // 시뮬레이션에서는 아무것도 하지 않음
#endif
}

int GPIOController::digitalRead(int pin)
{
#ifdef RASPBERRY_PI_BUILD
    return ::digitalRead(pin);          // WiringPi 디지털 입력
#else
    Q_UNUSED(pin)
    return LOW;                         // 시뮬레이션에서는 항상 LOW 반환
#endif
}
