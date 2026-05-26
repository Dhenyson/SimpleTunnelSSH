#pragma once

#include <functional>

#include <QList>
#include <QMenu>
#include <QObject>
#include <QSet>
#include <QSystemTrayIcon>

#include "Models/AppConfiguration.h"
#include "Models/PortConflict.h"
#include "Models/TunnelRuntimeSnapshot.h"
#include "Services/JsonConfigurationStore.h"
#include "Services/TunnelRuntimeService.h"

class QApplication;
class QCloseEvent;
class QWidget;

namespace SimpleTunnelSSH::App
{

class LogViewerForm;
class MainForm;

class TunnelApplicationContext final : public QObject
{
    Q_OBJECT

public:
    explicit TunnelApplicationContext(QApplication& application, QObject* parent = nullptr);
    ~TunnelApplicationContext() override;

    [[nodiscard]] QList<QUuid> getDraftSelection(const QUuid& connectionId) const;
    [[nodiscard]] bool isConnectionTransitioning(const QUuid& connectionId) const;
    void setLastSelectedConnection(const QUuid& connectionId);

    void showMainWindow();
    void showLogViewer();
    void handleMainFormClosing(QCloseEvent* event);

    void editSettings(QWidget* owner);
    void saveGroupColumnWidths(const QList<int>& widths);
    void savePortColumnWidths(const QList<int>& widths);
    void saveLogColumnWidths(const QList<int>& widths);
    void clearRuntimeLogs();
    [[nodiscard]] QString getRuntimeLogsText() const;

    void addConnection(QWidget* owner);
    void editConnection(const QUuid& connectionId, QWidget* owner);
    void deleteConnection(const QUuid& connectionId, QWidget* owner);
    void addGroup(const QUuid& connectionId, QWidget* owner);
    void editGroup(const QUuid& connectionId, const QUuid& groupId, QWidget* owner);
    void deleteGroup(const QUuid& connectionId, const QUuid& groupId, QWidget* owner);
    void addPort(const QUuid& connectionId, const QUuid& groupId, QWidget* owner);
    void editPort(const QUuid& connectionId, const QUuid& groupId, const QUuid& portId, QWidget* owner);
    void deletePort(const QUuid& connectionId, const QUuid& groupId, const QUuid& portId, QWidget* owner);
    void activateConnection(const QUuid& connectionId, const QList<QUuid>& groupIds, QWidget* owner = nullptr);
    void deactivateConnection();
    void toggleActiveGroup(const QUuid& connectionId, const QUuid& groupId, bool shouldBeActive, QWidget* owner = nullptr);
    void toggleActivePort(const QUuid& connectionId, const QUuid& groupId, const QUuid& portId, bool shouldBeActive, QWidget* owner = nullptr);
    void exportConfiguration(QWidget* owner);
    void importConfiguration(QWidget* owner);
    void exitApplication();

private slots:
    void onRuntimeStateChanged(const SimpleTunnelSSH::Core::Models::TunnelRuntimeSnapshot& snapshot);
    void onRuntimeLogsChanged();

private:
    void loadConfiguration();
    bool persistConfiguration(QWidget* owner = nullptr);
    void reapplyRuntime(QWidget* owner, bool showConflictMessage = true);
    void normalizeConfiguration();
    static void normalizeSettings(SimpleTunnelSSH::Core::Models::AppSettings& settings);
    static QList<int> normalizeColumnWidths(const QList<int>& widths, const QList<int>& defaultWidths);
    QSet<QUuid> normalizeGroupSelection(
        const SimpleTunnelSSH::Core::Models::SshConnectionProfile& connection,
        const QList<QUuid>& groupIds) const;
    bool validatePort(
        const SimpleTunnelSSH::Core::Models::TunnelGroup& group,
        const SimpleTunnelSSH::Core::Models::TunnelPort& candidatePort,
        QWidget* owner) const;
    bool tryApplyLinuxStartupPreference(bool enabled, QWidget* owner = nullptr, bool showError = true);
    static bool updateColumnWidths(QList<int>& currentWidths, const QList<int>& newWidths);
    void runConnectionTransition(const QUuid& connectionId, const std::function<void()>& action);
    void beginConnectionTransition(const QUuid& connectionId);
    void endConnectionTransition(const QUuid& connectionId);
    void refreshUi();
    void refreshLogViewer();
    void applyWindowSettingsToOpenForms();
    void updateTrayMenu();
    SimpleTunnelSSH::Core::Models::SshConnectionProfile* findConnection(const QUuid& connectionId);
    const SimpleTunnelSSH::Core::Models::SshConnectionProfile* findConnection(const QUuid& connectionId) const;
    SimpleTunnelSSH::Core::Models::TunnelGroup* findGroup(const QUuid& connectionId, const QUuid& groupId);
    const SimpleTunnelSSH::Core::Models::TunnelGroup* findGroup(const QUuid& connectionId, const QUuid& groupId) const;
    static void applyGroupSelection(
        SimpleTunnelSSH::Core::Models::SshConnectionProfile& connection,
        const QSet<QUuid>& activeGroupIds);
    static QList<SimpleTunnelSSH::Core::Models::TunnelGroup> getRunnableGroups(
        const QList<SimpleTunnelSSH::Core::Models::TunnelGroup>& groups);
    static QString buildConflictMessage(const QList<SimpleTunnelSSH::Core::Models::PortConflict>& conflicts);

    QApplication& _application;
    SimpleTunnelSSH::Core::Services::JsonConfigurationStore _configurationStore;
    SimpleTunnelSSH::Core::Services::TunnelRuntimeService _runtimeService;
    QSystemTrayIcon _notifyIcon;
    QMenu _trayMenu;
    SimpleTunnelSSH::Core::Models::AppConfiguration _configuration;
    SimpleTunnelSSH::Core::Models::TunnelRuntimeSnapshot _runtimeSnapshot;
    MainForm* _mainForm { nullptr };
    LogViewerForm* _logViewerForm { nullptr };
    bool _exitRequested { false };
    bool _minimizeHintShown { false };
    QUuid _transitionConnectionId;
    int _transitionDepth { 0 };
};

} // namespace SimpleTunnelSSH::App