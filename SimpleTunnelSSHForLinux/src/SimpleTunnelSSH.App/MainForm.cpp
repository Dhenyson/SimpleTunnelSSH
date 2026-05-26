#include "MainForm.h"

#include <algorithm>

#include <QCloseEvent>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "AppIconFactory.h"
#include "AppTheme.h"
#include "ConnectionPickerForm.h"
#include "Infrastructure/AppPaths.h"
#include "TunnelApplicationContext.h"

namespace SimpleTunnelSSH::App
{

namespace
{

QPushButton* buildButton(const QString& text, QWidget* parent)
{
    auto* button = new QPushButton(text, parent);
    button->setMinimumWidth(90);
    return button;
}

QString portSummary(const QList<SimpleTunnelSSH::Core::Models::TunnelPort>& ports)
{
    QStringList values;

    for (const auto& port : ports)
    {
        values.append(QStringLiteral("%1:%2:%3")
                          .arg(port.localPort)
                          .arg(port.remoteHost)
                          .arg(port.remotePort));
    }

    return values.join(QStringLiteral(", "));
}

} // namespace

MainForm::MainForm(TunnelApplicationContext* applicationContext, QWidget* parent)
    : QMainWindow(parent)
    , _applicationContext(applicationContext)
    , _connectionsList(new QListWidget(this))
    , _groupsTable(new QTableWidget(this))
    , _portsTable(new QTableWidget(this))
    , _connectButton(buildButton(QStringLiteral("Connect"), this))
    , _addConnectionButton(buildButton(QStringLiteral("Add"), this))
    , _editConnectionButton(buildButton(QStringLiteral("Edit"), this))
    , _deleteConnectionButton(buildButton(QStringLiteral("Delete"), this))
    , _addGroupButton(buildButton(QStringLiteral("Add"), this))
    , _toggleGroupButton(buildButton(QStringLiteral("Activate"), this))
    , _editGroupButton(buildButton(QStringLiteral("Edit"), this))
    , _deleteGroupButton(buildButton(QStringLiteral("Delete"), this))
    , _addPortButton(buildButton(QStringLiteral("Add"), this))
    , _togglePortButton(buildButton(QStringLiteral("Activate"), this))
    , _editPortButton(buildButton(QStringLiteral("Edit"), this))
    , _deletePortButton(buildButton(QStringLiteral("Delete"), this))
    , _statusLabel(new QLabel(QStringLiteral("Idle"), this))
    , _pathLabel(new QLabel(SimpleTunnelSSH::Core::Infrastructure::AppPaths::getConfigurationFilePath(), this))
    , _connectionIndicatorTimer(new QTimer(this))
    , _runtimeSnapshot(SimpleTunnelSSH::Core::Models::TunnelRuntimeSnapshot::idle())
{
    setWindowTitle(QStringLiteral("Simple Tunnel SSH"));
    setWindowIcon(AppIconFactory::applicationIcon());
    setMinimumSize(760, 520);

    auto* fileMenu = menuBar()->addMenu(QStringLiteral("File"));
    fileMenu->addAction(QStringLiteral("Import"), this, [this]() { _applicationContext->importConfiguration(this); });
    fileMenu->addAction(QStringLiteral("Export"), this, [this]() { _applicationContext->exportConfiguration(this); });
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("Exit"), this, [this]() { _applicationContext->exitApplication(); });

    auto* logsMenu = menuBar()->addMenu(QStringLiteral("Logs"));
    logsMenu->addAction(QStringLiteral("View Logs"), this, [this]() { _applicationContext->showLogViewer(); });

    auto* settingsMenu = menuBar()->addMenu(QStringLiteral("Settings"));
    settingsMenu->addAction(QStringLiteral("Open Settings"), this, [this]() { _applicationContext->editSettings(this); });

    _connectionsList->setSelectionMode(QAbstractItemView::SingleSelection);
    _connectionsList->setIconSize(QSize(16, 16));

    connect(_connectionsList, &QListWidget::itemSelectionChanged, this, [this]() {
        _applicationContext->setLastSelectedConnection(getSelectedConnectionId());
        populateGroupsAndPorts();
        updateButtons();
    });
    connect(_connectionsList, &QListWidget::itemDoubleClicked, this, [this]() {
        const auto connectionId = getSelectedConnectionId();

        if (!connectionId.isNull())
        {
            _applicationContext->editConnection(connectionId, this);
        }
    });

