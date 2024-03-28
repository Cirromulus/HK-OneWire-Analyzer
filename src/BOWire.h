#pragma once

#include <LogicPublicTypes.h>

#include <optional>
#include <utility>
#include <array>
#include <assert.h>

namespace BOWire
{
	using Ticks = unsigned;
	using Bits = unsigned;

	enum class WordState
	{
		start = 0,
		source,
		dest,
		command,
		// if it comes with data
		data1,
		data2,
		data3,
		data4,
		endBit,

		_num
	};

	constexpr
	std::optional<Bits>
	getBitsPerWord(const WordState& word)
	{
		switch (word)
		{
			// these are guesses
			case WordState::start:
				return 1;
			case WordState::source:
				return 4;
			case WordState::dest:
				return 4;
			case WordState::command:
				return 8;
			case WordState::data1:
			case WordState::data2:
			case WordState::data3:
			case WordState::data4:
				return 4;
			case WordState::endBit:
				return 1;
			default:
				return std::nullopt;
		}
	}

	constexpr
	bool
	hasWordStateData(const WordState& word)
	{
		switch (word)
		{
			case WordState::source:
			case WordState::dest:
			case WordState::command:
			case WordState::data1:
			case WordState::data2:
			case WordState::data3:
			case WordState::data4:
				return true;
			default:
				return false;
		}
	}

	enum class BitType
	{
		start = 0,
		data1,
		data0,
		end,
		// as BitType::data0 and BitType::end have
		// the same high-duration, it is important
		// that `data0` comes before `end`.
		_num
	};

	struct Waveform
	{
		Ticks low;
		static constexpr Ticks high = 2;	// always 2 (or infinity for end bit)

		// constexpr Waveform (const Ticks& low, const Ticks& high)
		// 	: low{low}, high{high} {};

		constexpr bool operator==(const Waveform& other) const
		{
			return low == other.low;
		}
	};

	constexpr std::optional<Waveform>
	getWaveformForBit(const BitType& type)
	{
		switch (type)
		{
		case BitType::start:
			return Waveform{11};
		case BitType::data1:
			return Waveform{7};
		case BitType::data0:
			return Waveform{2};
		case BitType::end:
			// end byte only differs by high-duration.
			// This is observed to about 8ms.
			// Could be checked by "busy" line,
			// but this is not necessary (just greater than 2 ticks)
			return Waveform{2};
		default:
			return std::nullopt;
		}
	}

	static constexpr Ticks maxLowTicks = 11;	// TODO: calculate max from table

	constexpr std::optional<BitType>
	getBitFromWaveform(const Waveform& waveform)
	{

		for (std::underlying_type_t<BitType> i = 0; i < std::to_underlying(BitType::_num); i++)
		{
			const auto& currentBitType = static_cast<BitType>(i);
			const auto maybeWaveform = getWaveformForBit(currentBitType);
			if (maybeWaveform.has_value() && *maybeWaveform == waveform)
			{
				return currentBitType;
			}
		}
		// not found
		return std::nullopt;
	}
	static_assert(*getBitFromWaveform(Waveform{11}) == BitType::start, "start no workey werkoy");
	static_assert(*getBitFromWaveform(Waveform{2}) == BitType::data0, "data0 no workey werkoy");

	constexpr
	U64
	getSamplesPerTick(const U32& timeBase_us, const U32& sampleRate_Hz)
	{
		const U32 samplesPerTimeBase = sampleRate_Hz / ((1000 * 1000) / timeBase_us);
		return samplesPerTimeBase;
	}
    static_assert(getSamplesPerTick(560, 8000) >= 4, "calculation is wrong?");

	static constexpr
	Ticks
	matchSamplesToTicks(const U32& samplesPerTick, const U32& recordedSamples)
	{
		const U32 wholeDivisions = recordedSamples / samplesPerTick;
		const U32 rest = recordedSamples % samplesPerTick;
		const Ticks ticks = rest > (samplesPerTick / 2) ? wholeDivisions + 1 : wholeDivisions;
		return ticks;
	}
	static_assert(matchSamplesToTicks(20, 30) == 1, "calculation is wrong?");
	static_assert(matchSamplesToTicks(20, 31) == 2, "calculation is wrong?");

	struct BOWireState
	{
		U32 startOfTransmission;
		U32 startOfCurrentWord;
		Bits currentNumberOfBitsReceived;
		WordState wordState;
		std::array<U8, std::to_underlying(WordState::_num)> data;

		constexpr BOWireState(U32 startOfTransmission = 0)
			: startOfTransmission{startOfTransmission},
			  startOfCurrentWord{startOfTransmission},
			  currentNumberOfBitsReceived{0},
			  wordState{WordState::start}
		{
			std::fill(data.begin(), data.end(), 0);
		}

		constexpr void
		reset()
		{
			*this = BOWireState();
		};
	};


	// very ugly
	static constexpr
	const char* getNameOfWordState(const WordState& state)
	{
		switch (state)
		{
			case WordState::start:
				return "start";
			case WordState::source:
				return "source";
			case WordState::dest:
				return "destination";
			case WordState::command:
				return "command";
			case WordState::data1:
			case WordState::data2:
			case WordState::data3:
			case WordState::data4:
				return "data";
			case WordState::endBit:
				return "end";
			default:
				return "unknown!";
		}
	}
}