#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QtNetwork>

namespace Ui {
class Client;
}

const int SERVICE_MSG = 0;
const int REGULAR_MSG = 1;
const int PRIVATE_MSG = 2;
const int USER_LEFT = 3;
const int USER_JOIN = 4;
const int AUTHO_ERROR = 5;

class Client : public QMainWindow
{
    Q_OBJECT

public:
    explicit Client(QWidget *parent = 0);
    ~Client();
    QString CurrentTime();
    void OpenServerInfo();
    void SendToPrivateMessage(QString UserName);
    void SendMessageToAll();
    void DeleteUserFromGui(QString UserName);

private:
    Ui::Client *Form;
    QString message;
    QTcpSocket *socket;

private slots:
    void ConnectToServer();
    void DisconnectFromServer();
    void SlotConnection();
    void SlotDisconnection();
    void SlotRead();
    void SendMessageToServer();
    void ClearAll();
    void on_actionSave_triggered();
};

#endif // CLIENT_H
