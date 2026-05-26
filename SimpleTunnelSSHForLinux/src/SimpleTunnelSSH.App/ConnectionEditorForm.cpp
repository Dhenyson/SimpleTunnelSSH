#include "ConnectionEditorForm.h"

#include "AppIconFactory.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace SimpleTunnelSSH::App
{

using SimpleTunnelSSH::Core::Models::SshConnectionProfile;

ConnectionEditorForm::ConnectionEditorForm(const SshConnectionProfile* connection, QWidget* parent)
    : QDialog(parent)
    , _nameEdit(new QLineEdit(this))
    , _hostEdit(new QLineEdit(this))
    , _portSpinBox(new QSpinBox(this))
    , _userEdit(new QLineEdit(this))
    , _identityFileEdit(new QLineEdit(this))
    , _result(connection == nullptr ? SshConnectionProfile {} : connection->deepClone())
{
    setWindowTitle(connection == nullptr ? QStringLiteral("Add connection") : QStringLiteral("Edit connection"));
    setModal(true);
    setMinimumWidth(520);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    _nameEdit->setText(_result.name);
    _hostEdit->setText(_result.host);
    _portSpinBox->setRange(1, 65535);
    _portSpinBox->setValue(_result.port <= 0 ? 22 : _result.port);
    _userEdit->setText(_result.userName);
    _identityFileEdit->setText(_result.identityFilePath);

    auto* browseButton = new QPushButton(QStringLiteral("Browse"), this);
    connect(browseButton, &QPushButton::clicked, this, &ConnectionEditorForm::browseIdentityFile);

    auto* identityLayout = new QHBoxLayout();
    identityLayout->setContentsMargins(0, 0, 0, 0);
    identityLayout->addWidget(_identityFileEdit);
    identityLayout->addWidget(browseButton);

    auto* formLayout = new QFormLayout();
    formLayout->addRow(QStringLiteral("Name"), _nameEdit);
    formLayout->addRow(QStringLiteral("Host"), _hostEdit);
    formLayout->addRow(QStringLiteral("Port"), _portSpinBox);
    formLayout->addRow(QStringLiteral("User"), _userEdit);
    formLayout->addRow(QStringLiteral("Identity file"), identityLayout);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    AppIconFactory::applyDialogButtonIcons(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConnectionEditorForm::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    layout->addLayout(formLayout);
    layout->addWidget(buttonBox);
}

SshConnectionProfile ConnectionEditorForm::result() const
{
    return _result;
}

std::optional<SshConnectionProfile> ConnectionEditorForm::showDialog(QWidget* owner, const SshConnectionProfile* connection)
{
    ConnectionEditorForm form(connection, owner);

    if (form.exec() != QDialog::Accepted)
    {
        return std::nullopt;
    }

    return form.result();
}

void ConnectionEditorForm::browseIdentityFile()
{
    const auto filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Choose identity file"),
        _identityFileEdit->text(),
        QStringLiteral("Private key files (*.pem *.ppk *.*)"));

    if (!filePath.isEmpty())
    {
        _identityFileEdit->setText(filePath);
    }
}

void ConnectionEditorForm::accept()
{
    if (_nameEdit->text().trimmed().isEmpty() || _hostEdit->text().trimmed().isEmpty() || _userEdit->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("Validation error"), QStringLiteral("Name, host and user are required."));
        return;
    }

    _result.name = _nameEdit->text().trimmed();
    _result.host = _hostEdit->text().trimmed();
    _result.port = _portSpinBox->value();
    _result.userName = _userEdit->text().trimmed();
    _result.identityFilePath = _identityFileEdit->text().trimmed();
    QDialog::accept();
}

} // namespace SimpleTunnelSSH::App