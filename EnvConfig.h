#ifndef ENVCONFIG_H
#define ENVCONFIG_H

#include <QString>
#include <QMap>

class EnvConfig
{
public:
    static void loadFromFile(const QString &filePath = ".env");
    static QString getValue(const QString &key, const QString &defaultValue = "");
    static int getIntValue(const QString &key, int defaultValue = 0);
    static bool getBoolValue(const QString &key, bool defaultValue = false);

private:
    static QMap<QString, QString> m_envVars;
    static void parseLine(const QString &line);
};

#endif // ENVCONFIG_H
