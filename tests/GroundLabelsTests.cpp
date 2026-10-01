// GroundLabels::PlanDetour: the entry hook of the label styles over D2Win #10177 / #10013, either
// on the original code or on top of D2GL's detour of the same entry. Bytes are the 1.13c D2Win
// prologues; a planned jmp is checked by decoding where it lands (x86 rel32: next instruction +
// displacement), independently of how PlanDetour computes it.
#include "doctest/doctest.h"

#include <cstring>

#include "Modules/Item/GroundLabels.h"

using GroundLabels::DetourPlan;
using GroundLabels::PlanDetour;

namespace {

// D2Win #10177 GetTextSize: push esi; mov esi,ecx; push edi; mov eax,esi | mov edi,edx; call ...
const BYTE kGetTextSize[] = { 0x56, 0x8B, 0xF1, 0x57, 0x8B, 0xC6, 0x8B, 0xFA, 0xE8, 0xA3, 0xF8, 0xFF, 0xFF };
// D2Win #10013 DrawFramedText: sub esp,0xC; push ebx; push ebp | push esi; mov esi,ecx; ...
const BYTE kDrawFramedText[] = { 0x83, 0xEC, 0x0C, 0x53, 0x55, 0x56, 0x8B, 0xF1, 0x57, 0x8B, 0xC6, 0x8B, 0xFA,
	0xE8, 0x0E, 0xF1, 0xFF, 0xFF };

const DWORD kEntry = 0x6F8F2700;
const DWORD kTrampoline = 0x00A10000;
const DWORD kHook = 0x10012340;

// Where the jmp rel32 at `bytes` (located at address `at`) lands.
DWORD JmpTarget(const BYTE* bytes, DWORD at) {
	REQUIRE(bytes[0] == 0xE9);
	int rel;
	memcpy(&rel, bytes + 1, 4);
	return at + 5 + rel;
}

// Writes `jmp to` as it would be encoded at address `at`.
void PutJmp(BYTE* out, DWORD at, DWORD to) {
	out[0] = 0xE9;
	int rel = (int)(to - (at + 5));
	memcpy(out + 1, &rel, 4);
}

void CheckHookCode(const DetourPlan& plan, const BYTE* code, int padding) {
	BYTE head[8];
	memcpy(head, &plan.hookCode, 8);
	CHECK(JmpTarget(head, kEntry) == kHook);
	for (int i = 5; i < 5 + padding; ++i)
		CHECK(head[i] == 0xCC);
	for (int i = 5 + padding; i < 8; ++i)
		CHECK(head[i] == code[i]);
}

}  // namespace

