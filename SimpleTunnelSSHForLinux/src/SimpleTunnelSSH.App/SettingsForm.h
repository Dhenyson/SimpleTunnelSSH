#pragma once

#include <optional>

#include <QDialog>

#include "Models/AppSettings.h"

class QCheckBox;
class QSpinBox;

namespace SimpleTunnelSSH::App
{

class SettingsForm final : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsForm(const SimpleTunnelSSH::Core::Models::AppSettings& settings, QWidget* parent = nullptr);

    [[nodiscard]] SimpleTunnelSSH::Core::Models::AppSettings result() const;

    static std::optional<SimpleTunnelSSH::Core::Models::AppSettings> showDialog(
        QWidget* owner,
        const SimpleTunnelSSH::Core::Models::AppSettings& settings);

private:
    QCheckBox* _launchAtStartupCheckBox;
    QCheckBox* _startMinimizedCheckBox;
    QSpinBox* _windowWidthSpinBox;
    QSpinBox* _windowHeightSpinBox;
    SimpleTunnelSSH::Core::Models::AppSettings _result;
};

} // namespace SimpleTunnelSSH::App