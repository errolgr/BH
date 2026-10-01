// Ground item visual keywords of the loot filter (%BG%, %OPACITY%, %FRAME%, %SIZE%, %BEAM%, %FLASH%,
// %ICON%): how BuildAction parses them, and the style GetGroundStyle resolves for an item across a
// filter's rules. Expected values follow the syntax in bh-harness docs/PRD-loot-filter-visuals.md §5:
// colours are hex palette indices like %MAP-XX%, %SIZE-S|M|L% are the D2 fonts 1 (the font ground
// labels use today, observed in game), 2 and 3, and a value outside the listed ones is not a keyword,
// so it is shown as typed like any unknown keyword.
#include "doctest/doctest.h"

#include <cstring>
#include <string>

#include "BH.h"
#include "LootFilter.h"

using support::LoadFilter;
using support::NameOf;
using support::TestItem;

namespace {

std::wstring Color(const wchar_t* code) {
	return std::wstring(L"\xFF" L"c") + code;
}

const Action& ActionOf(const std::wstring& action) {
	return support::ParseRule(L"", action)->action;
}

// The name `item` gets from a filter whose only rule matches every item and has `action` as output.
std::wstring NameWith(const std::string& action, TestItem& item, const std::wstring& baseName = L"Axe") {
	LoadFilter("ItemDisplay[]: " + action + "\n");
	return NameOf(item, baseName);
}

// GetGroundStyle for the item. The item's code is registered the way Item.cpp's data tables would
// have it, so GetGroundStyle can build the item info from the unit alone.
bool StyleOf(TestItem& item, GroundStyle* out) {
	ItemAttributeMap[item.info()->itemCode] = &item.attrs();
	return GetGroundStyle(item.unit(), out);
}

void CheckDefault(const GroundStyle& s) {
	CHECK(s.bgColor == UNDEFINED_COLOR);
	CHECK(s.bgOpacity == -1);
	CHECK(s.frameColor == UNDEFINED_COLOR);
	CHECK(s.labelFont == -1);
	CHECK(s.beamColor == UNDEFINED_COLOR);
	CHECK(s.flashColor == UNDEFINED_COLOR);
	CHECK(s.iconShape == ICON_SQUARE);
}

}  // namespace

TEST_SUITE("LootFilterVisuals") {

// ---- Parsing ----------------------------------------------------------------------------------

TEST_CASE("palette colour keywords take 1-2 hex digits and are removed from the name") {
	const Action& a = ActionOf(L"%NAME%%BG-20%%FRAME-0A%%BEAM-9B%%FLASH-84%");
	CHECK(a.bgColor == 0x20);
	CHECK(a.frameColor == 0x0A);
	CHECK(a.beamColor == 0x9B);
	CHECK(a.flashColor == 0x84);
	CHECK(a.name == L"%NAME%");
	CHECK(ActionOf(L"%BG-0%").bgColor == 0x00);
	CHECK(ActionOf(L"%BG-A%").bgColor == 0x0A);
	CHECK(ActionOf(L"%FRAME-FF%").frameColor == 0xFF);
}

TEST_CASE("visual keywords are case-insensitive and can sit anywhere in the output") {
	const Action& a = ActionOf(L"%bg-1f%%NAME%%Size-m% x %icon-diamond%%opacity-75%");
	CHECK(a.bgColor == 0x1F);
	CHECK(a.labelFont == 2);
	CHECK(a.iconShape == ICON_DIAMOND);
	CHECK(a.bgOpacity == 75);
	CHECK(a.name == L"%NAME% x ");
}

TEST_CASE("%OPACITY-n% takes 25, 50, 75 or 100") {
	CHECK(ActionOf(L"%OPACITY-25%").bgOpacity == 25);
	CHECK(ActionOf(L"%OPACITY-50%").bgOpacity == 50);
	CHECK(ActionOf(L"%OPACITY-75%").bgOpacity == 75);
	const Action& full = ActionOf(L"%NAME%%OPACITY-100%");
	CHECK(full.bgOpacity == 100);
	CHECK(full.name == L"%NAME%");
}

TEST_CASE("%SIZE-S|M|L% select the D2 fonts 1, 2 and 3") {
	CHECK(ActionOf(L"%SIZE-S%").labelFont == 1);
	CHECK(ActionOf(L"%SIZE-M%").labelFont == 2);
	const Action& large = ActionOf(L"%NAME%%SIZE-L%");
	CHECK(large.labelFont == 3);
	CHECK(large.name == L"%NAME%");
}

TEST_CASE("%ICON-shape% sets the automap marker shape") {
	CHECK(ActionOf(L"%ICON-SQUARE%").iconShape == ICON_SQUARE);
	CHECK(ActionOf(L"%ICON-CIRCLE%").iconShape == ICON_CIRCLE);
	CHECK(ActionOf(L"%ICON-DIAMOND%").iconShape == ICON_DIAMOND);
	CHECK(ActionOf(L"%ICON-STAR%").iconShape == ICON_STAR);
	CHECK(ActionOf(L"%ICON-TRIANGLE%").iconShape == ICON_TRIANGLE);
	CHECK(ActionOf(L"%ICON-CROSS%").iconShape == ICON_CROSS);
	CHECK(ActionOf(L"%NAME%%ICON-STAR%").name == L"%NAME%");
}

TEST_CASE("without visual keywords the action keeps today's look") {
	const Action& a = ActionOf(L"%RED%%NAME%%MAP-0A%%BORDER-20%{desc}%CONTINUE%");
	CHECK(a.bgColor == UNDEFINED_COLOR);
	CHECK(a.bgOpacity == -1);
	CHECK(a.frameColor == UNDEFINED_COLOR);
	CHECK(a.labelFont == -1);
	CHECK(a.beamColor == UNDEFINED_COLOR);
	CHECK(a.flashColor == UNDEFINED_COLOR);
	CHECK(a.iconShape == ICON_SQUARE);
}

TEST_CASE("a filter whose only visual keyword is any one of them styles the item") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	const char* tokens[] = { "%BG-0%", "%OPACITY-50%", "%FRAME-0%", "%SIZE-S%", "%BEAM-0%", "%FLASH-0%", "%ICON-SQUARE%" };
	for (const char* token : tokens) {
		CAPTURE(token);
		LoadFilter(std::string("ItemDisplay[]: %NAME%") + token + "\n");
		GroundStyle s;
		CHECK(StyleOf(item, &s));
	}
}

