// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QMap>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

struct PluginManifest {
	QString id;
	QString name;
	QString version;
	QString author;
	QString entry;
	QString kind;
	bool trusted = false;
};

struct PluginCapabilities {
	bool network = false;
	bool filesystem = false;
	bool notifications = false;
	bool uiOverlay = false;
};

class PluginSignatures {
public:
	[[nodiscard]] static bool verify(
		const QString &pluginId,
		const QByteArray &payload,
		const QByteArray &signatureHex);
	[[nodiscard]] static QByteArray fingerprint(const QByteArray &payload);
};

class PluginEngine {
public:
	static PluginEngine &instance();

	void initialize();

	[[nodiscard]] std::vector<PluginManifest> installed() const;
	[[nodiscard]] bool isActive(const QString &pluginId) const;
	void setActive(const QString &pluginId, bool active);

	bool installFromFile(const QString &filePath, QString *error = nullptr);
	void uninstall(const QString &pluginId);

	[[nodiscard]] bool hasWasmRuntime() const;
	[[nodiscard]] QString wasmRuntimeVersion() const;

private:
	PluginEngine();
	void load();
	void save();

	std::vector<PluginManifest> _installed;
	std::vector<QString> _active;
	bool _initialized = false;
};

class HookManager {
public:
	static HookManager &instance();

	void initialize();

	void registerHook(const QString &event, const QString &pluginId);
	void unregisterPlugin(const QString &pluginId);
	[[nodiscard]] std::vector<QString> pluginsForEvent(const QString &event) const;

private:
	HookManager();

	QMap<QString, std::vector<QString>> _hooks;
	bool _initialized = false;
};

class InAppNotifications {
public:
	static InAppNotifications &instance();

	void initialize();

	[[nodiscard]] bool isEnabled() const;
	void setEnabled(bool enabled);

	void notify(
		const QString &pluginId,
		const QString &title,
		const QString &body);

	[[nodiscard]] bool isPluginActive(const QString &pluginId) const;

private:
	InAppNotifications();

	bool _enabled = true;
	bool _initialized = false;
};

class PluginForge {
public:
	[[nodiscard]] static QString scaffoldCpp(const QString &pluginId, const QString &name);
	[[nodiscard]] static QString scaffoldRust(const QString &pluginId, const QString &name);
	[[nodiscard]] static QString scaffoldManifest(
		const QString &pluginId,
		const QString &name,
		const QString &version);
	[[nodiscard]] static bool validateId(const QString &pluginId);
};

} // namespace Miogram
