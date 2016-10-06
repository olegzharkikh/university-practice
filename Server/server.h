#ifndef SERVER_H
#define SERVER_H

#include <QMainWindow>
#include <QtNetwork>
#include <QTcpServer>
#include <QTcpSocket>
#include <QListWidgetItem>

namespace Ui {
class Server;
}

const int SERVICE_MSG = 0;
const int REGULAR_MSG = 1;
const int PRIVATE_MSG = 2;
const int USER_LEFT = 3;
const int USER_JOIN = 4;
const int AUTHO_ERROR = 5;

class Server : public QMainWindow
{
    Q_OBJECT

public:
    explicit Server(QWidget *parent = 0);
    ~Server();
    QString CurrentTime();
    void SendToAll();
    void SendToPrivate(QString ClientName);
    void SendUsersDatabase(int UserId);
    void SendUsersJoin(int UserId);
    void SendUserLeft(QString ClientName);
    void OpenServerInfo();
    void DeleteUserFromGui(QString ClientName);
    void ErrorAutho(int UserId);

private:
    Ui::Server *Form;
    QTcpServer *socket;
    QString message;
    QMap<int, QTcpSocket *> Clients;
    QMap<int, QString> UserName;
    QMap<int, quint64> TimeOnline;

private slots:
    void StartServer();
    void StopServer();
    void ClearAll();
    void ToolTip();
    void NewConnectionClient();
    void SlotRead();
    void UserDisconnected();
    void on_actionSave_triggered();
    void on_UserList_itemClicked(QListWidgetItem *item);
};

#endif // SERVER_H
