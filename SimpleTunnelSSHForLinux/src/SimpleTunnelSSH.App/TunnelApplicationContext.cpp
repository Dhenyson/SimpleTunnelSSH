#include "TunnelApplicationContext.h"

#include <algorithm>
#include <stdexcept>

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QSet>

#include "AppIconFactory.h"
#include "AppTheme.h"
#include "ConnectionEditorForm.h"
#include "GroupEditorForm.h"
#include "Infrastructure/AppPaths.h"
#include "LinuxStartupManager.h"
#include "LogViewerForm.h"
#include "MainForm.h"
#include "PortEditorForm.h"
#include "Services/PortConflictDetector.h"
#include "SettingsForm.h"

namespace SimpleTunnelSSH::App
{

using namespace SimpleTunnelSSH::Core::Models;
using namespace SimpleTunnelSSH::Core::Services;

TunnelApplicationContext::TunnelApplicationContext(QApplication& application, QObject* parent)
    : QObject(parent)
    , _application(application)
    , _configurationStore(SimpleTunnelSSH::Core::Infrastructure::AppPaths::getConfigurationFilePath())
    , _runtimeService(this)
    , _runtimeSnapshot(TunnelRuntimeSnapshot::idle())
{
    connect(&_runtimeService, &TunnelRuntimeService::stateChanged, this, &TunnelApplicationContext::onRuntimeStateChanged);
    connect(&_runtimeService, &TunnelRuntimeService::logsChanged, this, &TunnelApplicationContext::onRuntimeLogsChanged);

    AppTheme::applyMenu(&_trayMenu);

    if (QSystemTrayIcon::isSystemTrayAvailable())
    {
        _notifyIcon.setIcon(AppIconFactory::inactiveTrayIcon());
        _notifyIcon.setToolTip(QStringLiteral("Simple Tunnel SSH"));
        _notifyIcon.setContextMenu(&_trayMenu);
        _notifyIcon.show();
        connect(&_notifyIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
            {
                showMainWindow();
            }
        });
    }

    loadConfiguration();
    tryApplyLinuxStartupPreference(_configuration.settings.launchAtStartup, nullptr, false);
    updateTrayMenu();

    if (!_configuration.settings.startMinimizedToTray || !QSystemTrayIcon::isSystemTrayAvailable())
    {
        showMainWindow();
    }
}

TunnelApplicationContext::~TunnelApplicationContext()
{
    _notifyIcon.hide();
    _runtimeService.deactivate();
}

QList<QUuid> TunnelApplicationContext::getDraftSelection(const QUuid& connectionId) const
{
    const auto* connection = findConnection(connectionId);
    QList<QUuid> groupIds;

    if (connection == nullptr)
    {
        return groupIds;
    }

    for (const auto& group : connection->groups)
    {
        if (group.isEnabled)
        {
            groupIds.append(group.id);
        }
    }

    return groupIds;
}

bool TunnelApplicationContext::isConnectionTransitioning(const QUuid& connectionId) const
{
    return (_transitionDepth > 0 && _transitionConnectionId == connectionId)
        || (_runtimeSnapshot.isConnecting && !_runtimeSnapshot.isAwaitingAuthentication && _runtimeSnapshot.connectionId == connectionId);
}

void TunnelApplicationContext::setLastSelectedConnection(const QUuid& connectionId)
{
    _configuration.lastSelectedConnectionId = connectionId;
}

void TunnelApplicationContext::showMainWindow()
{
    if (_mainForm == nullptr)
    {
        _mainForm = new MainForm(this);
        _mainForm->setAttribute(Qt::WA_DeleteOnClose);
        connect(_mainForm, &QObject::destroyed, this, [this]() { _mainForm = nullptr; });
    }

    _mainForm->render(_configuration, _runtimeSnapshot);
    _mainForm->show();
    _mainForm->raise();
    _mainForm->activateWindow();
}

void TunnelApplicationContext::showLogViewer()
{
    if (_logViewerForm == nullptr)
    {
        _logViewerForm = new LogViewerForm(this);
        _logViewerForm->setAttribute(Qt::WA_DeleteOnClose);
        connect(_logViewerForm, &QObject::destroyed, this, [this]() { _logViewerForm = nullptr; });
    }

    _logViewerForm->render(_runtimeService.recentLogs(), _configuration.settings.logColumnWidths);
    _logViewerForm->show();
    _logViewerForm->raise();
    _logViewerForm->activateWindow();
}

void TunnelApplicationContext::handleMainFormClosing(QCloseEvent* event)
{
    if (_exitRequested || !QSystemTrayIcon::isSystemTrayAvailable())
    {
        return;
    }

    event->ignore();

    if (_mainForm != nullptr)
    {
        _mainForm->hide();
    }

    if (_minimizeHintShown)
    {
        return;
    }

    _notifyIcon.showMessage(
        QStringLiteral("Simple Tunnel SSH"),
        QStringLiteral("The window was hidden to the tray and the tunnel service is still running."),
        QSystemTrayIcon::Information,
        2500);
    _minimizeHintShown = true;
}

