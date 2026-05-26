#pragma once

#include <optional>

#include <QDialog>

#include "Models/TunnelGroup.h"

class QLineEdit;

namespace SimpleTunnelSSH::App
{

class GroupEditorForm final : public QDialog
{
    Q_OBJECT

public:
    explicit GroupEditorForm(const SimpleTunnelSSH::Core::Models::TunnelGroup* group, QWidget* parent = nullptr);

    [[nodiscard]] SimpleTunnelSSH::Core::Models::TunnelGroup result() const;

    static std::optional<SimpleTunnelSSH::Core::Models::TunnelGroup> showDialog(
        QWidget* owner,
        const SimpleTunnelSSH::Core::Models::TunnelGroup* group);

private slots:
    void accept() override;

private:
    QLineEdit* _nameEdit;
    SimpleTunnelSSH::Core::Models::TunnelGroup _result;
};

} // namespace SimpleTunnelSSH::App