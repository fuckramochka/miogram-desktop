// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_plugins.h"

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QCryptographicHash>
#include <QtCore/QRegularExpression>
#include <algorithm>

#include "ayu/features/miogram/miogram_config.h"
#include "core/application.h"

namespace Miogram {

bool PluginSignatures::verify(
		const QString &pluginId,
		const QByteArray &payload,
		const QByteArray &signatureHex) {
	if (pluginId.trimmed().isEmpty() || payload.isEmpty() || signatureHex.isEmpty()) {
		return false;
	}
	const auto expected = fingerprint(payload);
	return expected.toLower() == signatureHex.toLower();
}

QByteArray PluginSignatures::fingerprint(const QByteArray &payload) {
	return QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex();
}

PluginEngine &PluginEngine::instance() {
	static PluginEngine engine;
	return engine;
}

PluginEngine::PluginEngine() {
}

void PluginEngine::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	load();
	QDir().mkpath(cWorkingDir() + u"tdata/mio_plugins/"_q);
}

std::vector<PluginManifest> PluginEngine::installed() const {
	return _installed;
}

bool PluginEngine::isActive(const QString &pluginId) const {
	return std::find(_active.begin(), _active.end(), pluginId) != _active.end();
}

void PluginEngine::setActive(const QString &pluginId, bool active) {
	if (active && !isActive(pluginId)) {
		_active.push_back(pluginId);
	} else if (!active) {
		_active.erase(
			std::remove(_active.begin(), _active.end(), pluginId),
			_active.end());
	}
	save();
}

bool PluginEngine::installFromFile(const QString &filePath, QString *error) {
	QFile f(filePath);
	if (!f.open(QIODevice::ReadOnly)) {
		if (error) {
			*error = u"Cannot open file"_q;
		}
		return false;
	}
	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject()) {
		if (error) {
			*error = u"Invalid manifest"_q;
		}
		return false;
	}
	const auto obj = doc.object();
	PluginManifest manifest;
	manifest.id = obj.value(u"id"_q).toString().trimmed();
	manifest.name = obj.value(u"name"_q).toString();
	manifest.version = obj.value(u"version"_q).toString(u"1.0.0"_q);
	manifest.author = obj.value(u"author"_q).toString();
	manifest.entry = obj.value(u"entry"_q).toString();
	manifest.kind = obj.value(u"kind"_q).toString(u"cpp"_q);
	if (!PluginForge::validateId(manifest.id)) {
		if (error) {
			*error = u"Invalid plugin id"_q;
		}
		return false;
	}
	for (auto &existing : _installed) {
		if (existing.id == manifest.id) {
			existing = manifest;
			save();
			return true;
		}
	}
	_installed.push_back(manifest);
	save();
	return true;
}

void PluginEngine::uninstall(const QString &pluginId) {
	_installed.erase(
		std::remove_if(_installed.begin(), _installed.end(), [&](const PluginManifest &m) {
			return m.id == pluginId;
		}),
		_installed.end());
	setActive(pluginId, false);
	HookManager::instance().unregisterPlugin(pluginId);
}

bool PluginEngine::hasWasmRuntime() const {
	return false;
}

QString PluginEngine::wasmRuntimeVersion() const {
	return u"wamr-stub (no native runtime linked)"_q;
}

void PluginEngine::load() {
	QFile f(cWorkingDir() + u"tdata/mio_plugins.json"_q);
	if (!f.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject()) {
		return;
	}
	const auto obj = doc.object();
	_installed.clear();
	for (const auto &v : obj.value(u"installed"_q).toArray()) {
		const auto o = v.toObject();
		PluginManifest m;
		m.id = o.value(u"id"_q).toString();
		m.name = o.value(u"name"_q).toString();
		m.version = o.value(u"version"_q).toString();
		m.author = o.value(u"author"_q).toString();
		m.entry = o.value(u"entry"_q).toString();
		m.kind = o.value(u"kind"_q).toString();
		m.trusted = o.value(u"trusted"_q).toBool(false);
		if (!m.id.isEmpty()) {
			_installed.push_back(m);
		}
	}
	_active.clear();
	for (const auto &v : obj.value(u"active"_q).toArray()) {
		_active.push_back(v.toString());
	}
}

