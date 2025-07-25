#ifndef BBOXRECEIVER_H
#define BBOXRECEIVER_H

#include <QObject>
#include <QSslSocket>
#include <QByteArray>     // QByteArray 사용
#include <QJsonDocument>  // JSON 파싱
#include <QJsonObject>
#include <QJsonArray>
#include <QList>          // QList<BBox>

#include <QDataStream>     // 길이 파싱용 (BigEndian)
#include <QtEndian>        // qFromBigEndian
#include <QDebug>          // 디버깅 로그 출력

#include "BBox.h"

class BBoxReceiver : public QObject {
    Q_OBJECT
public:
    explicit BBoxReceiver(QSslSocket *socket, QObject *parent = nullptr);

public slots:
    void startReceiving();
    void stopReceiving();
    void onReadyRead();

signals:
    void bboxesParsed(QList<BBox> boxes, qint64 timestamp);

private:
    QSslSocket *m_socket;
    bool m_running = false;
    QByteArray m_buffer;

    QByteArray readExactly(int n);
};

#endif
