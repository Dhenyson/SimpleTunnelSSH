#include "Services/JsonConfigurationStore.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <stdexcept>

namespace SimpleTunnelSSH::Core::Services
{

namespace
{

using namespace SimpleTunnelSSH::Core::Models;

QJsonArray writeIntArray(const QList<int>& values)
{
    QJsonArray array;

    for (const auto value : values)
    {
        array.append(value);
    }

    return array;
}

QList<int> readIntArray(const QJsonValue& value, const QList<int>& fallback)
{
    if (!value.isArray())
    {
        return fallback;
    }

    QList<int> values;

    for (const auto& entry : value.toArray())
    {
        values.append(entry.toInt());
    }

    return values;
}

QJsonObject toJson(const TunnelPort& port)
{
    return QJsonObject {
        { QStringLiteral("id"), port.id.toString(QUuid::WithoutBraces) },
        { QStringLiteral("description"), port.description },
        { QStringLiteral("isEnabled"), port.isEnabled },
        { QStringLiteral("localPort"), port.localPort },
        { QStringLiteral("remoteHost"), port.remoteHost },
        { QStringLiteral("remotePort"), port.remotePort }
    };
}

QJsonObject toJson(const TunnelGroup& group)
{
    QJsonArray ports;

    for (const auto& port : group.ports)
    {
        ports.append(toJson(port));
    }

    return QJsonObject {
        { QStringLiteral("id"), group.id.toString(QUuid::WithoutBraces) },
        { QStringLiteral("name"), group.name },
        { QStringLiteral("isEnabled"), group.isEnabled },
        { QStringLiteral("ports"), ports }
    };
}

QJsonObject toJson(const SshConnectionProfile& connection)
{
    QJsonArray groups;

    for (const auto& group : connection.groups)
    {
        groups.append(toJson(group));
    }

    return QJsonObject {
        { QStringLiteral("id"), connection.id.toString(QUuid::WithoutBraces) },
        { QStringLiteral("name"), connection.name },
        { QStringLiteral("host"), connection.host },
        { QStringLiteral("port"), connection.port },
        { QStringLiteral("userName"), connection.userName },
        { QStringLiteral("identityFilePath"), connection.identityFilePath },
        { QStringLiteral("groups"), groups }
    };
}

QJsonObject toJson(const AppSettings& settings)
{
    return QJsonObject {
        { QStringLiteral("launchAtWindowsStartup"), settings.launchAtStartup },
        { QStringLiteral("startMinimizedToTray"), settings.startMinimizedToTray },
        { QStringLiteral("windowWidth"), settings.windowWidth },
        { QStringLiteral("windowHeight"), settings.windowHeight },
        { QStringLiteral("groupColumnWidths"), writeIntArray(settings.groupColumnWidths) },
        { QStringLiteral("portColumnWidths"), writeIntArray(settings.portColumnWidths) },
        { QStringLiteral("logColumnWidths"), writeIntArray(settings.logColumnWidths) }
    };
}

QJsonObject toJson(const AppConfiguration& configuration)
{
    QJsonArray connections;

    for (const auto& connection : configuration.connections)
    {
        connections.append(toJson(connection));
    }

    auto root = QJsonObject {
        { QStringLiteral("schemaVersion"), configuration.schemaVersion },
        { QStringLiteral("settings"), toJson(configuration.settings) },
        { QStringLiteral("connections"), connections }
    };

    if (!configuration.lastSelectedConnectionId.isNull())
    {
        root.insert(QStringLiteral("lastSelectedConnectionId"), configuration.lastSelectedConnectionId.toString(QUuid::WithoutBraces));
    }

    return root;
}

TunnelPort portFromJson(const QJsonObject& object)
{
    TunnelPort port;
    port.id = QUuid(object.value(QStringLiteral("id")).toString());
    port.description = object.value(QStringLiteral("description")).toString();
    port.isEnabled = object.value(QStringLiteral("isEnabled")).toBool(true);
    port.localPort = object.value(QStringLiteral("localPort")).toInt();
    port.remoteHost = object.value(QStringLiteral("remoteHost")).toString(QStringLiteral("localhost"));
    port.remotePort = object.value(QStringLiteral("remotePort")).toInt();
    return port;
}

TunnelGroup groupFromJson(const QJsonObject& object)
{
    TunnelGroup group;
    group.id = QUuid(object.value(QStringLiteral("id")).toString());
    group.name = object.value(QStringLiteral("name")).toString();
    group.isEnabled = object.value(QStringLiteral("isEnabled")).toBool(true);

    for (const auto& portValue : object.value(QStringLiteral("ports")).toArray())
    {
        if (portValue.isObject())
        {
            group.ports.append(portFromJson(portValue.toObject()));
        }
    }

    return group;
}

SshConnectionProfile connectionFromJson(const QJsonObject& object)
{
    SshConnectionProfile connection;
    connection.id = QUuid(object.value(QStringLiteral("id")).toString());
    connection.name = object.value(QStringLiteral("name")).toString();
    connection.host = object.value(QStringLiteral("host")).toString();
    connection.port = object.value(QStringLiteral("port")).toInt(22);
    connection.userName = object.value(QStringLiteral("userName")).toString();
    connection.identityFilePath = object.value(QStringLiteral("identityFilePath")).toString();

    for (const auto& groupValue : object.value(QStringLiteral("groups")).toArray())
    {
        if (groupValue.isObject())
        {
            connection.groups.append(groupFromJson(groupValue.toObject()));
        }
    }

    return connection;
}

AppSettings settingsFromJson(const QJsonObject& object)
{
    AppSettings settings;
    settings.launchAtStartup = object.value(QStringLiteral("launchAtSystemStartup")).toBool(
        object.value(QStringLiteral("launchAtWindowsStartup")).toBool(false));
    settings.startMinimizedToTray = object.value(QStringLiteral("startMinimizedToTray")).toBool(false);
    settings.windowWidth = object.value(QStringLiteral("windowWidth")).toInt(AppSettings::DefaultWindowWidth);
    settings.windowHeight = object.value(QStringLiteral("windowHeight")).toInt(AppSettings::DefaultWindowHeight);
    settings.groupColumnWidths = readIntArray(object.value(QStringLiteral("groupColumnWidths")), AppSettings::createDefaultGroupColumnWidths());
    settings.portColumnWidths = readIntArray(object.value(QStringLiteral("portColumnWidths")), AppSettings::createDefaultPortColumnWidths());
    settings.logColumnWidths = readIntArray(object.value(QStringLiteral("logColumnWidths")), AppSettings::createDefaultLogColumnWidths());
    return settings;
}

AppConfiguration configurationFromJson(const QJsonDocument& document)
{
    AppConfiguration configuration;
    const auto root = document.object();

    configuration.schemaVersion = root.value(QStringLiteral("schemaVersion")).toInt(2);
    configuration.lastSelectedConnectionId = QUuid(root.value(QStringLiteral("lastSelectedConnectionId")).toString());

    if (root.value(QStringLiteral("settings")).isObject())
    {
        configuration.settings = settingsFromJson(root.value(QStringLiteral("settings")).toObject());
    }

    for (const auto& connectionValue : root.value(QStringLiteral("connections")).toArray())
    {
        if (connectionValue.isObject())
        {
            configuration.connections.append(connectionFromJson(connectionValue.toObject()));
        }
    }

    return configuration;
}

AppConfiguration normalizeConfiguration(const AppConfiguration& original)
{
    auto configuration = original.deepClone();
    configuration.schemaVersion = 2;

    if (configuration.settings.windowWidth < 760)
    {
        configuration.settings.windowWidth = AppSettings::DefaultWindowWidth;
    }

    if (configuration.settings.windowHeight < 520)
    {
        configuration.settings.windowHeight = AppSettings::DefaultWindowHeight;
    }

    auto normalizeColumnWidths = [](QList<int>& widths, const QList<int>& defaults)
    {
        if (widths.size() != defaults.size())
        {
            widths = defaults;
            return;
        }

        for (int index = 0; index < widths.size(); ++index)
        {
            if (widths[index] < 24)
            {
                widths[index] = defaults[index];
            }
        }
    };

    normalizeColumnWidths(configuration.settings.groupColumnWidths, AppSettings::createDefaultGroupColumnWidths());
    normalizeColumnWidths(configuration.settings.portColumnWidths, AppSettings::createDefaultPortColumnWidths());
    normalizeColumnWidths(configuration.settings.logColumnWidths, AppSettings::createDefaultLogColumnWidths());

    for (auto& connection : configuration.connections)
    {
        if (connection.id.isNull())
        {
            connection.id = QUuid::createUuid();
        }

        connection.host = connection.host.trimmed();
        connection.userName = connection.userName.trimmed();
        connection.identityFilePath = connection.identityFilePath.trimmed();

        if (connection.port <= 0)
        {
            connection.port = 22;
        }

        if (connection.name.trimmed().isEmpty())
        {
            connection.name = connection.getEndpointLabel();
        }
        else
        {
            connection.name = connection.name.trimmed();
        }

        for (auto& group : connection.groups)
        {
            if (group.id.isNull())
            {
                group.id = QUuid::createUuid();
            }

            group.name = group.name.trimmed().isEmpty() ? QStringLiteral("Group") : group.name.trimmed();

            for (auto& port : group.ports)
            {
                if (port.id.isNull())
                {
                    port.id = QUuid::createUuid();
                }

                port.description = port.description.trimmed();
                port.remoteHost = port.remoteHost.trimmed().isEmpty() ? QStringLiteral("localhost") : port.remoteHost.trimmed();

                if (port.remotePort <= 0)
                {
                    port.remotePort = port.localPort;
                }
            }

            bool hasEnabledPort = false;

            for (const auto& port : group.ports)
            {
                if (port.isEnabled)
                {
                    hasEnabledPort = true;
                    break;
                }
            }

            if (!hasEnabledPort)
            {
                group.isEnabled = false;
            }
        }
    }

    bool selectionIsValid = false;

    for (const auto& connection : configuration.connections)
    {
        if (connection.id == configuration.lastSelectedConnectionId)
        {
            selectionIsValid = true;
            break;
        }
    }

    if (!selectionIsValid)
    {
        configuration.lastSelectedConnectionId = configuration.connections.isEmpty()
            ? QUuid {}
            : configuration.connections.first().id;
    }

    return configuration;
}

QByteArray serialize(const AppConfiguration& configuration)
{
    return QJsonDocument(toJson(configuration)).toJson(QJsonDocument::Indented);
}

AppConfiguration loadDocument(const QString& filePath)
{
    QFile file(filePath);

    if (!file.exists())
    {
        return AppConfiguration {};
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        throw std::runtime_error(QStringLiteral("Unable to open %1 for reading.").arg(filePath).toStdString());
    }

    const auto document = QJsonDocument::fromJson(file.readAll());
    return normalizeConfiguration(configurationFromJson(document));
}

void writeDocument(const QString& filePath, const AppConfiguration& configuration)
{
    const auto directory = QFileInfo(filePath).absolutePath();

    if (!directory.isEmpty())
    {
        QDir().mkpath(directory);
    }

    QSaveFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        throw std::runtime_error(QStringLiteral("Unable to open %1 for writing.").arg(filePath).toStdString());
    }

    file.write(serialize(normalizeConfiguration(configuration)));

    if (!file.commit())
    {
        throw std::runtime_error(QStringLiteral("Unable to commit %1.").arg(filePath).toStdString());
    }
}

} // namespace

JsonConfigurationStore::JsonConfigurationStore(QString configurationFilePath)
    : _configurationFilePath(std::move(configurationFilePath))
{
}

Models::AppConfiguration JsonConfigurationStore::load() const
{
    return loadDocument(_configurationFilePath);
}

void JsonConfigurationStore::save(const Models::AppConfiguration& configuration) const
{
    writeDocument(_configurationFilePath, configuration);
}

Models::AppConfiguration JsonConfigurationStore::importConfiguration(const QString& filePath) const
{
    return loadDocument(filePath);
}

void JsonConfigurationStore::exportConfiguration(const Models::AppConfiguration& configuration, const QString& filePath) const
{
    writeDocument(filePath, configuration);
}

} // namespace SimpleTunnelSSH::Core::Services