TEST_CASE("malformed visual keywords are shown as typed and set nothing") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	const char* tokens[] = {
		"%BG-ZZ%",         // not hex
		"%BG-100%",        // more than a palette index
		"%BG-%",           // no value
		"%FRAME-G1%",
		"%BEAM-0A0%",
		"%FLASH-XY%",
		"%OPACITY-30%",    // not one of 25/50/75/100
		"%OPACITY-0%",
		"%OPACITY-1000%",
		"%SIZE-XL%",       // unknown size
		"%SIZE-SM%",
		"%ICON-HEXAGON%",  // unknown shape
		"%ICON-STARS%",
	};
	for (const char* token : tokens) {
		CAPTURE(token);
		CHECK(NameWith(std::string("%NAME%") + token, item) == L"Axe" + std::wstring(token, token + strlen(token)));
		GroundStyle s;
		CHECK_FALSE(StyleOf(item, &s));
	}
}

TEST_CASE("a malformed keyword does not hide a valid one of the same kind") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(NameWith("%NAME%%BG-ZZ%%BG-0A%%SIZE-XL%%SIZE-L%", item) == L"Axe%BG-ZZ%%SIZE-XL%");
	CHECK(RuleList[0]->action.bgColor == 0x0A);
	CHECK(RuleList[0]->action.labelFont == 3);
}

TEST_CASE("visual keywords do not show in the label text") {
	TestItem item("r33", ITEM_QUALITY_NORMAL);
	// PRD §5 example (NeverSink-style top rune)
	CHECK(NameWith("%RED%%NAME%%BG-20%%OPACITY-100%%FRAME-0A%%SIZE-L%%BEAM-0A%%FLASH-84%%ICON-STAR%", item, L"Zod Rune") ==
		Color(L"1") + L"Zod Rune");
}

// ---- Notification (automap) rules ---------------------------------------------------------

TEST_CASE("visual keywords alone do not make a rule a notification rule") {
	// Notification rules print drop messages and play sounds; a label style must not start doing that.
	LoadFilter("ItemDisplay[hax]: %NAME%%BG-20%%FRAME-0A%%SIZE-L%%BEAM-0A%%FLASH-84%%ICON-STAR%%OPACITY-100%\n");
	REQUIRE(RuleList.size() == 1);
	CHECK(MapRuleList.empty());
}

TEST_CASE("the automap marker actions carry the rule's icon shape") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"ItemDisplay[hax]: %NAME%%MAP-0A%%ICON-STAR%%CONTINUE%\n"
		"ItemDisplay[UNI]: %NAME%%DOT-20%\n");
	std::vector<Action> actions = map_action_cache.Get(axe.info());
	REQUIRE(actions.size() == 2);
	CHECK(actions[0].colorOnMap == 0x0A);
	CHECK(actions[0].iconShape == ICON_STAR);
	CHECK(actions[1].dotColor == 0x20);
	CHECK(actions[1].iconShape == ICON_SQUARE);
}

