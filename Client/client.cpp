#include "client.h"
#include "ui_client.h"
#include <QTcpSocket>
#include <QTime>
#include <QString>
//192.168.1.152
Client::Client(QWidget *parent) :
    QMainWindow(parent),
    Form(new Ui::Client)
{
    Form->setupUi(this);

    socket = new QTcpSocket(this);

    connect(Form->ConnectButton,SIGNAL(clicked(bool)),this,SLOT(ConnectToServer()));
    connect(Form->DisconnectButton,SIGNAL(clicked(bool)),this,SLOT(SlotDisconnection()));
    connect(Form->SendButton,SIGNAL(clicked(bool)),this,SLOT(SendMessageToServer()));
    connect(Form->ChatLine,SIGNAL(returnPressed()),this,SLOT(SendMessageToServer()));
    connect(Form->ClearButton,SIGNAL(clicked(bool)),this,SLOT(ClearAll()));
    connect(socket,SIGNAL(connected()),this,SLOT(SlotConnection()));
    connect(socket,SIGNAL(readyRead()),this,SLOT(SlotRead()));
    connect(socket,SIGNAL(disconnected()),this,SLOT(DisconnectFromServer()));
    OpenServerInfo();
}

Client::~Client()
{
    message.clear();
    delete Form;
}

QString Client::CurrentTime()
{
    QTime t(QTime::currentTime());
    QString str;
    str = t.toString("hh:mm:ss");
    return str;
}

void Client::ConnectToServer()
{
    if (Form->NickNameEdit->text().isEmpty())
    {
        message = CurrentTime() + " Please enter you nickname";
        Form->textEdit->setTextColor(Qt::gray);
        Form->textEdit->append(message);
    }
    else
        socket->connectToHost(Form->IDEdit->text(),Form->PortSpinBox->value());
}

void Client::SlotConnection()
{
    Form->textEdit->setTextColor(Qt::gray);
    message = CurrentTime() + " You are connected to the server...";
    Form->textEdit->append(message);

    QByteArray  Block;
    QDataStream out(&Block, QIODevice::WriteOnly);
    out << quint16(0) << SERVICE_MSG << Form->NickNameEdit->text();
    out.device()->seek(0);
    out << quint16(Block.size() - sizeof(quint16));
    socket->write(Block);

    Form->SendButton->setEnabled(true);
    Form->DisconnectButton->setEnabled(true);
    Form->ConnectButton->setEnabled(false);
    Form->Who->addItem("All");
}


void Client::DisconnectFromServer()
{
    message = CurrentTime() + " Disconnected from server...";
    Form->textEdit->setTextColor(Qt::gray);
    Form->textEdit->append(message);
    Form->UserList->clear();
    Form->Who->clear();
    Form->DisconnectButton->setEnabled(false);
    Form->ConnectButton->setEnabled(true);
    Form->SendButton->setEnabled(false);
    socket->close();
}

void Client::SlotDisconnection()
{
    socket->disconnectFromHost();
}

void Client::SlotRead()
{
    QDataStream in(socket);
    quint16 NextBlockSize(0);
    for (;;)
    {
        if (!NextBlockSize)
        {
            if (socket->bytesAvailable() < sizeof(quint16))
            {
                break;
            }
            in >> NextBlockSize;
        }

        if (socket->bytesAvailable() < NextBlockSize)
        {
               break;
        }

        QString str;
        int msg_type;
        in >> msg_type;
        in >> str;

        switch (msg_type)
        {
        case SERVICE_MSG:
        {
            if (str == "")
                return;
            QStringList list =  str.split(",");
            Form->UserList->addItems(list);
            Form->Who->addItems(list);
            break;
        }

        case REGULAR_MSG:
            message = str;
            Form->textEdit->setTextColor(Qt::black);
            Form->textEdit->append(message);
            break;

        case PRIVATE_MSG:
            message = str;
            Form->textEdit->setTextColor(Qt::red);
            Form->textEdit->append(message);
            break;

        case USER_LEFT:
            DeleteUserFromGui(str);
            break;

        case USER_JOIN:
            Form->UserList->addItem(str);
            Form->Who->addItem(str);
            Form->textEdit->setTextColor(Qt::gray);
            message = CurrentTime() + " User [ " + str + " ] is connection to server. " ;
            Form->textEdit->append(message);
            break;

        case AUTHO_ERROR:
            message = CurrentTime() + " Name is not available";
            Form->textEdit->setTextColor(Qt::gray);
            Form->textEdit->append(message);
            socket->disconnectFromHost();
            break;
        }
     }

}

void Client::ClearAll()
{
    Form->textEdit->clear();
}

void Client::on_actionSave_triggered()
{
    QFile File("ServerInfo.txt");
    File.open(QIODevice::WriteOnly);
    QTextStream out(&File);
    out << Form->IDEdit->text() << " " << Form->PortSpinBox->value();
    File.close();
}

void Client::OpenServerInfo()
{
    QFile File("ServerInfo.txt");
    File.open(QIODevice::ReadOnly);
    QTextStream in(&File);
    QString Ip;
    int port;
    in >> Ip;
    in >> port;
    Form->IDEdit->setText(Ip);
    Form->PortSpinBox->setValue(port);
    File.close();
}
void Client::SendMessageToServer()
{
    if (Form->Who->currentText()== "All")
        SendMessageToAll();
    else
    {
        QString UserName;
        UserName = Form->Who->currentText();
        SendToPrivateMessage(UserName);
    }
}

void Client::SendMessageToAll()
{
    QByteArray  Block;
    QDataStream out(&Block, QIODevice::WriteOnly);
    out << quint16(0) << REGULAR_MSG << Form->ChatLine->text();
    out.device()->seek(0);
    out << quint16(Block.size() - sizeof(quint16));
    socket->write(Block);
    Form->ChatLine->setText("");
}

void Client::SendToPrivateMessage(QString UserName)
{
    QByteArray  Block;
    QDataStream out(&Block, QIODevice::WriteOnly);
    out << quint16(0) << PRIVATE_MSG << UserName << Form->ChatLine->text();
    out.device()->seek(0);
    out << quint16(Block.size() - sizeof(quint16));
    socket->write(Block);
    Form->ChatLine->setText("");
}

void Client::DeleteUserFromGui(QString UserName)
{
    message = CurrentTime() + " User [ " + UserName + " ] disconected from server.";
    Form->textEdit->setTextColor(Qt::gray);
    Form->textEdit->append(message);

    for (int i = 0; i < Form->UserList->count(); i++)
        if (Form->UserList->item(i)->text() == UserName)
        {
            QListWidgetItem* item = Form->UserList->takeItem(i);
            delete item;
            Form->Who->removeItem(i+1);
            break;
        }

}

