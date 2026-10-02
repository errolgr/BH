// Light beams above ground items (%BEAM-XX% permanent, %FLASH-XX% for a moment after the drop):
// beam geometry, the flash fade, which items flash, and the cap on simultaneous beams.
#include "doctest/doctest.h"

#include <Windows.h>

#include <algorithm>
#include <vector>

#include "Modules/Item/ItemBeams.h"

TEST_SUITE("Item beams") {
	using namespace ItemBeams;

	TEST_CASE("a flash starts at full intensity and fades out over 2 seconds") {
		CHECK(FlashIntensity(0) == kMaxIntensity);
		CHECK(FlashIntensity(500) < kMaxIntensity);
		CHECK(FlashIntensity(1000) < FlashIntensity(500));
		CHECK(FlashIntensity(1000) > kMaxIntensity / 2 - 3);
		CHECK(FlashIntensity(1000) < kMaxIntensity / 2 + 3);
		CHECK(FlashIntensity(1999) > 0);
		CHECK(FlashIntensity(2000) == 0);
		CHECK(FlashIntensity(3000) == 0);
		CHECK(FlashIntensity(0xFFFFFFF0) == 0);
	}

	TEST_CASE("the pulse is a triangle wave over 1.6 seconds") {
		CHECK(PulsePhase(0) == 0);
		CHECK(PulsePhase(800) == 255);
		CHECK(PulsePhase(1600) == 0);
		CHECK(PulsePhase(400) == PulsePhase(1200));
		CHECK(PulsePhase(400) > 100);
		CHECK(PulsePhase(400) < 155);
		CHECK(PulsePhase(3200 + 800) == 255);
	}

	TEST_CASE("an intensity of 0 draws nothing") {
		BeamRect r[kMaxBeamRects];
		CHECK(BeamRects(0, 0, r, kMaxBeamRects) == 0);
		CHECK(BeamRects(-5, 0, r, kMaxBeamRects) == 0);
	}

	struct Extent {
		int top = 0, bottom = -1000, left = 0, right = 0, rects = 0, half = 0;
		int widest = 0, narrowest = 1000;
	};
	Extent Measure(int intensity, int pulse) {
		BeamRect r[kMaxBeamRects];
		Extent e;
		e.rects = BeamRects(intensity, pulse, r, kMaxBeamRects);
		for (int i = 0; i < e.rects; i++) {
			e.top = (std::min)(e.top, r[i].y0);
			e.bottom = (std::max)(e.bottom, r[i].y1);
			e.left = (std::min)(e.left, r[i].x0);
			e.right = (std::max)(e.right, r[i].x1);
			e.widest = (std::max)(e.widest, r[i].x1 - r[i].x0);
			e.narrowest = (std::min)(e.narrowest, r[i].x1 - r[i].x0);
			e.half += r[i].mode == kModeTrans50 ? 1 : 0;
		}
		return e;
	}

	TEST_CASE("a full beam rises kBeamHeight pixels above the item, centred on it, with a half-opaque core") {
		const Extent e = Measure(kMaxIntensity, 0);
		CHECK(e.rects > 0);
		CHECK(e.top == -kBeamHeight);
		CHECK(e.bottom <= 3);   // the light pool on the ground reaches at most 2 px below the ground point
		CHECK(e.left == -(e.right - 1));
		CHECK(e.half > 0);
		CHECK(e.narrowest <= 3);    // core
		CHECK(e.widest >= 13);      // glow
		CHECK(e.widest <= 32);
	}

	TEST_CASE("every rectangle of a beam is symmetric about the item and uses the translucent draw modes") {
		for (int intensity : { 1, 100, 128, 200, kMaxIntensity })
			for (int pulse : { 0, 128, 255 }) {
				BeamRect r[kMaxBeamRects];
				const int n = BeamRects(intensity, pulse, r, kMaxBeamRects);
				for (int i = 0; i < n; i++) {
					CAPTURE(intensity);
					CAPTURE(pulse);
					CHECK(r[i].x0 == -(r[i].x1 - 1));
					CHECK(r[i].y0 < r[i].y1);
					CHECK((r[i].mode == kModeTrans25 || r[i].mode == kModeTrans50));
				}
			}
	}

	TEST_CASE("the glow breathes with the pulse") {
		CHECK(Measure(kMaxIntensity, 255).widest > Measure(kMaxIntensity, 0).widest);
	}

	TEST_CASE("a fading beam gets shorter and loses its half-opaque core") {
		const Extent full = Measure(kMaxIntensity, 0);
		const Extent faded = Measure(FlashIntensity(1800), 0);
		CHECK(faded.rects > 0);
		CHECK(faded.top > full.top);
		CHECK(faded.top < -kBeamHeight / 3);
		CHECK(faded.half == 0);
		CHECK(faded.widest < full.widest);
	}

	TEST_CASE("beam rectangles are cut at the caller's limit") {
		BeamRect r[2];
		CHECK(BeamRects(kMaxIntensity, 0, r, 2) == 2);
	}

	TEST_CASE("an item that drops in view flashes from the moment it is first seen") {
		FlashTracker t;
		DWORD start = 0;
		REQUIRE(t.Observe(7, true, 1000, &start));
		CHECK(start == 1000);
		REQUIRE(t.Observe(7, true, 2500, &start));
		CHECK(start == 1000);
		CHECK(FlashIntensity(2500 - start) > 0);
		REQUIRE(t.Observe(7, false, 3100, &start));  // the client may clear its new-drop flag later
		CHECK(FlashIntensity(3100 - start) == 0);
	}

	TEST_CASE("an item already on the ground when it came into view never flashes") {
		FlashTracker t;
		DWORD start = 0;
		CHECK_FALSE(t.Observe(9, false, 1000, &start));
		CHECK_FALSE(t.Observe(9, true, 1100, &start));
	}

	TEST_CASE("a forgotten item that comes back into view without the new-drop flag does not flash") {
		FlashTracker t;
		DWORD start = 0;
		t.Observe(5, true, 1000, &start);
		t.Observe(6, false, 1000, &start);
		t.Observe(6, false, 20000, &start);
		t.Prune(1000 + FlashTracker::kForgetMs + 1);
		CHECK(t.Size() == 1);  // 6 was seen recently, 5 is gone
		CHECK_FALSE(t.Observe(5, false, 40000, &start));
	}

	TEST_CASE("flash ages survive the tick counter wrapping") {
		FlashTracker t;
		DWORD start = 0;
		REQUIRE(t.Observe(1, true, 0xFFFFFF00u, &start));
		const DWORD now = 0x100;
		CHECK(FlashIntensity(now - start) > 0);
		t.Prune(now);
		CHECK(t.Size() == 1);
	}

	TEST_CASE("Clear forgets every item") {
		FlashTracker t;
		DWORD start = 0;
		t.Observe(1, false, 10, &start);
		t.Clear();
		CHECK(t.Observe(1, true, 20, &start));
	}

	TEST_CASE("the beam cap keeps the items nearest the player") {
		std::vector<Candidate> c;
		const long d[] = { 400, 9, 1600, 25, 100, 3600, 1, 900, 4, 2500, 49, 16, 64, 81, 36, 144, 225, 10000, 121, 169 };
		for (long v : d)
			c.push_back(Candidate{ v, 0, 0, 0x0A, kMaxIntensity });
		SelectNearest(c, kMaxBeams);
		REQUIRE(c.size() == (size_t)kMaxBeams);
		std::vector<long> kept;
		for (const Candidate& x : c)
			kept.push_back(x.distSq);
		std::sort(kept.begin(), kept.end());
		CHECK(kept == std::vector<long>{ 1, 4, 9, 16, 25, 36, 49, 64, 81, 100, 121, 144, 169, 225, 400, 900 });
	}

	TEST_CASE("fewer candidates than the cap are all kept") {
		std::vector<Candidate> c{ { 5, 0, 0, 1, 1 }, { 3, 0, 0, 1, 1 } };
		SelectNearest(c, kMaxBeams);
		CHECK(c.size() == 2);
	}

	TEST_CASE("with no panel open the whole screen width is visible, at any resolution") {
		const ScreenSpan s800 = VisibleSpan(0, 800);
		CHECK(s800.x0 == 0);
		CHECK(s800.x1 == 800);
		const ScreenSpan s1068 = VisibleSpan(0, 1068);
		CHECK(s1068.x0 == 0);
		CHECK(s1068.x1 == 1068);
	}

	TEST_CASE("a left panel (character, quests, waypoints...) hides the left half of the screen") {
		const ScreenSpan s = VisibleSpan(2, 1068);
		CHECK(s.x0 == 534);
		CHECK(s.x1 == 1068);
		CHECK(VisibleSpan(2, 800).x0 == 400);
	}

	TEST_CASE("a right panel (inventory, skills...) hides the right half of the screen") {
		const ScreenSpan s = VisibleSpan(1, 1068);
		CHECK(s.x0 == 0);
		CHECK(s.x1 == 534);
	}

	TEST_CASE("with both panels open nothing is visible") {
		const ScreenSpan s = VisibleSpan(3, 1068);
		CHECK(s.x0 >= s.x1);
		long x0 = 100, x1 = 110;
		CHECK_FALSE(ClipToSpan(s, &x0, &x1));
	}

	TEST_CASE("a beam rectangle crossing the panel edge is cut at the edge, one wholly behind it is dropped") {
		const ScreenSpan left = VisibleSpan(2, 1068);  // visible: 534 <= x < 1068
		long x0 = 525, x1 = 540;
		REQUIRE(ClipToSpan(left, &x0, &x1));
		CHECK(x0 == 534);
		CHECK(x1 == 540);
		x0 = 500; x1 = 534;
		CHECK_FALSE(ClipToSpan(left, &x0, &x1));
		const ScreenSpan right = VisibleSpan(1, 1068);  // visible: 0 <= x < 534
		x0 = 530; x1 = 545;
		REQUIRE(ClipToSpan(right, &x0, &x1));
		CHECK(x0 == 530);
		CHECK(x1 == 534);
		x0 = 534; x1 = 540;
		CHECK_FALSE(ClipToSpan(right, &x0, &x1));
	}

	TEST_CASE("rectangles inside the visible area are not changed; the screen edges clip too") {
		const ScreenSpan all = VisibleSpan(0, 1068);
		long x0 = 200, x1 = 215;
		REQUIRE(ClipToSpan(all, &x0, &x1));
		CHECK(x0 == 200);
		CHECK(x1 == 215);
		x0 = -6; x1 = 4;
		REQUIRE(ClipToSpan(all, &x0, &x1));
		CHECK(x0 == 0);
		CHECK(x1 == 4);
		x0 = 1060; x1 = 1075;
		REQUIRE(ClipToSpan(all, &x0, &x1));
		CHECK(x1 == 1068);
	}
}