void TunnelApplicationContext::editSettings(QWidget* owner)
{
    const auto editedSettings = SettingsForm::showDialog(owner, _configuration.settings);

    if (!editedSettings.has_value())
    {
        return;
    }

    auto normalizedSettings = editedSettings.value();
    normalizeSettings(normalizedSettings);

    if (!tryApplyLinuxStartupPreference(normalizedSettings.launchAtStartup, owner))
    {
        return;
    }

    _configuration.settings = normalizedSettings;

    if (!persistConfiguration(owner))
    {
        return;
    }

    applyWindowSettingsToOpenForms();
    refreshUi();
}

void TunnelApplicationContext::saveGroupColumnWidths(const QList<int>& widths)
{
    if (!updateColumnWidths(_configuration.settings.groupColumnWidths, widths))
    {
        return;
    }

    persistConfiguration(_mainForm);
}

void TunnelApplicationContext::savePortColumnWidths(const QList<int>& widths)
{
    if (!updateColumnWidths(_configuration.settings.portColumnWidths, widths))
    {
        return;
    }

    persistConfiguration(_mainForm);
}

void TunnelApplicationContext::saveLogColumnWidths(const QList<int>& widths)
{
    if (!updateColumnWidths(_configuration.settings.logColumnWidths, widths))
    {
        return;
    }

    persistConfiguration(_logViewerForm);
}

void TunnelApplicationContext::clearRuntimeLogs()
{
    _runtimeService.clearLogs();
}

QString TunnelApplicationContext::getRuntimeLogsText() const
{
    return _runtimeService.recentLogsText();
}

void TunnelApplicationContext::addConnection(QWidget* owner)
{
    const auto connection = ConnectionEditorForm::showDialog(owner, nullptr);

    if (!connection.has_value())
    {
        return;
    }

    _configuration.connections.append(connection.value());
    _configuration.lastSelectedConnectionId = connection->id;

    if (!persistConfiguration(owner))
    {
        return;
    }

    refreshUi();
}

void TunnelApplicationContext::editConnection(const QUuid& connectionId, QWidget* owner)
{
    auto* connection = findConnection(connectionId);

    if (connection == nullptr)
    {
        return;
    }

    const auto editedConnection = ConnectionEditorForm::showDialog(owner, connection);

    if (!editedConnection.has_value())
    {
        return;
    }

    *connection = editedConnection.value();

    if (!persistConfiguration(owner))
    {
        return;
    }

    if (_runtimeSnapshot.connectionId == editedConnection->id)
    {
        reapplyRuntime(owner);
    }

    refreshUi();
}

void TunnelApplicationContext::deleteConnection(const QUuid& connectionId, QWidget* owner)
{
    const auto* connection = findConnection(connectionId);

    if (connection == nullptr)
    {
        return;
    }

    const auto result = QMessageBox::question(
        owner,
        QStringLiteral("Delete connection"),
        QStringLiteral("Delete the connection '%1' and all of its groups?").arg(connection->name));

    if (result != QMessageBox::Yes)
    {
        return;
    }

    if (_runtimeSnapshot.connectionId == connectionId)
    {
        runConnectionTransition(connectionId, [this]() { _runtimeService.deactivate(); });
    }

    for (int index = 0; index < _configuration.connections.size(); ++index)
    {
        if (_configuration.connections[index].id == connectionId)
        {
            _configuration.connections.removeAt(index);
            break;
        }
    }

    if (_configuration.lastSelectedConnectionId == connectionId)
    {
        _configuration.lastSelectedConnectionId = _configuration.connections.isEmpty() ? QUuid {} : _configuration.connections.first().id;
    }

    if (!persistConfiguration(owner))
    {
        return;
    }

    refreshUi();
}

void TunnelApplicationContext::addGroup(const QUuid& connectionId, QWidget* owner)
{
    auto* connection = findConnection(connectionId);

    if (connection == nullptr)
    {
        return;
    }

    const auto group = GroupEditorForm::showDialog(owner, nullptr);

    if (!group.has_value())
    {
        return;
    }

    auto createdGroup = group.value();
    bool hasEnabledPort = false;

    for (const auto& port : createdGroup.ports)
    {
        if (port.isEnabled)
        {
            hasEnabledPort = true;
            break;
        }
    }

    createdGroup.isEnabled = hasEnabledPort;
    connection->groups.append(createdGroup);

    if (!persistConfiguration(owner))
    {
        return;
    }

    refreshUi();
}

void TunnelApplicationContext::editGroup(const QUuid& connectionId, const QUuid& groupId, QWidget* owner)
{
    auto* group = findGroup(connectionId, groupId);

    if (group == nullptr)
    {
        return;
    }

    const auto editedGroup = GroupEditorForm::showDialog(owner, group);

    if (!editedGroup.has_value())
    {
        return;
    }

    group->name = editedGroup->name;

    if (!persistConfiguration(owner))
    {
        return;
    }

    refreshUi();
}

