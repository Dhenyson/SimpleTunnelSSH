#pragma once

#include <optional>

#include <QDialog>

#include "Models/TunnelPort.h"

class QLineEdit;
class QSpinBox;

namespace SimpleTunnelSSH::App
{

class PortEditorForm final : public QDialog
{
    Q_OBJECT

public:
    explicit PortEditorForm(const SimpleTunnelSSH::Core::Models::TunnelPort* port, QWidget* parent = nullptr);

    [[nodiscard]] SimpleTunnelSSH::Core::Models::TunnelPort result() const;

    static std::optional<SimpleTunnelSSH::Core::Models::TunnelPort> showDialog(
        QWidget* owner,
        const SimpleTunnelSSH::Core::Models::TunnelPort* port);

private slots:
    void accept() override;

private:
    QLineEdit* _descriptionEdit;
    QSpinBox* _localPortSpinBox;
    QLineEdit* _remoteHostEdit;
    QSpinBox* _remotePortSpinBox;
    SimpleTunnelSSH::Core::Models::TunnelPort _result;
};

} // namespace SimpleTunnelSSH::App