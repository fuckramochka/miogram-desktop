// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

enum class AiPersona {
	Ame = 0,
	Kangel = 1,
	Forge = 2,
	Neutral = 3,
};

class AiService {
public:
	static AiService &instance();

	void initialize();

	[[nodiscard]] QStringList getApiKeys() const;
	void setApiKeys(const QStringList &keys);
	void addApiKey(const QString &key);
	[[nodiscard]] bool hasApiKey() const;

	[[nodiscard]] AiPersona activePersona() const;
	void setActivePersona(AiPersona persona);
	[[nodiscard]] QString personaTitle(AiPersona persona) const;
	[[nodiscard]] QString personaPrompt(AiPersona persona) const;

	void generate(
		const QString &prompt,
		const QString &systemInstruction,
		Fn<void(const QString &result, const QString &error)> callback,
		const QString &model = QStringLiteral("gemini-2.5-flash"));

	void summarize(const QString &text, Fn<void(const QString &)> callback);
	void rewrite(const QString &text, const QString &style, Fn<void(const QString &)> callback);
	void translate(const QString &text, const QString &targetLang, Fn<void(const QString &)> callback);
	void fixGrammar(const QString &text, Fn<void(const QString &)> callback);
	void forgePlugin(const QString &prompt, Fn<void(const QString &)> callback);

private:
	AiService();
	void loadSettings();
	void saveSettings();
	[[nodiscard]] QString getNextApiKey();

	QStringList _apiKeys;
	int _keyIndex = 0;
	AiPersona _persona = AiPersona::Ame;
	bool _initialized = false;
};

} // namespace Miogram