void TunnelApplicationContext::deleteGroup(const QUuid& connectionId, const QUuid& groupId, QWidget* owner)
{
    auto* connection = findConnection(connectionId);
    auto* group = findGroup(connectionId, groupId);

    if (connection == nullptr || group == nullptr)
    {
        return;
    }

    const auto result = QMessageBox::question(
        owner,
        QStringLiteral("Delete group"),
        QStringLiteral("Delete the group '%1' and all of its ports?").arg(group->name));

    if (result != QMessageBox::Yes)
    {
        return;
    }

    const auto shouldReapply = _runtimeSnapshot.connectionId == connection->id && group->isEnabled;

    for (int index = 0; index < connection->groups.size(); ++index)
    {
        if (connection->groups[index].id == groupId)
        {
            connection->groups.removeAt(index);
            break;
        }
    }

    if (!persistConfiguration(owner))
    {
        return;
    }

    if (shouldReapply)
    {
        reapplyRuntime(owner, false);
    }

    refreshUi();
}

void TunnelApplicationContext::addPort(const QUuid& connectionId, const QUuid& groupId, QWidget* owner)
{
    auto* group = findGroup(connectionId, groupId);

    if (group == nullptr)
    {
        return;
    }

    const auto port = PortEditorForm::showDialog(owner, nullptr);

    if (!port.has_value() || !validatePort(*group, port.value(), owner))
    {
        return;
    }

    group->ports.append(port.value());

    if (!persistConfiguration(owner))
    {
        return;
    }

    if (_runtimeSnapshot.connectionId == connectionId && group->isEnabled)
    {
        reapplyRuntime(owner);
    }

    refreshUi();
}

void TunnelApplicationContext::editPort(const QUuid& connectionId, const QUuid& groupId, const QUuid& portId, QWidget* owner)
{
    auto* group = findGroup(connectionId, groupId);

    if (group == nullptr)
    {
        return;
    }

    TunnelPort* currentPort = nullptr;

    for (auto& port : group->ports)
    {
        if (port.id == portId)
        {
            currentPort = &port;
            break;
        }
    }

    if (currentPort == nullptr)
    {
        return;
    }

    const auto editedPort = PortEditorForm::showDialog(owner, currentPort);

    if (!editedPort.has_value() || !validatePort(*group, editedPort.value(), owner))
    {
        return;
    }

    *currentPort = editedPort.value();

    bool hasEnabledPort = false;

    for (const auto& port : group->ports)
    {
        if (port.isEnabled)
        {
            hasEnabledPort = true;
            break;
        }
    }

    if (!hasEnabledPort)
    {
        group->isEnabled = false;
    }

    if (!persistConfiguration(owner))
    {
        return;
    }

    if (_runtimeSnapshot.connectionId == connectionId && group->isEnabled)
    {
        reapplyRuntime(owner);
    }

    refreshUi();
}

void TunnelApplicationContext::deletePort(const QUuid& connectionId, const QUuid& groupId, const QUuid& portId, QWidget* owner)
{
    auto* group = findGroup(connectionId, groupId);

    if (group == nullptr)
    {
        return;
    }

    TunnelPort* port = nullptr;

    for (auto& currentPort : group->ports)
    {
        if (currentPort.id == portId)
        {
            port = &currentPort;
            break;
        }
    }

    if (port == nullptr)
    {
        return;
    }

    const auto result = QMessageBox::question(
        owner,
        QStringLiteral("Delete port"),
        QStringLiteral("Delete port %1:%2:%3?").arg(port->localPort).arg(port->remoteHost).arg(port->remotePort));

    if (result != QMessageBox::Yes)
    {
        return;
    }

    const auto shouldReapply = _runtimeSnapshot.connectionId == connectionId && _runtimeSnapshot.activeGroupIds.contains(groupId);

    for (int index = 0; index < group->ports.size(); ++index)
    {
        if (group->ports[index].id == portId)
        {
            group->ports.removeAt(index);
            break;
        }
    }

    bool hasEnabledPort = false;

    for (const auto& currentPort : group->ports)
    {
        if (currentPort.isEnabled)
        {
            hasEnabledPort = true;
            break;
        }
    }

    if (!hasEnabledPort)
    {
        group->isEnabled = false;
    }

    if (!persistConfiguration(owner))
    {
        return;
    }

    if (shouldReapply)
    {
        reapplyRuntime(owner, false);
    }

    refreshUi();
}