// ---- GetGroundStyle -----------------------------------------------------------------------

TEST_CASE("a filter without visual keywords gives no item a ground style") {
	TestItem unique("hax", ITEM_QUALITY_UNIQUE);
	TestItem rune("r33", ITEM_QUALITY_NORMAL);
	TestItem gem("gcr", ITEM_QUALITY_NORMAL);
	LoadFilter(
		"Alias[TAG]: %GOLD%\n"
		"ItemDisplay[UNI]: %TAG%%NAME%%MAP-0A%%BORDER-20%%CONTINUE%\n"
		"ItemDisplay[r33]: %RED%%NAME%{Zod}%DOT-62%%TIER-1%\n"
		"ItemDisplay[gcr]:\n"
		"ItemDisplay[]: %NAME%\n");
	GroundStyle s;
	CHECK_FALSE(StyleOf(unique, &s));
	CheckDefault(s);
	CHECK_FALSE(StyleOf(rune, &s));
	CheckDefault(s);
	CHECK_FALSE(StyleOf(gem, &s));
	CheckDefault(s);
}

TEST_CASE("with no filter loaded no item has a ground style") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	GroundStyle s;
	CHECK_FALSE(StyleOf(axe, &s));
	CheckDefault(s);
}

TEST_CASE("the matching rule's visual keywords are the item's style") {
	TestItem rune("r33", ITEM_QUALITY_NORMAL);
	LoadFilter("ItemDisplay[r33]: %NAME%%BG-20%%OPACITY-75%%FRAME-0A%%SIZE-M%%BEAM-9B%%FLASH-84%%ICON-CROSS%\n");
	GroundStyle s;
	REQUIRE(StyleOf(rune, &s));
	CHECK(s.bgColor == 0x20);
	CHECK(s.bgOpacity == 75);
	CHECK(s.frameColor == 0x0A);
	CHECK(s.labelFont == 2);
	CHECK(s.beamColor == 0x9B);
	CHECK(s.flashColor == 0x84);
	CHECK(s.iconShape == ICON_CROSS);
}

TEST_CASE("an item no styled rule matches has no ground style") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter("ItemDisplay[r33]: %NAME%%BG-20%\n");
	GroundStyle s;
	CHECK_FALSE(StyleOf(axe, &s));
	CheckDefault(s);
}

TEST_CASE("only the keywords a rule sets differ from the default style") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter("ItemDisplay[]: %NAME%%FRAME-0A%\n");
	GroundStyle s;
	REQUIRE(StyleOf(axe, &s));
	CHECK(s.frameColor == 0x0A);
	CHECK(s.bgColor == UNDEFINED_COLOR);
	CHECK(s.bgOpacity == -1);
	CHECK(s.labelFont == -1);
	CHECK(s.beamColor == UNDEFINED_COLOR);
	CHECK(s.flashColor == UNDEFINED_COLOR);
	CHECK(s.iconShape == ICON_SQUARE);
}

TEST_CASE("with %CONTINUE% a later matching rule overrides the fields it sets and keeps the rest") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"ItemDisplay[hax]: %NAME%%BG-20%%FRAME-0A%%SIZE-L%%ICON-CIRCLE%%CONTINUE%\n"
		"ItemDisplay[SET]: %NAME%%BG-01%%BEAM-01%%CONTINUE%\n"   // does not match
		"ItemDisplay[UNI]: %NAME% [u]%CONTINUE%\n"              // matches, sets no visual keyword
		"ItemDisplay[]: %NAME%%BG-62%%BEAM-84%%ICON-SQUARE%\n"
		"ItemDisplay[]: %NAME%%FLASH-0A%\n");                    // after the chain ended
	GroundStyle s;
	REQUIRE(StyleOf(axe, &s));
	CHECK(s.bgColor == 0x62);
	CHECK(s.frameColor == 0x0A);
	CHECK(s.labelFont == 3);
	CHECK(s.beamColor == 0x84);
	CHECK(s.iconShape == ICON_SQUARE);
	CHECK(s.bgOpacity == -1);
	CHECK(s.flashColor == UNDEFINED_COLOR);
	CHECK(NameOf(axe, L"Axe") == L"Axe [u]");
}

