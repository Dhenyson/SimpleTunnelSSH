#include "AskPassPromptForm.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include "AppIconFactory.h"

namespace SimpleTunnelSSH::App
{

AskPassPromptForm::AskPassPromptForm(const QString& prompt, QWidget* parent)
    : QDialog(parent)
    , _promptLabel(new QLabel(this))
    , _secretEdit(new QLineEdit(this))
{
    setWindowTitle(QStringLiteral("SSH Authentication"));
    setWindowIcon(AppIconFactory::applicationIcon());
    setModal(true);
    setMinimumWidth(520);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);

    _promptLabel->setWordWrap(true);
    _promptLabel->setText(prompt.trimmed().isEmpty() ? QStringLiteral("SSH authentication is required.") : prompt.trimmed());

    _secretEdit->setEchoMode(QLineEdit::Password);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Continue"));
    AppIconFactory::applyDialogButtonIcons(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    layout->addWidget(_promptLabel);
    layout->addWidget(_secretEdit);
    layout->addWidget(buttonBox);

    QTimer::singleShot(0, this, [this]() {
        _secretEdit->selectAll();
        _secretEdit->setFocus();
    });
}

QString AskPassPromptForm::secret() const
{
    return _secretEdit->text();
}

std::optional<QString> AskPassPromptForm::showPrompt(const QString& prompt)
{
    AskPassPromptForm form(prompt);

    if (form.exec() != QDialog::Accepted)
    {
        return std::nullopt;
    }

    return form.secret();
}

} // namespace SimpleTunnelSSH::App