// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <vector>
#include "base/basic_types.h"

namespace Miogram {

struct FeedItem {
	quint64 dialogId = 0;
	int messageId = 0;
	QString title;
	QString snippet;
	qint64 date = 0;
	bool unread = false;
	bool muted = false;
	double score = 0.;
};

struct FeedDigest {
	QString summary;
	std::vector<QString> bullets;
	QString model;
	qint64 createdAt = 0;
};

class SmartFeedService {
public:
	static SmartFeedService &instance();

	void initialize();

	[[nodiscard]] bool isEnabled() const;
	void setEnabled(bool enabled);

	[[nodiscard]] std::vector<FeedItem> ranked(
		std::vector<FeedItem> items,
		int limit = 50) const;

	[[nodiscard]] bool isMuted(quint64 dialogId) const;
	void setMuted(quint64 dialogId, bool muted);

	void requestDigest(
		const std::vector<FeedItem> &items,
		Fn<void(FeedDigest)> callback);

private:
	SmartFeedService();
	void loadMuted();

	std::vector<quint64> _muted;
	bool _enabled = true;
	bool _initialized = false;
};

} // namespace Miogram