void TunnelApplicationContext::activateConnection(const QUuid& connectionId, const QList<QUuid>& groupIds, QWidget* owner)
{
    auto* connection = findConnection(connectionId);

    if (connection == nullptr)
    {
        return;
    }

    const auto desiredGroupIds = normalizeGroupSelection(*connection, groupIds);
    QList<TunnelGroup> selectedGroups;

    for (const auto& group : connection->groups)
    {
        if (desiredGroupIds.contains(group.id))
        {
            selectedGroups.append(group.deepClone());
        }
    }

    const auto runnableGroups = getRunnableGroups(selectedGroups);

    if (selectedGroups.isEmpty())
    {
        QMessageBox::information(owner, QStringLiteral("No group selected"), QStringLiteral("Activate at least one group before activating the connection."));
        refreshUi();
        return;
    }

    if (runnableGroups.isEmpty())
    {
        QMessageBox::information(owner, QStringLiteral("No active port"), QStringLiteral("Activate at least one port before activating the connection."));
        refreshUi();
        return;
    }

    const auto conflicts = PortConflictDetector::detect(runnableGroups);

    if (!conflicts.isEmpty())
    {
        QMessageBox::warning(owner, QStringLiteral("Port conflict detected"), buildConflictMessage(conflicts));
        refreshUi();
        return;
    }

    if (!_runtimeSnapshot.connectionId.isNull() && _runtimeSnapshot.connectionId != connectionId)
    {
        const auto result = QMessageBox::question(
            owner,
            QStringLiteral("Switch connection"),
            QStringLiteral("Another connection is already active. Switch to the selected connection?"));

        if (result != QMessageBox::Yes)
        {
            refreshUi();
            return;
        }
    }

    _configuration.lastSelectedConnectionId = connectionId;
    QSet<QUuid> runnableGroupIds;

    for (const auto& group : runnableGroups)
    {
        runnableGroupIds.insert(group.id);
    }

    applyGroupSelection(*connection, runnableGroupIds);

    if (!persistConfiguration(owner))
    {
        return;
    }

    runConnectionTransition(connectionId, [this, connectionCopy = connection->deepClone(), runnableGroups]() {
        _runtimeService.activate(connectionCopy, runnableGroups);
    });
    refreshUi();
}

void TunnelApplicationContext::deactivateConnection()
{
    const auto connectionId = _runtimeSnapshot.connectionId;
    runConnectionTransition(connectionId, [this]() { _runtimeService.deactivate(); });
    refreshUi();
}

void TunnelApplicationContext::toggleActiveGroup(const QUuid& connectionId, const QUuid& groupId, bool shouldBeActive, QWidget* owner)
{
    auto* connection = findConnection(connectionId);
    auto* group = findGroup(connectionId, groupId);

    if (connection == nullptr || group == nullptr)
    {
        return;
    }

    if (shouldBeActive)
    {
        bool hasEnabledPort = false;

        for (const auto& port : group->ports)
        {
            if (port.isEnabled)
            {
                hasEnabledPort = true;
                break;
            }
        }

        if (!hasEnabledPort)
        {
            QMessageBox::information(
                owner,
                QStringLiteral("No active port"),
                QStringLiteral("Activate at least one port in '%1' before activating the group.").arg(group->name));
            refreshUi();
            return;
        }
    }

    QSet<QUuid> activeSelection;

    for (const auto& currentGroup : connection->groups)
    {
        if (currentGroup.isEnabled)
        {
            activeSelection.insert(currentGroup.id);
        }
    }

    if (shouldBeActive)
    {
        activeSelection.insert(groupId);
    }
    else
    {
        activeSelection.remove(groupId);
    }

    if (activeSelection.isEmpty())
    {
        applyGroupSelection(*connection, {});

        if (!persistConfiguration(owner))
        {
            return;
        }

        if (_runtimeSnapshot.connectionId == connectionId)
        {
            deactivateConnection();
        }
        else
        {
            refreshUi();
        }

        return;
    }

    auto buildSelectedGroups = [&connection, &activeSelection]() {
        QList<TunnelGroup> groups;

        for (const auto& currentGroup : connection->groups)
        {
            if (activeSelection.contains(currentGroup.id))
            {
                auto selectedGroup = currentGroup.deepClone();
                selectedGroup.isEnabled = true;
                groups.append(selectedGroup);
            }
        }

        return groups;
    };

    auto selectedGroups = buildSelectedGroups();
    auto runnableGroups = getRunnableGroups(selectedGroups);

    if (runnableGroups.isEmpty())
    {
        applyGroupSelection(*connection, {});

        if (!persistConfiguration(owner))
        {
            return;
        }

        if (_runtimeSnapshot.connectionId == connectionId)
        {
            deactivateConnection();
        }
        else
        {
            refreshUi();
        }

        return;
    }

    auto conflicts = PortConflictDetector::detect(runnableGroups);

    if (!conflicts.isEmpty() && shouldBeActive)
    {
        QSet<QUuid> conflictingGroupIds;

        for (const auto& conflict : conflicts)
        {
            if (conflict.firstGroupId == groupId)
            {
                conflictingGroupIds.insert(conflict.secondGroupId);
            }
            else if (conflict.secondGroupId == groupId)
            {
                conflictingGroupIds.insert(conflict.firstGroupId);
            }
        }

        if (!conflictingGroupIds.isEmpty())
        {
            QStringList conflictingGroups;

            for (const auto& currentGroup : connection->groups)
            {
                if (conflictingGroupIds.contains(currentGroup.id))
                {
                    conflictingGroups.append(currentGroup.name);
                }
            }

            const auto result = QMessageBox::question(
                owner,
                QStringLiteral("Replace conflicting groups"),
                QStringLiteral("Activating this group requires disabling: %1. Continue?").arg(conflictingGroups.join(QStringLiteral(", "))));

            if (result == QMessageBox::Yes)
            {
                for (const auto& conflictingGroupId : conflictingGroupIds)
                {
                    activeSelection.remove(conflictingGroupId);
                }

                selectedGroups = buildSelectedGroups();
                runnableGroups = getRunnableGroups(selectedGroups);
                conflicts = PortConflictDetector::detect(runnableGroups);
            }
        }
    }

    if (!conflicts.isEmpty())
    {
        QMessageBox::warning(owner, QStringLiteral("Port conflict detected"), buildConflictMessage(conflicts));
        refreshUi();
        return;
    }

    QSet<QUuid> runnableGroupIds;

    for (const auto& runnableGroup : runnableGroups)
    {
        runnableGroupIds.insert(runnableGroup.id);
    }

    applyGroupSelection(*connection, runnableGroupIds);

    if (!persistConfiguration(owner))
    {
        return;
    }

    if (_runtimeSnapshot.connectionId == connectionId)
    {
        runConnectionTransition(connectionId, [this, connectionCopy = connection->deepClone(), runnableGroups]() {
            _runtimeService.activate(connectionCopy, runnableGroups);
        });
    }

    refreshUi();
}