    _groupsTable->setColumnCount(3);
    _groupsTable->setHorizontalHeaderLabels({ QStringLiteral("Group"), QStringLiteral("Ports"), QStringLiteral("Port list") });
    _groupsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _groupsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    _groupsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _groupsTable->verticalHeader()->setVisible(false);
    _groupsTable->horizontalHeader()->setStretchLastSection(false);
    connect(_groupsTable->horizontalHeader(), &QHeaderView::sectionResized, this, [this]() {
        if (!_suppressColumnWidthPersistence)
        {
            _applicationContext->saveGroupColumnWidths(currentGroupColumnWidths());
        }
    });
    connect(_groupsTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        populatePorts();
        updateButtons();
    });
    connect(_groupsTable, &QTableWidget::itemDoubleClicked, this, [this]() {
        const auto connectionId = getSelectedConnectionId();
        const auto groupId = getSelectedGroupId();

        if (!connectionId.isNull() && !groupId.isNull())
        {
            _applicationContext->editGroup(connectionId, groupId, this);
        }
    });

    _portsTable->setColumnCount(4);
    _portsTable->setHorizontalHeaderLabels({ QStringLiteral("Local"), QStringLiteral("Remote host"), QStringLiteral("Remote port"), QStringLiteral("Description") });
    _portsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _portsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    _portsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _portsTable->verticalHeader()->setVisible(false);
    _portsTable->horizontalHeader()->setStretchLastSection(false);
    connect(_portsTable->horizontalHeader(), &QHeaderView::sectionResized, this, [this]() {
        if (!_suppressColumnWidthPersistence)
        {
            _applicationContext->savePortColumnWidths(currentPortColumnWidths());
        }
    });
    connect(_portsTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        updateButtons();
    });
    connect(_portsTable, &QTableWidget::itemDoubleClicked, this, [this]() {
        const auto connectionId = getSelectedConnectionId();
        const auto groupId = getSelectedGroupId();
        const auto portId = getSelectedPortId();

        if (!connectionId.isNull() && !groupId.isNull() && !portId.isNull())
        {
            _applicationContext->editPort(connectionId, groupId, portId, this);
        }
    });

    connect(_addConnectionButton, &QPushButton::clicked, this, [this]() { _applicationContext->addConnection(this); });
    connect(_editConnectionButton, &QPushButton::clicked, this, [this]() {
        const auto connectionId = getSelectedConnectionId();

        if (!connectionId.isNull())
        {
            _applicationContext->editConnection(connectionId, this);
        }
    });
    connect(_deleteConnectionButton, &QPushButton::clicked, this, [this]() {
        const auto connectionId = getSelectedConnectionId();

        if (!connectionId.isNull())
        {
            _applicationContext->deleteConnection(connectionId, this);
        }
    });
    connect(_connectButton, &QPushButton::clicked, this, [this]() { handleConnectButtonClick(); });

    connect(_addGroupButton, &QPushButton::clicked, this, [this]() {
        const auto connectionId = getSelectedConnectionId();

        if (connectionId.isNull())
        {
            showSelectionMessage(QStringLiteral("Select a connection before adding a group."));
            return;
        }

        _applicationContext->addGroup(connectionId, this);
    });
    connect(_toggleGroupButton, &QPushButton::clicked, this, [this]() { handleToggleGroupButtonClick(); });
    connect(_editGroupButton, &QPushButton::clicked, this, [this]() {
        const auto connectionId = getSelectedConnectionId();
        const auto groupId = getSelectedGroupId();

        if (!connectionId.isNull() && !groupId.isNull())
        {
            _applicationContext->editGroup(connectionId, groupId, this);
        }
    });
    connect(_deleteGroupButton, &QPushButton::clicked, this, [this]() {
        const auto connectionId = getSelectedConnectionId();
        const auto groupId = getSelectedGroupId();

        if (!connectionId.isNull() && !groupId.isNull())
        {
            _applicationContext->deleteGroup(connectionId, groupId, this);
        }
    });

    connect(_addPortButton, &QPushButton::clicked, this, [this]() {
        const auto connectionId = getSelectedConnectionId();
        const auto groupId = getSelectedGroupId();

        if (connectionId.isNull())
        {
            showSelectionMessage(QStringLiteral("Select a connection before adding a port."));
            return;
        }

        if (groupId.isNull())
        {
            showSelectionMessage(QStringLiteral("Select a group before adding a port."));
            return;
        }

        _applicationContext->addPort(connectionId, groupId, this);
    });
    connect(_togglePortButton, &QPushButton::clicked, this, [this]() { handleTogglePortButtonClick(); });
    connect(_editPortButton, &QPushButton::clicked, this, [this]() {
        const auto connectionId = getSelectedConnectionId();
        const auto groupId = getSelectedGroupId();
        const auto portId = getSelectedPortId();

        if (!connectionId.isNull() && !groupId.isNull() && !portId.isNull())
        {
            _applicationContext->editPort(connectionId, groupId, portId, this);
        }
    });
    connect(_deletePortButton, &QPushButton::clicked, this, [this]() {
        const auto connectionId = getSelectedConnectionId();
        const auto groupId = getSelectedGroupId();
        const auto portId = getSelectedPortId();

        if (!connectionId.isNull() && !groupId.isNull() && !portId.isNull())
        {
            _applicationContext->deletePort(connectionId, groupId, portId, this);
        }
    });

    auto buildPanel = [this](const QString& title, QWidget* content, const QList<QPushButton*>& buttons) {
        auto* groupBox = new QGroupBox(title, this);
        auto* layout = new QVBoxLayout(groupBox);
        layout->setContentsMargins(10, 12, 10, 10);
        layout->setSpacing(10);
        layout->addWidget(content, 1);

        auto* buttonsLayout = new QGridLayout();
        buttonsLayout->setHorizontalSpacing(8);
        buttonsLayout->setVerticalSpacing(8);

        for (int index = 0; index < buttons.size(); ++index)
        {
            buttonsLayout->addWidget(buttons[index], index / 2, index % 2);
        }

        layout->addLayout(buttonsLayout);
        return groupBox;
    };

    auto* central = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(8);
    rootLayout->addWidget(buildPanel(QStringLiteral("Connections"), _connectionsList,
                                     { _addConnectionButton, _editConnectionButton, _deleteConnectionButton, _connectButton }), 31);
    rootLayout->addWidget(buildPanel(QStringLiteral("Groups"), _groupsTable,
                                     { _addGroupButton, _toggleGroupButton, _editGroupButton, _deleteGroupButton }), 31);
    rootLayout->addWidget(buildPanel(QStringLiteral("Ports"), _portsTable,
                                     { _addPortButton, _togglePortButton, _editPortButton, _deletePortButton }), 38);
    setCentralWidget(central);

    statusBar()->addWidget(_statusLabel, 1);
    statusBar()->addPermanentWidget(_pathLabel);

    _connectionIndicatorTimer->setInterval(120);
    connect(_connectionIndicatorTimer, &QTimer::timeout, this, [this]() {
        _connectionIndicatorFrame = (_connectionIndicatorFrame + 1) % 12;
        populateConnections();
    });

    AppTheme::apply(this);
}

