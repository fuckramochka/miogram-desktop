// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#pragma once

#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtCore/QMap>
#include <vector>
#include <memory>
#include "base/basic_types.h"

class QPainter;

namespace Miogram {

inline constexpr uint64 kFounderUserId = 8011880648ULL;
inline const QString kSupabaseUrl = u"https://dbxsnjoeyiqvqtrluvwu.supabase.co"_q;
inline const QString kSupabaseAnonKey = u"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImRieHNuam9leWlxdnF0cmx1dnd1Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODg1NDI1MzEsImV4cCI6MjEwNDExODUzMX0.KJ0kvON1HXZu4MzlZjapSJEhEzWYlEqQoNEstWCgIjA"_q;

struct BadgeDefinition {
	QString id;
	QString code;
	QString titleUk;
	QString titleRu;
	QString titleEn;
	QColor primaryColor;
	QColor secondaryColor;
	QString icon;
	QColor glowColor;

	[[nodiscard]] QString localizedTitle() const;
};

struct BadgeRecord {
	uint64 userId = 0;
	BadgeDefinition badgeType;
	QString title;
	QString obtainedReason;
	QString obtainedAt;
	bool isActive = true;
	bool verified = false;
	uint64 grantorId = 0;

	[[nodiscard]] bool isFounder() const {
		return (userId == kFounderUserId) && (verified || grantorId == kFounderUserId);
	}
};

class SupabaseBridge {
public:
	static SupabaseBridge &instance();

	void initialize();
	void fetchBadgesFromCloud();
	[[nodiscard]] bool hasBadge(uint64 userId) const;
	[[nodiscard]] BadgeRecord getBadgeForUser(uint64 userId) const;
	[[nodiscard]] const std::vector<BadgeDefinition> &getAllBadges() const;
	[[nodiscard]] BadgeDefinition getBadgeById(const QString &id) const;

	void reportPresence(uint64 userId, const QString &clientVersion = QStringLiteral("Miogram Desktop 7.0.9"));
	void paintBadge(QPainter &p, const BadgeRecord &record, int x, int y, int height) const;

private:
	SupabaseBridge();
	void initCanonicalBadges();
	void loadCache();
	void saveCache();
	[[nodiscard]] BadgeRecord sanitizeRecord(uint64 userId, const QJsonObject &obj) const;

	std::vector<BadgeDefinition> _canonicalBadges;
	QMap<QString, BadgeDefinition> _badgesById;
	QMap<uint64, BadgeRecord> _cache;
	bool _initialized = false;
};

} // namespace Miogram
