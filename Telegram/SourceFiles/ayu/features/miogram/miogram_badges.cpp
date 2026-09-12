// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_badges.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QFontMetrics>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include "core/application.h"
#include "lang/lang_instance.h"

namespace Miogram {

namespace {

QString BadgesCachePath() {
	return cWorkingDir() + u"tdata/mio_badges.json"_q;
}

} // namespace

QString BadgeDefinition::localizedTitle() const {
	const auto lang = Lang::GetInstance().id().toLower();
	if (lang.startsWith(u"uk"_q) || lang.startsWith(u"ua"_q)) {
		return titleUk;
	} else if (lang.startsWith(u"ru"_q) || lang.startsWith(u"be"_q)) {
		return titleRu;
	}
	return titleEn;
}

SupabaseBridge &SupabaseBridge::instance() {
	static SupabaseBridge bridge;
	return bridge;
}

SupabaseBridge::SupabaseBridge() {
	initCanonicalBadges();
}

void SupabaseBridge::initCanonicalBadges() {
	_canonicalBadges = {
		BadgeDefinition{
			u"original"_q,
			u"01 — ORIGINAL"_q,
			u"Класичний варіант"_q,
			u"Классический вариант"_q,
			u"Classic style"_q,
			QColor(255, 105, 180),
			QColor(255, 20, 147),
			QString::fromUtf8("໒꒱"),
			QColor(255, 105, 180, 102),
		},
		BadgeDefinition{
			u"pink"_q,
			u"02 — PINK"_q,
			u"Рожевий стиль"_q,
			u"Розовый стиль"_q,
			u"Pink style"_q,
			QColor(255, 45, 85),
			QColor(255, 105, 180),
			QString::fromUtf8("✦"),
			QColor(255, 45, 85, 102),
		},
		BadgeDefinition{
			u"cyan"_q,
			u"03 — CYAN"_q,
			u"Блакитний стиль"_q,
			u"Голубой стиль"_q,
			u"Cyan style"_q,
			QColor(0, 210, 255),
			QColor(0, 122, 255),
			QString::fromUtf8("✧"),
			QColor(0, 210, 255, 102),
		},
		BadgeDefinition{
			u"dark"_q,
			u"04 — DARK"_q,
			u"Темний варіант"_q,
			u"Темный вариант"_q,
			u"Dark style"_q,
			QColor(138, 43, 226),
			QColor(26, 26, 26),
			QString::fromUtf8("◆"),
			QColor(138, 43, 226, 76),
		},
		BadgeDefinition{
			u"angel"_q,
			u"05 — ANGEL"_q,
			u"З німбом"_q,
			u"С нимбом"_q,
			u"Angel with halo"_q,
			QColor(230, 230, 250),
			QColor(255, 215, 0),
			QString::fromUtf8("🪽"),
			QColor(255, 215, 0, 102),
		},
		BadgeDefinition{
			u"devil"_q,
			u"06 — DEVIL"_q,
			u"З ріжками"_q,
			u"С рожками"_q,
			u"Devil with horns"_q,
			QColor(255, 59, 48),
			QColor(139, 0, 0),
			QString::fromUtf8("🦇"),
			QColor(255, 59, 48, 102),
		},
		BadgeDefinition{
			u"rainbow"_q,
			u"07 — RAINBOW"_q,
			u"Веселковий"_q,
			u"Радужный"_q,
			u"Rainbow style"_q,
			QColor(255, 0, 127),
			QColor(0, 240, 255),
			QString::fromUtf8("🌈"),
			QColor(255, 0, 127, 102),
		},
		BadgeDefinition{
			u"outline"_q,
			u"08 — OUTLINE"_q,
			u"Контурний"_q,
			u"Контурный"_q,
			u"Outline style"_q,
			QColor(255, 255, 255),
			QColor(160, 160, 160),
			QString::fromUtf8("◇"),
			QColor(255, 255, 255, 64),
		},
		BadgeDefinition{
			u"glitch"_q,
			u"09 — GLITCH"_q,
			u"Глітч-стиль"_q,
			u"Глитч-стиль"_q,
			u"Glitch style"_q,
			QColor(255, 0, 85),
			QColor(0, 255, 255),
			QString::fromUtf8("⚡"),
			QColor(0, 255, 255, 128),
		},
		BadgeDefinition{
			u"premium"_q,
			u"10 — PREMIUM"_q,
			u"Преміум варіант"_q,
			u"Премиум вариант"_q,
			u"Premium style"_q,
			QColor(255, 215, 0),
			QColor(255, 165, 0),
			QString::fromUtf8("👑"),
			QColor(255, 215, 0, 128),
		},
	};

	for (const auto &b : _canonicalBadges) {
		_badgesById.insert(b.id, b);
	}
}

const std::vector<BadgeDefinition> &SupabaseBridge::getAllBadges() const {
	return _canonicalBadges;
}

BadgeDefinition SupabaseBridge::getBadgeById(const QString &id) const {
	const auto it = _badgesById.find(id.trimmed().toLower());
	if (it != _badgesById.end()) {
		return it.value();
	}
	return _canonicalBadges.front();
}

void SupabaseBridge::initialize() {
	if (_initialized) return;
	_initialized = true;

	loadCache();
	fetchBadgesFromCloud();
}

void SupabaseBridge::loadCache() {
	QFile f(BadgesCachePath());
	if (!f.open(QIODevice::ReadOnly)) return;

	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject()) return;

