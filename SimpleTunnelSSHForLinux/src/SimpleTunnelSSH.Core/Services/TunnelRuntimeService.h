#pragma once

#include <memory>

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QTimer>

#include "Models/RuntimeLogEntry.h"
#include "Models/SshConnectionProfile.h"
#include "Models/TunnelGroup.h"
#include "Models/TunnelRuntimeSnapshot.h"

class QLockFile;
namespace SimpleTunnelSSH::Core::Services
{

class TunnelRuntimeService final : public QObject
{
    Q_OBJECT

public:
    explicit TunnelRuntimeService(QObject* parent = nullptr);
    ~TunnelRuntimeService() override;

    [[nodiscard]] Models::TunnelRuntimeSnapshot snapshot() const;
    [[nodiscard]] QList<Models::RuntimeLogEntry> recentLogs() const;
    [[nodiscard]] QString recentLogsText() const;

    void clearLogs();
    void activate(const Models::SshConnectionProfile& connection, const QList<Models::TunnelGroup>& groups);
    void deactivate();

signals:
    void stateChanged(const Models::TunnelRuntimeSnapshot& snapshot);
    void logsChanged();

private slots:
    void startProcess();
    void onProcessReadyReadStandardOutput();
    void onProcessReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void pollAuthenticationPromptState();

private:
    static constexpr int MaxRuntimeLogEntries = 300;
    static constexpr int MaxReconnectAttempts = 5;

    void appendMessage(const QString& message);
    void scheduleReconnect(const QString& reason);
    void addLog(const QString& level, const QString& message);
    void raiseStateChanged();
    void raiseLogsChanged();
    void configureAskPass(class QProcessEnvironment& environment);
    void stopCurrentProcess();
    void stopAskPassMonitoring();
    void transitionToFaultedState();
    void cleanupProcess();
    void resetStreamBuffers();
    void drainStream(QByteArray& buffer, const QByteArray& data);
    [[nodiscard]] QString buildExitMessage(int exitCode) const;
    [[nodiscard]] bool hasDesiredSession() const;
    [[nodiscard]] bool shouldStopAfterInteractiveAuthFailure(const QString& exitMessage) const;
    [[nodiscard]] bool looksLikeSuccessfulConnectionMessage(const QString& message) const;
    [[nodiscard]] bool isInteractivePromptActive() const;
    [[nodiscard]] QString resolveSshExecutablePath() const;

    QTimer _reconnectTimer;
    QTimer _authPromptMonitorTimer;
    QString _sshExecutablePath;
    QProcess* _process { nullptr };
    Models::SshConnectionProfile _activeConnection;
    bool _hasActiveConnection { false };
    QList<Models::TunnelGroup> _activeGroups;
    QStringList _recentMessages;
    QList<Models::RuntimeLogEntry> _logEntries;
    std::unique_ptr<QLockFile> _askPassLockFile;
    bool _stopRequested { true };
    bool _isConnectionConfirmed { false };
    bool _isAwaitingAuthentication { false };
    bool _authenticationPromptShownForCurrentProcess { false };
    QUuid _faultedConnectionId;
    int _reconnectAttempt { 0 };
    QString _lastError;
    QDateTime _lastAuthenticationPromptClosedAtUtc;
    QDateTime _connectedAtUtc;
    QDateTime _reconnectAtUtc;
    QByteArray _stdoutBuffer;
    QByteArray _stderrBuffer;
};

} // namespace SimpleTunnelSSH::Core::Services