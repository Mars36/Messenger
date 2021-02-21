#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog( QWidget* parent )
    : QMainWindow( parent )
    , ui( new Ui::Dialog )
{
    ui->setupUi( this );
//    ui->Attach->setDisabled( true );
    ui->Send->setDisabled( true );
    ui->AddContact->setDisabled( true );

    if ( QFile::exists( ( "settings.ini" ) ) ) {
        QSettings settings( "settings.ini", QSettings::IniFormat );
        settings.beginGroup( "WidgetPosition" );
        int x = settings.value( "x", -1 ).toInt();
        int y = settings.value( "y", -1 ).toInt();
        int width = settings.value( "width", -1 ).toInt();
        int height = settings.value( "height", -1 ).toInt();
        settings.endGroup();
        this->setGeometry( x, y, width, height );
    }
}

Dialog::~Dialog()
{
    QSettings settings( "settings.ini", QSettings::IniFormat );
    settings.beginGroup( "WidgetPosition" );
    settings.setValue( "x", this->x() );
    settings.setValue( "y", this->y() );
    settings.setValue( "width", this->width() );
    settings.setValue( "height", this->height() );
    settings.endGroup();
    slotSendToServer("disconnect",NULL);
    delete ui;
}

void Dialog::slotReadyRead()
{
    QDataStream in( TcpSocket );
    QTime time;
    QString message;
    in >> time >> message;
    QStringList ServerMessage = message.split( "|" );

    if ( ServerMessage[0] == "errorLogin" ) {
        ui->Chat->setText( "\nServer->" + ServerMessage[1] );
        ui->Connect->show();
    } else if ( ServerMessage[0] == "loginSuccessful" ) {
        for ( int i = 1; i < ServerMessage.length(); i++ ) {
            if ( ( ServerMessage[i] != "" ) && ( nameOftheAuthorizedUser != ServerMessage[i] ) && ( ServerMessage[i][0] != "^" ) ) {
                ui->listContacts->addItem( ServerMessage[i] );
            }
        }

        ui->Chat->setText( "\nServer->The connection is made" );
    } else if ( ServerMessage[0] == "updateContactList" ) {
        ui->listContacts->addItem( ServerMessage[1] );
    } else if ( ServerMessage[0] == "recoverContactList" ) {
        for ( int i = 1; i < ServerMessage.length(); i++ ) {
            if ( ( ServerMessage[i] != "" ) && ( nameOftheAuthorizedUser != ServerMessage[i] ) ) {
                ui->listContacts->addItem( ServerMessage[i] );
            }
        }
    } else if ( ServerMessage[0] == "errorAddContact" ) {
        ui->Chat->setText( ui->Chat->text() + "\nServer->" + ServerMessage[1] );
    } else if ( ServerMessage[0] == "PersonalContactList" ) {
        for ( int i = 1; i < ServerMessage.length(); i++ ) {
            ui->listContacts->addItem( ServerMessage[i] );
        }
    } else if ( ServerMessage[0] == "fromUser" ) {
        ui->Chat->setText( ui->Chat->text() + "\n" + ServerMessage[1] + "->" + ServerMessage[2] );
    } else if ( ServerMessage[0] == "unreadMessage" ) {
        QStringList unreadMessage = ServerMessage[1].split( "%" );

        for ( int i = 0; i < unreadMessage.size(); i++ ) {
            ui->Chat->setText( ui->Chat->text() + "\n" + unreadMessage[i] );
        }
    } else if ( ServerMessage[0] == "successfulAddContact" ) {
        ui->Chat->setText( ui->Chat->text() + "\n" + ServerMessage[1] );
    } else if ( ServerMessage[0] == "countUnreadMessage" ) {
        QStringList countMessage = ServerMessage[1].split( "%" );

        for ( int i = 0; i < ui->listContacts->count(); i++ ) {
            if ( ui->listContacts->item( i )->text() == countMessage[0] ) {
                ui->listContacts->addItem( ui->listContacts->item( i )->text() + "(" + countMessage[1] + ")" );
            }
        }
    } else {
        qDebug() << ServerMessage;
    }
}

void Dialog::slotError( QAbstractSocket::SocketError )
{

}

void Dialog::slotSendToServer( QString command, QString message )
{
    QByteArray arrBlock;
    QDataStream out( &arrBlock, QIODevice::WriteOnly );
    out << QTime::currentTime() << command + "|" + message;
    TcpSocket->write( arrBlock );
}

