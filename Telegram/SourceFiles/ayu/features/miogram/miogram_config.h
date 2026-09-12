// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <QtCore/QJsonObject>
#include "base/basic_types.h"

namespace Miogram {

enum class ConfigDomain {
	Visuals = 0,
	Privacy = 1,
	Network = 2,
	Ai = 3,
	Player = 4,
	Layouts = 5,
	Plugins = 6,
	Feed = 7,
	Folders = 8,
	Kanban = 9,
	Performance = 10,
};

class MiogramConfig {
public:
	static MiogramConfig &instance();

	void initialize();
	void migrateLegacyKeys();

	[[nodiscard]] QJsonObject getDomain(ConfigDomain domain) const;
	void setDomain(ConfigDomain domain, const QJsonObject &value);

	[[nodiscard]] QString stringValue(
		ConfigDomain domain,
		const QString &key,
		const QString &fallback = QString()) const;
	void setStringValue(
		ConfigDomain domain,
		const QString &key,
		const QString &value);

	[[nodiscard]] int intValue(
		ConfigDomain domain,
		const QString &key,
		int fallback = 0) const;
	void setIntValue(ConfigDomain domain, const QString &key, int value);

	[[nodiscard]] bool boolValue(
		ConfigDomain domain,
		const QString &key,
		bool fallback = false) const;
	void setBoolValue(ConfigDomain domain, const QString &key, bool value);

	[[nodiscard]] double doubleValue(
		ConfigDomain domain,
		const QString &key,
		double fallback = 0.) const;
	void setDoubleValue(ConfigDomain domain, const QString &key, double value);

	void resetDomain(ConfigDomain domain);
	void resetAll();

private:
	MiogramConfig();
	void load();
	void save();

	QJsonObject _root;
	bool _initialized = false;
};

[[nodiscard]] QString ConfigDomainKey(ConfigDomain domain);

} // namespace Miogram
