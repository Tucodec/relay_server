#pragma once
#include "SimpleUVExport.h"
#include <string>
#include <stdint.h>
uint32_t SUV_EXPORT ip_string2int(std::string ip);
std::string SUV_EXPORT ip_int2string(uint32_t ip);
uint16_t SUV_EXPORT port_ntohs(uint16_t port);