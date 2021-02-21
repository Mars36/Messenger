#include "server.h"

Server::Server( QObject* parent ) : QObject( parent )
{
    TcpServer = new QTcpServer( this );
    connect( TcpServer,  &QTcpServer::newConnection, this, &Server::slotNewConnection );
    connect( this, &Server::AuthenticateUsers, &Server::slotAuthenticateUsers );
    connect( this, &Server::AddUserToDataBase, &Server::slotAddUserToDataBase );

    logConsole->P7_INFO( 0, TM( "Server start" ) );

    int Port = 2323;

    if ( !TcpServer->listen( QHostAddress::Any, Port ) ) {
        TcpServer->close();
        logConsole->P7_CRITICAL( 0, TM( "Error of listen port" ) );
        return;
    }

    logConsole->P7_INFO( 0, TM( "Waiting for connections" ) );

    QString path = QDir::currentPath() + "/database.db";
    QFileInfo checkDataBase( path );

    if ( !( checkDataBase.exists() && checkDataBase.isFile() ) ) {
        dataBase.setDatabaseName( path );
        QSqlQuery query;
        dataBase.open();
        query.exec( sqlCommand[0] + sqlNameTable[0] + "(" + sqlVariable[0] + "," + sqlVariable[1] + "," + sqlVariable[2] + "," +
                    sqlVariable[3] + ")" );

        query.exec( sqlCommand[0] + sqlNameTable[1] + "(" + sqlVariable[0] + "," + sqlVariable[4] + "," + sqlVariable[5] + "," +
                    sqlVariable[6] + ")" );
        logConsole->P7_INFO( 0, TM( "The database was created" ) );
    } else {
        dataBase.setDatabaseName( path );
        dataBase.open();
        logConsole->P7_INFO( 0, TM( "Database is exists" ) );
    }
}


void Server::sendToClient( QTcpSocket* Socket, const QString& command, const QString& message )
{
    QByteArray  arrBlock;
    QDataStream out( &arrBlock, QIODevice::WriteOnly );
    out << QTime::currentTime() << command + "|" + message;
    Socket->write( arrBlock );
}

Server::UserData Server::FindUser( QTcpSocket* Socket )
{
    for ( int i = 0; i < UsersInOnline.length(); i++ ) {
        if ( UsersInOnline[i].Socket == Socket ) {
            return UsersInOnline[i];
        }
    }

    UserData ReturnErrorName;
    ReturnErrorName.Name = "Error";
    return ReturnErrorName;
}

Server::UserData Server::FindUser( QString UserName )
{
    for ( int i = 0; i < UsersInOnline.length(); i++ ) {
        if ( UsersInOnline[i].Name == UserName ) {
            return UsersInOnline[i];
        }
    }

    UserData ReturnErrorName;
    ReturnErrorName.Name = "Error";
    return ReturnErrorName;
}

void Server::slotNewConnection()
{
    QTcpSocket* ClientSocket = TcpServer->nextPendingConnection();
    logConsole->P7_INFO( 0, TM( "New Connect" ) );
    connect( ClientSocket, &QAbstractSocket::disconnected, this, &Server::slotDeleteUserInOnline );
    connect( ClientSocket, &QAbstractSocket::disconnected, ClientSocket, &QAbstractSocket::deleteLater );
    connect( ClientSocket, &QIODevice::readyRead, this,  &Server::slotReadClient );
    connect( this, &Server::SendUnreadMessage, this,  &Server::slotSendUnreadMessage );
}