void TunnelApplicationContext::toggleActivePort(const QUuid& connectionId, const QUuid& groupId, const QUuid& portId, bool shouldBeActive, QWidget* owner)
{
    auto* connection = findConnection(connectionId);
    auto* group = findGroup(connectionId, groupId);

    if (connection == nullptr || group == nullptr)
    {
        return;
    }

    TunnelPort* port = nullptr;

    for (auto& currentPort : group->ports)
    {
        if (currentPort.id == portId)
        {
            port = &currentPort;
            break;
        }
    }

    if (port == nullptr)
    {
        return;
    }

    const auto previousPortState = port->isEnabled;
    const auto previousGroupState = group->isEnabled;
    port->isEnabled = shouldBeActive;

    bool hasEnabledPort = false;

    for (const auto& currentPort : group->ports)
    {
        if (currentPort.isEnabled)
        {
            hasEnabledPort = true;
            break;
        }
    }

    if (!hasEnabledPort)
    {
        group->isEnabled = false;
    }

    if (shouldBeActive && group->isEnabled)
    {
        QList<TunnelGroup> enabledGroups;

        for (const auto& currentGroup : connection->groups)
        {
            if (currentGroup.isEnabled)
            {
                enabledGroups.append(currentGroup.deepClone());
            }
        }

        const auto conflicts = PortConflictDetector::detect(enabledGroups);

        if (!conflicts.isEmpty())
        {
            port->isEnabled = previousPortState;
            group->isEnabled = previousGroupState;
            QMessageBox::warning(owner, QStringLiteral("Port conflict detected"), buildConflictMessage(conflicts));
            refreshUi();
            return;
        }
    }

    if (!persistConfiguration(owner))
    {
        return;
    }

    if (_runtimeSnapshot.connectionId == connectionId && _runtimeSnapshot.activeGroupIds.contains(groupId))
    {
        reapplyRuntime(owner, false);
    }

    refreshUi();
}

void TunnelApplicationContext::exportConfiguration(QWidget* owner)
{
    const auto filePath = QFileDialog::getSaveFileName(
        owner,
        QStringLiteral("Export configuration"),
        QStringLiteral("simple-tunnel-ssh-config.json"),
        QStringLiteral("JSON files (*.json)"));

    if (filePath.isEmpty())
    {
        return;
    }

    try
    {
        _configurationStore.exportConfiguration(_configuration, filePath);
    }
    catch (const std::exception& exception)
    {
        QMessageBox::warning(owner, QStringLiteral("Export configuration"), QString::fromUtf8(exception.what()));
    }
}

void TunnelApplicationContext::importConfiguration(QWidget* owner)
{
    const auto filePath = QFileDialog::getOpenFileName(
        owner,
        QStringLiteral("Import configuration"),
        QString(),
        QStringLiteral("JSON files (*.json)"));

    if (filePath.isEmpty())
    {
        return;
    }

    const auto result = QMessageBox::question(
        owner,
        QStringLiteral("Import configuration"),
        QStringLiteral("Importing a configuration replaces the current connections, groups, ports and settings. Continue?"));

    if (result != QMessageBox::Yes)
    {
        return;
    }

    _runtimeService.deactivate();

    try
    {
        _configuration = _configurationStore.importConfiguration(filePath);
    }
    catch (const std::exception& exception)
    {
        QMessageBox::warning(owner, QStringLiteral("Import configuration"), QString::fromUtf8(exception.what()));
        return;
    }

    normalizeConfiguration();
    tryApplyLinuxStartupPreference(_configuration.settings.launchAtStartup, owner);

    if (!persistConfiguration(owner))
    {
        return;
    }

    applyWindowSettingsToOpenForms();
    refreshUi();
}

void TunnelApplicationContext::exitApplication()
{
    if (_exitRequested)
    {
        return;
    }

    _exitRequested = true;
    _runtimeService.deactivate();
    _notifyIcon.hide();

    if (_logViewerForm != nullptr)
    {
        _logViewerForm->close();
    }

    if (_mainForm != nullptr)
    {
        _mainForm->close();
    }

    _application.quit();
}

void TunnelApplicationContext::onRuntimeStateChanged(const TunnelRuntimeSnapshot& snapshot)
{
    _runtimeSnapshot = snapshot;
    refreshUi();
}

