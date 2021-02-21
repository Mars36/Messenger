#include "server.h"

int main( int argc, char* argv[] )
{
    QCoreApplication a( argc, argv );
    Server server( 0 );
    return a.exec();
}