void PluginEngine::save() {
	QJsonObject obj;
	QJsonArray installed;
	for (const auto &m : _installed) {
		QJsonObject o;
		o.insert(u"id"_q, m.id);
		o.insert(u"name"_q, m.name);
		o.insert(u"version"_q, m.version);
		o.insert(u"author"_q, m.author);
		o.insert(u"entry"_q, m.entry);
		o.insert(u"kind"_q, m.kind);
		o.insert(u"trusted"_q, m.trusted);
		installed.append(o);
	}
	obj.insert(u"installed"_q, installed);
	QJsonArray active;
	for (const auto &id : _active) {
		active.append(id);
	}
	obj.insert(u"active"_q, active);
	QFile f(cWorkingDir() + u"tdata/mio_plugins.json"_q);
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
	}
}

HookManager &HookManager::instance() {
	static HookManager manager;
	return manager;
}

HookManager::HookManager() {
}

void HookManager::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
}

void HookManager::registerHook(const QString &event, const QString &pluginId) {
	auto &list = _hooks[event];
	if (std::find(list.begin(), list.end(), pluginId) == list.end()) {
		list.push_back(pluginId);
	}
}

void HookManager::unregisterPlugin(const QString &pluginId) {
	for (auto it = _hooks.begin(); it != _hooks.end(); ++it) {
		auto &list = it.value();
		list.erase(std::remove(list.begin(), list.end(), pluginId), list.end());
	}
}

std::vector<QString> HookManager::pluginsForEvent(const QString &event) const {
	const auto it = _hooks.find(event);
	return (it != _hooks.end()) ? it.value() : std::vector<QString>{};
}

InAppNotifications &InAppNotifications::instance() {
	static InAppNotifications notifications;
	return notifications;
}

InAppNotifications::InAppNotifications() {
}

void InAppNotifications::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	_enabled = MiogramConfig::instance().boolValue(
		ConfigDomain::Plugins,
		u"inAppNotifications"_q,
		true);
}

bool InAppNotifications::isEnabled() const {
	return _enabled;
}

void InAppNotifications::setEnabled(bool enabled) {
	_enabled = enabled;
	MiogramConfig::instance().setBoolValue(
		ConfigDomain::Plugins,
		u"inAppNotifications"_q,
		enabled);
}

void InAppNotifications::notify(
		const QString &pluginId,
		const QString &title,
		const QString &body) {
	Q_UNUSED(pluginId);
	Q_UNUSED(title);
	Q_UNUSED(body);
	if (!_enabled) {
		return;
	}
	if (!isPluginActive(pluginId)) {
		return;
	}
}

bool InAppNotifications::isPluginActive(const QString &pluginId) const {
	if (!_enabled) {
		return false;
	}
	return PluginEngine::instance().isActive(pluginId);
}

QString PluginForge::scaffoldCpp(const QString &pluginId, const QString &name) {
	return u"// Miogram plugin: %1 (%2)\n"
		u"#include <string>\n\n"
		u"extern \"C\" const char *miogram_plugin_id() { return \"%3\"; }\n"
		u"extern \"C\" const char *miogram_plugin_name() { return \"%4\"; }\n"_q
		.arg(name, pluginId, pluginId, name);
}

QString PluginForge::scaffoldRust(const QString &pluginId, const QString &name) {
	return u"// Miogram plugin: %1 (%2)\n"
		u"use miogram_plugin_sdk::register;\n\n"
		u"#[no_mangle]\n"
		u"pub extern \"C\" fn miogram_plugin_id() -> *const u8 { b\"%3\\0\".as_ptr() }\n"_q
		.arg(name, pluginId, pluginId);
}

QString PluginForge::scaffoldManifest(
		const QString &pluginId,
		const QString &name,
		const QString &version) {
	QJsonObject obj;
	obj.insert(u"id"_q, pluginId);
	obj.insert(u"name"_q, name);
	obj.insert(u"version"_q, version);
	obj.insert(u"kind"_q, u"wasm"_q);
	return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}

bool PluginForge::validateId(const QString &pluginId) {
	static const QRegularExpression re(u"^[a-z0-9_\\-]{3,64}$"_q);
	return re.match(pluginId.trimmed().toLower()).hasMatch();
}

} // namespace Miogram