void TunnelApplicationContext::onRuntimeLogsChanged()
{
    refreshLogViewer();
}

void TunnelApplicationContext::loadConfiguration()
{
    try
    {
        _configuration = _configurationStore.load();
    }
    catch (const std::exception& exception)
    {
        _configuration = AppConfiguration {};
        QMessageBox::warning(nullptr, QStringLiteral("Configuration"), QString::fromUtf8(exception.what()));
    }

    normalizeConfiguration();
}

bool TunnelApplicationContext::persistConfiguration(QWidget* owner)
{
    try
    {
        _configurationStore.save(_configuration);
        return true;
    }
    catch (const std::exception& exception)
    {
        QMessageBox::warning(owner, QStringLiteral("Configuration"), QStringLiteral("Unable to save configuration: %1").arg(QString::fromUtf8(exception.what())));
        return false;
    }
}

void TunnelApplicationContext::reapplyRuntime(QWidget* owner, bool showConflictMessage)
{
    if (_runtimeSnapshot.connectionId.isNull())
    {
        return;
    }

    auto* connection = findConnection(_runtimeSnapshot.connectionId);

    if (connection == nullptr)
    {
        runConnectionTransition(_runtimeSnapshot.connectionId, [this]() { _runtimeService.deactivate(); });
        return;
    }

    QList<TunnelGroup> enabledGroups;

    for (const auto& group : connection->groups)
    {
        if (group.isEnabled)
        {
            enabledGroups.append(group.deepClone());
        }
    }

    const auto runnableGroups = getRunnableGroups(enabledGroups);

    if (runnableGroups.isEmpty())
    {
        runConnectionTransition(connection->id, [this]() { _runtimeService.deactivate(); });
        return;
    }

    const auto conflicts = PortConflictDetector::detect(runnableGroups);

    if (!conflicts.isEmpty())
    {
        runConnectionTransition(connection->id, [this]() { _runtimeService.deactivate(); });

        if (showConflictMessage)
        {
            QMessageBox::warning(
                owner,
                QStringLiteral("Active tunnel stopped"),
                QStringLiteral("The active tunnel was stopped because the updated configuration created a port conflict. Review the active groups and ports, then activate the connection again."));
        }

        return;
    }

    runConnectionTransition(connection->id, [this, connectionCopy = connection->deepClone(), runnableGroups]() {
        _runtimeService.activate(connectionCopy, runnableGroups);
    });
}

void TunnelApplicationContext::normalizeConfiguration()
{
    _configuration.schemaVersion = 2;
    normalizeSettings(_configuration.settings);

    for (auto& connection : _configuration.connections)
    {
        if (connection.id.isNull())
        {
            connection.id = QUuid::createUuid();
        }

        connection.host = connection.host.trimmed();
        connection.userName = connection.userName.trimmed();
        connection.identityFilePath = connection.identityFilePath.trimmed();
        connection.name = connection.name.trimmed().isEmpty() ? connection.getEndpointLabel() : connection.name.trimmed();

        if (connection.port <= 0)
        {
            connection.port = 22;
        }

        for (auto& group : connection.groups)
        {
            if (group.id.isNull())
            {
                group.id = QUuid::createUuid();
            }

            group.name = group.name.trimmed().isEmpty() ? QStringLiteral("Group") : group.name.trimmed();
            bool hasEnabledPort = false;

            for (auto& port : group.ports)
            {
                if (port.id.isNull())
                {
                    port.id = QUuid::createUuid();
                }

                port.description = port.description.trimmed();
                port.remoteHost = port.remoteHost.trimmed().isEmpty() ? QStringLiteral("localhost") : port.remoteHost.trimmed();

                if (port.remotePort <= 0)
                {
                    port.remotePort = port.localPort;
                }

                if (port.isEnabled)
                {
                    hasEnabledPort = true;
                }
            }

            if (!hasEnabledPort)
            {
                group.isEnabled = false;
            }
        }
    }

    bool validSelection = false;

    for (const auto& connection : _configuration.connections)
    {
        if (connection.id == _configuration.lastSelectedConnectionId)
        {
            validSelection = true;
            break;
        }
    }

    if (!validSelection)
    {
        _configuration.lastSelectedConnectionId = _configuration.connections.isEmpty() ? QUuid {} : _configuration.connections.first().id;
    }
}

void TunnelApplicationContext::normalizeSettings(AppSettings& settings)
{
    settings.windowWidth = settings.windowWidth < 760 ? AppSettings::DefaultWindowWidth : settings.windowWidth;
    settings.windowHeight = settings.windowHeight < 520 ? AppSettings::DefaultWindowHeight : settings.windowHeight;
    settings.groupColumnWidths = normalizeColumnWidths(settings.groupColumnWidths, AppSettings::createDefaultGroupColumnWidths());
    settings.portColumnWidths = normalizeColumnWidths(settings.portColumnWidths, AppSettings::createDefaultPortColumnWidths());
    settings.logColumnWidths = normalizeColumnWidths(settings.logColumnWidths, AppSettings::createDefaultLogColumnWidths());
}

