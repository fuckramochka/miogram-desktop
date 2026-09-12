// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

class PrivacyShield {
public:
	[[nodiscard]] static QString sanitize(const QString &text);
	[[nodiscard]] static bool containsSecrets(const QString &text);
};

class CompanionPrefs {
public:
	static CompanionPrefs &instance();

	void initialize();

	[[nodiscard]] bool isEnabled() const;
	void setEnabled(bool enabled);

	[[nodiscard]] QString personaId() const;
	void setPersonaId(const QString &id);

	[[nodiscard]] bool historyEnabled() const;
	void setHistoryEnabled(bool enabled);

private:
	CompanionPrefs();

	bool _enabled = true;
	QString _personaId = u"ame"_q;
	bool _history = true;
	bool _initialized = false;
};

struct CompanionMessage {
	QString role;
	QString text;
	qint64 at = 0;
};

class CompanionToolbox {
public:
	static CompanionToolbox &instance();

	void initialize();

	void ask(
		const QString &prompt,
		Fn<void(QString)> callback);
	void summarizeChat(
		const QString &chatText,
		Fn<void(QString)> callback);

	[[nodiscard]] std::vector<CompanionMessage> history() const;
	void clearHistory();

private:
	CompanionToolbox();

	std::vector<CompanionMessage> _history;
	bool _initialized = false;
};

class SttEngine {
public:
	static SttEngine &instance();

	void initialize();

	[[nodiscard]] bool isAvailable() const;
	void transcribeFile(
		const QString &audioPath,
		Fn<void(QString text, QString error)> callback);

	[[nodiscard]] static std::vector<int> tokenizeBpe(const QString &text);
	[[nodiscard]] static QString detokenizeBpe(const std::vector<int> &ids);

private:
	SttEngine();

	bool _available = false;
	bool _initialized = false;
};

} // namespace Miogram