void MainForm::applyDefaultWindowSize(const SimpleTunnelSSH::Core::Models::AppSettings& settings)
{
    const QSize requestedSize(settings.windowWidth, settings.windowHeight);
    const QRect workingArea = QGuiApplication::primaryScreen() == nullptr
        ? QRect(0, 0, requestedSize.width(), requestedSize.height())
        : QGuiApplication::primaryScreen()->availableGeometry();
    const int maxWidth = qMax(320, workingArea.width() - 32);
    const int maxHeight = qMax(240, workingArea.height() - 32);
    const int width = qMin(qMax(760, requestedSize.width()), maxWidth);
    const int height = qMin(qMax(520, requestedSize.height()), maxHeight);
    resize(width, height);
}

void MainForm::render(
    const SimpleTunnelSSH::Core::Models::AppConfiguration& configuration,
    const SimpleTunnelSSH::Core::Models::TunnelRuntimeSnapshot& runtimeSnapshot)
{
    _configuration = configuration.deepClone();
    _runtimeSnapshot = runtimeSnapshot;

    populateConnections();
    populateGroupsAndPorts();
    updateConnectionIndicatorAnimation();
    applyConfiguredColumnWidths(_configuration.settings);
    updateButtons();
    updateStatus();
}

void MainForm::closeEvent(QCloseEvent* event)
{
    _applicationContext->handleMainFormClosing(event);
}

