#ifndef __FRAME_PARSER_H_
#define __FRAME_PARSER_H_

#ifdef  __cpluscplus
extern "C"
{
#endif

#if 0
#include <stdint.h>
#else
typedef unsigned char uint8_t;
#endif
#include <string.h>

/*********************************************************
 *
 * :   2    :   1   :   1   :     N     :   1   :
 * : Header :  Type :  Len  :  Payload  :  Xor  :
 * : 0xAB   :
 * : 0x55   :
 *
 ********************************************************/



#define PACKET_MEM_HEADER_0    0xAB
#define PACKET_MEM_HEADER_1    0x55
#define PACKET_BUF_SIZE        255

typedef struct
{
	uint8_t header[2];
	uint8_t type;
	uint8_t len;
	uint8_t data[1];
}Packet_t;

extern uint8_t* FrameUnpack(uint8_t Data);
extern uint8_t* FramePack(uint8_t* buf, uint8_t type, uint8_t* pdata, uint8_t len);

#ifdef  __cpluscplus
}
#endif

#endif  // __FRAME_PARSER_H_
