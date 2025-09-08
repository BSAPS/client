#include "EnvConfig.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QCoreApplication>

QMap<QString, QString> EnvConfig::m_envVars;

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

QString EnvConfig::getValue(const QString &key, const QString &defaultValue)
{
    return m_envVars.value(key, defaultValue);
}

int EnvConfig::getIntValue(const QString &key, int defaultValue)
{
    bool ok;
    int value = m_envVars.value(key, QString::number(defaultValue)).toInt(&ok);
    return ok ? value : defaultValue;
}

bool EnvConfig::getBoolValue(const QString &key, bool defaultValue)
{
    QString value = m_envVars.value(key, defaultValue ? "true" : "false").toLower();
    return (value == "true" || value == "1" || value == "yes");
}

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
