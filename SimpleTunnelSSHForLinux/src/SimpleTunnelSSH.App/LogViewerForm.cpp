#include "LogViewerForm.h"

#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "TunnelApplicationContext.h"

namespace SimpleTunnelSSH::App
{

LogViewerForm::LogViewerForm(TunnelApplicationContext* applicationContext, QWidget* parent)
    : QDialog(parent)
    , _applicationContext(applicationContext)
    , _logTable(new QTableWidget(this))
    , _summaryLabel(new QLabel(QStringLiteral("Showing the most recent runtime events."), this))
{
    setWindowTitle(QStringLiteral("Recent Logs"));
    setMinimumSize(760, 360);
    resize(920, 520);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    _summaryLabel->setWordWrap(true);

    auto* copyButton = new QPushButton(QStringLiteral("Copy Logs"), this);
    auto* clearButton = new QPushButton(QStringLiteral("Clear Logs"), this);

    connect(copyButton, &QPushButton::clicked, this, [this]() {
        const auto logsText = _applicationContext->getRuntimeLogsText();

        if (logsText.trimmed().isEmpty())
        {
            QMessageBox::information(this, QStringLiteral("Recent Logs"), QStringLiteral("There are no logs to copy."));
            return;
        }

        QApplication::clipboard()->setText(logsText);
    });

    connect(clearButton, &QPushButton::clicked, this, [this]() {
        if (_logTable->rowCount() == 0)
        {
            return;
        }

        const auto result = QMessageBox::question(
            this,
            QStringLiteral("Recent Logs"),
            QStringLiteral("Clear all runtime logs?"));

        if (result == QMessageBox::Yes)
        {
            _applicationContext->clearRuntimeLogs();
        }
    });

    auto* actionsLayout = new QHBoxLayout();
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->addWidget(copyButton);
    actionsLayout->addWidget(clearButton);
    actionsLayout->addStretch(1);

    _logTable->setColumnCount(3);
    _logTable->setHorizontalHeaderLabels({ QStringLiteral("Time"), QStringLiteral("Level"), QStringLiteral("Message") });
    _logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _logTable->setSelectionMode(QAbstractItemView::SingleSelection);
    _logTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _logTable->setAlternatingRowColors(false);
    _logTable->verticalHeader()->setVisible(false);
    _logTable->horizontalHeader()->setStretchLastSection(false);

    connect(_logTable->horizontalHeader(), &QHeaderView::sectionResized, this, [this]() {
        if (_suppressColumnWidthPersistence)
        {
            return;
        }

        _applicationContext->saveLogColumnWidths(currentColumnWidths());
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);
    layout->addWidget(_summaryLabel);
    layout->addLayout(actionsLayout);
    layout->addWidget(_logTable, 1);
}

void LogViewerForm::render(
    const QList<SimpleTunnelSSH::Core::Models::RuntimeLogEntry>& entries,
    const QList<int>& columnWidths)
{
    applyColumnWidths(columnWidths);

    _logTable->setRowCount(entries.size());

    for (int row = 0; row < entries.size(); ++row)
    {
        const auto& entry = entries[row];
        _logTable->setItem(row, 0, new QTableWidgetItem(entry.timestampUtc.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));
        _logTable->setItem(row, 1, new QTableWidgetItem(entry.level));
        _logTable->setItem(row, 2, new QTableWidgetItem(entry.message));
    }

    _summaryLabel->setText(QStringLiteral("Showing the latest %1 runtime event(s).").arg(entries.size()));
    setWindowTitle(QStringLiteral("Recent Logs (%1)").arg(entries.size()));

    if (_logTable->rowCount() > 0)
    {
        _logTable->scrollToBottom();
    }
}

void LogViewerForm::applyColumnWidths(const QList<int>& widths)
{
    if (_logTable->columnCount() != widths.size())
    {
        return;
    }

    _suppressColumnWidthPersistence = true;

    for (int index = 0; index < widths.size(); ++index)
    {
        _logTable->setColumnWidth(index, widths[index]);
    }

    _suppressColumnWidthPersistence = false;
}

QList<int> LogViewerForm::currentColumnWidths() const
{
    QList<int> widths;

    for (int index = 0; index < _logTable->columnCount(); ++index)
    {
        widths.append(_logTable->columnWidth(index));
    }

    return widths;
}

} // namespace SimpleTunnelSSH::App