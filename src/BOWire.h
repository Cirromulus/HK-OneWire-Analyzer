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
		// data optional
		data,
		end,
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
			case WordState::data:
				return 16;
			case WordState::end:
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
			case WordState::data:
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

	constexpr bool
	isBitValidInState(const WordState& wordState, const BitType& currentBit)
	{
		switch (currentBit)
		{
			case BitType::start:
				return wordState == WordState::start;
			case BitType::data0:
			case BitType::data1:
				return hasWordStateData(wordState);
			case BitType::end:
				return wordState == WordState::data ||
						wordState == WordState::end;
			default:
				return false;
		}
	}
	static_assert(isBitValidInState(WordState::start, BitType::start), "noy");
	static_assert(!isBitValidInState(WordState::start, BitType::end), "noy");
	static_assert(!isBitValidInState(WordState::source, BitType::start), "noy");
	static_assert(isBitValidInState(WordState::source, BitType::data0), "noy");
	static_assert(isBitValidInState(WordState::data, BitType::end), "noy");
	static_assert(isBitValidInState(WordState::end, BitType::end), "noy");


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

	struct Payload
	{
		U8 source : 4;
		U8 dest : 4;
		U8 command;
		U16 data;

		constexpr Payload(U8 s, U8 d, U8 c, U16 dat)
			: source{s}, dest{d}, command{c}, data{dat}
			{}

		constexpr Payload(U32 serialized)
		{
			source = serialized & 0x0F;
			dest = (serialized & 0xF0) >> 4;
			command = (serialized & 0xFF00) >> 8;
			data = (serialized & 0xFFFF0000) >> 16;
		}

		constexpr
		U32
		getSerialized() const
		{
			U32 ret = 0;
			ret |= source & 0xF;
			ret |= (dest & 0xF) << 4;
			ret |= command << 8;
			ret |= data << 16;
			return ret;
		}

		// note: Does not reset bit
		constexpr void
		setBit(const WordState& state, const U8& bitOffset)
		{
			if (bitOffset >= getBitsPerWord(state))
			{
				// this should not happen.
				return;
			}
			switch (state)
			{
			case WordState::source:
				source |= 1 << bitOffset;
				break;
			case WordState::dest:
				dest |= 1 << bitOffset;
				break;
			case WordState::command:
				command |= 1 << bitOffset;
				break;
			case WordState::data:
				data |= 1 << bitOffset;
				break;
			default:
				break;
			}
		}

		constexpr
		U16
		getWord(const WordState& wordstate) const
		{
			switch(wordstate)
			{
			case WordState::source:
				return source & 0xF;
			case WordState::dest:
				return dest & 0xF;
			case WordState::command:
				return command;
			case WordState::data:
				return data;
			default:
				// should not happen
				return 0;
			}
		}
	};

	struct BOWireState
	{
		U32 startOfTransmission;
		U32 startOfCurrentWord;
		Bits currentNumberOfBitsReceived;
		WordState wordState;	// read: _expecting_ this state.
		Payload payload;

		constexpr BOWireState(U32 startOfTransmission = 0)
			: startOfTransmission{startOfTransmission},
			  startOfCurrentWord{startOfTransmission},
			  currentNumberOfBitsReceived{0},
			  wordState{WordState::start},
			  payload{0,0,0,0}
		{
		}

		constexpr std::optional<bool>
		canAdvanceState(const BitType& currentBit)
		{
			const auto expectedBitsInThisState = getBitsPerWord(wordState);
			if (!expectedBitsInThisState.has_value())
			{
				// invalid state or something
				return std::nullopt;
			}

			if (!isBitValidInState(wordState, currentBit))
			{
				return std::nullopt;
			}
			// from here on, a end-bit is always valid

			// normal transition
			return currentNumberOfBitsReceived == *expectedBitsInThisState ||
			       (currentBit == BitType::end && currentNumberOfBitsReceived == 1); // the first
		}

		constexpr void
		setState(const WordState& newState)
		{
			wordState = newState;
			currentNumberOfBitsReceived = 0;
		}

		constexpr void
		advanceState()
		{
			setState(static_cast<WordState>(std::to_underlying(wordState) + 1));
		}

		constexpr void
		setCurrentBit(bool value)
		{
			if (value)
			{
				payload.setBit(wordState, currentNumberOfBitsReceived);
			}
			else
			{
				// NO reset implemented (and needed?)
			}
			// does NOT advance `currentNumberOfBitsReceived`!
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
			case WordState::data:
				return "data";
			case WordState::end:
				return "end";
			default:
				return "unknown!";
		}
	}
}