#include "AppIconFactory.h"

#include <QDialogButtonBox>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QSize>

#include "AppTheme.h"

namespace SimpleTunnelSSH::App
{

namespace
{

enum class DialogGlyph
{
    Confirm,
    Cancel
};

QRectF shellBoundsForSize(int size, bool includeIndicator)
{
    return includeIndicator
        ? QRectF(size * 0.05, size * 0.05, size * 0.78, size * 0.78)
        : QRectF(size * 0.08, size * 0.08, size * 0.84, size * 0.84);
}

QPixmap createShellPixmap(int size, bool includeIndicator, const QColor& indicatorColor)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF shellBounds = shellBoundsForSize(size, includeIndicator);

    QPainterPath shellPath;
    shellPath.addRoundedRect(shellBounds, size * 0.18, size * 0.18);

    QLinearGradient shellGradient(shellBounds.topLeft(), shellBounds.bottomRight());
    shellGradient.setColorAt(0.0, QColor(38, 48, 59));
    shellGradient.setColorAt(1.0, QColor(19, 24, 30));
    painter.fillPath(shellPath, shellGradient);
    painter.setPen(QPen(QColor(92, 103, 116), qMax(1.5, size * 0.045)));
    painter.drawPath(shellPath);

    QPen linePen(QColor(121, 214, 255), qMax(2.8, size * 0.10));
    linePen.setCapStyle(Qt::RoundCap);
    painter.setPen(linePen);

    const qreal leftX = shellBounds.left() + (shellBounds.width() * 0.33);
    const qreal rightX = shellBounds.left() + (shellBounds.width() * 0.67);
    const qreal topY = shellBounds.top() + (shellBounds.height() * 0.26);
    const qreal bottomY = shellBounds.top() + (shellBounds.height() * 0.74);
    const qreal centerY = shellBounds.top() + (shellBounds.height() * 0.50);

    painter.drawLine(QPointF(leftX, topY), QPointF(leftX, bottomY));
    painter.drawLine(QPointF(rightX, topY), QPointF(rightX, bottomY));
    painter.drawLine(QPointF(leftX, centerY), QPointF(rightX, centerY));

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(237, 243, 251));
    const qreal nodeSize = shellBounds.width() * 0.14;

    painter.drawEllipse(QRectF(leftX - (nodeSize / 2.0), topY - (nodeSize / 2.0), nodeSize, nodeSize));
    painter.drawEllipse(QRectF(leftX - (nodeSize / 2.0), bottomY - (nodeSize / 2.0), nodeSize, nodeSize));
    painter.drawEllipse(QRectF(rightX - (nodeSize / 2.0), topY - (nodeSize / 2.0), nodeSize, nodeSize));
    painter.drawEllipse(QRectF(rightX - (nodeSize / 2.0), bottomY - (nodeSize / 2.0), nodeSize, nodeSize));

    if (includeIndicator)
    {
        const qreal indicatorSize = size * 0.28;
        const qreal indicatorX = size - indicatorSize - (size * 0.07);
        const qreal indicatorY = size - indicatorSize - (size * 0.07);

        painter.setBrush(indicatorColor);
        painter.setPen(QPen(QColor(245, 247, 250), qMax(1.5, size * 0.035)));
        painter.drawEllipse(QRectF(indicatorX, indicatorY, indicatorSize, indicatorSize));
    }

    return pixmap;
}

QIcon createShellIcon(bool includeIndicator, const QColor& indicatorColor)
{
    QIcon icon;

    for (const int size : {16, 20, 24, 32, 48, 64, 128, 256})
    {
        icon.addPixmap(createShellPixmap(size, includeIndicator, indicatorColor));
    }

    return icon;
}

