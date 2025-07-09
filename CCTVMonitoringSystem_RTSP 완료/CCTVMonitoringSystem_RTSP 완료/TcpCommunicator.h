#ifndef TCPCOMMUNICATOR_H
#define TCPCOMMUNICATOR_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QPoint>

struct ImageData {
    QString imagePath;
    QString timestamp;
    QString logText;
};

class TcpCommunicator : public QObject
{
    Q_OBJECT

public:
    explicit TcpCommunicator(QObject *parent = nullptr);
    ~TcpCommunicator();

    void connectToServer(const QString &host, int port);
    void disconnectFromServer();
    bool isConnected() const;

    void sendLineCoordinates(const QPoint &start, const QPoint &end);
    void requestImagesByDateAndHour(const QString &date, int hour);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);
    void dataReceived(const QString &data);
    void packetReceived(int requestId, int success, const QString &data1, const QString &data2, const QString &data3);
    void imagesReceived(const QList<ImageData> &images);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onDataReceived();

private:
    void processReceivedData(const QByteArray &data);
    void processImagesResponse(const QJsonObject &json);

    QTcpSocket *m_socket;
    bool m_isConnected;
    int m_currentRequestId;

    QByteArray m_receivedBuffer;
    qint64 m_expectedDataSize;
    qint64 m_receivedDataSize;
};

#endif // TCPCOMMUNICATOR_H
