#ifndef ENVCONFIG_H
#define ENVCONFIG_H

#include <QString>
#include <QMap>

/**
 * @brief 환경설정 관리 클래스
 * @details .env 파일을 로드하고 환경 변수 값을 제공합니다.
 */
class EnvConfig
{
public:
    /**
     * @brief .env 파일 로드
     * @param filePath 파일 경로
     */
    static void loadFromFile(const QString &filePath = ".env");
    /**
     * @brief 환경 변수 값 반환
     * @param key 환경 변수명
     * @param defaultValue 기본값
     * @return 환경 변수 값
     */
    static QString getValue(const QString &key, const QString &defaultValue = "");
    /**
     * @brief 환경 변수 int 값 반환
     * @param key 환경 변수명
     * @param defaultValue 기본값
     * @return 환경 변수 int 값
     */
    static int getIntValue(const QString &key, int defaultValue = 0);
    /**
     * @brief 환경 변수 bool 값 반환
     * @param key 환경 변수명
     * @param defaultValue 기본값
     * @return 환경 변수 bool 값
     */
    static bool getBoolValue(const QString &key, bool defaultValue = false);

private:
    /** @brief 환경 변수 맵 */
    static QMap<QString, QString> m_envVars;
    /**
     * @brief 한 줄 파싱
     * @param line 파싱할 줄
     */
    static void parseLine(const QString &line);
};

#endif // ENVCONFIG_H
