#include "behaviorutils.h"

std::string toDisplayName(const std::string& name) {
	std::vector<char> chars;
	chars.reserve(name.size());
	for (char c : name) {
		if (chars.size() == 0) {
			chars.push_back(std::toupper(c));
		}
		else if (std::isupper(c)) {
			chars.push_back(' ');
			chars.push_back(c);
		}
		else {
			chars.push_back(c);
		}
	}
	return std::string(chars.begin(), chars.end());
}
