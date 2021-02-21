#ifndef SERVER2_H
#define SERVER2_H

#include "qtincludes.h"

class Server : public QObject
{
    Q_OBJECT
private:
    struct UserData {
        QTcpSocket* Socket;
        QString    Name;
        QString    Password;
        QString    Token;
        int        id;
        bool       SendHistory = false;
    };
    QTcpServer* TcpServer;
    QList<UserData> UsersInOnline;
    QString bufferMessage = "";
    QSqlDatabase dataBase = QSqlDatabase::addDatabase( "QSQLITE" );
    QString sqlCommand[8] = {
        "create table ",     //0
        "INSERT INTO ",      //1
        "SELECT * FROM ",    //2
        "VALUES ",           //3
        " WHERE ",            //4
        " AND ",              //5
        "UPDATE ",               //6
        " SET ",                //7
    };
    QString sqlNameTable[2] = {"users", "contacts"};
    QString sqlVariable[7] = {
        """id integer primary key", //0
        """Name text(255)",          //1
        """Password text(255)",      //2
        """Token text(255)",         //3
        """User text(255)",          //4
        """Contact text(255)",       //5
        """SaveMessage text(255)",   //6
    };
    QString sqlNameAttributes[6] = {
        "Name",         //0
        "Password",     //1
        "Token",        //2
        "User",         //3
        "Contact",      //4
        "SaveMessage"   //5
    };
    IP7_Client* logClientForConsole = P7_Create_Client( TM( "/P7.Sink=Console /P7.Format=\"{%cn} [%tf] %lv %ms\"" ) );
    IP7_Client* logClientForFileTxt = P7_Create_Client( TM( "/P7.Sink=FileTxt /P7.Format=\"{%cn} [%tf] %lv %ms\"" ) );
    IP7_Trace* logConsole = P7_Create_Trace( logClientForConsole, TM( "Server log" ) );
    IP7_Trace* logFileTxt = P7_Create_Trace( logClientForFileTxt, TM( "Server log" ) );
private:
    void sendToClient( QTcpSocket* Socket, const QString& command, const QString& message );
    UserData FindUser( QTcpSocket* Socket );
    UserData FindUser( QString UserName );

public:
    explicit Server( QObject* parent = nullptr );
private slots:
    virtual void slotNewConnection();
    void slotReadClient();
    void slotAuthenticateUsers( QTcpSocket* Socket, const QString& str );
    void slotDeleteUserInOnline();
    void slotAddUserToDataBase( UserData UsersData );
    void slotSendListContacts( QTcpSocket* Socket );
    void slotSendPersonalListContact( QTcpSocket* Socket );
    void slotAddUserToContactList( QTcpSocket* Socket, QString nameUserAdd );
    void slotSaveMessage( Server::UserData userData, QString recipientsMessage, QString Message );
    void slotSendCountUnreadMessage( Server::UserData userData );
    void slotSendUnreadMessage( Server::UserData userData, QString command, QString sender );

signals:
    void AuthenticateUsers( QTcpSocket* Socket, const QString& str );
    void AddUserToDataBase( UserData UsersData );
    void WriteHistoryToDataBase( QString Messages, UserData dataUser );
    void UserFindInDataBase( QString nameUser, QTcpSocket* requestersSocket );
    void SendUnreadMessage( Server::UserData userData, QString command, QString sender  );
};

#endif // SERVER2_H
