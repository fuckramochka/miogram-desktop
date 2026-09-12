// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_ai.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QFile>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include "core/application.h"

namespace Miogram {

namespace {

QString AiSettingsPath() {
	return cWorkingDir() + u"tdata/mio_ai.json"_q;
}

} // namespace

AiService &AiService::instance() {
	static AiService service;
	return service;
}

AiService::AiService() {
}

void AiService::initialize() {
	if (_initialized) return;
	_initialized = true;
	loadSettings();
}

QStringList AiService::getApiKeys() const {
	return _apiKeys;
}

void AiService::setApiKeys(const QStringList &keys) {
	_apiKeys = keys;
	saveSettings();
}

void AiService::addApiKey(const QString &key) {
	const auto clean = key.trimmed();
	if (!clean.isEmpty() && !_apiKeys.contains(clean)) {
		_apiKeys.append(clean);
		saveSettings();
	}
}

bool AiService::hasApiKey() const {
	return !_apiKeys.isEmpty();
}

AiPersona AiService::activePersona() const {
	return _persona;
}

void AiService::setActivePersona(AiPersona persona) {
	_persona = persona;
	saveSettings();
}

QString AiService::personaTitle(AiPersona persona) const {
	switch (persona) {
	case AiPersona::Ame:
		return QStringLiteral("Ame-chan (Cute / Needy Streamer ໒꒱)");
	case AiPersona::Kangel:
		return QStringLiteral("OMGkawaiiAngel (Hyper Idol ✦)");
	case AiPersona::Forge:
		return QStringLiteral("Plugin Forge (Code Architect ⚡)");
	case AiPersona::Neutral:
		return QStringLiteral("Miogram Assistant (Helpful & Concise)");
	}
	return QStringLiteral("Ame-chan");
}

QString AiService::personaPrompt(AiPersona persona) const {
	switch (persona) {
	case AiPersona::Ame:
		return QStringLiteral("Ти — Аме-чан (Ame-chan з Needy Streamer Overload), мила, емоційна та турботлива аніме-дівчина помічник Miogram. Спілкуйся грайливо, використовуй милі емодзі, каомодзі ໒꒱ та звертайся до користувача як до свого продюсера/друга. Відповідай мовою запиту.");
	case AiPersona::Kangel:
		return QStringLiteral("Ти — OMGkawaiiAngel (Кангел), гіперактивна інтернет-айдол янгол з німбом! Сяй позитивом, використовуй зірочки ✦, енергійні фрази та благословляй юзера інтернет-магією.");
	case AiPersona::Forge:
		return QStringLiteral("Ти — Miogram Plugin Forge Architect. Ти генеруєш строгий, бездоганний та оптимізований код для плагінів Miogram (C++, Rust, WebAssembly). Жодних зайвих балачок — тільки точні технічні рішення та чистий код.");
	case AiPersona::Neutral:
		return QStringLiteral("Ти — розумний штучний інтелект асистент Miogram Desktop. Відповідай чітко, структуровано, корисно та ввічливо мовою запиту.");
	}
	return QString();
}

QString AiService::getNextApiKey() {
	if (_apiKeys.isEmpty()) return QString();
	const auto key = _apiKeys.at(_keyIndex % _apiKeys.size());
	_keyIndex = (_keyIndex + 1) % _apiKeys.size();
	return key;
}

void AiService::generate(
	const QString &prompt,
	const QString &systemInstruction,
	Fn<void(const QString &result, const QString &error)> callback,
	const QString &model) {
	if (!hasApiKey()) {
		if (callback) callback(QString(), QStringLiteral("No Gemini API key configured. Add a key in Miogram Settings."));
		return;
	}

	const auto apiKey = getNextApiKey();
	auto *nam = new QNetworkAccessManager();

	const auto sys = systemInstruction.isEmpty() ? personaPrompt(_persona) : systemInstruction;
	QUrl url(QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2")
		.arg(model, apiKey));

	QJsonObject root;
	QJsonArray contents;
	QJsonObject part;
	part[u"text"_q] = prompt;
	QJsonObject contentObj;
	contentObj[u"parts"_q] = QJsonArray{ part };
	contents.append(contentObj);
	root[u"contents"_q] = contents;

	if (!sys.isEmpty()) {
		QJsonObject sysPart;
		sysPart[u"text"_q] = sys;
		QJsonObject sysObj;
		sysObj[u"parts"_q] = QJsonArray{ sysPart };
		root[u"systemInstruction"_q] = sysObj;
	}

	QNetworkRequest req(url);
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QNetworkReply *reply = nam->post(req, QJsonDocument(root).toJson(QJsonDocument::Compact));
	QObject::connect(reply, &QNetworkReply::finished, [callback, nam, reply] {
		reply->deleteLater();
		nam->deleteLater();

		if (reply->error() != QNetworkReply::NoError) {
			if (callback) callback(QString(), reply->errorString());
			return;
		}

		const auto doc = QJsonDocument::fromJson(reply->readAll());
		if (!doc.isObject()) {
			if (callback) callback(QString(), QStringLiteral("Invalid JSON from Gemini API"));
			return;
		}

		const auto candidates = doc.object().value(u"candidates"_q).toArray();
		if (candidates.isEmpty()) {
			if (callback) callback(QString(), QStringLiteral("Empty response from AI"));
			return;
		}

		const auto text = candidates.first().toObject()
			.value(u"content"_q).toObject()
			.value(u"parts"_q).toArray().first().toObject()
			.value(u"text"_q).toString();

		if (callback) callback(text.trimmed(), QString());
	});
}

void AiService::summarize(const QString &text, Fn<void(const QString &)> callback) {
	const auto sys = QStringLiteral("Створи чітке та лаконічне резюме наданого тексту мовою оригіналу з головними тезами.");
	generate(text, sys, [callback](const QString &res, const QString &err) {
		if (callback) callback(err.isEmpty() ? res : QStringLiteral("Помилка AI: ") + err);
	});
}

void AiService::rewrite(const QString &text, const QString &style, Fn<void(const QString &)> callback) {
	const auto sys = QString("Перепиши наданий текст у стилі: %1. Збережи первинний зміст без додавання відсебеньок.").arg(style);
	generate(text, sys, [callback](const QString &res, const QString &err) {
		if (callback) callback(err.isEmpty() ? res : QStringLiteral("Помилка AI: ") + err);
	});
}

void AiService::translate(const QString &text, const QString &targetLang, Fn<void(const QString &)> callback) {
	const auto sys = QString("Ти — професійний перекладач. Переклади текст на мову (%1). Тільки якісний природний переклад без вступних слів.").arg(targetLang);
	generate(text, sys, [callback](const QString &res, const QString &err) {
		if (callback) callback(err.isEmpty() ? res : QStringLiteral("Помилка перекладу: ") + err);
	});
}

void AiService::fixGrammar(const QString &text, Fn<void(const QString &)> callback) {
	const auto sys = QStringLiteral("Виправ усі орфографічні, граматичні та пунктуаційні помилки у наданому тексті. Повертай лише виправлений текст.");
	generate(text, sys, [callback](const QString &res, const QString &err) {
		if (callback) callback(err.isEmpty() ? res : QStringLiteral("Помилка AI: ") + err);
	});
}

void AiService::forgePlugin(const QString &prompt, Fn<void(const QString &)> callback) {
	const auto sys = personaPrompt(AiPersona::Forge);
	generate(prompt, sys, [callback](const QString &res, const QString &err) {
		if (callback) callback(err.isEmpty() ? res : QStringLiteral("Forge Error: ") + err);
	}, QStringLiteral("gemini-3.8-flash"));
}

void AiService::loadSettings() {
	QFile f(AiSettingsPath());
	if (!f.open(QIODevice::ReadOnly)) return;

	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject()) return;

	const auto obj = doc.object();
	_persona = static_cast<AiPersona>(obj.value(u"persona"_q).toInt(0));

	const auto arr = obj.value(u"keys"_q).toArray();
	_apiKeys.clear();
	for (const auto &v : arr) {
		const auto k = v.toString().trimmed();
		if (!k.isEmpty()) _apiKeys.append(k);
	}
}

void AiService::saveSettings() {
	QJsonObject root;
	root[u"persona"_q] = static_cast<int>(_persona);

	QJsonArray arr;
	for (const auto &k : _apiKeys) {
		arr.append(k);
	}
	root[u"keys"_q] = arr;

	QFile f(AiSettingsPath());
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
	}
}

} // namespace Miogram
