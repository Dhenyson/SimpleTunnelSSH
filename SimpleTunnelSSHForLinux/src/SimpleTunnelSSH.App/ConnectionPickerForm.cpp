#include "ConnectionPickerForm.h"

#include "AppIconFactory.h"

#include <algorithm>

#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

namespace SimpleTunnelSSH::App
{

using SimpleTunnelSSH::Core::Models::SshConnectionProfile;

ConnectionPickerForm::ConnectionPickerForm(
    const QList<SshConnectionProfile>& connections,
    const QUuid& preselectedConnectionId,
    QWidget* parent)
    : QDialog(parent)
    , _connectionsList(new QListWidget(this))
    , _connectButton(new QPushButton(QStringLiteral("Connect"), this))
{
    setWindowTitle(QStringLiteral("Choose connection"));
    setModal(true);
    resize(460, 340);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    auto sortedConnections = connections;
    std::sort(sortedConnections.begin(), sortedConnections.end(), [](const auto& left, const auto& right)
    {
        return QString::compare(left.name, right.name, Qt::CaseInsensitive) < 0;
    });

    for (const auto& connection : sortedConnections)
    {
        auto* item = new QListWidgetItem(
            QStringLiteral("%1 (%2) - %3 group(s)").arg(connection.name, connection.getEndpointLabel()).arg(connection.groups.size()),
            _connectionsList);
        item->setData(Qt::UserRole, connection.id.toString(QUuid::WithoutBraces));
    }

    for (int index = 0; index < _connectionsList->count(); ++index)
    {
        auto* item = _connectionsList->item(index);

        if (QUuid(item->data(Qt::UserRole).toString()) == preselectedConnectionId)
        {
            _connectionsList->setCurrentItem(item);
            break;
        }
    }

    if (_connectionsList->currentItem() == nullptr && _connectionsList->count() > 0)
    {
        _connectionsList->setCurrentRow(0);
    }

    auto* hintLabel = new QLabel(
        QStringLiteral("Choose the SSH connection to activate. The saved group selection for that connection will be used."),
        this);
    hintLabel->setWordWrap(true);

    auto* buttonBox = new QDialogButtonBox(this);
    buttonBox->addButton(_connectButton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    AppIconFactory::applyDialogButtonIcons(buttonBox);
    _connectButton->setEnabled(_connectionsList->currentItem() != nullptr);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(_connectionsList, &QListWidget::itemSelectionChanged, this, [this]() {
        _connectButton->setEnabled(_connectionsList->currentItem() != nullptr);
    });
    connect(_connectionsList, &QListWidget::itemDoubleClicked, this, [this]() {
        if (_connectionsList->currentItem() != nullptr)
        {
            accept();
        }
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    layout->addWidget(hintLabel);
    layout->addWidget(_connectionsList, 1);
    layout->addWidget(buttonBox);
}

QUuid ConnectionPickerForm::selectedConnectionId() const
{
    const auto* item = _connectionsList->currentItem();
    return item == nullptr ? QUuid {} : QUuid(item->data(Qt::UserRole).toString());
}

std::optional<QUuid> ConnectionPickerForm::showDialog(
    QWidget* owner,
    const QList<SshConnectionProfile>& connections,
    const QUuid& preselectedConnectionId)
{
    ConnectionPickerForm form(connections, preselectedConnectionId, owner);

    if (form.exec() != QDialog::Accepted)
    {
        return std::nullopt;
    }

    const auto connectionId = form.selectedConnectionId();
    return connectionId.isNull() ? std::nullopt : std::optional<QUuid>(connectionId);
}

} // namespace SimpleTunnelSSH::App