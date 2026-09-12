// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "settings/settings_builder.h"

namespace Settings::AyuBuilder {
class AyuSectionBuilder;
} // namespace Settings::AyuBuilder

namespace Settings {

void BuildMiogramSections(
	Builder::SectionBuilder &builder,
	AyuBuilder::AyuSectionBuilder &ayu);

} // namespace Settings
