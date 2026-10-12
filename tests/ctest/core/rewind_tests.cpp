// SPDX-License-Identifier: GPL-3.0+
#include "SaveState.h"
#include "RewindHistory.h"
#include "common/Error.h"
#include <gtest/gtest.h>

TEST(Rewind, BoundedCaptureDoesNotGrow)
{
	std::vector<u8> data(8);
	const auto capacity = data.capacity();
	memSavingState state(data, true);
	u64 value = 0x12345678;
	state.Freeze(value);
	EXPECT_TRUE(state.IsOkay());
	state.Freeze(value);
	EXPECT_TRUE(state.HasError());
	EXPECT_EQ(data.capacity(), capacity);
}

TEST(Rewind, MemoryRoundTrip)
{
	std::vector<u8> data(128);
	memSavingState save(data, true);
	std::deque<u32> input{1, 42, 123};
	save.FreezeDeque(input);
	ASSERT_TRUE(save.IsOkay());
	memLoadingState load(data);
	std::deque<u32> output;
	load.FreezeDeque(output);
	EXPECT_TRUE(load.IsOkay());
	EXPECT_EQ(input, output);
}

TEST(Rewind, RejectIncompleteArchiveBeforeMutatingVM)
{
	ArchiveEntryList archive;
	Error error;
	EXPECT_FALSE(SaveState_RestoreMemory(archive, &error));
}

TEST(Rewind, AccountsAllocatedCapacity)
{
	ArchiveEntryList archive;
	archive.GetBuffer().reserve(1024);
	archive.GetBuffer().resize(1);
	EXPECT_GE(archive.GetMemoryUsage(), 1024u + sizeof(archive));
}

TEST(Rewind, EvictionAndAbandonedFuture)
{
	RewindHistory history;
	const size_t budget = 3 * RewindHistory::CaptureBytes;
	for (u8 i = 0; i < 5; i++)
	{
		history.Prepare(budget);
		auto state = std::make_unique<ArchiveEntryList>();
		state->GetBuffer().resize(RewindHistory::CaptureBytes);
		state->GetBuffer()[0] = i;
		ASSERT_TRUE(history.Push(std::move(state), budget));
		EXPECT_LE(history.Bytes() + RewindHistory::ScratchBytes, budget);
	}
	EXPECT_EQ(history.Pop()->GetBuffer()[0], 4);
	EXPECT_EQ(history.Pop()->GetBuffer()[0], 3);
	EXPECT_TRUE(history.Empty());
	auto branch = std::make_unique<ArchiveEntryList>();
	branch->GetBuffer().push_back(42);
	ASSERT_TRUE(history.Push(std::move(branch), budget));
	EXPECT_EQ(history.Pop()->GetBuffer()[0], 42);
	EXPECT_TRUE(history.Empty());
	history.Clear();
	EXPECT_EQ(history.Bytes(), 0u);
}
TEST(Rewind, RejectOverBudgetSnapshot)
{
	RewindHistory history;
	auto state = std::make_unique<ArchiveEntryList>();
	state->GetBuffer().resize(1024);
	EXPECT_FALSE(history.Push(std::move(state), 1024));
	EXPECT_TRUE(history.Empty());
}
