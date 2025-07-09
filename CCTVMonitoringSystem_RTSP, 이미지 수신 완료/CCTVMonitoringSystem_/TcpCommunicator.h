#ifndef TCPCOMMUNICATOR_H
#define TCPCOMMUNICATOR_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QPoint>
#include <QTimer>

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
    void onProcessingTimeout();

private:
    void processReceivedData(const QByteArray &data);
    void processImagesResponse(const QJsonObject &json);
    void processBinaryImageData(const QByteArray &data);
    void processImageBuffer();
    void tryExtractJpegImages(QList<ImageData> &imageList, const QString &tempDir);
    void saveRawDataForDebug(const QByteArray &data);
    void tryDifferentImageFormats(const QByteArray &data, QList<ImageData> &imageList, const QString &tempDir);

    QTcpSocket *m_socket;
    bool m_isConnected;
    int m_currentRequestId;

    QByteArray m_receivedBuffer;
    QByteArray m_imageBuffer;
    qint64 m_expectedDataSize;
    qint64 m_receivedDataSize;

    QTimer *m_processingTimer;
    bool m_waitingForMoreData;
    int m_lastBufferSize;
};

#endif // TCPCOMMUNICATOR_H
