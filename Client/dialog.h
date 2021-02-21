#ifndef DIALOG_H
#define DIALOG_H

#include <QMainWindow>
#include "qtincludes.h"


QT_BEGIN_NAMESPACE
namespace Ui
{
class Dialog;
}
QT_END_NAMESPACE

class Dialog : public QMainWindow
{
    Q_OBJECT
private:
    QTcpSocket* TcpSocket;
    QTextEdit*  m_ptxtInfo;
    QLineEdit*  m_ptxtInput;
    QStandardItemModel* model = new QStandardItemModel;
    QStandardItem* item;
    QString pathFileUser = "UsersData";
    QString pathFileUserHistory = "UsersData";
    QString nameOftheAuthorizedUser;
    QTcpServer* TcpServer;
public:
    Dialog( QWidget* parent = nullptr );
    ~Dialog();
signals:
    void messageFromConnect( const QString& Host, int Port );
    void SendUserNameToServer();
private slots:
    void slotReadyRead();
    void slotError( QAbstractSocket::SocketError );
    void slotSendToServer( QString command, QString message );
    void slotConnected();
    void SlotSendUserNameToServer();
    void slotLossConnect();

    void on_SwitchToContactsMode_activated( int index );

    void on_Send_clicked();

    void on_Attach_clicked();

    void on_Connect_clicked();

    void on_AddContact_clicked();

    void on_listContacts_itemSelectionChanged();

    void on_InputChat_textEdited( const QString& arg1 );

    void on_listContacts_currentTextChanged( const QString& currentText );

private:
    Ui::Dialog* ui;
};
#endif // DIALOG_H
