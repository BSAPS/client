#ifndef NETWORKCONFIGDIALOG_H
#define NETWORKCONFIGDIALOG_H

#include <QDialog>
#include <QLineEdit>

class NetworkConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NetworkConfigDialog(QWidget *parent = nullptr);
    ~NetworkConfigDialog();

    QString getRtspUrl() const;
    void setRtspUrl(const QString &url);

    QString getTcpHost() const;
    void setTcpHost(const QString &host);

    int getTcpPort() const;
    void setTcpPort(int port);

private:
    void setupUI();

    QLineEdit *m_rtspUrlEdit;
    QLineEdit *m_tcpHostEdit;
    QLineEdit *m_tcpPortEdit;
};

#endif // NETWORKCONFIGDIALOG_H
