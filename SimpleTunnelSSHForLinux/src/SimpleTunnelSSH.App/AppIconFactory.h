#pragma once

#include <QIcon>

class QDialogButtonBox;

namespace SimpleTunnelSSH::App
{

enum class ConnectionIndicatorState
{
    Inactive,
    Active,
    Attention,
    Loading
};

class AppIconFactory
{
public:
    static QIcon applicationIcon();
    static QIcon inactiveTrayIcon();
    static QIcon activeTrayIcon();
    static QIcon attentionTrayIcon();
    static QIcon connectionIndicatorIcon(ConnectionIndicatorState state, int frame = 0);
    static void applyDialogButtonIcons(QDialogButtonBox* buttonBox);
};

} // namespace SimpleTunnelSSH::App