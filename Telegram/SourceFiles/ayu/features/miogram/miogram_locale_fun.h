// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <QtCore/QMap>
#include "base/basic_types.h"

namespace Miogram {

enum class MioLang {
	Ukrainian = 0,
	Russian = 1,
	English = 2,
};

class MiogramLocale {
public:
	static MiogramLocale &instance();

	void initialize();

	[[nodiscard]] MioLang current() const;
	void setCurrent(MioLang lang);
	void detectFromSystem(const QString &systemLangId);

	[[nodiscard]] QString get(
		const QString &uk,
		const QString &ru,
		const QString &en) const;

	[[nodiscard]] QString code() const;

private:
	MiogramLocale();

	MioLang _current = MioLang::Ukrainian;
	bool _initialized = false;
};

class LocalizerEngine {
public:
	static LocalizerEngine &instance();

	void initialize();

	void setOverride(const QString &key, const QString &value);
	[[nodiscard]] QString value(const QString &key, const QString &fallback) const;
	void clearOverride(const QString &key);
	void clearAll();
	[[nodiscard]] bool hasOverride(const QString &key) const;

private:
	LocalizerEngine();
	void load();
	void save();

	QMap<QString, QString> _overrides;
	bool _initialized = false;
};

class MusorDrop {
public:
	[[nodiscard]] static bool isTrigger(const QString &url);
	[[nodiscard]] static QString triggerUrl();
	[[nodiscard]] static QString locateMediaFile();
	[[nodiscard]] static QString missingHint(
		const QString &uk,
		const QString &ru,
		const QString &en);
};

} // namespace Miogram
