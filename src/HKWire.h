#pragma once

#include <LogicPublicTypes.h>

#include <optional>
#include <utility>
#include <array>
#include <assert.h>
#include <endian.h>
#include <map>	// until something static memory is available

namespace HKWire
{
	// -------- BITLEVEL

	using Ticks = unsigned;
	using Bits = unsigned;

	enum class WordState
	{
		start = 0,
		source,
		dest,
		command,
		// data optional
		data1,
		data2,
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
			case WordState::data1:
			case WordState::data2:
				return 8;
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
			case WordState::data1:
			case WordState::data2:
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
				return wordState == WordState::data1 || wordState == WordState::data2 ||
						wordState == WordState::end;
			default:
				return false;
		}
	}
	static_assert(isBitValidInState(WordState::start, BitType::start), "noy");
	static_assert(!isBitValidInState(WordState::start, BitType::end), "noy");
	static_assert(!isBitValidInState(WordState::source, BitType::start), "noy");
	static_assert(isBitValidInState(WordState::source, BitType::data0), "noy");
	static_assert(isBitValidInState(WordState::data1, BitType::end), "noy");
	static_assert(isBitValidInState(WordState::data2, BitType::end), "noy");
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


	// ----- PROTOCOL LEVEL

	using ID = U8;
	using Command = U8;
	using DataWord = U8;
	using Data = U16;

	struct Payload
	{
		using MaybeDataWord = std::optional<DataWord>;

		ID source : 4;
		ID dest : 4;
		Command command;
		MaybeDataWord data1;	// Big endian bit
		MaybeDataWord data2;	// Big endian bit

		constexpr Payload(ID s, ID d, Command c,
		                  MaybeDataWord dat1 = std::nullopt, MaybeDataWord dat2 = std::nullopt)
			: source{s}, dest{d}, command{c}, data1{dat1}, data2{dat2}
			{}

		static constexpr size_t hasData1SerializationOffset = sizeof(U32);
		static constexpr size_t hasData2SerializationOffset = sizeof(U32) + 1;

		constexpr Payload(U64 serialized)
		{
			source = serialized & 0x0F;
			dest = (serialized & 0xF0) >> 4;
			command = (serialized & 0xFF00) >> 8;
			// todo: Knowledge about one/two data word is lossy here, encode in upper bits
			if (serialized & (1 << hasData1SerializationOffset))
				data1 = (serialized & 0x00FF0000) >> 16;
			if (serialized & (1 << hasData2SerializationOffset))
				data2 = (serialized & 0xFF000000) >> (16 + 8);
		}

		constexpr
		U64
		getSerialized() const
		{
			U32 ret = 0;
			ret |= source & 0xF;
			ret |= (dest & 0xF) << 4;
			ret |= command << 8;
			if (data1.has_value())
			{
				ret |= 1 << hasData1SerializationOffset;
				ret |= *data1 << 16;
			}
			if (data2.has_value())
			{
				ret |= 1 << hasData2SerializationOffset;
				ret |= *data2 << (16 + 8);
			}
			return ret;
		}

		// note: Should also be used if bit is zero, to activate data payload-havingness
		constexpr void
		setBit(const WordState& state, const U8& bitOffset, const bool value)
		{
			const auto expectedBits = getBitsPerWord(state).value_or(0);
			if (bitOffset >= expectedBits)
			{
				// this should not happen.
				return;
			}
			const U8 msbFirstOffset = (expectedBits - 1) - bitOffset;
			switch (state)
			{
			case WordState::source:
				source |= value << msbFirstOffset;
				break;
			case WordState::dest:
				dest |= value << msbFirstOffset;
				break;
			case WordState::command:
				command |= value << msbFirstOffset;
				break;
			case WordState::data1:
				data1 = data1.value_or(0) | value << msbFirstOffset;
				break;
			case WordState::data2:
				data2 = data2.value_or(0) | value << msbFirstOffset;
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
			case WordState::data1:
				return data1.value_or(0);
			case WordState::data2:
				return data2.value_or(0);
			default:
				// should not happen
				return 0;
			}
		}

		constexpr
		size_t
		getDataLength() const
		{
			return data1.has_value() ? *getBitsPerWord(WordState::data1) : 0
			       + data2.has_value() ? *getBitsPerWord(WordState::data2) : 0;
		}

		constexpr
		Data
		getDataInHostOrder() const
		{
			Data ret{0};
			if (data1.has_value())
			{
				if (data2.has_value())
				{
					// In two byte mode, the MSB comes first
					ret = *data1 << 8 | *data2;	// automatically in host order through the shift?!
				}
				else
				{
					// in one byte mode, the Byte is Low
					ret = *data1;
				}
			}

			return ret;
		}

	};

	// AKA: One Transmission
	struct HKWireState
	{
		U32 startOfTransmission;
		U32 startOfCurrentWord;
		Bits currentNumberOfBitsReceived;
		WordState wordState;	// read: _expecting_ this state.
		Payload payload;

		constexpr HKWireState(U32 startOfTransmission = 0)
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

			const bool enoughBitsReceived = currentNumberOfBitsReceived == *expectedBitsInThisState;
			const bool correctlyReceivedEndBit = currentBit == BitType::end && currentNumberOfBitsReceived == 1;

			// normal transition
			return enoughBitsReceived || correctlyReceivedEndBit; // the first
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
			payload.setBit(wordState, currentNumberOfBitsReceived, value);
			// does NOT advance `currentNumberOfBitsReceived`!
		}

		constexpr void
		reset()
		{
			*this = HKWireState();
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
				return "data1";
			case WordState::data2:
				return "data2";
			case WordState::end:
				return "end";
			default:
				return "unknown!";
		}
	}

	using IdDescriptionMap = std::map<ID, const char*>;
	using CommandDescriptionMap = std::map<HKWire::Command, const char*>;
	using TargetCommandDescriptionMap = std::map<ID, const CommandDescriptionMap&>;

	extern const IdDescriptionMap knownIDs;
	extern const TargetCommandDescriptionMap knownCommands;
}