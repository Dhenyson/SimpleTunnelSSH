#pragma once

#include <optional>

#include <QDialog>

class QLabel;
class QLineEdit;

namespace SimpleTunnelSSH::App
{

class AskPassPromptForm final : public QDialog
{
    Q_OBJECT

public:
    explicit AskPassPromptForm(const QString& prompt, QWidget* parent = nullptr);

    [[nodiscard]] QString secret() const;

    static std::optional<QString> showPrompt(const QString& prompt);

private:
    QLabel* _promptLabel;
    QLineEdit* _secretEdit;
};

} // namespace SimpleTunnelSSH::App