void MainForm::populateConnections()
{
    const auto selectedId = !getSelectedConnectionId().isNull() ? getSelectedConnectionId() : _configuration.lastSelectedConnectionId;
    QSignalBlocker blocker(_connectionsList);
    _connectionsList->clear();

    auto connections = _configuration.connections;
    std::sort(connections.begin(), connections.end(), [](const auto& left, const auto& right) {
        return QString::compare(left.name, right.name, Qt::CaseInsensitive) < 0;
    });

    QListWidgetItem* itemToSelect = nullptr;

    for (const auto& connection : connections)
    {
        const auto isActive = _runtimeSnapshot.isActive && _runtimeSnapshot.connectionId == connection.id;
        const auto isLoading = _applicationContext->isConnectionTransitioning(connection.id)
            || (_runtimeSnapshot.isConnecting && !_runtimeSnapshot.isAwaitingAuthentication && _runtimeSnapshot.connectionId == connection.id);
        const auto isAttention = (_runtimeSnapshot.isAwaitingAuthentication && _runtimeSnapshot.connectionId == connection.id)
            || (_runtimeSnapshot.isFaulted && _runtimeSnapshot.faultedConnectionId == connection.id);

        ConnectionIndicatorState state = ConnectionIndicatorState::Inactive;

        if (isLoading)
        {
            state = ConnectionIndicatorState::Loading;
        }
        else if (isAttention)
        {
            state = ConnectionIndicatorState::Attention;
        }
        else if (isActive)
        {
            state = ConnectionIndicatorState::Active;
        }

        auto* item = new QListWidgetItem(
            AppIconFactory::connectionIndicatorIcon(state, _connectionIndicatorFrame),
            QStringLiteral("%1 (%2)").arg(connection.name, connection.getEndpointLabel()),
            _connectionsList);
        item->setData(Qt::UserRole, connection.id.toString(QUuid::WithoutBraces));
        item->setData(Qt::UserRole + 1, static_cast<int>(state));

        if (connection.id == selectedId)
        {
            itemToSelect = item;
        }
    }

    if (itemToSelect == nullptr && _connectionsList->count() > 0)
    {
        itemToSelect = _connectionsList->item(0);
    }

    if (itemToSelect != nullptr)
    {
        _connectionsList->setCurrentItem(itemToSelect);
    }
}

void MainForm::populateGroupsAndPorts()
{
    populateGroups();
    populatePorts();
}

void MainForm::populateGroups()
{
    const auto currentGroupId = getSelectedGroupId();
    QSignalBlocker blocker(_groupsTable);
    _groupsTable->setRowCount(0);

    const auto* connection = getSelectedConnection();

    if (connection == nullptr)
    {
        return;
    }

    auto groups = connection->groups;
    std::sort(groups.begin(), groups.end(), [](const auto& left, const auto& right) {
        return QString::compare(left.name, right.name, Qt::CaseInsensitive) < 0;
    });

    int rowToSelect = -1;

    for (const auto& group : groups)
    {
        const auto row = _groupsTable->rowCount();
        _groupsTable->insertRow(row);

        auto* groupItem = new QTableWidgetItem(group.name);
        groupItem->setData(Qt::UserRole, group.id.toString(QUuid::WithoutBraces));
        groupItem->setIcon(AppIconFactory::connectionIndicatorIcon(
            group.isEnabled ? ConnectionIndicatorState::Active : ConnectionIndicatorState::Inactive));
        _groupsTable->setItem(row, 0, groupItem);
        _groupsTable->setItem(row, 1, new QTableWidgetItem(QString::number(group.ports.size())));
        _groupsTable->setItem(row, 2, new QTableWidgetItem(portSummary(group.ports)));

        if (group.id == currentGroupId)
        {
            rowToSelect = row;
        }
    }

    if (rowToSelect < 0 && _groupsTable->rowCount() > 0)
    {
        rowToSelect = 0;
    }

    if (rowToSelect >= 0)
    {
        _groupsTable->selectRow(rowToSelect);
    }
}

void MainForm::populatePorts()
{
    const auto currentPortId = getSelectedPortId();
    QSignalBlocker blocker(_portsTable);
    _portsTable->setRowCount(0);

    const auto* group = getSelectedGroup();

    if (group == nullptr)
    {
        return;
    }

    auto ports = group->ports;
    std::sort(ports.begin(), ports.end(), [](const auto& left, const auto& right) {
        if (left.localPort != right.localPort)
        {
            return left.localPort < right.localPort;
        }

        const auto hostComparison = QString::compare(left.remoteHost, right.remoteHost, Qt::CaseInsensitive);

        if (hostComparison != 0)
        {
            return hostComparison < 0;
        }

        return left.remotePort < right.remotePort;
    });

    int rowToSelect = -1;

    for (const auto& port : ports)
    {
        const auto row = _portsTable->rowCount();
        _portsTable->insertRow(row);

        auto* localItem = new QTableWidgetItem(QString::number(port.localPort));
        localItem->setData(Qt::UserRole, port.id.toString(QUuid::WithoutBraces));
        localItem->setIcon(AppIconFactory::connectionIndicatorIcon(
            port.isEnabled ? ConnectionIndicatorState::Active : ConnectionIndicatorState::Inactive));
        _portsTable->setItem(row, 0, localItem);
        _portsTable->setItem(row, 1, new QTableWidgetItem(port.remoteHost));
        _portsTable->setItem(row, 2, new QTableWidgetItem(QString::number(port.remotePort)));
        _portsTable->setItem(row, 3, new QTableWidgetItem(port.description));

        if (port.id == currentPortId)
        {
            rowToSelect = row;
        }
    }

    if (rowToSelect < 0 && _portsTable->rowCount() > 0)
    {
        rowToSelect = 0;
    }

    if (rowToSelect >= 0)
    {
        _portsTable->selectRow(rowToSelect);
    }
}

