#include "server.h"
#include "ui_server.h"
#include <QTime>
#include <QTcpServer>
#include <QTcpSocket>

Server::Server(QWidget *parent) :
    QMainWindow(parent),
    Form(new Ui::Server)
{
    Form->setupUi(this);
    socket = new QTcpServer(this);
    connect(Form->StartButton,SIGNAL(clicked(bool)),this,SLOT(StartServer()));
    connect(Form->StopButton,SIGNAL(clicked(bool)),this,SLOT(StopServer()));
    connect(Form->ClearButton,SIGNAL(clicked(bool)),this,SLOT(ClearAll()));
    connect(Form->UserList,SIGNAL(clicked(QModelIndex)),this,SLOT(ToolTip()));
    connect(socket,SIGNAL(newConnection()),this,SLOT(NewConnectionClient()));
    OpenServerInfo();
}

Server::~Server()
{
    message.clear();
    delete Form;
}

void Server::StartServer()
{
    if (socket->listen(QHostAddress::Any,Form->PortSpinBox->value()))
    {
        Form->textEdit->setTextColor(Qt::gray);
        message = CurrentTime() + " Server is running...";
        Form->textEdit->append(message);
        Form->StartButton->setEnabled(false);
        Form->StopButton->setEnabled(true);
    }
    else
    {
        Form->textEdit->setTextColor(Qt::gray);
        message = CurrentTime() + " Connection error...";
        Form->textEdit->append(message);
        socket->close();
    }
}

void Server::StopServer()
{

    foreach(int i, Clients.keys())
    {
        Clients[i]->close();
        Clients.remove(i);
    }

    foreach(int i, UserName.keys())
    {
        UserName[i].clear();
        UserName.remove(i);
    }

    Form->textEdit->setTextColor(Qt::gray);
    message = CurrentTime() + " Server is down...";
    Form->textEdit->append(message);
    Form->StartButton->setEnabled(true);
    Form->StopButton->setEnabled(false);
    Form->UserList->clear();
    socket->close();
}

void Server::ClearAll()
{
    Form->textEdit->clear();
}

QString Server::CurrentTime()
{
    QTime t(QTime::currentTime());
    QString str;
    str = t.toString("hh:mm:ss");
    return str;
}

void Server::NewConnectionClient()
{
    QTcpSocket *tcp_socket = socket->nextPendingConnection();
    int ID_UserSoc = tcp_socket->socketDescriptor();
    Clients[ID_UserSoc] = tcp_socket;
    connect(Clients[ID_UserSoc],SIGNAL(readyRead()),this,SLOT(SlotRead()));
    connect(Clients[ID_UserSoc],SIGNAL(disconnected()),this,SLOT(UserDisconnected()));
    connect(Clients[ID_UserSoc],SIGNAL(disconnected()),Clients[ID_UserSoc],SLOT(deleteLater()));
}

void Server::SlotRead()
{
    QTcpSocket *tcp_socket = (QTcpSocket*)sender();
    QDataStream in(tcp_socket);
    int ID_UserSoc = tcp_socket->socketDescriptor();
    quint16 NextBlockSize(0);
    for (;;)
    {
        if (!NextBlockSize)
        {
            if (tcp_socket->bytesAvailable() < sizeof(quint16))
            {
                break;
            }
            in >> NextBlockSize;
        }

        if (tcp_socket->bytesAvailable() < NextBlockSize)
        {
               break;
        }

        QString str, user;
        int type_msg;
        in >> type_msg;

        switch (type_msg)
        {
        case SERVICE_MSG:
            in >> str;

            foreach(int i, UserName.keys())
                if (UserName[i] == str)
                {
                    ErrorAutho(ID_UserSoc);
                    return;
                }
            UserName[ID_UserSoc] = str;
            TimeOnline[ID_UserSoc] = QDateTime::currentMSecsSinceEpoch() / 1000 ;
            Form->UserList->addItem(UserName[ID_UserSoc]);
            Form->textEdit->setTextColor(Qt::gray);
            message = CurrentTime() + " Client [ " + UserName[ID_UserSoc] + " ] is connection to server. " ;
            Form->textEdit->append(message);
            SendUsersDatabase(ID_UserSoc);
            SendUsersJoin(ID_UserSoc);
            break;

        case REGULAR_MSG:
            in >> str;
            Form->textEdit->setTextColor(Qt::black);
            message = CurrentTime() + " [" + UserName[ID_UserSoc] + "] to [ All ] " + str;
            Form->textEdit->append(message);
            SendToAll();
            break;

        case PRIVATE_MSG:
            int UserId;
            in >> user;
            in >> str;
            Form->textEdit->setTextColor(Qt::red);
            message = CurrentTime() + " [" + UserName[ID_UserSoc] + "] to [ " + user + " ] " + str;
            Form->textEdit->append(message);

            foreach(int i, UserName.keys())
            {
                if (UserName[i] == user)
                  UserId = i;
            }
            SendToPrivate(UserName[ID_UserSoc]);
            SendToPrivate(UserName[UserId]);
            break;

        break;
        }
    }

}

