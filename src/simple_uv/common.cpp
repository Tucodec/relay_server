#include "common.h"
#include "net_base.h"
#include <sstream>
#include <string>
static bool littleEndien=IsLittleendian();
// ip string -> nl
uint32_t ip_string2int(std::string ip) {
	std::stringstream ss(ip);
	int res = 0, temp;
	char ch;
	for (int i = 0; !ss.eof(); ++i) {
		ss >> temp;
		ss >> ch;
		res |= temp << (24 - 8 * i);
	}
	if (littleEndien) {
		char *p = reinterpret_cast<char*>(&res);
		swap(p[0], p[3]);
		swap(p[1], p[2]);
	}
	return res;
}
// ip nl -> string
std::string ip_int2string(uint32_t ip) {
	if (littleEndien) {
		return std::to_string((ip & 0xff)) + "."
			+ std::to_string((ip & 0xff00) >> 8) + "."
			+ std::to_string((ip & 0xff0000) >> 16) + "."
			+ std::to_string((ip & 0xff000000) >> 24);
	}
	return std::to_string((ip & 0xff000000) >> 24) + "."
		+ std::to_string((ip & 0xff0000) >> 16) + "."
		+ std::to_string((ip & 0xff00) >> 8) + "."
		+ std::to_string((ip & 0xff));
}

uint16_t port_ntohs(uint16_t port) {
	if (littleEndien) {
		uint8_t* p = reinterpret_cast<uint8_t*>(& port);
		swap(p[0], p[1]);
		return port;
	}
	return port;
}