QPixmap createDialogActionPixmap(int size, DialogGlyph glyph)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF badgeBounds(size * 0.10, size * 0.10, size * 0.80, size * 0.80);
    QPainterPath badgePath;
    badgePath.addRoundedRect(badgeBounds, size * 0.24, size * 0.24);

    if (glyph == DialogGlyph::Confirm)
    {
        QLinearGradient gradient(badgeBounds.topLeft(), badgeBounds.bottomRight());
        gradient.setColorAt(0.0, QColor(82, 199, 140));
        gradient.setColorAt(1.0, QColor(43, 156, 106));
        painter.fillPath(badgePath, gradient);
        painter.setPen(QPen(QColor(208, 245, 226, 180), qMax(1.0, size * 0.05)));
    }
    else
    {
        QLinearGradient gradient(badgeBounds.topLeft(), badgeBounds.bottomRight());
        gradient.setColorAt(0.0, QColor(86, 96, 109));
        gradient.setColorAt(1.0, QColor(57, 66, 77));
        painter.fillPath(badgePath, gradient);
        painter.setPen(QPen(QColor(189, 198, 208, 145), qMax(1.0, size * 0.05)));
    }

    painter.drawPath(badgePath);

    QPen glyphPen(QColor(245, 248, 252), qMax(1.8, size * 0.11));
    glyphPen.setCapStyle(Qt::RoundCap);
    glyphPen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(glyphPen);

    if (glyph == DialogGlyph::Confirm)
    {
        painter.drawLine(QPointF(size * 0.30, size * 0.54), QPointF(size * 0.45, size * 0.69));
        painter.drawLine(QPointF(size * 0.45, size * 0.69), QPointF(size * 0.72, size * 0.36));
    }
    else
    {
        painter.drawLine(QPointF(size * 0.34, size * 0.34), QPointF(size * 0.66, size * 0.66));
        painter.drawLine(QPointF(size * 0.66, size * 0.34), QPointF(size * 0.34, size * 0.66));
    }

    return pixmap;
}

QIcon createDialogActionIcon(DialogGlyph glyph)
{
    QIcon icon;

    for (const int size : {16, 18, 20, 24, 32})
    {
        icon.addPixmap(createDialogActionPixmap(size, glyph));
    }

    return icon;
}

void configureDialogButton(QDialogButtonBox* buttonBox, QDialogButtonBox::StandardButton standardButton, const QIcon& icon)
{
    if (buttonBox == nullptr)
    {
        return;
    }

    auto* button = buttonBox->button(standardButton);

    if (button == nullptr)
    {
        return;
    }

    button->setIcon(icon);
    button->setIconSize(QSize(18, 18));
}

QColor indicatorColor(ConnectionIndicatorState state)
{
    switch (state)
    {
    case ConnectionIndicatorState::Active:
        return AppTheme::connectionConnectedColor();
    case ConnectionIndicatorState::Attention:
        return AppTheme::connectionAttentionColor();
    case ConnectionIndicatorState::Loading:
    case ConnectionIndicatorState::Inactive:
    default:
        return AppTheme::connectionDisconnectedColor();
    }
}

} // namespace

QIcon AppIconFactory::applicationIcon()
{
    return createShellIcon(false, AppTheme::connectionDisconnectedColor());
}

QIcon AppIconFactory::inactiveTrayIcon()
{
    return createShellIcon(true, AppTheme::connectionDisconnectedColor());
}

QIcon AppIconFactory::activeTrayIcon()
{
    return createShellIcon(true, AppTheme::connectionConnectedColor());
}

QIcon AppIconFactory::attentionTrayIcon()
{
    return createShellIcon(true, AppTheme::connectionAttentionColor());
}

QIcon AppIconFactory::connectionIndicatorIcon(ConnectionIndicatorState state, int frame)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QRectF bounds(2.0, 2.0, 12.0, 12.0);

    if (state == ConnectionIndicatorState::Loading)
    {
        painter.setPen(QPen(AppTheme::connectionDisconnectedColor(), 2.0));
        painter.drawEllipse(bounds);

        QPen arcPen(AppTheme::connectionConnectedColor(), 2.4);
        arcPen.setCapStyle(Qt::RoundCap);
        painter.setPen(arcPen);
        painter.drawArc(bounds, frame * 30 * 16, 220 * 16);
        return QIcon(pixmap);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(indicatorColor(state));
    painter.drawEllipse(bounds);
    return QIcon(pixmap);
}

void AppIconFactory::applyDialogButtonIcons(QDialogButtonBox* buttonBox)
{
    const auto confirmIcon = createDialogActionIcon(DialogGlyph::Confirm);
    const auto cancelIcon = createDialogActionIcon(DialogGlyph::Cancel);

    configureDialogButton(buttonBox, QDialogButtonBox::Save, confirmIcon);
    configureDialogButton(buttonBox, QDialogButtonBox::Ok, confirmIcon);
    configureDialogButton(buttonBox, QDialogButtonBox::Cancel, cancelIcon);
}

} // namespace SimpleTunnelSSH::App