#pragma once

#include <QMainWindow>

#include "Models/AppConfiguration.h"
#include "Models/TunnelRuntimeSnapshot.h"

class QCloseEvent;
class QLabel;
class QListWidget;
class QPushButton;
class QTableWidget;
class QTimer;

namespace SimpleTunnelSSH::App
{

class TunnelApplicationContext;

class MainForm final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainForm(TunnelApplicationContext* applicationContext, QWidget* parent = nullptr);

    void applyDefaultWindowSize(const SimpleTunnelSSH::Core::Models::AppSettings& settings);
    void render(
        const SimpleTunnelSSH::Core::Models::AppConfiguration& configuration,
        const SimpleTunnelSSH::Core::Models::TunnelRuntimeSnapshot& runtimeSnapshot);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void populateConnections();
    void populateGroupsAndPorts();
    void populateGroups();
    void populatePorts();
    void updateButtons();
    void updateStatus();
    void updateConnectionIndicatorAnimation();
    void applyConfiguredColumnWidths(const SimpleTunnelSSH::Core::Models::AppSettings& settings);
    QList<int> currentGroupColumnWidths() const;
    QList<int> currentPortColumnWidths() const;
    QUuid getSelectedConnectionId() const;
    QUuid getSelectedGroupId() const;
    QUuid getSelectedPortId() const;
    const SimpleTunnelSSH::Core::Models::SshConnectionProfile* getSelectedConnection() const;
    const SimpleTunnelSSH::Core::Models::TunnelGroup* getSelectedGroup() const;
    const SimpleTunnelSSH::Core::Models::TunnelPort* getSelectedPort() const;
    void showSelectionMessage(const QString& message);
    void handleConnectButtonClick();
    void handleToggleGroupButtonClick();
    void handleTogglePortButtonClick();

    TunnelApplicationContext* _applicationContext;
    QListWidget* _connectionsList;
    QTableWidget* _groupsTable;
    QTableWidget* _portsTable;
    QPushButton* _connectButton;
    QPushButton* _addConnectionButton;
    QPushButton* _editConnectionButton;
    QPushButton* _deleteConnectionButton;
    QPushButton* _addGroupButton;
    QPushButton* _toggleGroupButton;
    QPushButton* _editGroupButton;
    QPushButton* _deleteGroupButton;
    QPushButton* _addPortButton;
    QPushButton* _togglePortButton;
    QPushButton* _editPortButton;
    QPushButton* _deletePortButton;
    QLabel* _statusLabel;
    QLabel* _pathLabel;
    QTimer* _connectionIndicatorTimer;
    SimpleTunnelSSH::Core::Models::AppConfiguration _configuration;
    SimpleTunnelSSH::Core::Models::TunnelRuntimeSnapshot _runtimeSnapshot;
    int _connectionIndicatorFrame { 0 };
    bool _suppressColumnWidthPersistence { false };
};

} // namespace SimpleTunnelSSH::App