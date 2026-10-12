// SPDX-License-Identifier: GPL-3.0+
#pragma once
#include "SaveState.h"

class RewindHistory
{
public:
	static constexpr size_t ScratchBytes = 16 * 1024 * 1024;
	static constexpr size_t CaptureBytes = 64 * 1024 * 1024;
	bool Empty() const { return m_states.empty(); }
	size_t Bytes() const
	{
		size_t bytes = m_states.capacity() * sizeof(m_states[0]);
		for (const auto& state : m_states)
			bytes += state->GetMemoryUsage();
		return bytes;
	}
	void Prepare(size_t budget)
	{
		while (!Empty() && Bytes() + CaptureBytes + ScratchBytes > budget)
			m_states.erase(m_states.begin());
	}
	bool Push(std::unique_ptr<ArchiveEntryList> state, size_t budget)
	{
		if (!state || state->GetMemoryUsage() + Bytes() + ScratchBytes + sizeof(state) > budget)
			return false;
		m_states.push_back(std::move(state));
		return true;
	}
	std::unique_ptr<ArchiveEntryList> Pop()
	{
		if (Empty())
			return nullptr;
		auto state = std::move(m_states.back());
		m_states.pop_back();
		return state;
	}
	void Clear() { std::vector<std::unique_ptr<ArchiveEntryList>>().swap(m_states); }

private:
	std::vector<std::unique_ptr<ArchiveEntryList>> m_states;
};
