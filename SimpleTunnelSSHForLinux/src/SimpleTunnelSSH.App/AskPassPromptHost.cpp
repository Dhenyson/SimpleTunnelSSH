#include "AskPassPromptHost.h"

#include <memory>

#include <QApplication>
#include <QLockFile>
#include <QTextStream>

#include "AppIconFactory.h"
#include "AppTheme.h"
#include "AskPassPromptForm.h"

namespace SimpleTunnelSSH::App
{

namespace
{

constexpr auto AskPassModeEnvironmentVariable = "SIMPLE_TUNNEL_ASKPASS_MODE";
constexpr auto AskPassLockEnvironmentVariable = "SIMPLE_TUNNEL_ASKPASS_LOCK_FILE";

} // namespace

bool AskPassPromptHost::isAskPassMode()
{
    return qEnvironmentVariable(AskPassModeEnvironmentVariable) == QStringLiteral("1");
}

int AskPassPromptHost::run(const QStringList& args)
{
    char applicationName[] = "SimpleTunnelSSH";
    char* argv[] = { applicationName, nullptr };
    int argc = 1;
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("SimpleTunnelSSH"));
    application.setApplicationDisplayName(QStringLiteral("Simple Tunnel SSH"));
    application.setWindowIcon(AppIconFactory::applicationIcon());
    AppTheme::apply(application);

    std::unique_ptr<QLockFile> promptLock;
    const auto lockPath = qEnvironmentVariable(AskPassLockEnvironmentVariable);

    if (!lockPath.trimmed().isEmpty())
    {
        promptLock = std::make_unique<QLockFile>(lockPath);
        promptLock->setStaleLockTime(0);
        promptLock->lock();
    }

    const auto prompt = args.isEmpty() ? QStringLiteral("SSH authentication is required.") : args.join(' ');
    const auto secret = AskPassPromptForm::showPrompt(prompt);

    if (promptLock)
    {
        promptLock->unlock();
    }

    if (!secret.has_value())
    {
        return 1;
    }

    QTextStream(stdout) << secret.value() << Qt::endl;
    return 0;
}

} // namespace SimpleTunnelSSH::App