TEST_SUITE("GroundLabels") {

TEST_CASE("an original entry: its first whole instructions move to the trampoline, which jumps back after them") {
	DetourPlan plan;
	REQUIRE(PlanDetour(kGetTextSize, kEntry, kGetTextSize, sizeof kGetTextSize, 6, kTrampoline, kHook, &plan));
	CHECK(plan.stolen == 6);
	CHECK(plan.trampolineLen == 11);
	CHECK(memcmp(plan.trampoline, kGetTextSize, 6) == 0);
	CHECK(JmpTarget(plan.trampoline + 6, kTrampoline + 6) == kEntry + 6);
	// jmp hook + one int3 over the 6th relocated byte; bytes 6..7 stay.
	CheckHookCode(plan, kGetTextSize, 1);
}

TEST_CASE("a 5-byte prologue is replaced by the jmp alone") {
	DetourPlan plan;
	REQUIRE(PlanDetour(kDrawFramedText, kEntry, kDrawFramedText, sizeof kDrawFramedText, 5, kTrampoline, kHook, &plan));
	CHECK(plan.stolen == 5);
	CHECK(plan.trampolineLen == 10);
	CHECK(memcmp(plan.trampoline, kDrawFramedText, 5) == 0);
	CHECK(JmpTarget(plan.trampoline + 5, kTrampoline + 5) == kEntry + 5);
	CheckHookCode(plan, kDrawFramedText, 0);
}

TEST_CASE("an entry D2GL already detoured: the trampoline goes to D2GL's hook, from the trampoline's own address") {
	// D2GL's hook below and above the trampoline (negative and positive displacement from it).
	const DWORD d2glHooks[] = { 0x00401230, 0x7A001230 };
	for (DWORD d2gl : d2glHooks) {
		CAPTURE(d2gl);
		BYTE code[sizeof kGetTextSize];
		memcpy(code, kGetTextSize, sizeof code);
		PutJmp(code, kEntry, d2gl);
		code[5] = 0xCC;
		DetourPlan plan;
		REQUIRE(PlanDetour(code, kEntry, kGetTextSize, sizeof kGetTextSize, 6, kTrampoline, kHook, &plan));
		CHECK(plan.stolen == 6);
		CHECK(plan.trampolineLen == 5);
		CHECK(JmpTarget(plan.trampoline, kTrampoline) == d2gl);
		CheckHookCode(plan, code, 1);
	}
}

TEST_CASE("a detour over a 5-byte prologue, without padding") {
	BYTE code[sizeof kDrawFramedText];
	memcpy(code, kDrawFramedText, sizeof code);
	PutJmp(code, kEntry, 0x05001000);
	DetourPlan plan;
	REQUIRE(PlanDetour(code, kEntry, kDrawFramedText, sizeof kDrawFramedText, 5, kTrampoline, kHook, &plan));
	CHECK(plan.stolen == 5);
	CHECK(JmpTarget(plan.trampoline, kTrampoline) == 0x05001000);
	CheckHookCode(plan, code, 0);
}

TEST_CASE("code that is not the expected function is refused") {
	DetourPlan plan;
	SUBCASE("a byte differs after the prologue") {
		BYTE code[sizeof kGetTextSize];
		memcpy(code, kGetTextSize, sizeof code);
		code[9] ^= 0x01; // another call target
		CHECK_FALSE(PlanDetour(code, kEntry, kGetTextSize, sizeof kGetTextSize, 6, kTrampoline, kHook, &plan));
	}
	SUBCASE("a byte of the prologue differs") {
		BYTE code[sizeof kGetTextSize];
		memcpy(code, kGetTextSize, sizeof code);
		code[2] = 0xF2; // mov esi,edx
		CHECK_FALSE(PlanDetour(code, kEntry, kGetTextSize, sizeof kGetTextSize, 6, kTrampoline, kHook, &plan));
	}
	SUBCASE("a detour whose code after the padding differs") {
		BYTE code[sizeof kGetTextSize];
		memcpy(code, kGetTextSize, sizeof code);
		PutJmp(code, kEntry, 0x05001000);
		code[5] = 0xCC;
		code[7] = 0xF9; // mov edi,ecx
		CHECK_FALSE(PlanDetour(code, kEntry, kGetTextSize, sizeof kGetTextSize, 6, kTrampoline, kHook, &plan));
	}
	SUBCASE("a call at the entry is not a detour") {
		BYTE code[sizeof kGetTextSize];
		memcpy(code, kGetTextSize, sizeof code);
		code[0] = 0xE8;
		CHECK_FALSE(PlanDetour(code, kEntry, kGetTextSize, sizeof kGetTextSize, 6, kTrampoline, kHook, &plan));
	}
	SUBCASE("a detour padded so far that too little is left to recognise the function") {
		BYTE code[sizeof kGetTextSize];
		memcpy(code, kGetTextSize, sizeof code);
		PutJmp(code, kEntry, 0x05001000);
		for (int i = 5; i < 10; ++i)
			code[i] = 0xCC;
		CHECK_FALSE(PlanDetour(code, kEntry, kGetTextSize, sizeof kGetTextSize, 6, kTrampoline, kHook, &plan));
	}
}

}
