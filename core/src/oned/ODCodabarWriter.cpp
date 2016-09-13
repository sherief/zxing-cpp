/*
* Copyright 2016 Huy Cuong Nguyen
* Copyright 2016 ZXing authors
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "oned/ODCodabarWriter.h"
#include "oned/ODWriterHelper.h"
#include "EncodeHints.h"
#include "EncodeStatus.h"

#include <cctype>

namespace ZXing {
namespace OneD {

static const char* START_END_CHARS =  "ABCD";
static const char* ALT_START_END_CHARS = "TN*E";
static const char* CHARS_WHICH_ARE_TEN_LENGTH_EACH_AFTER_DECODED = "/:+.";
static const wchar_t DEFAULT_GUARD = START_END_CHARS[0];

static const int ALPHABET_COUNT = 20;
static const char* ALPHABET = "0123456789-$:/.+ABCD";

/**
* These represent the encodings of characters, as patterns of wide and narrow bars. The 7 least-significant bits of
* each int correspond to the pattern of wide and narrow, with 1s representing "wide" and 0s representing narrow.
*/
static const int CHARACTER_ENCODINGS[] = {
	0x003, 0x006, 0x009, 0x060, 0x012, 0x042, 0x021, 0x024, 0x030, 0x048, // 0-9
	0x00c, 0x018, 0x045, 0x051, 0x054, 0x015, 0x01A, 0x029, 0x00B, 0x00E, // -$:/.+ABCD
};

static inline bool Contains(const char* str, int c)
{
	return strchr(str, c) != nullptr;
}

EncodeStatus
CodabarWriter::Encode(const std::wstring& contents_, int width, int height, const EncodeHints& hints, BitMatrix& output)
{
	std::wstring contents = contents_;
	if (contents.length() < 2) {
		// Can't have a start/end guard, so tentatively add default guards
		contents = DEFAULT_GUARD + contents + DEFAULT_GUARD;
	}
	else {
		// Verify input and calculate decoded length.
		char firstChar = std::toupper(contents[0]);
		char lastChar = std::toupper(contents[contents.length() - 1]);
		bool startsNormal = Contains(START_END_CHARS, firstChar);
		bool endsNormal = Contains(START_END_CHARS, lastChar);
		bool startsAlt = Contains(ALT_START_END_CHARS, firstChar);
		bool endsAlt = Contains(ALT_START_END_CHARS, lastChar);
		if (startsNormal) {
			if (!endsNormal) {
				return EncodeStatus::WithError("Invalid start/end guards");
			}
			// else already has valid start/end
		}
		else if (startsAlt) {
			if (!endsAlt) {
				return EncodeStatus::WithError("Invalid start/end guards");
			}
			// else already has valid start/end
		}
		else {
			// Doesn't start with a guard
			if (endsNormal || endsAlt) {
				return EncodeStatus::WithError("Invalid start/end guards");
			}
			// else doesn't end with guard either, so add a default
			contents = DEFAULT_GUARD + contents + DEFAULT_GUARD;
		}
	}

	// The start character and the end character are decoded to 10 length each.
	int resultLength = 20;
	for (size_t i = 1; i + 1 < contents.length(); ++i) {
		int c = contents[i];
		if (c >= '0' && c <= '9' || c == '-' || c == '$') {
			resultLength += 9;
		}
		else if (Contains(CHARS_WHICH_ARE_TEN_LENGTH_EACH_AFTER_DECODED, c)) {
			resultLength += 10;
		}
		else {
			return EncodeStatus::WithError(std::string("Cannot encode : '") + static_cast<char>(c) + std::string("'"));
		}
	}
	// A blank is placed between each character.
	resultLength += contents.length() - 1;

	std::vector<bool> result(resultLength, false);
	int position = 0;
	for (size_t index = 0; index < contents.length(); ++index) {
		int c = std::toupper(contents[index]);
		if (index == 0 || index == contents.length() - 1) {
			// The start/end chars are not in the CodaBarReader.ALPHABET.
			switch (c) {
			case 'T':
				c = 'A';
				break;
			case 'N':
				c = 'B';
				break;
			case '*':
				c = 'C';
				break;
			case 'E':
				c = 'D';
				break;
			}
		}
		int code = 0;
		for (int i = 0; i < ALPHABET_COUNT; i++) {
			// Found any, because I checked above.
			if (c == ALPHABET[i]) {
				code = CHARACTER_ENCODINGS[i];
				break;
			}
		}
		bool color = true;
		int counter = 0;
		int bit = 0;
		while (bit < 7) { // A character consists of 7 digit.
			result[position] = color;
			position++;
			if (((code >> (6 - bit)) & 1) == 0 || counter == 1) {
				color = !color; // Flip the color.
				bit++;
				counter = 0;
			}
			else {
				counter++;
			}
		}
		if (index < contents.length() - 1) {
			result[position] = false;
			position++;
		}
	}

	int sidesMargin = hints.margin();
	if (sidesMargin < 0)
	{
		sidesMargin = 10;
	}
	WriterHelper::RenderResult(result, width, height, sidesMargin, output);

	return EncodeStatus::Success();
}

} // OneD
} // ZXing
