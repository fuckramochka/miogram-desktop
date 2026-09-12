// This is part of Miogram Desktop.
//
// Copyright (c) 2026 Miogram Team.
// Distributed under GNU General Public License v3.0.
//
// Desktop mirror of exteraless/supabase_schema.sql (public.miogram_badges).
// The table stores global badge resolution, community presence and lore.

#pragma once

#include <QtCore/QString>
#include <vector>

namespace Miogram {

struct SupabaseColumn {
	QString name;
	QString type;
	QString def;
};

class SupabaseSchema {
public:
	[[nodiscard]] static QString tableName();
	[[nodiscard]] static std::vector<SupabaseColumn> badgeColumns();
	[[nodiscard]] static QString selectActiveBadges();
	[[nodiscard]] static QString upsertBadge();
	[[nodiscard]] static quint64 founderUserId();
};

} // namespace Miogram
