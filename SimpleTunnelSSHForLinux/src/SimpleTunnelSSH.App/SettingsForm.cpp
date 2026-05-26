#include "SettingsForm.h"

#include "AppIconFactory.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

namespace SimpleTunnelSSH::App
{

using SimpleTunnelSSH::Core::Models::AppSettings;

SettingsForm::SettingsForm(const AppSettings& settings, QWidget* parent)
    : QDialog(parent)
    , _launchAtStartupCheckBox(new QCheckBox(QStringLiteral("Launch with Linux startup"), this))
    , _startMinimizedCheckBox(new QCheckBox(QStringLiteral("Start minimized in the tray"), this))
    , _windowWidthSpinBox(new QSpinBox(this))
    , _windowHeightSpinBox(new QSpinBox(this))
    , _result(settings.deepClone())
{
    setWindowTitle(QStringLiteral("Settings"));
    setModal(true);
    setMinimumWidth(480);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    _launchAtStartupCheckBox->setChecked(_result.launchAtStartup);
    _startMinimizedCheckBox->setChecked(_result.startMinimizedToTray);
    _windowWidthSpinBox->setRange(760, 4096);
    _windowWidthSpinBox->setValue(_result.windowWidth);
    _windowHeightSpinBox->setRange(520, 2160);
    _windowHeightSpinBox->setValue(_result.windowHeight);

    auto* introLabel = new QLabel(QStringLiteral("General preferences for the main window and startup behavior."), this);
    introLabel->setWordWrap(true);

    auto* formLayout = new QFormLayout();
    formLayout->addRow(QStringLiteral("Default window width"), _windowWidthSpinBox);
    formLayout->addRow(QStringLiteral("Default window height"), _windowHeightSpinBox);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    AppIconFactory::applyDialogButtonIcons(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    layout->addWidget(introLabel);
    layout->addLayout(formLayout);
    layout->addWidget(_launchAtStartupCheckBox);
    layout->addWidget(_startMinimizedCheckBox);
    layout->addWidget(buttonBox);
}

AppSettings SettingsForm::result() const
{
    auto result = _result.deepClone();
    result.launchAtStartup = _launchAtStartupCheckBox->isChecked();
    result.startMinimizedToTray = _startMinimizedCheckBox->isChecked();
    result.windowWidth = _windowWidthSpinBox->value();
    result.windowHeight = _windowHeightSpinBox->value();
    return result;
}

std::optional<AppSettings> SettingsForm::showDialog(QWidget* owner, const AppSettings& settings)
{
    SettingsForm form(settings, owner);

    if (form.exec() != QDialog::Accepted)
    {
        return std::nullopt;
    }

    return form.result();
}

} // namespace SimpleTunnelSSH::App