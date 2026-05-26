#pragma once

#include <optional>

#include <QDialog>

#include "Models/SshConnectionProfile.h"

class QListWidget;
class QPushButton;

namespace SimpleTunnelSSH::App
{

class ConnectionPickerForm final : public QDialog
{
    Q_OBJECT

public:
    ConnectionPickerForm(
        const QList<SimpleTunnelSSH::Core::Models::SshConnectionProfile>& connections,
        const QUuid& preselectedConnectionId,
        QWidget* parent = nullptr);

    [[nodiscard]] QUuid selectedConnectionId() const;

    static std::optional<QUuid> showDialog(
        QWidget* owner,
        const QList<SimpleTunnelSSH::Core::Models::SshConnectionProfile>& connections,
        const QUuid& preselectedConnectionId);

private:
    QListWidget* _connectionsList;
    QPushButton* _connectButton;
};

} // namespace SimpleTunnelSSH::App