void MainForm::updateButtons()
{
    const auto connectionId = getSelectedConnectionId();
    const auto groupId = getSelectedGroupId();
    const auto portId = getSelectedPortId();
    const auto* selectedGroup = getSelectedGroup();
    const auto* selectedPort = getSelectedPort();
    const auto connectionBusy = !connectionId.isNull() && _applicationContext->isConnectionTransitioning(connectionId);

    _connectButton->setText((_runtimeSnapshot.isActive || _runtimeSnapshot.isConnecting) ? QStringLiteral("Disconnect") : QStringLiteral("Connect"));
    _connectButton->setEnabled((_runtimeSnapshot.isActive || _runtimeSnapshot.isConnecting || !_configuration.connections.isEmpty()) && !connectionBusy);

    _editConnectionButton->setEnabled(!connectionId.isNull());
    _deleteConnectionButton->setEnabled(!connectionId.isNull());
    _addGroupButton->setEnabled(!connectionId.isNull());
    _toggleGroupButton->setEnabled(!connectionId.isNull() && !groupId.isNull() && !connectionBusy);
    _toggleGroupButton->setText(selectedGroup != nullptr && selectedGroup->isEnabled ? QStringLiteral("Deactivate") : QStringLiteral("Activate"));
    _editGroupButton->setEnabled(!connectionId.isNull() && !groupId.isNull());
    _deleteGroupButton->setEnabled(!connectionId.isNull() && !groupId.isNull());
    _addPortButton->setEnabled(!connectionId.isNull() && !groupId.isNull());
    _togglePortButton->setEnabled(!connectionId.isNull() && !groupId.isNull() && !portId.isNull() && !connectionBusy);
    _togglePortButton->setText(selectedPort != nullptr && selectedPort->isEnabled ? QStringLiteral("Deactivate") : QStringLiteral("Activate"));
    _editPortButton->setEnabled(!connectionId.isNull() && !groupId.isNull() && !portId.isNull());
    _deletePortButton->setEnabled(!connectionId.isNull() && !groupId.isNull() && !portId.isNull());
}

void MainForm::updateStatus()
{
    _statusLabel->setText(_runtimeSnapshot.statusText);
    _pathLabel->setText(SimpleTunnelSSH::Core::Infrastructure::AppPaths::getConfigurationFilePath());
}

void MainForm::updateConnectionIndicatorAnimation()
{
    bool shouldAnimate = false;

    for (int index = 0; index < _connectionsList->count(); ++index)
    {
        if (_connectionsList->item(index)->data(Qt::UserRole + 1).toInt() == static_cast<int>(ConnectionIndicatorState::Loading))
        {
            shouldAnimate = true;
            break;
        }
    }

    if (shouldAnimate)
    {
        if (!_connectionIndicatorTimer->isActive())
        {
            _connectionIndicatorTimer->start();
        }
    }
    else
    {
        _connectionIndicatorTimer->stop();
    }
}

void MainForm::applyConfiguredColumnWidths(const SimpleTunnelSSH::Core::Models::AppSettings& settings)
{
    _suppressColumnWidthPersistence = true;

    if (_groupsTable->columnCount() == settings.groupColumnWidths.size())
    {
        for (int index = 0; index < settings.groupColumnWidths.size(); ++index)
        {
            _groupsTable->setColumnWidth(index, settings.groupColumnWidths[index]);
        }
    }

    if (_portsTable->columnCount() == settings.portColumnWidths.size())
    {
        for (int index = 0; index < settings.portColumnWidths.size(); ++index)
        {
            _portsTable->setColumnWidth(index, settings.portColumnWidths[index]);
        }
    }

    _suppressColumnWidthPersistence = false;
}

