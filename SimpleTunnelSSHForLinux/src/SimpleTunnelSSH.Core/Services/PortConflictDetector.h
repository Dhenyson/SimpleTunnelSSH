#pragma once

#include <QList>

#include "Models/PortConflict.h"
#include "Models/TunnelGroup.h"

namespace SimpleTunnelSSH::Core::Services
{

class PortConflictDetector
{
public:
    static QList<Models::PortConflict> detect(const QList<Models::TunnelGroup>& groups);
};

} // namespace SimpleTunnelSSH::Core::Services