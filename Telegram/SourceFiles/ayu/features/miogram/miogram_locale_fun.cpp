// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_locale_fun.h"

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QStandardPaths>
#include <QtCore/QLocale>

#include "ayu/features/miogram/miogram_config.h"
#include "core/application.h"

namespace Miogram {

MiogramLocale &MiogramLocale::instance() {
	static MiogramLocale locale;
	return locale;
}

MiogramLocale::MiogramLocale() {
}

void MiogramLocale::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	const auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Visuals);
	_current = static_cast<MioLang>(domain.value(u"mioLang"_q).toInt(0));
}

MioLang MiogramLocale::current() const {
	return _current;
}

void MiogramLocale::setCurrent(MioLang lang) {
	_current = lang;
	auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Visuals);
	domain.insert(u"mioLang"_q, static_cast<int>(lang));
	MiogramConfig::instance().setDomain(ConfigDomain::Visuals, domain);
}

void MiogramLocale::detectFromSystem(const QString &systemLangId) {
	const auto id = systemLangId.toLower();
	if (id.startsWith(u"uk"_q)) {
		setCurrent(MioLang::Ukrainian);
	} else if (id.startsWith(u"ru"_q)) {
		setCurrent(MioLang::Russian);
	} else {
		setCurrent(MioLang::English);
	}
}

QString MiogramLocale::get(
		const QString &uk,
		const QString &ru,
		const QString &en) const {
	switch (_current) {
	case MioLang::Ukrainian: return uk;
	case MioLang::Russian: return ru;
	case MioLang::English: return en;
	}
	return uk;
}

QString MiogramLocale::code() const {
	switch (_current) {
	case MioLang::Ukrainian: return u"uk"_q;
	case MioLang::Russian: return u"ru"_q;
	case MioLang::English: return u"en"_q;
	}
	return u"uk"_q;
}

LocalizerEngine &LocalizerEngine::instance() {
	static LocalizerEngine engine;
	return engine;
}

LocalizerEngine::LocalizerEngine() {
}

void LocalizerEngine::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	load();
}

void LocalizerEngine::setOverride(const QString &key, const QString &value) {
	if (key.trimmed().isEmpty()) {
		return;
	}
	_overrides.insert(key.trimmed(), value);
	save();
}

QString LocalizerEngine::value(const QString &key, const QString &fallback) const {
	const auto it = _overrides.find(key);
	return (it != _overrides.end()) ? it.value() : fallback;
}

void LocalizerEngine::clearOverride(const QString &key) {
	_overrides.remove(key);
	save();
}

void LocalizerEngine::clearAll() {
	_overrides.clear();
	save();
}

bool LocalizerEngine::hasOverride(const QString &key) const {
	return _overrides.contains(key);
}

void LocalizerEngine::load() {
	QFile f(cWorkingDir() + u"tdata/mio_localizer.json"_q);
	if (!f.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject()) {
		return;
	}
	const auto obj = doc.object();
	for (auto it = obj.begin(); it != obj.end(); ++it) {
		_overrides.insert(it.key(), it.value().toString());
	}
}

void LocalizerEngine::save() {
	QJsonObject obj;
	for (auto it = _overrides.begin(); it != _overrides.end(); ++it) {
		obj.insert(it.key(), it.value());
	}
	QFile f(cWorkingDir() + u"tdata/mio_localizer.json"_q);
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
	}
}

bool MusorDrop::isTrigger(const QString &url) {
	auto u = url.trimmed().toLower();
	while (u.endsWith(u"/"_q) || u.endsWith(u" "_q) || u.endsWith(u"?"_q)) {
		u.chop(1);
	}
	return (u == u"tg://musor_drop"_q) || (u == u"tg:musor_drop"_q);
}

QString MusorDrop::triggerUrl() {
	return u"tg://musor_drop"_q;
}

QString MusorDrop::locateMediaFile() {
	const auto appDir = QCoreApplication::applicationDirPath() + u"/musordrop.mp4"_q;
	if (QFile::exists(appDir) && QFileInfo(appDir).size() > 0) {
		return appDir;
	}
	const auto candidates = {
		cWorkingDir() + u"tdata/musordrop.mp4"_q,
		QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
			+ u"/musordrop.mp4"_q,
		QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
			+ u"/musordrop.mp4"_q,
	};
	for (const auto &path : candidates) {
		if (QFile::exists(path) && QFileInfo(path).size() > 0) {
			return path;
		}
	}
	return QString();
}

QString MusorDrop::missingHint(
		const QString &uk,
		const QString &ru,
		const QString &en) {
	Q_UNUSED(uk);
	Q_UNUSED(ru);
	Q_UNUSED(en);
	return MiogramLocale::instance().get(
		u"🗑️ МУСОРДРОП НЕ ЗНАЙДЕНО 🗑️\nПокладіть musordrop.mp4 у папку Завантаження"_q,
		u"🗑️ МУСОРДРОП НЕ НАЙДЕН 🗑️\nПоложите musordrop.mp4 в папку Загрузки"_q,
		u"🗑️ MUSORDROP NOT FOUND 🗑️\nPlace musordrop.mp4 into Downloads"_q);
}

} // namespace Miogram
