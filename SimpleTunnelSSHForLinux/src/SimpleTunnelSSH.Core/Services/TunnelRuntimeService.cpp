#include "Services/TunnelRuntimeService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLockFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

#include <stdexcept>

#include "Models/TunnelPort.h"
#include "Services/SshCommandBuilder.h"

namespace SimpleTunnelSSH::Core::Services
{

namespace
{

QString lockDirectory()
{
    const auto runtimeDirectory = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);

    if (!runtimeDirectory.trimmed().isEmpty())
    {
        return runtimeDirectory;
    }

    return QDir::tempPath();
}

QString formatTimestamp(const QDateTime& timestampUtc)
{
    return timestampUtc.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

} // namespace

TunnelRuntimeService::TunnelRuntimeService(QObject* parent)
    : QObject(parent)
    , _sshExecutablePath(resolveSshExecutablePath())
{
    _reconnectTimer.setSingleShot(true);
    _authPromptMonitorTimer.setSingleShot(false);
    _authPromptMonitorTimer.setInterval(150);

    connect(&_reconnectTimer, &QTimer::timeout, this, &TunnelRuntimeService::startProcess);
    connect(&_authPromptMonitorTimer, &QTimer::timeout, this, &TunnelRuntimeService::pollAuthenticationPromptState);
}

TunnelRuntimeService::~TunnelRuntimeService()
{
    deactivate();
}

Models::TunnelRuntimeSnapshot TunnelRuntimeService::snapshot() const
{
    const auto isRunning = _process != nullptr && _process->state() != QProcess::NotRunning;
    const auto isConfirmedConnected = isRunning && _isConnectionConfirmed;
    const auto awaitingAuthentication = _isAwaitingAuthentication;
    const auto faulted = !_faultedConnectionId.isNull();
    const auto desiredSession = hasDesiredSession();

    QString statusText;

    if (faulted)
    {
        statusText = _lastError.trimmed().isEmpty()
            ? QStringLiteral("Connection failed after %1 attempts. Click Connect to retry.").arg(MaxReconnectAttempts)
            : QStringLiteral("Connection failed after %1 attempts: %2").arg(MaxReconnectAttempts).arg(_lastError);
    }
    else if (!desiredSession)
    {
        statusText = _lastError.trimmed().isEmpty()
            ? QStringLiteral("Idle")
            : QStringLiteral("Idle: %1").arg(_lastError);
    }
    else if (awaitingAuthentication)
    {
        statusText = QStringLiteral("Awaiting authentication for %1").arg(_activeConnection.getEndpointLabel());
    }
    else if (isConfirmedConnected)
    {
        statusText = QStringLiteral("Connected to %1").arg(_activeConnection.getEndpointLabel());
    }
    else if (_reconnectAtUtc.isValid())
    {
        const auto remainingSeconds = qMax(0LL, QDateTime::currentDateTimeUtc().secsTo(_reconnectAtUtc));
        const auto nextAttempt = qMin(MaxReconnectAttempts, _reconnectAttempt + 1);
        statusText = _lastError.trimmed().isEmpty()
            ? QStringLiteral("Retrying in %1s (attempt %2 of %3)").arg(remainingSeconds).arg(nextAttempt).arg(MaxReconnectAttempts)
            : QStringLiteral("Retrying in %1s (attempt %2 of %3): %4").arg(remainingSeconds).arg(nextAttempt).arg(MaxReconnectAttempts).arg(_lastError);
    }
    else
    {
        const auto currentAttempt = qMin(MaxReconnectAttempts, qMax(1, _reconnectAttempt + 1));
        statusText = currentAttempt <= 1
            ? QStringLiteral("Connecting to %1...").arg(_activeConnection.getEndpointLabel())
            : QStringLiteral("Connecting to %1 (attempt %2 of %3)...").arg(_activeConnection.getEndpointLabel()).arg(currentAttempt).arg(MaxReconnectAttempts);
    }

    Models::TunnelRuntimeSnapshot snapshot;
    snapshot.isActive = isConfirmedConnected;
    snapshot.isConnecting = desiredSession && !isConfirmedConnected;
    snapshot.isAwaitingAuthentication = awaitingAuthentication;
    snapshot.isFaulted = faulted;
    snapshot.connectionId = _hasActiveConnection ? _activeConnection.id : QUuid {};
    snapshot.faultedConnectionId = _faultedConnectionId;
    snapshot.statusText = statusText;
    snapshot.connectedAtUtc = _connectedAtUtc;
    snapshot.lastError = _lastError;

    for (const auto& group : _activeGroups)
    {
        snapshot.activeGroupIds.append(group.id);
    }

    return snapshot;
}

QList<Models::RuntimeLogEntry> TunnelRuntimeService::recentLogs() const
{
    return _logEntries;
}

QString TunnelRuntimeService::recentLogsText() const
{
    QStringList lines;

    for (const auto& entry : _logEntries)
    {
        lines.append(QStringLiteral("%1\t%2\t%3")
                         .arg(formatTimestamp(entry.timestampUtc), entry.level, entry.message));
    }

    return lines.join('\n');
}

void TunnelRuntimeService::clearLogs()
{
    _recentMessages.clear();
    _logEntries.clear();
    raiseLogsChanged();
}

void TunnelRuntimeService::activate(const Models::SshConnectionProfile& connection, const QList<Models::TunnelGroup>& groups)
{
    if (groups.isEmpty())
    {
        throw std::runtime_error("At least one tunnel group must be selected.");
    }

    bool hasActivePort = false;

    for (const auto& group : groups)
    {
        if (!group.isEnabled)
        {
            continue;
        }

        for (const auto& port : group.ports)
        {
            if (port.isEnabled)
            {
                hasActivePort = true;
                break;
            }
        }

        if (hasActivePort)
        {
            break;
        }
    }

    if (!hasActivePort)
    {
        throw std::runtime_error("At least one active tunnel port must be selected.");
    }

    _reconnectTimer.stop();
    _stopRequested = false;
    _activeConnection = connection.deepClone();
    _hasActiveConnection = true;
    _activeGroups = groups;
    _connectedAtUtc = QDateTime {};
    _isConnectionConfirmed = false;
    _reconnectAttempt = 0;
    _reconnectAtUtc = QDateTime {};
    _authenticationPromptShownForCurrentProcess = false;
    _faultedConnectionId = QUuid {};
    _lastAuthenticationPromptClosedAtUtc = QDateTime {};
    _recentMessages.clear();
    _lastError.clear();
    resetStreamBuffers();

    stopCurrentProcess();
    stopAskPassMonitoring();

    addLog(QStringLiteral("Info"), QStringLiteral("Activating SSH tunnel for %1 with %2 group(s).").arg(_activeConnection.getEndpointLabel()).arg(_activeGroups.size()));
    raiseStateChanged();
    startProcess();
}

void TunnelRuntimeService::deactivate()
{
    const auto endpointLabel = _hasActiveConnection ? _activeConnection.getEndpointLabel() : QString {};

    _stopRequested = true;
    _reconnectTimer.stop();
    _hasActiveConnection = false;
    _activeConnection = Models::SshConnectionProfile {};
    _activeGroups.clear();
    _connectedAtUtc = QDateTime {};
    _isConnectionConfirmed = false;
    _reconnectAttempt = 0;
    _reconnectAtUtc = QDateTime {};
    _authenticationPromptShownForCurrentProcess = false;
    _faultedConnectionId = QUuid {};
    _lastAuthenticationPromptClosedAtUtc = QDateTime {};
    _recentMessages.clear();
    _lastError.clear();
    resetStreamBuffers();

    stopCurrentProcess();
    stopAskPassMonitoring();

    if (!endpointLabel.trimmed().isEmpty())
    {
        addLog(QStringLiteral("Info"), QStringLiteral("Stopped SSH tunnel for %1.").arg(endpointLabel));
    }

    raiseStateChanged();
}

void TunnelRuntimeService::startProcess()
{
    if (_stopRequested || !_hasActiveConnection || _activeGroups.isEmpty() || _process != nullptr)
    {
        return;
    }

    auto* process = new QProcess(this);
    process->setProgram(_sshExecutablePath);
    process->setArguments(SshCommandBuilder::buildArguments(_activeConnection, _activeGroups));

    auto environment = QProcessEnvironment::systemEnvironment();
    configureAskPass(environment);
    process->setProcessEnvironment(environment);

    connect(process, &QProcess::readyReadStandardOutput, this, &TunnelRuntimeService::onProcessReadyReadStandardOutput);
    connect(process, &QProcess::readyReadStandardError, this, &TunnelRuntimeService::onProcessReadyReadStandardError);
    connect(process, &QProcess::finished, this, &TunnelRuntimeService::onProcessFinished);

    addLog(QStringLiteral("Info"), QStringLiteral("Starting ssh for %1.").arg(_activeConnection.getEndpointLabel()));
    process->start();

    if (!process->waitForStarted(5000))
    {
        const auto failureMessage = process->errorString().trimmed().isEmpty()
            ? QStringLiteral("The OpenSSH client did not start.")
            : process->errorString().trimmed();
        disconnect(process, nullptr, this, nullptr);
        delete process;
        stopAskPassMonitoring();
        _lastError = failureMessage;
        addLog(QStringLiteral("Error"), failureMessage);
        scheduleReconnect(failureMessage);
        return;
    }

    _process = process;
    _connectedAtUtc = QDateTime {};
    _isConnectionConfirmed = false;
    _lastError.clear();
    _authPromptMonitorTimer.start();
    addLog(QStringLiteral("Info"), QStringLiteral("ssh started with PID %1.").arg(process->processId()));
    raiseStateChanged();
}

void TunnelRuntimeService::onProcessReadyReadStandardOutput()
{
    if (_process == nullptr)
    {
        return;
    }

    drainStream(_stdoutBuffer, _process->readAllStandardOutput());
}

void TunnelRuntimeService::onProcessReadyReadStandardError()
{
    if (_process == nullptr)
    {
        return;
    }

    drainStream(_stderrBuffer, _process->readAllStandardError());
}

void TunnelRuntimeService::onProcessFinished(int exitCode, QProcess::ExitStatus)
{
    if (!_stdoutBuffer.isEmpty())
    {
        appendMessage(QString::fromUtf8(_stdoutBuffer));
        _stdoutBuffer.clear();
    }

    if (!_stderrBuffer.isEmpty())
    {
        appendMessage(QString::fromUtf8(_stderrBuffer));
        _stderrBuffer.clear();
    }

    const auto exitMessage = buildExitMessage(exitCode);
    _lastError = exitMessage;
    _connectedAtUtc = QDateTime {};
    _isConnectionConfirmed = false;

    const auto promptFailure = shouldStopAfterInteractiveAuthFailure(exitMessage);

    if (promptFailure)
    {
        transitionToFaultedState();
    }

    const auto shouldReconnect = !_stopRequested && _hasActiveConnection && !_activeGroups.isEmpty();
    const auto wasStopRequested = _stopRequested;

    cleanupProcess();
    stopAskPassMonitoring();

    if (!wasStopRequested || promptFailure)
    {
        addLog(shouldReconnect ? QStringLiteral("Warning") : QStringLiteral("Error"), exitMessage);
    }

    if (shouldReconnect)
    {
        scheduleReconnect(exitMessage);
        return;
    }

    raiseStateChanged();
}

void TunnelRuntimeService::pollAuthenticationPromptState()
{
    if (_stopRequested || !_hasActiveConnection || !_askPassLockFile)
    {
        return;
    }

    const auto awaitingAuthentication = isInteractivePromptActive();

    if (awaitingAuthentication == _isAwaitingAuthentication)
    {
        return;
    }

    _isAwaitingAuthentication = awaitingAuthentication;

    if (awaitingAuthentication)
    {
        _authenticationPromptShownForCurrentProcess = true;
    }
    else
    {
        _lastAuthenticationPromptClosedAtUtc = QDateTime::currentDateTimeUtc();
    }

    addLog(awaitingAuthentication ? QStringLiteral("Auth") : QStringLiteral("Info"),
           awaitingAuthentication ? QStringLiteral("SSH requested interactive authentication.") : QStringLiteral("Authentication prompt closed."));
    raiseStateChanged();
}

void TunnelRuntimeService::appendMessage(const QString& message)
{
    const auto trimmedMessage = message.trimmed();

    if (trimmedMessage.isEmpty())
    {
        return;
    }

    _recentMessages.append(trimmedMessage);

    while (_recentMessages.size() > 12)
    {
        _recentMessages.removeFirst();
    }

    auto shouldRaiseState = false;
    QString connectedEndpointLabel;

    if (!_isConnectionConfirmed && _hasActiveConnection && looksLikeSuccessfulConnectionMessage(trimmedMessage))
    {
        _isConnectionConfirmed = true;
        _connectedAtUtc = QDateTime::currentDateTimeUtc();
        _reconnectAttempt = 0;
        _reconnectAtUtc = QDateTime {};
        _faultedConnectionId = QUuid {};
        _lastError.clear();
        connectedEndpointLabel = _activeConnection.getEndpointLabel();
        shouldRaiseState = true;
    }

    addLog(QStringLiteral("SSH"), trimmedMessage);

    if (!connectedEndpointLabel.isEmpty())
    {
        addLog(QStringLiteral("Info"), QStringLiteral("SSH tunnel connected to %1.").arg(connectedEndpointLabel));
    }

    if (shouldRaiseState)
    {
        raiseStateChanged();
    }
}

void TunnelRuntimeService::scheduleReconnect(const QString& reason)
{
    if (_stopRequested || !_hasActiveConnection || _activeGroups.isEmpty())
    {
        return;
    }

    _reconnectAttempt++;

    if (!reason.trimmed().isEmpty())
    {
        _lastError = reason;
    }

    if (_reconnectAttempt >= MaxReconnectAttempts)
    {
        transitionToFaultedState();
        addLog(QStringLiteral("Error"), _lastError.trimmed().isEmpty()
            ? QStringLiteral("SSH connection failed %1 times. Waiting for manual retry.").arg(MaxReconnectAttempts)
            : QStringLiteral("SSH connection failed %1 times. Waiting for manual retry: %2").arg(MaxReconnectAttempts).arg(_lastError));
        raiseStateChanged();
        return;
    }

    const auto delaySeconds = qMin(30, 1 << _reconnectAttempt);
    _reconnectAtUtc = QDateTime::currentDateTimeUtc().addSecs(delaySeconds);
    _reconnectTimer.start(delaySeconds * 1000);

    addLog(QStringLiteral("Warning"), _lastError.trimmed().isEmpty()
        ? QStringLiteral("Retrying SSH connection in %1s.").arg(delaySeconds)
        : QStringLiteral("Retrying SSH connection in %1s: %2").arg(delaySeconds).arg(_lastError));
    raiseStateChanged();
}

void TunnelRuntimeService::addLog(const QString& level, const QString& message)
{
    const auto trimmedMessage = message.trimmed();

    if (trimmedMessage.isEmpty())
    {
        return;
    }

    Models::RuntimeLogEntry entry;
    entry.timestampUtc = QDateTime::currentDateTimeUtc();
    entry.level = level;
    entry.message = trimmedMessage;

    _logEntries.append(entry);

    while (_logEntries.size() > MaxRuntimeLogEntries)
    {
        _logEntries.removeFirst();
    }

    raiseLogsChanged();
}

void TunnelRuntimeService::raiseStateChanged()
{
    emit stateChanged(snapshot());
}

void TunnelRuntimeService::raiseLogsChanged()
{
    emit logsChanged();
}

void TunnelRuntimeService::configureAskPass(QProcessEnvironment& environment)
{
    stopAskPassMonitoring();

    const auto currentProcessPath = QCoreApplication::applicationFilePath();

    if (currentProcessPath.trimmed().isEmpty() || !QFileInfo::exists(currentProcessPath))
    {
        addLog(QStringLiteral("Warning"), QStringLiteral("Interactive SSH authentication helper is unavailable because the current executable path could not be resolved."));
        return;
    }

    const auto inheritedEnvironment = QProcessEnvironment::systemEnvironment();
    const auto displayValue = inheritedEnvironment.value(QStringLiteral("DISPLAY"));
    const auto waylandValue = inheritedEnvironment.value(QStringLiteral("WAYLAND_DISPLAY"));

    if (displayValue.trimmed().isEmpty() && waylandValue.trimmed().isEmpty())
    {
        addLog(QStringLiteral("Warning"), QStringLiteral("Interactive SSH authentication helper is unavailable because no graphical session was detected."));
        return;
    }

    const auto askPassLockPath = QDir(lockDirectory()).filePath(
        QStringLiteral("simpletunnelssh-askpass-%1.lock").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    _askPassLockFile = std::make_unique<QLockFile>(askPassLockPath);
    _askPassLockFile->setStaleLockTime(0);

    environment.insert(QStringLiteral("SSH_ASKPASS"), currentProcessPath);
    environment.insert(QStringLiteral("SSH_ASKPASS_REQUIRE"), QStringLiteral("force"));
    environment.insert(QStringLiteral("SIMPLE_TUNNEL_ASKPASS_MODE"), QStringLiteral("1"));
    environment.insert(QStringLiteral("SIMPLE_TUNNEL_ASKPASS_LOCK_FILE"), askPassLockPath);

    if (displayValue.trimmed().isEmpty())
    {
        environment.insert(QStringLiteral("DISPLAY"), QStringLiteral(":0"));
    }
}

void TunnelRuntimeService::stopCurrentProcess()
{
    if (_process == nullptr)
    {
        return;
    }

    disconnect(_process, nullptr, this, nullptr);

    if (_process->state() != QProcess::NotRunning)
    {
        _process->kill();
        _process->waitForFinished(5000);
    }

    delete _process;
    _process = nullptr;
}

void TunnelRuntimeService::stopAskPassMonitoring()
{
    _authPromptMonitorTimer.stop();
    _askPassLockFile.reset();
    _isAwaitingAuthentication = false;
}

void TunnelRuntimeService::transitionToFaultedState()
{
    if (_hasActiveConnection)
    {
        _faultedConnectionId = _activeConnection.id;
    }

    _stopRequested = true;
    _hasActiveConnection = false;
    _activeConnection = Models::SshConnectionProfile {};
    _activeGroups.clear();
    _connectedAtUtc = QDateTime {};
    _isConnectionConfirmed = false;
    _reconnectAtUtc = QDateTime {};
}

void TunnelRuntimeService::cleanupProcess()
{
    stopCurrentProcess();
    resetStreamBuffers();
}

void TunnelRuntimeService::resetStreamBuffers()
{
    _stdoutBuffer.clear();
    _stderrBuffer.clear();
}

void TunnelRuntimeService::drainStream(QByteArray& buffer, const QByteArray& data)
{
    buffer.append(data);
    int newlineIndex = buffer.indexOf('\n');

    while (newlineIndex >= 0)
    {
        auto line = buffer.left(newlineIndex);

        if (!line.isEmpty() && line.endsWith('\r'))
        {
            line.chop(1);
        }

        appendMessage(QString::fromUtf8(line));
        buffer.remove(0, newlineIndex + 1);
        newlineIndex = buffer.indexOf('\n');
    }
}

QString TunnelRuntimeService::buildExitMessage(int exitCode) const
{
    const auto lastMessage = _recentMessages.isEmpty() ? QString {} : _recentMessages.constLast();
    return lastMessage.trimmed().isEmpty()
        ? QStringLiteral("ssh exited with code %1.").arg(exitCode)
        : QStringLiteral("ssh exited with code %1: %2").arg(exitCode).arg(lastMessage);
}

bool TunnelRuntimeService::hasDesiredSession() const
{
    return !_stopRequested && _hasActiveConnection && !_activeGroups.isEmpty();
}

bool TunnelRuntimeService::shouldStopAfterInteractiveAuthFailure(const QString& exitMessage) const
{
    if (!_authenticationPromptShownForCurrentProcess || !_lastAuthenticationPromptClosedAtUtc.isValid())
    {
        return false;
    }

    if (_lastAuthenticationPromptClosedAtUtc.secsTo(QDateTime::currentDateTimeUtc()) > 5)
    {
        return false;
    }

    return exitMessage.contains(QStringLiteral("Permission denied"), Qt::CaseInsensitive)
        || exitMessage.contains(QStringLiteral("authentication"), Qt::CaseInsensitive)
        || exitMessage.contains(QStringLiteral("passphrase"), Qt::CaseInsensitive)
        || exitMessage.contains(QStringLiteral("publickey"), Qt::CaseInsensitive)
        || exitMessage.contains(QStringLiteral("keyboard-interactive"), Qt::CaseInsensitive)
        || exitMessage.contains(QStringLiteral("exited with code 255"), Qt::CaseInsensitive);
}

bool TunnelRuntimeService::looksLikeSuccessfulConnectionMessage(const QString& message) const
{
    return message.contains(QStringLiteral("Authenticated to "), Qt::CaseInsensitive)
        || message.contains(QStringLiteral("Entering interactive session."), Qt::CaseInsensitive);
}

bool TunnelRuntimeService::isInteractivePromptActive() const
{
    if (!_askPassLockFile)
    {
        return false;
    }

    if (_askPassLockFile->tryLock(0))
    {
        _askPassLockFile->unlock();
        return false;
    }

    return true;
}

QString TunnelRuntimeService::resolveSshExecutablePath() const
{
    const auto executable = QStandardPaths::findExecutable(QStringLiteral("ssh"));
    return executable.trimmed().isEmpty() ? QStringLiteral("ssh") : executable;
}

} // namespace SimpleTunnelSSH::Core::Services