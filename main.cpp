#include <QtGui>
#include <QApplication>
#include "agent.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    KaixinAgent agent(argv[1], argv[2]);
    agent.login();
    agent.getFriends();
    agent.updateGardens();
    return app.exec();
}