QList<int> MainForm::currentGroupColumnWidths() const
{
    QList<int> widths;

    for (int index = 0; index < _groupsTable->columnCount(); ++index)
    {
        widths.append(_groupsTable->columnWidth(index));
    }

    return widths;
}

QList<int> MainForm::currentPortColumnWidths() const
{
    QList<int> widths;

    for (int index = 0; index < _portsTable->columnCount(); ++index)
    {
        widths.append(_portsTable->columnWidth(index));
    }

    return widths;
}

QUuid MainForm::getSelectedConnectionId() const
{
    const auto* item = _connectionsList->currentItem();
    return item == nullptr ? QUuid {} : QUuid(item->data(Qt::UserRole).toString());
}

QUuid MainForm::getSelectedGroupId() const
{
    const auto row = _groupsTable->currentRow();

    if (row < 0 || _groupsTable->item(row, 0) == nullptr)
    {
        return QUuid {};
    }

    return QUuid(_groupsTable->item(row, 0)->data(Qt::UserRole).toString());
}

QUuid MainForm::getSelectedPortId() const
{
    const auto row = _portsTable->currentRow();

    if (row < 0 || _portsTable->item(row, 0) == nullptr)
    {
        return QUuid {};
    }

    return QUuid(_portsTable->item(row, 0)->data(Qt::UserRole).toString());
}

const SimpleTunnelSSH::Core::Models::SshConnectionProfile* MainForm::getSelectedConnection() const
{
    const auto connectionId = getSelectedConnectionId();

    for (const auto& connection : _configuration.connections)
    {
        if (connection.id == connectionId)
        {
            return &connection;
        }
    }

    return nullptr;
}

const SimpleTunnelSSH::Core::Models::TunnelGroup* MainForm::getSelectedGroup() const
{
    const auto* connection = getSelectedConnection();
    const auto groupId = getSelectedGroupId();

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

const SimpleTunnelSSH::Core::Models::TunnelPort* MainForm::getSelectedPort() const
{
    const auto* group = getSelectedGroup();
    const auto portId = getSelectedPortId();

    if (group == nullptr)
    {
        return nullptr;
    }

    for (const auto& port : group->ports)
    {
        if (port.id == portId)
        {
            return &port;
        }
    }

    return nullptr;
}

void MainForm::showSelectionMessage(const QString& message)
{
    QMessageBox::information(this, QStringLiteral("Simple Tunnel SSH"), message);
}

void MainForm::handleConnectButtonClick()
{
    if (_runtimeSnapshot.isActive || _runtimeSnapshot.isConnecting)
    {
        _applicationContext->deactivateConnection();
        return;
    }

    if (_configuration.connections.isEmpty())
    {
        showSelectionMessage(QStringLiteral("Create a connection before connecting."));
        return;
    }

    auto connectionId = getSelectedConnectionId();
    QList<QUuid> groupIds;

    if (connectionId.isNull())
    {
        const auto chosenConnectionId = ConnectionPickerForm::showDialog(this, _configuration.connections, _configuration.lastSelectedConnectionId);

        if (!chosenConnectionId.has_value())
        {
            return;
        }

        connectionId = chosenConnectionId.value();
        _applicationContext->setLastSelectedConnection(connectionId);
        groupIds = _applicationContext->getDraftSelection(connectionId);
    }
    else
    {
        groupIds = _applicationContext->getDraftSelection(connectionId);
    }

    _applicationContext->activateConnection(connectionId, groupIds, this);
}

void MainForm::handleToggleGroupButtonClick()
{
    const auto connectionId = getSelectedConnectionId();
    const auto groupId = getSelectedGroupId();
    const auto* group = getSelectedGroup();

    if (!connectionId.isNull() && !groupId.isNull() && group != nullptr)
    {
        _applicationContext->toggleActiveGroup(connectionId, groupId, !group->isEnabled, this);
    }
}

void MainForm::handleTogglePortButtonClick()
{
    const auto connectionId = getSelectedConnectionId();
    const auto groupId = getSelectedGroupId();
    const auto portId = getSelectedPortId();
    const auto* port = getSelectedPort();

    if (!connectionId.isNull() && !groupId.isNull() && !portId.isNull() && port != nullptr)
    {
        _applicationContext->toggleActivePort(connectionId, groupId, portId, !port->isEnabled, this);
    }
}

} // namespace SimpleTunnelSSH::App