void Server::UserDisconnected()
{
    foreach (int i, Clients.keys())
    {
        if (Clients[i]->state() == QAbstractSocket::UnconnectedState)
        {
            Clients.remove(i);

            if (UserName[i]!= "")
            {
                DeleteUserFromGui(UserName[i]);
                SendUserLeft(UserName[i]);
                UserName.remove(i);
            }
        }
    }
}

void Server::SendToAll()
{
    foreach(int i, Clients.keys())
    {
        QByteArray  Block;
        QDataStream out(&Block, QIODevice::WriteOnly);
        out << quint16(0) << REGULAR_MSG << message;
        out.device()->seek(0);
        out << quint16(Block.size() - sizeof(quint16));
        Clients[i]->write(Block);
    }
}

void Server::SendToPrivate(QString ClientName)
{
    int UserId;
    foreach(int i, UserName.keys())
    {
        if (UserName[i] == ClientName)
          UserId = i;
    }

    QByteArray  Block;
    QDataStream out(&Block, QIODevice::WriteOnly);
    out << quint16(0) << PRIVATE_MSG << message;
    out.device()->seek(0);
    out << quint16(Block.size() - sizeof(quint16));
    Clients[UserId]->write(Block);
}

void Server::SendUsersDatabase(int UserId)
{
    QStringList list;
    foreach (int i, Clients.keys())
    {
        if (i != UserId)
            list << UserName[i];
    }
    QString str;
    for (int i = 0; i < list.length(); i++)
            str = str + list.at(i)+(QString)",";
    str.remove(str.length()-1, 1);

    QByteArray  Block;
    QDataStream out(&Block, QIODevice::WriteOnly);
    out << quint16(0) << SERVICE_MSG << str;
    out.device()->seek(0);
    out << quint16(Block.size() - sizeof(quint16));
    Clients[UserId]->write(Block);
}

void Server::SendUsersJoin(int UserId)
{
    foreach (int i, UserName.keys())
    {
        if (i != UserId)
        {
            QByteArray  Block;
            QDataStream out(&Block, QIODevice::WriteOnly);
            out << quint16(0) << USER_JOIN << UserName[UserId];
            out.device()->seek(0);
            out << quint16(Block.size() - sizeof(quint16));
            Clients[i]->write(Block);
        }
   }
}

void Server::on_actionSave_triggered()
{
    QFile File("ServerInfo.txt");
    File.open(QIODevice::WriteOnly);
    QTextStream out(&File);
    out << Form->IPEdit->text() << " " << Form->PortSpinBox->value();
    File.close();
}

void Server::OpenServerInfo()
{
    QFile File("ServerInfo.txt");
    File.open(QIODevice::ReadOnly);
    QTextStream in(&File);
    QString Ip;
    int port;
    in >> Ip;
    in >> port;
    Form->IPEdit->setText(Ip);
    Form->PortSpinBox->setValue(port);
    File.close();
}

void Server::SendUserLeft(QString ClientName)
{
    foreach(int i, Clients.keys())
    {
        QByteArray  Block;
        QDataStream out(&Block, QIODevice::WriteOnly);
        out << quint16(0) << USER_LEFT << ClientName;
        out.device()->seek(0);
        out << quint16(Block.size() - sizeof(quint16));
        Clients[i]->write(Block);
    }
}

void Server::DeleteUserFromGui(QString ClientName )
{
    message = CurrentTime() + " Client [ " + ClientName + " ] disconected from server.";
    Form->textEdit->setTextColor(Qt::gray);
    Form->textEdit->append(message);

    for (int i = 0; i < Form->UserList->count(); i++)
        if (Form->UserList->item(i)->text() == ClientName)
        {
            QListWidgetItem* item = Form->UserList->takeItem(i);
            delete item;
            break;
        }
}

void Server::ErrorAutho(int UserId)
{
    QByteArray  Block;
    QDataStream out(&Block, QIODevice::WriteOnly);
    out << quint16(0) << AUTHO_ERROR;
    out.device()->seek(0);
    out << quint16(Block.size() - sizeof(quint16));
    Clients[UserId]->write(Block);
}

void Server::on_UserList_itemClicked(QListWidgetItem *item)
{
    QString second, minute, hour;
    int ss, mm = 0, hh = 0;
    foreach (int i, UserName.keys())
        if (UserName[i] == item->text())
        {
            ss = QDateTime::currentMSecsSinceEpoch() / 1000 - TimeOnline[i];
        }
    if (ss > 60)
    {
        mm = ss / 60;
        ss = ss % 60;
        if (mm > 60)
        {
            hh = mm / 60;
            mm = mm % 60;
        }
    }
    hour.setNum(hh);
    minute.setNum(mm);
    second.setNum(ss);
    message = item->text() + " online: " + hour + " h " + minute + " m " + second + " s.";
    Form->UserList->setToolTip(message);
}