QList<int> TunnelApplicationContext::normalizeColumnWidths(const QList<int>& widths, const QList<int>& defaultWidths)
{
    if (widths.size() != defaultWidths.size())
    {
        return defaultWidths;
    }

    auto normalized = widths;

    for (int index = 0; index < normalized.size(); ++index)
    {
        if (normalized[index] < 24)
        {
            normalized[index] = defaultWidths[index];
        }
    }

    return normalized;
}

QSet<QUuid> TunnelApplicationContext::normalizeGroupSelection(
    const SshConnectionProfile& connection,
    const QList<QUuid>& groupIds) const
{
    QSet<QUuid> validGroupIds;

    for (const auto& group : connection.groups)
    {
        validGroupIds.insert(group.id);
    }

    QSet<QUuid> selection;

    for (const auto& groupId : groupIds)
    {
        if (validGroupIds.contains(groupId))
        {
            selection.insert(groupId);
        }
    }

    if (selection.isEmpty())
    {
        const auto draftSelection = getDraftSelection(connection.id);

        for (const auto& groupId : draftSelection)
        {
            if (validGroupIds.contains(groupId))
            {
                selection.insert(groupId);
            }
        }
    }

    return selection;
}

bool TunnelApplicationContext::validatePort(const TunnelGroup& group, const TunnelPort& candidatePort, QWidget* owner) const
{
    for (const auto& port : group.ports)
    {
        if (port.id != candidatePort.id && port.localPort == candidatePort.localPort)
        {
            QMessageBox::warning(
                owner,
                QStringLiteral("Duplicate local port"),
                QStringLiteral("The group '%1' already contains a forward for local port %2.").arg(group.name).arg(candidatePort.localPort));
            return false;
        }
    }

    return true;
}

bool TunnelApplicationContext::tryApplyLinuxStartupPreference(bool enabled, QWidget* owner, bool showError)
{
    try
    {
        LinuxStartupManager::setEnabled(enabled);
        return true;
    }
    catch (const std::exception& exception)
    {
        if (showError)
        {
            QMessageBox::warning(owner, QStringLiteral("Startup setting"), QStringLiteral("Unable to update Linux startup: %1").arg(QString::fromUtf8(exception.what())));
        }

        return false;
    }
}

bool TunnelApplicationContext::updateColumnWidths(QList<int>& currentWidths, const QList<int>& newWidths)
{
    if (newWidths.isEmpty())
    {
        return false;
    }

    if (currentWidths.size() != newWidths.size())
    {
        currentWidths.clear();

        for (const auto width : newWidths)
        {
            currentWidths.append(qMax(24, width));
        }

        return true;
    }

    bool changed = false;

    for (int index = 0; index < newWidths.size(); ++index)
    {
        const auto normalizedWidth = qMax(24, newWidths[index]);

        if (currentWidths[index] == normalizedWidth)
        {
            continue;
        }

        currentWidths[index] = normalizedWidth;
        changed = true;
    }

    return changed;
}

void TunnelApplicationContext::runConnectionTransition(const QUuid& connectionId, const std::function<void()>& action)
{
    beginConnectionTransition(connectionId);

    try
    {
        action();
    }
    catch (...)
    {
        endConnectionTransition(connectionId);
        throw;
    }

    endConnectionTransition(connectionId);
}

void TunnelApplicationContext::beginConnectionTransition(const QUuid& connectionId)
{
    if (connectionId.isNull())
    {
        return;
    }

    _transitionConnectionId = connectionId;
    ++_transitionDepth;
    refreshUi();
}

void TunnelApplicationContext::endConnectionTransition(const QUuid& connectionId)
{
    if (connectionId.isNull())
    {
        return;
    }

    if (_transitionDepth > 0)
    {
        --_transitionDepth;
    }

    if (_transitionDepth == 0 && _transitionConnectionId == connectionId)
    {
        _transitionConnectionId = QUuid {};
    }

    refreshUi();
}

void TunnelApplicationContext::refreshUi()
{
    updateTrayMenu();

    if (_mainForm != nullptr)
    {
        _mainForm->render(_configuration, _runtimeSnapshot);
    }

    refreshLogViewer();
}

void TunnelApplicationContext::refreshLogViewer()
{
    if (_logViewerForm == nullptr)
    {
        return;
    }

    _logViewerForm->render(_runtimeService.recentLogs(), _configuration.settings.logColumnWidths);
}

void TunnelApplicationContext::applyWindowSettingsToOpenForms()
{
    if (_mainForm != nullptr)
    {
        _mainForm->applyDefaultWindowSize(_configuration.settings);
    }
}

