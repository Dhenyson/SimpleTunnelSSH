#include "AppTheme.h"

#include <QApplication>
#include <QMenu>
#include <QPalette>
#include <QStyleFactory>
#include <QWidget>

namespace SimpleTunnelSSH::App
{

namespace
{

QString globalStyleSheet()
{
    return QStringLiteral(R"(
        QWidget {
            background-color: #181b1f;
            color: #eceff4;
        }
        QMainWindow, QDialog, QMenuBar, QStatusBar, QToolTip {
            background-color: #181b1f;
            color: #eceff4;
        }
        QMenuBar::item:selected, QMenu::item:selected {
            background-color: #304a78;
        }
        QGroupBox {
            border: 1px solid #484f59;
            border-radius: 8px;
            margin-top: 10px;
            padding-top: 10px;
            background-color: #1f242a;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
        }
        QPushButton {
            background-color: #282e36;
            border: 1px solid #484f59;
            border-radius: 6px;
            padding: 6px 10px;
        }
        QPushButton:hover {
            background-color: #304a78;
        }
        QPushButton:pressed {
            background-color: #1f242a;
        }
        QPushButton:disabled {
            color: #a2abb6;
            background-color: #1f242a;
        }
        QLineEdit, QSpinBox, QListWidget, QTableWidget, QPlainTextEdit {
            background-color: #282e36;
            border: 1px solid #484f59;
            border-radius: 6px;
            selection-background-color: #304a78;
            selection-color: #eceff4;
        }
        QHeaderView::section {
            background-color: #2e353e;
            color: #eceff4;
            border: 1px solid #484f59;
            padding: 6px;
        }
        QCheckBox, QLabel {
            color: #eceff4;
        }
        QStatusBar {
            border-top: 1px solid #484f59;
        }
        QSplitter::handle {
            background-color: #484f59;
        }
    )");
}

} // namespace

QColor AppTheme::backgroundColor()
{
    return QColor(24, 27, 31);
}

QColor AppTheme::surfaceColor()
{
    return QColor(31, 36, 42);
}

QColor AppTheme::surfaceAltColor()
{
    return QColor(40, 46, 54);
}

QColor AppTheme::headerColor()
{
    return QColor(46, 53, 62);
}

QColor AppTheme::borderColor()
{
    return QColor(72, 79, 89);
}

QColor AppTheme::foregroundColor()
{
    return QColor(236, 239, 244);
}

QColor AppTheme::mutedForegroundColor()
{
    return QColor(162, 171, 182);
}

QColor AppTheme::selectionColor()
{
    return QColor(48, 74, 120);
}

QColor AppTheme::connectionConnectedColor()
{
    return QColor(66, 191, 110);
}

QColor AppTheme::connectionDisconnectedColor()
{
    return QColor(124, 132, 143);
}

QColor AppTheme::connectionAttentionColor()
{
    return QColor(214, 76, 76);
}

void AppTheme::apply(QApplication& application)
{
    application.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QPalette palette;
    palette.setColor(QPalette::Window, backgroundColor());
    palette.setColor(QPalette::WindowText, foregroundColor());
    palette.setColor(QPalette::Base, surfaceAltColor());
    palette.setColor(QPalette::AlternateBase, surfaceColor());
    palette.setColor(QPalette::ToolTipBase, surfaceColor());
    palette.setColor(QPalette::ToolTipText, foregroundColor());
    palette.setColor(QPalette::Text, foregroundColor());
    palette.setColor(QPalette::Button, surfaceAltColor());
    palette.setColor(QPalette::ButtonText, foregroundColor());
    palette.setColor(QPalette::BrightText, foregroundColor());
    palette.setColor(QPalette::Link, QColor(120, 211, 255));
    palette.setColor(QPalette::Highlight, selectionColor());
    palette.setColor(QPalette::HighlightedText, foregroundColor());
    palette.setColor(QPalette::PlaceholderText, mutedForegroundColor());
    application.setPalette(palette);
    application.setStyleSheet(globalStyleSheet());
}

void AppTheme::apply(QWidget* widget)
{
    if (widget == nullptr)
    {
        return;
    }

    widget->setAutoFillBackground(true);
}

void AppTheme::applyMenu(QMenu* menu)
{
    if (menu == nullptr)
    {
        return;
    }

    menu->setStyleSheet(QStringLiteral(R"(
        QMenu {
            background-color: #1f242a;
            border: 1px solid #484f59;
            color: #eceff4;
        }
        QMenu::item:selected {
            background-color: #304a78;
        }
    )"));
}

} // namespace SimpleTunnelSSH::App