void Server::slotReadClient()
{
    QTcpSocket* ClientSocket = ( QTcpSocket* )sender();
    QDataStream in( ClientSocket );
    in.setVersion( QDataStream::Qt_4_2 );
    QTime time;
    QString strLogMessage;
    in >> time >> strLogMessage;
    QStringList UserMessage = strLogMessage.split( "|" );

    if ( UserMessage[0] == "Login" ) {
        slotAuthenticateUsers( ClientSocket, UserMessage[1] );
    } else if ( UserMessage[0] == "FindContact" ) {
        emit UserFindInDataBase( UserMessage[1], ClientSocket );
    } else if ( UserMessage[0] == "recoverContactList" ) {
        slotSendListContacts( ClientSocket );
    } else if ( UserMessage[0] == "addContactInPersonalList" ) {
        slotAddUserToContactList( ClientSocket, UserMessage[1] );
    } else if ( UserMessage[0] == "recoverPersonalContactList" ) {
        slotSendPersonalListContact( ClientSocket );
    } else if ( UserMessage[0] == "Message" ) {
        UserData userData;
        userData = FindUser( UserMessage[1] );

        if ( userData.Name != "Error" ) {
            sendToClient( FindUser( UserMessage[1] ).Socket, "fromUser", UserMessage[2] + "|" + UserMessage[3] );
        } else {
            slotSaveMessage( FindUser( UserMessage[2] ), UserMessage[1], UserMessage[3] );
        }
    } else if ( UserMessage[0] == "sendUnreadMessage" ) {
        slotSendUnreadMessage( FindUser( UserMessage[2] ), "unreadMessage", UserMessage[1] );
    } else if ( UserMessage[0] == "disconnect" ) {
        emit slotDeleteUserInOnline();
        logConsole->P7_INFO( 0, TM( "Waiting for connections" ) );
    } else {
        emit WriteHistoryToDataBase( strLogMessage, FindUser( ClientSocket ) );
    }
}

void Server::slotAuthenticateUsers( QTcpSocket* Socket, const QString& str )
{
    UserData userData;
    userData.Socket = Socket;
    userData.Name = str;
    QSqlQuery query;
    QString usersInDataBase;
    query.exec( sqlCommand[2] + sqlNameTable[0] + sqlCommand[4] + sqlNameAttributes[0] + "= '" + userData.Name + "'" );
    QSqlRecord record = query.record();
    const int nameIndex = record.indexOf( sqlNameAttributes[0] );
    bool inDataBase = false, inOnline = false;
    query.next();

    if ( !query.value( nameIndex ).isNull() ) {
        inDataBase = true;
    }

    for ( int i = 0; i < UsersInOnline.length(); i++ ) {
        if ( userData.Name == UsersInOnline[i].Name ) {
            inOnline = true;
        }
    }

    if ( !inOnline ) {
        if ( inDataBase ) {
            logConsole->P7_INFO( 0, TM( "User is in data base" ) );
            UsersInOnline << userData;
        } else {
            slotAddUserToDataBase( userData );
        }

        UsersInOnline << userData;
    } else {
        sendToClient( Socket, "errorLogin", "This user is already logged in" );
    }

    slotSendListContacts( Socket );
}

void Server::slotDeleteUserInOnline()
{
    QTcpSocket* SenderPoint = ( QTcpSocket* ) sender();

    for ( int i = 0; i < UsersInOnline.length(); i++ ) {
        if ( UsersInOnline[i].Socket == SenderPoint ) {
            UsersInOnline.removeAt( i );
            break;
        }
    }
}

void Server::slotAddUserToDataBase( Server::UserData userData )
{
    QSqlQuery query;
    query.prepare( sqlCommand[1] + sqlNameTable[0] + "(" + sqlNameAttributes[0] + "," + sqlNameAttributes[1] + ")" +
                   sqlCommand[3] + "(" + ":" + sqlNameAttributes[0] + "," + ":" + sqlNameAttributes[1] + ")" );
    query.bindValue( ":" + sqlNameAttributes[0], userData.Name );
    QString test = "tests";
    query.bindValue( ":" + sqlNameAttributes[1], test ) ;

    if ( !query.exec() ) {
        logConsole->P7_CRITICAL( 0, TM( "Error sql" ) );
    } else {
        logConsole->P7_INFO( 0, TM( "User inserted in data base" ) );
    }

}

