#pragma once
#include <stdint.h>
#pragma pack(1)
typedef struct _UdpPacket {
	uint32_t userId; // src
	uint8_t index; // timer index
	uint8_t type : 7;// msg_type
	uint8_t del : 1;   // 
	union{
		uint32_t dstUserId;  // dst
		uint32_t sessionId;
	};
	uint16_t localPort;
}UdpPacket;
#pragma pack()