	const auto root = doc.object();
	for (auto it = root.begin(); it != root.end(); ++it) {
		const auto uid = it.key().toULongLong();
		if (uid && it.value().isObject()) {
			_cache.insert(uid, sanitizeRecord(uid, it.value().toObject()));
		}
	}
}

void SupabaseBridge::saveCache() {
	QJsonObject root;
	for (auto it = _cache.begin(); it != _cache.end(); ++it) {
		const auto &rec = it.value();
		QJsonObject obj;
		obj[u"badge_id"_q] = rec.badgeType.id;
		obj[u"title"_q] = rec.title;
		obj[u"obtained_reason"_q] = rec.obtainedReason;
		obj[u"obtained_at"_q] = rec.obtainedAt;
		obj[u"is_active"_q] = rec.isActive;
		obj[u"verified"_q] = rec.verified;
		obj[u"grantor_id"_q] = static_cast<qint64>(rec.grantorId);
		root[QString::number(it.key())] = obj;
	}

	QFile f(BadgesCachePath());
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
	}
}

BadgeRecord SupabaseBridge::sanitizeRecord(uint64 userId, const QJsonObject &obj) const {
	BadgeRecord r;
	r.userId = userId;
	r.title = obj.value(u"title"_q).toString(u"Miogram Community ໒꒱"_q);
	r.obtainedReason = obj.value(u"obtained_reason"_q).toString(u"Верифікований учасник спільноти Miogram"_q);
	r.obtainedAt = obj.value(u"obtained_at"_q).toString(u"01.09.2026"_q);
	r.isActive = obj.value(u"is_active"_q).toBool(true);
	r.verified = obj.value(u"verified"_q).toBool(false);
	r.grantorId = static_cast<uint64>(obj.value(u"grantor_id"_q).toInteger(0));
	r.badgeType = getBadgeById(obj.value(u"badge_id"_q).toString(u"original"_q));

	// Anti-abuse validation: only genuine founder (8011880648) can have founder titles
	const bool isGenuineFounder = (userId == kFounderUserId) && (r.verified || r.grantorId == kFounderUserId);
	if (!isGenuineFounder) {
		if (r.title.contains(u"Засновник"_q, Qt::CaseInsensitive)
			|| r.title.contains(u"Founder"_q, Qt::CaseInsensitive)
			|| r.title.contains(u"Архітектор"_q, Qt::CaseInsensitive)) {
			r.title = u"Miogram Community ໒꒱"_q;
			r.obtainedReason = u"Учасник спільноти Miogram"_q;
		}
	}

	return r;
}

bool SupabaseBridge::hasBadge(uint64 userId) const {
	const auto it = _cache.find(userId);
	return (it != _cache.end()) && it.value().isActive;
}