void Server::slotSendListContacts( QTcpSocket* Socket )
{
    QSqlQuery query, queryContacts;
    QString usersInDataBase;
    int count = 0;
    query.exec( sqlCommand[2] + sqlNameTable[0] );
    QSqlRecord record = query.record();
    int nameIndex = record.indexOf( sqlNameAttributes[0]  );

    while ( query.next() ) {
        queryContacts.exec( sqlCommand[2] + sqlNameTable[1] + sqlCommand[4] + sqlNameAttributes[3] + "= '" + query.value(
                                nameIndex ).toString()
                            + "'" + sqlCommand[5] + sqlNameAttributes[4] + " = '" + FindUser( Socket ).Name + "'" + sqlCommand[5] +
                            sqlNameAttributes[5] + " != 'null'" );
        QSqlRecord recordContacts = queryContacts.record();
        int unreadMessageIndex = recordContacts.indexOf( sqlNameAttributes[5] );

        if ( !queryContacts.exec() ) {
            logConsole->P7_CRITICAL( 0, TM( "Error sql" ) );
        }

        queryContacts.next();
        QStringList unreadMessage = queryContacts.value( unreadMessageIndex ).toString().split( "%" );

        for ( int i = 0 ; i < unreadMessage.size(); i++ ) {
            count++;
        }

        count--;

        if ( count > 0 ) {
            usersInDataBase += query.value( nameIndex ).toString() + " (" + QString::number( count ) + ")" + "|";
        } else {
            usersInDataBase += query.value( nameIndex ).toString() + "|";
        }

        count = 0;
    }

    sendToClient( Socket, "recoverContactList", usersInDataBase );
}

void Server::slotSendPersonalListContact( QTcpSocket* Socket )
{
    QSqlQuery queryContacts;
    QString personalContactsList;
    queryContacts.exec( sqlCommand[2] + sqlNameTable[1] + sqlCommand[4] + sqlNameAttributes[3] + " = '" + FindUser(
                            Socket ).Name + "'" );
    QSqlRecord recordContacts = queryContacts.record();
    int contactIndexContacts = recordContacts.indexOf( sqlNameAttributes[4]  );
    queryContacts.exec();

    while ( queryContacts.next() ) {
        personalContactsList += queryContacts.value( contactIndexContacts ).toString() + "|";
    }

    sendToClient( Socket, "PersonalContactList", personalContactsList );
}

void Server::slotAddUserToContactList( QTcpSocket* Socket, QString nameUserAdd )
{
    QSqlQuery queryUsers, queryContacts;

    queryUsers.exec( sqlCommand[2] + sqlNameTable[0] );
    QSqlRecord recordUsers = queryUsers.record();
    queryUsers.exec();

    queryContacts.exec( sqlCommand[2] + sqlNameTable[1] + sqlCommand[4] + sqlNameAttributes[3] + " = '" + FindUser(
                            Socket ).Name + "' " + sqlCommand[5] + sqlNameAttributes[4] + "= '" +
                        nameUserAdd + "'" );
    QSqlRecord recordContacts = queryContacts.record();
    int userIndexContacts = recordContacts.indexOf( sqlNameAttributes[3]  );

    if ( !queryContacts.exec() ) {
        logConsole->P7_CRITICAL( 0, TM( "Error sql" ) );
    }

    queryContacts.next();

    if ( queryContacts.value( userIndexContacts ).isNull() ) {
        queryUsers.prepare( sqlCommand[1] + sqlNameTable[1] + "(" + sqlNameAttributes[4] + "," + sqlNameAttributes[3]  +
                            ")"
                            + sqlCommand[3] + "(" + ":" + sqlNameAttributes[4] + "," + ":" + sqlNameAttributes[3] + ")" );

        queryUsers.bindValue( ":" + sqlNameAttributes[4],  nameUserAdd );
        queryUsers.bindValue( ":" + sqlNameAttributes[3],  FindUser( Socket ).Name );

        if ( !queryUsers.exec() ) {
            logConsole->P7_CRITICAL( 0, TM( "Error sql" ) );
        } else {
            logConsole->P7_INFO( 0, TM( "User inserted in contact list" ) );
        }

        sendToClient( Socket, "successfulAddContact", "Сontact added successfully" );
    } else {
        sendToClient( Socket, "errorAddContact", "Contact has already been added" );
    }
}