void Dialog::slotConnected()
{

}

void Dialog::SlotSendUserNameToServer()
{

}

void Dialog::slotLossConnect()
{
    ui->Connect->show();
    ui->listContacts->clear();
    ui->Chat->setText( ui->Chat->text() + "\nLost connection to the server" );
    ui->Send->setDisabled( true );
    ui->AddContact->setDisabled( true );
}


void Dialog::on_SwitchToContactsMode_activated( int index )
{
    if ( index == 1 ) {
        ui->AddContact->hide();
        ui->listContacts->clear();
        slotSendToServer( "recoverPersonalContactList", NULL );
    } else if ( index == 0 ) {
        ui->AddContact->show();
        ui->listContacts->clear();
        slotSendToServer( "recoverContactList", NULL );
    }
}

void Dialog::on_Send_clicked()
{
    QString recipientsName;
    QModelIndexList selectedIndexes = ui->listContacts->selectionModel()->selectedIndexes();
    QStringList selectedContact;

    foreach ( const QModelIndex& idx, selectedIndexes ) {
        selectedContact << idx.data( Qt::DisplayRole ).toString();
    }

    recipientsName = selectedContact.join( ',' );
    selectedContact = recipientsName.split( " " );
    slotSendToServer( "Message", selectedContact[0] + "|" + nameOftheAuthorizedUser + "|" +
                      ui->InputChat->text() );
    ui->Chat->setText( ui->Chat->text() + "\n" + "->" + selectedContact[0] + ":" + ui->InputChat->text() );
    ui->InputChat->setText( "" );
    ui->Send->setDisabled( true );
}

void Dialog::on_Attach_clicked()
{
    qApp->quit();
}

void Dialog::on_Connect_clicked()
{
    QString NameUser = QInputDialog::getText( this, "Login", "Your name: " );

    if ( NameUser != "" ) {
        TcpSocket = new QTcpSocket( this );
        TcpSocket->connectToHost( "localhost", 2323 );
        connect( TcpSocket, SIGNAL( connected() ), SLOT( slotConnected() ) );
        connect( TcpSocket, SIGNAL( readyRead() ), SLOT( slotReadyRead() ) );
        connect( TcpSocket, &QAbstractSocket::disconnected, this, &Dialog::slotLossConnect );
        slotSendToServer( "Login", NameUser );
        nameOftheAuthorizedUser = NameUser;
        ui->Connect->hide();
    } else {
        ui->Chat->setText( "Invalid name" );
    }
}

void Dialog::on_AddContact_clicked()
{
    QString Result, message;
    QModelIndexList selectedIndexes = ui->listContacts->selectionModel()->selectedIndexes();
    QStringList selectedContact;

    foreach ( const QModelIndex& idx, selectedIndexes ) {
        selectedContact << idx.data( Qt::DisplayRole ).toString();
    }

    Result = selectedContact.join( ',' );
    slotSendToServer( "addContactInPersonalList", Result.split( " " )[0] );
}

void Dialog::on_listContacts_itemSelectionChanged()
{
    ui->AddContact->setDisabled( false );
}

void Dialog::on_InputChat_textEdited( const QString& arg1 )
{
    QString Result;
    QModelIndexList selectedIndexes = ui->listContacts->selectionModel()->selectedIndexes();
    QStringList selectedContact;

    foreach ( const QModelIndex& idx, selectedIndexes ) {
        selectedContact << idx.data( Qt::DisplayRole ).toString();
    }

    Result = selectedContact.join( ',' );

    if ( ( arg1 != "" ) && ( Result != "" ) && ( arg1 != " " ) ) {
        ui->Send->setDisabled( false );
    } else {
        ui->Send->setDisabled( true );
    }
}

void Dialog::on_listContacts_currentTextChanged( const QString& currentText )
{
    QStringList nameContact = currentText.split( " " );

    if ( nameContact.size() > 1 ) {
        slotSendToServer( "sendUnreadMessage", nameContact[0] + "|" + nameOftheAuthorizedUser );
        QListWidgetItem* oldValue = ui->listContacts->takeItem( ui->listContacts->currentRow() );
        QListWidgetItem* newValue = new QListWidgetItem;
        delete oldValue;
        newValue->setText( nameContact[0] );
        ui->listContacts->insertItem( ui->listContacts->currentRow(), newValue );
    }
}
