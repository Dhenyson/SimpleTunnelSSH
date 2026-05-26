#pragma once

#include <optional>

#include <QDialog>

#include "Models/SshConnectionProfile.h"

class QLineEdit;
class QSpinBox;

namespace SimpleTunnelSSH::App
{

class ConnectionEditorForm final : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionEditorForm(const SimpleTunnelSSH::Core::Models::SshConnectionProfile* connection, QWidget* parent = nullptr);

    [[nodiscard]] SimpleTunnelSSH::Core::Models::SshConnectionProfile result() const;

    static std::optional<SimpleTunnelSSH::Core::Models::SshConnectionProfile> showDialog(
        QWidget* owner,
        const SimpleTunnelSSH::Core::Models::SshConnectionProfile* connection);

private slots:
    void browseIdentityFile();
    void accept() override;

private:
    QLineEdit* _nameEdit;
    QLineEdit* _hostEdit;
    QSpinBox* _portSpinBox;
    QLineEdit* _userEdit;
    QLineEdit* _identityFileEdit;
    SimpleTunnelSSH::Core::Models::SshConnectionProfile _result;
};

} // namespace SimpleTunnelSSH::App