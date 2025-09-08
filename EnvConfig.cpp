#include "EnvConfig.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QCoreApplication>

/**
 * @brief 환경 변수 맵 초기화
 */
QMap<QString, QString> EnvConfig::m_envVars;

/**
 * @brief .env 파일 로드
 * @details 지정된 경로의 .env 파일을 읽어 환경 변수 맵에 저장합니다.
 * @param filePath 파일 경로
 */
void EnvConfig::loadFromFile(const QString &filePath)
{
    QString fullPath = QCoreApplication::applicationDirPath() + "/" + filePath;
    QFile file(fullPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[EnvConfig] .env 파일을 찾을 수 없습니다:" << fullPath;
        return;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        parseLine(line);
    }
    qDebug() << "[EnvConfig] .env 파일 로드 완료:" << m_envVars.size() << "개 변수";
}

/**
 * @brief 환경 변수 값 반환
 * @param key 환경 변수명
 * @param defaultValue 기본값
 * @return 환경 변수 값
 */
QString EnvConfig::getValue(const QString &key, const QString &defaultValue)
{
    return m_envVars.value(key, defaultValue);
}

/**
 * @brief 환경 변수 int 값 반환
 * @param key 환경 변수명
 * @param defaultValue 기본값
 * @return 환경 변수 int 값
 */
int EnvConfig::getIntValue(const QString &key, int defaultValue)
{
    bool ok;
    int value = m_envVars.value(key, QString::number(defaultValue)).toInt(&ok);
    return ok ? value : defaultValue;
}

/**
 * @brief 환경 변수 bool 값 반환
 * @param key 환경 변수명
 * @param defaultValue 기본값
 * @return 환경 변수 bool 값
 */
bool EnvConfig::getBoolValue(const QString &key, bool defaultValue)
{
    QString value = m_envVars.value(key, defaultValue ? "true" : "false").toLower();
    return (value == "true" || value == "1" || value == "yes");
}

/**
 * @brief 한 줄 파싱
 * @details 빈 줄이나 주석을 제외하고 KEY=VALUE 형식으로 파싱하여 환경 변수 맵에 저장합니다.
 * @param line 파싱할 줄
 */
void EnvConfig::parseLine(const QString &line)
{
    // 빈 줄이나 주석 제외
    if (line.isEmpty() || line.startsWith('#')) {
        return;
    }
    
    // KEY=VALUE 형식 파싱
    int equalIndex = line.indexOf('=');
    if (equalIndex > 0) {
        QString key = line.left(equalIndex).trimmed();
        QString value = line.mid(equalIndex + 1).trimmed();
        
        // 따옴표 제거
        if ((value.startsWith('"') && value.endsWith('"')) ||
            (value.startsWith('\'') && value.endsWith('\''))) {
            value = value.mid(1, value.length() - 2);
        }
        m_envVars[key] = value;
        qDebug() << "[EnvConfig]" << key << "=" << value;
    }
}
