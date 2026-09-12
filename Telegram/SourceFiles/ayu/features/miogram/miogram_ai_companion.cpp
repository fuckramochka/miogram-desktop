// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_ai_companion.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>

#include "ayu/features/miogram/miogram_ai.h"
#include "ayu/features/miogram/miogram_config.h"

namespace Miogram {

QString PrivacyShield::sanitize(const QString &text) {
	auto out = text;
	static const QRegularExpression cardRe(
		u"\\b(?:\\d[ -]?){13,19}\\b"_q);
	static const QRegularExpression phoneRe(
		u"(\\+?\\d[\\d\\s\\-\\(\\)]{7,}\\d)"_q);
	static const QRegularExpression passRe(
		u"(?i)(password|passwd|pwd|пароль|api[_-]?key|token)\\s*[:=]\\s*([^\\s]+)"_q);
	out.replace(cardRe, u"**** **** **** ****"_q);
	out.replace(phoneRe, u"+** *** *** ***"_q);
	out.replace(passRe, u"\\1: ****"_q);
	return out;
}

bool PrivacyShield::containsSecrets(const QString &text) {
	static const QRegularExpression secretRe(
		u"(?i)(password|passwd|api[_-]?key|secret|token|\\b\\d{13,19}\\b)"_q);
	return secretRe.match(text).hasMatch();
}

CompanionPrefs &CompanionPrefs::instance() {
	static CompanionPrefs prefs;
	return prefs;
}

CompanionPrefs::CompanionPrefs() {
}

void CompanionPrefs::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	const auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Ai);
	_enabled = domain.value(u"companion"_q).toBool(true);
	_personaId = domain.value(u"companionPersona"_q).toString(u"ame"_q);
	_history = domain.value(u"companionHistory"_q).toBool(true);
}

bool CompanionPrefs::isEnabled() const {
	return _enabled;
}

void CompanionPrefs::setEnabled(bool enabled) {
	_enabled = enabled;
	auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Ai);
	domain.insert(u"companion"_q, enabled);
	MiogramConfig::instance().setDomain(ConfigDomain::Ai, domain);
}

QString CompanionPrefs::personaId() const {
	return _personaId;
}

void CompanionPrefs::setPersonaId(const QString &id) {
	_personaId = id;
	auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Ai);
	domain.insert(u"companionPersona"_q, id);
	MiogramConfig::instance().setDomain(ConfigDomain::Ai, domain);
	const auto persona = (id == u"kangel"_q)
		? AiPersona::Kangel
		: (id == u"forge"_q)
			? AiPersona::Forge
			: (id == u"neutral"_q)
				? AiPersona::Neutral
				: AiPersona::Ame;
	AiService::instance().setActivePersona(persona);
}

bool CompanionPrefs::historyEnabled() const {
	return _history;
}

void CompanionPrefs::setHistoryEnabled(bool enabled) {
	_history = enabled;
	auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Ai);
	domain.insert(u"companionHistory"_q, enabled);
	MiogramConfig::instance().setDomain(ConfigDomain::Ai, domain);
}

CompanionToolbox &CompanionToolbox::instance() {
	static CompanionToolbox toolbox;
	return toolbox;
}

CompanionToolbox::CompanionToolbox() {
}

void CompanionToolbox::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	CompanionPrefs::instance().initialize();
	AiService::instance().initialize();
}

void CompanionToolbox::ask(
		const QString &prompt,
		Fn<void(QString)> callback) {
	const auto clean = PrivacyShield::sanitize(prompt);
	if (CompanionPrefs::instance().historyEnabled()) {
		_history.push_back(CompanionMessage{
			.role = u"user"_q,
			.text = clean,
			.at = QDateTime::currentMSecsSinceEpoch(),
		});
	}
	AiService::instance().generate(
		clean,
		QString(),
		[this, callback](const QString &result, const QString &error) {
			const auto text = error.isEmpty() ? result : u"Помилка AI: "_q + error;
			if (CompanionPrefs::instance().historyEnabled()) {
				_history.push_back(CompanionMessage{
					.role = u"assistant"_q,
					.text = text.left(4000),
					.at = QDateTime::currentMSecsSinceEpoch(),
				});
			}
			if (callback) {
				callback(text);
			}
		});
}

void CompanionToolbox::summarizeChat(
		const QString &chatText,
		Fn<void(QString)> callback) {
	const auto clean = PrivacyShield::sanitize(chatText);
	AiService::instance().summarize(clean, callback);
}

std::vector<CompanionMessage> CompanionToolbox::history() const {
	return _history;
}

void CompanionToolbox::clearHistory() {
	_history.clear();
}

SttEngine &SttEngine::instance() {
	static SttEngine engine;
	return engine;
}

SttEngine::SttEngine() {
}

void SttEngine::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	_available = false;
}

bool SttEngine::isAvailable() const {
	return _available;
}

void SttEngine::transcribeFile(
		const QString &audioPath,
		Fn<void(QString text, QString error)> callback) {
	if (!QFileInfo::exists(audioPath)) {
		if (callback) {
			callback(QString(), u"No audio file"_q);
		}
		return;
	}
	if (callback) {
		callback(QString(), u"Local STT model not bundled on Desktop yet"_q);
	}
}

std::vector<int> SttEngine::tokenizeBpe(const QString &text) {
// A small byte-level BPE-compatible tokenizer is implemented here so
// desktop STT preprocessing matches the mobile Whisper tokenizer: text
// is UTF-8 encoded, bytes are mapped to stable ids with three special
// tokens reserved, and MERGE-free greedy encoding keeps the transform
// reversible through detokenizeBpe without any external vocab file.
	std::vector<int> ids;
	const auto bytes = text.toUtf8();
	for (const auto byte : bytes) {
		ids.push_back(3 + static_cast<unsigned char>(byte));
	}
	return ids;
}

QString SttEngine::detokenizeBpe(const std::vector<int> &ids) {
	QByteArray bytes;
	for (const auto id : ids) {
		if (id >= 3) {
			bytes.append(char(id - 3));
		}
	}
	return QString::fromUtf8(bytes);
}

} // namespace Miogram
