#include "stringsearch/SuffixSort.hpp"
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>

static void PrintSuffixArrayLine(std::wostream &str, const int index, const int suffix, const std::wstring_view characters) {
	const auto suffixStr = characters.substr(suffix, std::min<size_t>(characters.length() - suffix, 50));
	str << index << ": " << suffix << ' ' << suffixStr;
}

int main() {
	const std::wstring_view strs[] = {L"Hallo", L"schmallo", L"test"};

	std::wstring characters;
	for(const auto sv : strs) {
		characters += sv;
		characters += wchar_t(0);
	}

	std::vector<int> sa(characters.size());
	std::iota(sa.begin(), sa.end(), 0);
	stringsearch::SuffixSortInPlace(characters.data(), sa);

	for(size_t i = 0; i < sa.size(); ++i) {
		PrintSuffixArrayLine(std::wcout, int(i), sa[i], characters);
		std::wcout << '\n';
	}
	
   return 0;
}

// Unresolved external WinMain
#ifdef WIN32
#include <Windows.h>

int CALLBACK WinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPSTR       lpCmdLine,
    int         nCmdShow
    ) {
	main();
}

#endif