TEST_CASE("the first matching rule without %CONTINUE% ends the style lookup") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	SUBCASE("styled first rule") {
		LoadFilter(
			"ItemDisplay[hax]: %NAME%%BG-20%\n"
			"ItemDisplay[]: %NAME%%BG-62%%FRAME-0A%\n");
		GroundStyle s;
		REQUIRE(StyleOf(axe, &s));
		CHECK(s.bgColor == 0x20);
		CHECK(s.frameColor == UNDEFINED_COLOR);
	}
	SUBCASE("unstyled first rule") {
		LoadFilter(
			"ItemDisplay[hax]: %NAME%\n"
			"ItemDisplay[]: %NAME%%BG-62%\n");
		GroundStyle s;
		CHECK_FALSE(StyleOf(axe, &s));
		CheckDefault(s);
	}
}

TEST_CASE("a style alias expands into every rule that uses it") {
	TestItem rune("r33", ITEM_QUALITY_NORMAL);
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"Alias[T1STYLE]: %BG-20%%OPACITY-100%%FRAME-0A%%SIZE-L%%BEAM-0A%%ICON-STAR%\n"
		"ItemDisplay[r33]: %RED%%NAME%%T1STYLE%\n"
		"ItemDisplay[hax]: %GOLD%%NAME%%T1STYLE%\n");
	CHECK(NameOf(rune, L"Zod Rune") == Color(L"1") + L"Zod Rune");
	CHECK(NameOf(axe, L"Axe") == Color(L"4") + L"Axe");
	for (TestItem* item : { &rune, &axe }) {
		GroundStyle s;
		REQUIRE(StyleOf(*item, &s));
		CHECK(s.bgColor == 0x20);
		CHECK(s.bgOpacity == 100);
		CHECK(s.frameColor == 0x0A);
		CHECK(s.labelFont == 3);
		CHECK(s.beamColor == 0x0A);
		CHECK(s.flashColor == UNDEFINED_COLOR);
		CHECK(s.iconShape == ICON_STAR);
	}
}

TEST_CASE("FILTLVL conditions pick the style for the chosen filter level after the caches reset") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	App.lootfilter.filterLevel.uValue = 1;
	LoadFilter(
		"ItemDisplayFilterName[]: Relaxed\n"
		"ItemDisplayFilterName[]: Strict\n"
		"ItemDisplay[FILTLVL=1 hax]: %NAME%%BEAM-0A%%CONTINUE%\n"
		"ItemDisplay[hax]: %NAME%%BG-20%\n");
	GroundStyle s;
	REQUIRE(StyleOf(axe, &s));
	CHECK(s.beamColor == 0x0A);
	CHECK(s.bgColor == 0x20);
	// Item::ChangeFilterLevels / Item::OnDraw reset the caches when the level changes.
	App.lootfilter.filterLevel.uValue = 2;
	ResetCaches();
	REQUIRE(StyleOf(axe, &s));
	CHECK(s.beamColor == UNDEFINED_COLOR);
	CHECK(s.bgColor == 0x20);
}

TEST_CASE("reloading the filter replaces the cached styles") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	GroundStyle s;
	LoadFilter("ItemDisplay[hax]: %NAME%%BG-20%\n");
	REQUIRE(StyleOf(axe, &s));
	CHECK(s.bgColor == 0x20);
	LoadFilter("ItemDisplay[hax]: %NAME%%BG-62%\n");
	REQUIRE(StyleOf(axe, &s));
	CHECK(s.bgColor == 0x62);
	LoadFilter("ItemDisplay[hax]: %NAME%\n");
	CHECK_FALSE(StyleOf(axe, &s));
	CheckDefault(s);
	LoadFilter("ItemDisplay[hax]: %NAME%%FRAME-0A%\n");
	REQUIRE(StyleOf(axe, &s));
	CHECK(s.frameColor == 0x0A);
	CHECK(s.bgColor == UNDEFINED_COLOR);
}

TEST_CASE("styles are per item and recomputed when the item's flags change") {
	TestItem low("hax", ITEM_QUALITY_UNIQUE);
	TestItem high("axe", ITEM_QUALITY_UNIQUE);
	high.ItemLevel(60);
	LoadFilter("ItemDisplay[ILVL>50]: %NAME%%BG-20%\n");
	GroundStyle s;
	CHECK_FALSE(StyleOf(low, &s));
	CHECK(StyleOf(high, &s));
	// identifying, socketing or picking up an item changes its flags
	low.ItemLevel(60);
	low.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	REQUIRE(StyleOf(low, &s));
	CHECK(s.bgColor == 0x20);
}

TEST_CASE("an item whose code BH does not know has no ground style") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter("ItemDisplay[]: %NAME%%BG-20%\n");
	GroundStyle s;
	CHECK_FALSE(GetGroundStyle(axe.unit(), &s));  // code not in ItemAttributeMap
	CheckDefault(s);
}

}  // TEST_SUITE
