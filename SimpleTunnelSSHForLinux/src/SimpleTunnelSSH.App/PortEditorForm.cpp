#include "PortEditorForm.h"

#include "AppIconFactory.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QVBoxLayout>

namespace SimpleTunnelSSH::App
{

using SimpleTunnelSSH::Core::Models::TunnelPort;

PortEditorForm::PortEditorForm(const TunnelPort* port, QWidget* parent)
    : QDialog(parent)
    , _descriptionEdit(new QLineEdit(this))
    , _localPortSpinBox(new QSpinBox(this))
    , _remoteHostEdit(new QLineEdit(this))
    , _remotePortSpinBox(new QSpinBox(this))
    , _result(port == nullptr ? TunnelPort {} : port->deepClone())
{
    setWindowTitle(port == nullptr ? QStringLiteral("Add port") : QStringLiteral("Edit port"));
    setModal(true);
    setMinimumWidth(480);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    _descriptionEdit->setText(_result.description);
    _localPortSpinBox->setRange(1, 65535);
    _localPortSpinBox->setValue(_result.localPort <= 0 ? 3000 : _result.localPort);
    _remoteHostEdit->setText(_result.remoteHost.isEmpty() ? QStringLiteral("localhost") : _result.remoteHost);
    _remotePortSpinBox->setRange(1, 65535);
    _remotePortSpinBox->setValue(_result.remotePort <= 0 ? (_result.localPort <= 0 ? 3000 : _result.localPort) : _result.remotePort);

    auto* formLayout = new QFormLayout();
    formLayout->addRow(QStringLiteral("Description"), _descriptionEdit);
    formLayout->addRow(QStringLiteral("Local port"), _localPortSpinBox);
    formLayout->addRow(QStringLiteral("Remote host"), _remoteHostEdit);
    formLayout->addRow(QStringLiteral("Remote port"), _remotePortSpinBox);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    AppIconFactory::applyDialogButtonIcons(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &PortEditorForm::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    layout->addLayout(formLayout);
    layout->addWidget(buttonBox);
}

TunnelPort PortEditorForm::result() const
{
    return _result;
}

std::optional<TunnelPort> PortEditorForm::showDialog(QWidget* owner, const TunnelPort* port)
{
    PortEditorForm form(port, owner);

    if (form.exec() != QDialog::Accepted)
    {
        return std::nullopt;
    }

    return form.result();
}

void PortEditorForm::accept()
{
    if (_remoteHostEdit->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("Validation error"), QStringLiteral("Remote host is required."));
        return;
    }

    _result.description = _descriptionEdit->text().trimmed();
    _result.localPort = _localPortSpinBox->value();
    _result.remoteHost = _remoteHostEdit->text().trimmed();
    _result.remotePort = _remotePortSpinBox->value();
    QDialog::accept();
}

} // namespace SimpleTunnelSSH::App