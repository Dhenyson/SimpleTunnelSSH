#include <QApplication>
#include <QDir>
#include <QLockFile>
#include <QStringList>
#include <QStandardPaths>

#include "AppIconFactory.h"
#include "AppTheme.h"
#include "AskPassPromptHost.h"
#include "TunnelApplicationContext.h"

namespace
{

QString singletonLockPath()
{
    const auto runtimeDirectory = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    const auto baseDirectory = runtimeDirectory.trimmed().isEmpty() ? QDir::tempPath() : runtimeDirectory;
    return QDir(baseDirectory).filePath(QStringLiteral("SimpleTunnelSSH.Singleton.lock"));
}

QStringList promptArguments(int argc, char* argv[])
{
    QStringList arguments;

    for (int index = 1; index < argc; ++index)
    {
        arguments.append(QString::fromLocal8Bit(argv[index]));
    }

    return arguments;
}

} // namespace

int main(int argc, char* argv[])
{
    if (SimpleTunnelSSH::App::AskPassPromptHost::isAskPassMode())
    {
        return SimpleTunnelSSH::App::AskPassPromptHost::run(promptArguments(argc, argv));
    }

    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("SimpleTunnelSSH"));
    application.setApplicationDisplayName(QStringLiteral("Simple Tunnel SSH"));
    application.setQuitOnLastWindowClosed(false);
    application.setWindowIcon(SimpleTunnelSSH::App::AppIconFactory::applicationIcon());
    SimpleTunnelSSH::App::AppTheme::apply(application);

    QLockFile singleInstanceLock(singletonLockPath());
    singleInstanceLock.setStaleLockTime(0);

    if (!singleInstanceLock.tryLock())
    {
        return 0;
    }

    SimpleTunnelSSH::App::TunnelApplicationContext context(application);
    return application.exec();
}