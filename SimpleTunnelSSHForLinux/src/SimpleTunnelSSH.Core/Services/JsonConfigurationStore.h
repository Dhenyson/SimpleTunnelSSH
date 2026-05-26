#pragma once

#include <QString>

#include "Models/AppConfiguration.h"

namespace SimpleTunnelSSH::Core::Services
{

class JsonConfigurationStore
{
public:
    explicit JsonConfigurationStore(QString configurationFilePath);

    [[nodiscard]] Models::AppConfiguration load() const;
    void save(const Models::AppConfiguration& configuration) const;
    [[nodiscard]] Models::AppConfiguration importConfiguration(const QString& filePath) const;
    void exportConfiguration(const Models::AppConfiguration& configuration, const QString& filePath) const;

private:
    QString _configurationFilePath;
};

} // namespace SimpleTunnelSSH::Core::Services