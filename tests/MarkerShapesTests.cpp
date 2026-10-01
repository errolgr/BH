// Automap item marker shapes (%ICON-<shape>%): the pixels each shape covers at the marker sizes
// 8 (%BORDER%), 6 (%MAP%), 4 (%DOT%) and 2 (%PX%).
#include "doctest/doctest.h"

#include <Windows.h>

#include <set>
#include <utility>
#include <vector>

#include "Modules/Item/ItemDisplay.h"
#include "Modules/MapNotify/MarkerShapes.h"

namespace {

using Pixel = std::pair<int, int>;  // (dx, dy) from the marker centre

const int kShapes[] = { ICON_SQUARE, ICON_CIRCLE, ICON_DIAMOND, ICON_STAR, ICON_TRIANGLE, ICON_CROSS };
const int kSizes[] = { 2, 4, 6, 8 };

std::set<Pixel> Pixels(int shape, int size) {
	std::set<Pixel> out;
	for (int dy = -size; dy <= size; dy++)
		for (int dx = -size; dx <= size; dx++)
			if (MarkerShapes::Contains(shape, size, dx, dy))
				out.insert(Pixel(dx, dy));
	return out;
}

// Number of pixels of each row, top to bottom.
std::vector<int> RowWidths(int shape, int size) {
	std::vector<int> out;
	for (int dy = -size / 2; dy < size / 2; dy++) {
		int n = 0;
		for (int dx = -size / 2; dx < size / 2; dx++)
			n += MarkerShapes::Contains(shape, size, dx, dy) ? 1 : 0;
		out.push_back(n);
	}
	return out;
}

std::vector<int> Widths(std::initializer_list<int> w) {
	return std::vector<int>(w);
}

}  // namespace

