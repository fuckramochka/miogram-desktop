// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.

#include "ayu/features/miogram/miogram_supabase.h"

namespace Miogram {

QString SupabaseSchema::tableName() {
	return u"public.miogram_badges"_q;
}

std::vector<SupabaseColumn> SupabaseSchema::badgeColumns() {
	return {
		{ u"user_id"_q, u"bigint"_q, u"primary key"_q },
		{ u"badge_id"_q, u"text"_q, u"'original'"_q },
		{ u"title"_q, u"text"_q, u"''"_q },
		{ u"obtained_reason"_q, u"text"_q, u"''"_q },
		{ u"obtained_at"_q, u"timestamptz"_q, u"now()"_q },
		{ u"is_active"_q, u"boolean"_q, u"true"_q },
		{ u"client_version"_q, u"text"_q, u"'Miogram Desktop'"_q },
		{ u"created_at"_q, u"timestamptz"_q, u"now()"_q },
		{ u"updated_at"_q, u"timestamptz"_q, u"now()"_q },
	};
}

QString SupabaseSchema::selectActiveBadges() {
	return u"select user_id,badge_id,title,obtained_reason,obtained_at,is_active "
		u"from public.miogram_badges where is_active=true"_q;
}

QString SupabaseSchema::upsertBadge() {
	return u"insert into public.miogram_badges "
		u"(user_id,badge_id,title,obtained_reason) values ($1,$2,$3,$4) "
		u"on conflict (user_id) do update set "
		u"badge_id=excluded.badge_id,title=excluded.title,"
		u"obtained_reason=excluded.obtained_reason"_q;
}

quint64 SupabaseSchema::founderUserId() {
	return 8011880648ULL;
}

} // namespace Miogram