void TunnelApplicationContext::updateTrayMenu()
{
    _trayMenu.clear();
    _trayMenu.addAction(QStringLiteral("Open"), this, &TunnelApplicationContext::showMainWindow);

    if (!_configuration.connections.isEmpty())
    {
        auto* connectionsMenu = _trayMenu.addMenu(QStringLiteral("Connections"));
        AppTheme::applyMenu(connectionsMenu);

        auto connections = _configuration.connections;
        std::sort(connections.begin(), connections.end(), [](const auto& left, const auto& right) {
            return QString::compare(left.name, right.name, Qt::CaseInsensitive) < 0;
        });

        for (const auto& connection : connections)
        {
            auto* action = connectionsMenu->addAction(connection.name);
            action->setCheckable(true);
            action->setChecked(_runtimeSnapshot.connectionId == connection.id);
            connect(action, &QAction::triggered, this, [this, connectionId = connection.id]() {
                if (_runtimeSnapshot.connectionId == connectionId)
                {
                    deactivateConnection();
                }
                else
                {
                    activateConnection(connectionId, getDraftSelection(connectionId), _mainForm);
                }
            });
        }
    }

    const auto* activeConnection = _runtimeSnapshot.connectionId.isNull() ? nullptr : findConnection(_runtimeSnapshot.connectionId);
    auto* groupsMenu = _trayMenu.addMenu(QStringLiteral("Active Groups"));
    AppTheme::applyMenu(groupsMenu);
    groupsMenu->setEnabled(activeConnection != nullptr);

    if (activeConnection != nullptr)
    {
        auto groups = activeConnection->groups;
        std::sort(groups.begin(), groups.end(), [](const auto& left, const auto& right) {
            return QString::compare(left.name, right.name, Qt::CaseInsensitive) < 0;
        });

        for (const auto& group : groups)
        {
            auto* action = groupsMenu->addAction(group.name);
            action->setCheckable(true);
            action->setChecked(group.isEnabled);
            connect(action, &QAction::triggered, this, [this, connectionId = activeConnection->id, groupId = group.id](bool checked) {
                toggleActiveGroup(connectionId, groupId, checked, _mainForm);
            });
        }
    }

    _trayMenu.addSeparator();
    _trayMenu.addAction(QStringLiteral("Exit"), this, &TunnelApplicationContext::exitApplication);

    if (QSystemTrayIcon::isSystemTrayAvailable())
    {
        _notifyIcon.setIcon(_runtimeSnapshot.isAwaitingAuthentication || _runtimeSnapshot.isFaulted
                ? AppIconFactory::attentionTrayIcon()
                : _runtimeSnapshot.isActive
                    ? AppIconFactory::activeTrayIcon()
                    : AppIconFactory::inactiveTrayIcon());

        const auto tooltip = (_runtimeSnapshot.isActive || _runtimeSnapshot.isConnecting || _runtimeSnapshot.isFaulted)
            ? QStringLiteral("Simple Tunnel SSH - %1").arg(_runtimeSnapshot.statusText)
            : QStringLiteral("Simple Tunnel SSH");
        _notifyIcon.setToolTip(tooltip.left(63));
    }
}

SshConnectionProfile* TunnelApplicationContext::findConnection(const QUuid& connectionId)
{
    for (auto& connection : _configuration.connections)
    {
        if (connection.id == connectionId)
        {
            return &connection;
        }
    }

    return nullptr;
}

const SshConnectionProfile* TunnelApplicationContext::findConnection(const QUuid& connectionId) const
{
    for (const auto& connection : _configuration.connections)
    {
        if (connection.id == connectionId)
        {
            return &connection;
        }
    }

    return nullptr;
}

TunnelGroup* TunnelApplicationContext::findGroup(const QUuid& connectionId, const QUuid& groupId)
{
    auto* connection = findConnection(connectionId);

    if (connection == nullptr)
    {
        return nullptr;
    }

    for (auto& group : connection->groups)
    {
        if (group.id == groupId)
        {
            return &group;
        }
    }

    return nullptr;
}

const TunnelGroup* TunnelApplicationContext::findGroup(const QUuid& connectionId, const QUuid& groupId) const
{
    const auto* connection = findConnection(connectionId);

    if (connection == nullptr)
    {
        return nullptr;
    }

    for (const auto& group : connection->groups)
    {
        if (group.id == groupId)
        {
            return &group;
        }
    }

    return nullptr;
}

void TunnelApplicationContext::applyGroupSelection(SshConnectionProfile& connection, const QSet<QUuid>& activeGroupIds)
{
    for (auto& group : connection.groups)
    {
        group.isEnabled = activeGroupIds.contains(group.id);
    }
}

QList<TunnelGroup> TunnelApplicationContext::getRunnableGroups(const QList<TunnelGroup>& groups)
{
    QList<TunnelGroup> runnableGroups;

    for (const auto& group : groups)
    {
        if (!group.isEnabled)
        {
            continue;
        }

        bool hasEnabledPort = false;

        for (const auto& port : group.ports)
        {
            if (port.isEnabled)
            {
                hasEnabledPort = true;
                break;
            }
        }

        if (hasEnabledPort)
        {
            runnableGroups.append(group);
        }
    }

    return runnableGroups;
}

QString TunnelApplicationContext::buildConflictMessage(const QList<PortConflict>& conflicts)
{
    QStringList lines;

    for (const auto& conflict : conflicts)
    {
        lines.append(QStringLiteral("Local port %1 is shared by '%2' and '%3'.")
                         .arg(conflict.localPort)
                         .arg(conflict.firstGroupName)
                         .arg(conflict.secondGroupName));
    }

    return lines.join('\n');
}

} // namespace SimpleTunnelSSH::App