using System.Text.Json;
using SimpleTunnelSSH.Core.Models;

namespace SimpleTunnelSSH.Core.Services;

public sealed class JsonConfigurationStore
{
    private static readonly JsonSerializerOptions SerializerOptions = new()
    {
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        WriteIndented = true
    };

    private readonly string _configurationFilePath;

    public JsonConfigurationStore(string configurationFilePath)
    {
        _configurationFilePath = configurationFilePath;
    }

    public async Task<AppConfiguration> LoadAsync(CancellationToken cancellationToken = default)
    {
        if (!File.Exists(_configurationFilePath))
        {
            return new AppConfiguration();
        }

        await using var inputStream = File.OpenRead(_configurationFilePath);
        var configuration = await JsonSerializer.DeserializeAsync<AppConfiguration>(
            inputStream,
            SerializerOptions,
            cancellationToken).ConfigureAwait(false);

        return configuration?.DeepClone() ?? new AppConfiguration();
    }

    public async Task SaveAsync(AppConfiguration configuration, CancellationToken cancellationToken = default)
    {
        var directoryPath = Path.GetDirectoryName(_configurationFilePath);

        if (!string.IsNullOrWhiteSpace(directoryPath))
        {
            Directory.CreateDirectory(directoryPath);
        }

        var temporaryFilePath = _configurationFilePath + ".tmp";

        await using (var outputStream = File.Create(temporaryFilePath))
        {
            await JsonSerializer.SerializeAsync(
                outputStream,
                configuration,
                SerializerOptions,
                cancellationToken).ConfigureAwait(false);
        }

        File.Move(temporaryFilePath, _configurationFilePath, true);
    }

    public async Task<AppConfiguration> ImportAsync(string filePath, CancellationToken cancellationToken = default)
    {
        await using var inputStream = File.OpenRead(filePath);
        var configuration = await JsonSerializer.DeserializeAsync<AppConfiguration>(
            inputStream,
            SerializerOptions,
            cancellationToken).ConfigureAwait(false);

        return configuration?.DeepClone() ?? new AppConfiguration();
    }

    public async Task ExportAsync(AppConfiguration configuration, string filePath, CancellationToken cancellationToken = default)
    {
        var directoryPath = Path.GetDirectoryName(filePath);

        if (!string.IsNullOrWhiteSpace(directoryPath))
        {
            Directory.CreateDirectory(directoryPath);
        }

        await using var outputStream = File.Create(filePath);
        await JsonSerializer.SerializeAsync(
            outputStream,
            configuration,
            SerializerOptions,
            cancellationToken).ConfigureAwait(false);
    }
}