BadgeRecord SupabaseBridge::getBadgeForUser(uint64 userId) const {
	const auto it = _cache.find(userId);
	if (it != _cache.end()) {
		return it.value();
	}
	// Fallback for founder
	if (userId == kFounderUserId) {
		BadgeRecord r;
		r.userId = kFounderUserId;
		r.badgeType = getBadgeById(u"original"_q);
		r.title = u"Засновник Miogram ໒꒱"_q;
		r.obtainedReason = u"Автор та архітектор екосистеми Miogram"_q;
		r.obtainedAt = u"2026"_q;
		r.verified = true;
		return r;
	}
	return BadgeRecord{};
}

void SupabaseBridge::fetchBadgesFromCloud() {
	auto *nam = new QNetworkAccessManager();
	QUrl url(kSupabaseUrl + u"/rest/v1/miogram_badges?is_active=eq.true&select=*"_q);
	QNetworkRequest req(url);
	req.setRawHeader("apikey", kSupabaseAnonKey.toUtf8());
	req.setRawHeader("Authorization", ("Bearer " + kSupabaseAnonKey).toUtf8());
	req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

	QNetworkReply *reply = nam->get(req);
	QObject::connect(reply, &QNetworkReply::finished, [this, nam, reply] {
		reply->deleteLater();
		nam->deleteLater();

		if (reply->error() != QNetworkReply::NoError) {
			return;
		}

		const auto doc = QJsonDocument::fromJson(reply->readAll());
		if (!doc.isArray()) return;

		const auto arr = doc.array();
		for (const auto &val : arr) {
			if (!val.isObject()) continue;
			const auto obj = val.toObject();
			const auto userId = static_cast<uint64>(obj.value(u"user_id"_q).toInteger(0));
			if (!userId) continue;

			_cache.insert(userId, sanitizeRecord(userId, obj));
		}

		saveCache();
	});
}

void SupabaseBridge::reportPresence(uint64 userId, const QString &clientVersion) {
	if (!userId) return;

	auto *nam = new QNetworkAccessManager();
	QUrl url(kSupabaseUrl + u"/rest/v1/miogram_users"_q);
	QNetworkRequest req(url);
	req.setRawHeader("apikey", kSupabaseAnonKey.toUtf8());
	req.setRawHeader("Authorization", ("Bearer " + kSupabaseAnonKey).toUtf8());
	req.setRawHeader("Content-Type", "application/json");
	req.setRawHeader("Prefer", "resolution=merge-duplicates");

	QJsonObject body;
	body[u"user_id"_q] = static_cast<qint64>(userId);
	body[u"last_seen_at"_q] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
	body[u"client_version"_q] = clientVersion;

	QNetworkReply *reply = nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
	QObject::connect(reply, &QNetworkReply::finished, [nam, reply] {
		reply->deleteLater();
		nam->deleteLater();
	});
}

void SupabaseBridge::paintBadge(QPainter &p, const BadgeRecord &record, int x, int y, int height) const {
	if (!record.isActive) return;

	p.save();
	p.setRenderHint(QPainter::Antialiasing);

	const auto &badge = record.badgeType;
	QFont font = p.font();
	font.setBold(true);
	font.setPixelSize(qMax(9, height - 4));
	p.setFont(font);

	const QString text = badge.icon + u" "_q + (record.isFounder() ? u"Founder"_q : badge.id.toUpper());
	const QFontMetrics fm(font);
	const int textWidth = fm.horizontalAdvance(text);
	const int pad = 6;
	const QRect pillRect(x, y + (height - (fm.height() + 4)) / 2, textWidth + pad * 2, fm.height() + 4);

	// Pill background with subtle glow
	QPainterPath path;
	path.addRoundedRect(pillRect, pillRect.height() / 2.0, pillRect.height() / 2.0);
	p.fillPath(path, badge.glowColor);

	QPen borderPen(badge.primaryColor);
	borderPen.setWidth(1);
	p.setPen(borderPen);
	p.drawPath(path);

	// Icon + Label
	p.setPen(badge.primaryColor);
	p.drawText(pillRect, Qt::AlignCenter, text);

	p.restore();
}

} // namespace Miogram
