// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_feed.h"

#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <algorithm>

#include "ayu/features/miogram/miogram_ai.h"
#include "ayu/features/miogram/miogram_config.h"

namespace Miogram {

SmartFeedService &SmartFeedService::instance() {
	static SmartFeedService service;
	return service;
}

SmartFeedService::SmartFeedService() {
}

void SmartFeedService::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;
	_enabled = MiogramConfig::instance().boolValue(
		ConfigDomain::Feed,
		u"enabled"_q,
		true);
	loadMuted();
}

bool SmartFeedService::isEnabled() const {
	return _enabled;
}

void SmartFeedService::setEnabled(bool enabled) {
	_enabled = enabled;
	MiogramConfig::instance().setBoolValue(ConfigDomain::Feed, u"enabled"_q, enabled);
}

std::vector<FeedItem> SmartFeedService::ranked(
		std::vector<FeedItem> items,
		int limit) const {
	for (auto &item : items) {
		double score = 0.;
		if (item.unread) {
			score += 10.;
		}
		if (item.muted || isMuted(item.dialogId)) {
			score -= 100.;
		}
		const auto now = QDateTime::currentMSecsSinceEpoch();
		const auto ageHours = double(now - item.date) / 3600000.;
		score -= std::min(20., ageHours * 0.5);
		score += std::min(5., double(item.snippet.size()) / 200.);
		item.score = score;
	}
	std::sort(items.begin(), items.end(), [](const FeedItem &a, const FeedItem &b) {
		return a.score > b.score;
	});
	if (limit > 0 && int(items.size()) > limit) {
		items.resize(limit);
	}
	return items;
}

bool SmartFeedService::isMuted(quint64 dialogId) const {
	return std::find(_muted.begin(), _muted.end(), dialogId) != _muted.end();
}

void SmartFeedService::setMuted(quint64 dialogId, bool muted) {
	if (muted && !isMuted(dialogId)) {
		_muted.push_back(dialogId);
	} else if (!muted) {
		_muted.erase(
			std::remove(_muted.begin(), _muted.end(), dialogId),
			_muted.end());
	}
	QJsonArray arr;
	for (const auto id : _muted) {
		arr.append(QString::number(id));
	}
	auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Feed);
	domain.insert(u"muted"_q, arr);
	MiogramConfig::instance().setDomain(ConfigDomain::Feed, domain);
}

void SmartFeedService::requestDigest(
		const std::vector<FeedItem> &items,
		Fn<void(FeedDigest)> callback) {
	QString combined;
	for (const auto &item : ranked(items, 20)) {
		combined += item.title + u": "_q + item.snippet + u"\n"_q;
	}
	if (combined.trimmed().isEmpty()) {
		if (callback) {
			callback(FeedDigest{});
		}
		return;
	}
	AiService::instance().summarize(combined, [callback](const QString &summary) {
		FeedDigest digest;
		digest.summary = summary;
		digest.model = u"gemini-2.5-flash"_q;
		digest.createdAt = QDateTime::currentMSecsSinceEpoch();
		if (callback) {
			callback(digest);
		}
	});
}

void SmartFeedService::loadMuted() {
	const auto domain = MiogramConfig::instance().getDomain(ConfigDomain::Feed);
	const auto arr = domain.value(u"muted"_q).toArray();
	_muted.clear();
	for (const auto &v : arr) {
		_muted.push_back(v.toString().toULongLong());
	}
}

} // namespace Miogram
