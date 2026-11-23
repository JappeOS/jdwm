#include "util/environment.hpp"
#include <cassert>
#include <initializer_list>

int main() {
	using zenith::config::flag_value;
	assert(!flag_value(nullptr));
	for (auto value : {"", "0", "fAlSe", "OFF", "No"}) assert(!flag_value(value));
	for (auto value : {"1", "true", "on", "yes", "01", "other"}) assert(flag_value(value));
}