TEST_SUITE("Automap marker shapes") {
	TEST_CASE("the square covers today's box: size/2 pixels left of and above the centre, size/2 - 1 right and below") {
		CHECK(Pixels(ICON_SQUARE, 8).size() == 64);
		CHECK(MarkerShapes::Contains(ICON_SQUARE, 8, -4, -4));
		CHECK(MarkerShapes::Contains(ICON_SQUARE, 8, 3, 3));
		CHECK_FALSE(MarkerShapes::Contains(ICON_SQUARE, 8, 4, 0));
		CHECK_FALSE(MarkerShapes::Contains(ICON_SQUARE, 8, 0, -5));
		CHECK(Pixels(ICON_SQUARE, 2) == std::set<Pixel>{ { -1, -1 }, { 0, -1 }, { -1, 0 }, { 0, 0 } });
	}

	TEST_CASE("no shape leaves the square's box") {
		for (int shape : kShapes)
			for (int size : kSizes)
				for (const Pixel& p : Pixels(shape, size)) {
					CAPTURE(shape);
					CAPTURE(size);
					CHECK(MarkerShapes::Contains(ICON_SQUARE, size, p.first, p.second));
				}
	}

	TEST_CASE("every shape is mirror-symmetric left to right about the centre") {
		for (int shape : kShapes)
			for (int size : kSizes)
				for (const Pixel& p : Pixels(shape, size)) {
					CAPTURE(shape);
					CAPTURE(size);
					CHECK(MarkerShapes::Contains(shape, size, -1 - p.first, p.second));
				}
	}

	TEST_CASE("a circle cuts the corners: rows of 4, 6, 8, 8, 8, 8, 6, 4 at 8 px") {
		CHECK(RowWidths(ICON_CIRCLE, 8) == Widths({ 4, 6, 8, 8, 8, 8, 6, 4 }));
		CHECK_FALSE(MarkerShapes::Contains(ICON_CIRCLE, 8, -4, -4));
		CHECK_FALSE(MarkerShapes::Contains(ICON_CIRCLE, 8, -3, -4));
		CHECK(MarkerShapes::Contains(ICON_CIRCLE, 8, -2, -4));
		CHECK(MarkerShapes::Contains(ICON_CIRCLE, 8, -4, -1));
		CHECK(RowWidths(ICON_CIRCLE, 6) == Widths({ 4, 6, 6, 6, 6, 4 }));
	}

	TEST_CASE("a diamond of radius r keeps the pixels within r steps of the centre") {
		CHECK(RowWidths(ICON_DIAMOND, 8) == Widths({ 2, 4, 6, 8, 8, 6, 4, 2 }));
		CHECK(MarkerShapes::Contains(ICON_DIAMOND, 8, -1, -4));
		CHECK(MarkerShapes::Contains(ICON_DIAMOND, 8, 0, -4));
		CHECK_FALSE(MarkerShapes::Contains(ICON_DIAMOND, 8, 1, -4));
		CHECK(MarkerShapes::Contains(ICON_DIAMOND, 8, -4, -1));
		CHECK_FALSE(MarkerShapes::Contains(ICON_DIAMOND, 8, -4, -2));
		CHECK(RowWidths(ICON_DIAMOND, 6) == Widths({ 2, 4, 6, 6, 4, 2 }));
	}

	TEST_CASE("a triangle points up: 2 pixels at the top, the full width at the bottom") {
		CHECK(RowWidths(ICON_TRIANGLE, 8) == Widths({ 2, 2, 4, 4, 6, 6, 8, 8 }));
		CHECK(RowWidths(ICON_TRIANGLE, 6) == Widths({ 2, 2, 4, 4, 6, 6 }));
		CHECK(MarkerShapes::Contains(ICON_TRIANGLE, 8, -1, -4));
		CHECK_FALSE(MarkerShapes::Contains(ICON_TRIANGLE, 8, -2, -4));
		CHECK(MarkerShapes::Contains(ICON_TRIANGLE, 8, -4, 3));
	}

	TEST_CASE("a cross is a plus with 2 pixel thick arms reaching the box edges") {
		CHECK(RowWidths(ICON_CROSS, 8) == Widths({ 2, 2, 2, 8, 8, 2, 2, 2 }));
		CHECK(RowWidths(ICON_CROSS, 6) == Widths({ 2, 2, 6, 6, 2, 2 }));
		CHECK(MarkerShapes::Contains(ICON_CROSS, 8, 0, 3));
		CHECK(MarkerShapes::Contains(ICON_CROSS, 8, -4, 0));
		CHECK_FALSE(MarkerShapes::Contains(ICON_CROSS, 8, -2, -2));
	}

	TEST_CASE("a star has a 2 pixel top point, a full-width row of arms and two separate feet") {
		CHECK(Pixels(ICON_STAR, 8).count(Pixel(-1, -4)) == 1);
		CHECK(Pixels(ICON_STAR, 8).count(Pixel(0, -4)) == 1);
		CHECK(RowWidths(ICON_STAR, 8)[2] == 8);
		// bottom row: one foot each side, nothing between them
		CHECK(MarkerShapes::Contains(ICON_STAR, 8, -3, 3));
		CHECK(MarkerShapes::Contains(ICON_STAR, 8, 2, 3));
		for (int dx = -2; dx <= 1; dx++)
			CHECK_FALSE(MarkerShapes::Contains(ICON_STAR, 8, dx, 3));
		CHECK(RowWidths(ICON_STAR, 6)[1] == 6);
	}

	TEST_CASE("shapes are told apart at the drawn sizes 6 and 8") {
		for (int size : { 6, 8 })
			for (int a : kShapes)
				for (int b : kShapes)
					if (a < b) {
						CAPTURE(size);
						CAPTURE(a);
						CAPTURE(b);
						CHECK(Pixels(a, size) != Pixels(b, size));
					}
	}

	TEST_CASE("a smaller layer of a shape lies inside the larger one, so a border outlines the marker") {
		for (int shape : kShapes)
			for (int size : { 4, 6 }) {
				std::set<Pixel> outer = Pixels(shape, size + 2);
				for (const Pixel& p : Pixels(shape, size)) {
					CAPTURE(shape);
					CAPTURE(size);
					CHECK(outer.count(p) == 1);
				}
			}
	}

	TEST_CASE("rasterised spans cover exactly the shape's pixels, one span per run, top to bottom") {
		for (int shape : kShapes)
			for (int size : kSizes) {
				MarkerShapes::Span spans[16];
				const int n = MarkerShapes::Rasterize(shape, size, spans, 16);
				std::set<Pixel> covered;
				for (int i = 0; i < n; i++) {
					CHECK(spans[i].x0 < spans[i].x1);
					if (i > 0) {
						const bool after = spans[i].dy > spans[i - 1].dy ||
							(spans[i].dy == spans[i - 1].dy && spans[i].x0 > spans[i - 1].x1);  // a gap between runs
						CHECK(after);
					}
					for (int x = spans[i].x0; x < spans[i].x1; x++)
						covered.insert(Pixel(x, spans[i].dy));
				}
				CAPTURE(shape);
				CAPTURE(size);
				CHECK(covered == Pixels(shape, size));
			}
	}

	TEST_CASE("the star's bottom row is two spans, one per foot") {
		MarkerShapes::Span spans[16];
		const int n = MarkerShapes::Rasterize(ICON_STAR, 8, spans, 16);
		REQUIRE(n >= 2);
		CHECK(spans[n - 2].dy == 3);
		CHECK(spans[n - 2].x0 == -3);
		CHECK(spans[n - 2].x1 == -2);
		CHECK(spans[n - 1].dy == 3);
		CHECK(spans[n - 1].x0 == 2);
		CHECK(spans[n - 1].x1 == 3);
	}

	TEST_CASE("rasterising stops at the span limit") {
		MarkerShapes::Span spans[3];
		CHECK(MarkerShapes::Rasterize(ICON_SQUARE, 8, spans, 3) == 3);
		CHECK(spans[2].dy == -2);
	}

	TEST_CASE("a lone %DOT% or %PX% shape is drawn at 6 px; layers keep their order and spacing") {
		CHECK(MarkerShapes::DrawSize(ICON_STAR, 4, 4) == 6);
		CHECK(MarkerShapes::DrawSize(ICON_STAR, 2, 2) == 6);
		CHECK(MarkerShapes::DrawSize(ICON_CIRCLE, 2, 4) == 4);  // %DOT% + %PX%: 6 and 4
		CHECK(MarkerShapes::DrawSize(ICON_DIAMOND, 2, 8) == 2);  // %BORDER% + %PX%: unchanged
		CHECK(MarkerShapes::DrawSize(ICON_DIAMOND, 6, 6) == 6);
	}

	TEST_CASE("squares keep today's sizes") {
		CHECK(MarkerShapes::DrawSize(ICON_SQUARE, 2, 2) == 2);
		CHECK(MarkerShapes::DrawSize(ICON_SQUARE, 4, 4) == 4);
	}
}
