#include "GroupEditorForm.h"

#include "AppIconFactory.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>

namespace SimpleTunnelSSH::App
{

using SimpleTunnelSSH::Core::Models::TunnelGroup;

GroupEditorForm::GroupEditorForm(const TunnelGroup* group, QWidget* parent)
    : QDialog(parent)
    , _nameEdit(new QLineEdit(this))
    , _result(group == nullptr ? TunnelGroup {} : group->deepClone())
{
    setWindowTitle(group == nullptr ? QStringLiteral("Add group") : QStringLiteral("Edit group"));
    setModal(true);
    setMinimumWidth(420);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    _nameEdit->setText(_result.name);

    auto* formLayout = new QFormLayout();
    formLayout->addRow(QStringLiteral("Name"), _nameEdit);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    AppIconFactory::applyDialogButtonIcons(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &GroupEditorForm::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    layout->addLayout(formLayout);
    layout->addWidget(buttonBox);
}

TunnelGroup GroupEditorForm::result() const
{
    return _result;
}

std::optional<TunnelGroup> GroupEditorForm::showDialog(QWidget* owner, const TunnelGroup* group)
{
    GroupEditorForm form(group, owner);

    if (form.exec() != QDialog::Accepted)
    {
        return std::nullopt;
    }

    return form.result();
}

void GroupEditorForm::accept()
{
    if (_nameEdit->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("Validation error"), QStringLiteral("Group name is required."));
        return;
    }

    _result.name = _nameEdit->text().trimmed();
    QDialog::accept();
}

} // namespace SimpleTunnelSSH::App