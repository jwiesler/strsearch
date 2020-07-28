#pragma once
#include <chrono>

namespace stringsearch::api {
	enum class Result {
		Ok = 0,
		InvalidInstance = 1,
		NullPointer = 2,
		OffsetOutOfBounds
	};

	#define strsearchdll_CALLING_CONVENCTION __cdecl

	using LogCallback = void(strsearchdll_CALLING_CONVENCTION *) (const char *message);
	using InstanceHandle = void *;

	using TimeDuration = std::chrono::high_resolution_clock::rep;
	
	struct FindUniqueItemsResult {
		size_t TotalResults;
		size_t Count;
		size_t Consumed;
	};

	struct FindUniqueItemsTimings {
		TimeDuration Find;
		TimeDuration Unique;
	};

	struct FindUniqueItemsKeywordsTimings : FindUniqueItemsTimings {
		TimeDuration Parse;
	};

	enum class KeywordsMatch {
		All,
		AtLeastOne
	};
}