void Server::slotSaveMessage( Server::UserData userData, QString recipientsMessage, QString Message )
{
    QSqlQuery queryContacts( sqlCommand[2] + sqlNameTable[1] + " WHERE User ='" + userData.Name + "'" + " AND Contact ='" +
                             recipientsMessage + "'" );
    QSqlRecord recordContacts = queryContacts.record();
    int userIndexContacts = recordContacts.indexOf( sqlNameAttributes[3]  );
    int pastMessagesIndexContacts = recordContacts.indexOf( sqlNameAttributes[5]  );

    if ( !queryContacts.exec() ) {
        logConsole->P7_CRITICAL( 0, TM( "Errol sql" ) );
    }

    queryContacts.next();

    if (  !queryContacts.value( userIndexContacts ).isNull() ) {
        QString MessageforDataBase = queryContacts.value( pastMessagesIndexContacts ).toString() + Message + "%";
        queryContacts.prepare( sqlCommand[6] + sqlNameTable[1] + sqlCommand[7] + sqlNameAttributes[5] + "=:" +
                               sqlNameAttributes[5] + sqlCommand[4] + sqlNameAttributes[3] + "=:" + sqlNameAttributes[3] +
                               sqlCommand[5] + sqlNameAttributes[4] + "=:" + sqlNameAttributes[4] );
        queryContacts.bindValue( ":" + sqlNameAttributes[3],
                                 userData.Name );
        queryContacts.bindValue( ":" + sqlNameAttributes[4],
                                 recipientsMessage );
        queryContacts.bindValue( ":" + sqlNameAttributes[5],
                                 MessageforDataBase );


        if ( !queryContacts.exec() ) {
            logConsole->P7_CRITICAL( 0, TM( "Error sql" ) );
        } else {
            logConsole->P7_INFO( 0, TM( "Save Message" ) );
        }
    }
}

void Server::slotSendUnreadMessage( Server::UserData userData, QString command, QString sender )
{
    QSqlQuery queryContacts;
    QString unreadMessages;
    queryContacts.exec( sqlCommand[2] + sqlNameTable[1] + " WHERE User = '" + sender + "' AND Contact = '" +
                        userData.Name + "'" );
    QSqlRecord recordContacts = queryContacts.record();
    int pastMessagesIndexContacts = recordContacts.indexOf( sqlNameAttributes[5]  );

    if ( !queryContacts.exec() ) {
        logConsole->P7_CRITICAL( 0, TM( "Error sql" ) );
    }

    queryContacts.next();
    unreadMessages = queryContacts.value( pastMessagesIndexContacts ).toString();
    sendToClient( userData.Socket, command, unreadMessages );

    queryContacts.prepare( sqlCommand[6] + sqlNameTable[1] + sqlCommand[7] +  sqlNameAttributes[5] + "=:"
                           +  sqlNameAttributes[5] + sqlCommand[4] + sqlNameAttributes[3] + "=:" + sqlNameAttributes[3] + sqlCommand[5] +
                           sqlNameAttributes[4] + "=:" +
                           sqlNameAttributes[4]
                         );
    queryContacts.bindValue( ":" + sqlNameAttributes[5], "" );
    queryContacts.bindValue( ":" + sqlNameAttributes[4],  FindUser( userData.Socket ).Name );
    queryContacts.bindValue( ":" + sqlNameAttributes[3],  sender );

    if ( !queryContacts.exec() ) {
        logConsole->P7_CRITICAL( 0, TM( "Error sql" ) );
    } else {
        logConsole->P7_INFO( 0, TM( "Clear unread message" ) );
    }
}

void Server::slotSendCountUnreadMessage( Server::UserData userData )
{
    QSqlQuery queryContacts;
    QSqlRecord recordContacts = queryContacts.record();
    int count = 0;
    int userIndexContacts = recordContacts.indexOf( sqlNameAttributes[3]  );
    int pastMessagesIndexContacts = recordContacts.indexOf( sqlNameAttributes[5]  );

    if ( !queryContacts.exec() ) {
        logConsole->P7_CRITICAL( 0, TM( "Error sql" ) );
    }

    queryContacts.next();
    QStringList listUnreadMessage = queryContacts.value( pastMessagesIndexContacts ).toString().split( "%" );
    count = listUnreadMessage.size();
    QString result = queryContacts.value( userIndexContacts ).toString() + "%" + QString::number( count );
    sendToClient( userData.Socket, "countUnreadMessage", result );
}
