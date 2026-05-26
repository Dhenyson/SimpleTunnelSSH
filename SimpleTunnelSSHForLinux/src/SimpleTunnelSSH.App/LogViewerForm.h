#pragma once

#include <QList>

#include <QDialog>

#include "Models/RuntimeLogEntry.h"

class QLabel;
class QTableWidget;

namespace SimpleTunnelSSH::App
{

class TunnelApplicationContext;

class LogViewerForm final : public QDialog
{
    Q_OBJECT

public:
    explicit LogViewerForm(TunnelApplicationContext* applicationContext, QWidget* parent = nullptr);

    void render(
        const QList<SimpleTunnelSSH::Core::Models::RuntimeLogEntry>& entries,
        const QList<int>& columnWidths);

private:
    void applyColumnWidths(const QList<int>& widths);
    QList<int> currentColumnWidths() const;

    TunnelApplicationContext* _applicationContext;
    QTableWidget* _logTable;
    QLabel* _summaryLabel;
    bool _suppressColumnWidthPersistence { false };
};

} // namespace SimpleTunnelSSH::App