// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_config.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "core/application.h"

namespace Miogram {

namespace {

QString ConfigPath() {
	return cWorkingDir() + u"tdata/mio_config.json"_q;
}

} // namespace

QString ConfigDomainKey(ConfigDomain domain) {
	switch (domain) {
	case ConfigDomain::Visuals: return u"visuals"_q;
	case ConfigDomain::Privacy: return u"privacy"_q;
	case ConfigDomain::Network: return u"network"_q;
	case ConfigDomain::Ai: return u"ai"_q;
	case ConfigDomain::Player: return u"player"_q;
	case ConfigDomain::Layouts: return u"layouts"_q;
	case ConfigDomain::Plugins: return u"plugins"_q;
	case ConfigDomain::Feed: return u"feed"_q;
	case ConfigDomain::Folders: return u"folders"_q;
	case ConfigDomain::Kanban: return u"kanban"_q;
	case ConfigDomain::Performance: return u"performance"_q;
	}
	return u"visuals"_q;
}

MiogramConfig &MiogramConfig::instance() {
	static MiogramConfig config;
	return config;
}

MiogramConfig::MiogramConfig() {
}

void MiogramConfig::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	load();
	migrateLegacyKeys();
}

void MiogramConfig::migrateLegacyKeys() {
	auto migrateFile = [&](const QString &filename, ConfigDomain domain) {
		QFile f(cWorkingDir() + u"tdata/"_q + filename);
		if (!f.open(QIODevice::ReadOnly)) {
			return;
		}
		const auto doc = QJsonDocument::fromJson(f.readAll());
		if (!doc.isObject()) {
			return;
		}
		auto existing = getDomain(domain);
		const auto obj = doc.object();
		for (auto it = obj.begin(); it != obj.end(); ++it) {
			if (!existing.contains(it.key())) {
				existing.insert(it.key(), it.value());
			}
		}
		setDomain(domain, existing);
	};
	migrateFile(u"mio_ai.json"_q, ConfigDomain::Ai);
	migrateFile(u"mio_divine.json"_q, ConfigDomain::Layouts);
	migrateFile(u"mio_kanban.json"_q, ConfigDomain::Kanban);
}

QJsonObject MiogramConfig::getDomain(ConfigDomain domain) const {
	return _root.value(ConfigDomainKey(domain)).toObject();
}

void MiogramConfig::setDomain(ConfigDomain domain, const QJsonObject &value) {
	_root.insert(ConfigDomainKey(domain), value);
	save();
}

QString MiogramConfig::stringValue(
		ConfigDomain domain,
		const QString &key,
		const QString &fallback) const {
	const auto obj = getDomain(domain);
	return obj.value(key).toString(fallback);
}

void MiogramConfig::setStringValue(
		ConfigDomain domain,
		const QString &key,
		const QString &value) {
	auto obj = getDomain(domain);
	obj.insert(key, value);
	setDomain(domain, obj);
}

int MiogramConfig::intValue(
		ConfigDomain domain,
		const QString &key,
		int fallback) const {
	const auto obj = getDomain(domain);
	return obj.value(key).toInt(fallback);
}

void MiogramConfig::setIntValue(ConfigDomain domain, const QString &key, int value) {
	auto obj = getDomain(domain);
	obj.insert(key, value);
	setDomain(domain, obj);
}

bool MiogramConfig::boolValue(
		ConfigDomain domain,
		const QString &key,
		bool fallback) const {
	const auto obj = getDomain(domain);
	return obj.value(key).toBool(fallback);
}

void MiogramConfig::setBoolValue(ConfigDomain domain, const QString &key, bool value) {
	auto obj = getDomain(domain);
	obj.insert(key, value);
	setDomain(domain, obj);
}

double MiogramConfig::doubleValue(
		ConfigDomain domain,
		const QString &key,
		double fallback) const {
	const auto obj = getDomain(domain);
	return obj.value(key).toDouble(fallback);
}

void MiogramConfig::setDoubleValue(ConfigDomain domain, const QString &key, double value) {
	auto obj = getDomain(domain);
	obj.insert(key, value);
	setDomain(domain, obj);
}

void MiogramConfig::resetDomain(ConfigDomain domain) {
	_root.insert(ConfigDomainKey(domain), QJsonObject());
	save();
}

void MiogramConfig::resetAll() {
	_root = QJsonObject();
	save();
}

void MiogramConfig::load() {
	QFile f(ConfigPath());
	if (!f.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (doc.isObject()) {
		_root = doc.object();
	}
}

void MiogramConfig::save() {
	QFile f(ConfigPath());
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		f.write(QJsonDocument(_root).toJson(QJsonDocument::Compact));
	}
}

} // namespace Miogram
