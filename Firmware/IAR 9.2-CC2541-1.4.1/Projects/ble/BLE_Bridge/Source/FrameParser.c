//#include <stdint.h>
#include <string.h>

#include "FrameParser.h"

typedef enum
{
	S_WAIT = 0,
	S_HEADER,
	S_TYPE,
	S_LEN,
	S_PAYLOAD,
	S_CHECKSUM,
}FSM_State_t;

static uint8_t* ScanHeader_Pre(uint8_t Data)
{
	static uint8_t buf[2];
	static uint8_t index = 0;
	uint8_t* result = NULL;

	buf[index++] = Data;

	if(2 == index)
	{
		// Full Matched
		if ((PACKET_MEM_HEADER_0 == buf[0]) && (PACKET_MEM_HEADER_1 == buf[1]))
		{
			// Find it
			result = buf;
			index  = 0;
			goto END;
		}

		// Last Byte Matched
		if ((PACKET_MEM_HEADER_0 != buf[0]) && (PACKET_MEM_HEADER_0 == buf[1]))
		{
			buf[0] = buf[1];
			index = 1;
			goto END;
		}

		// Not Matched
		index = 0;
	}

END:

	return result;
}

uint8_t* FrameUnpack(uint8_t Data)
{
    static FSM_State_t state = S_WAIT;
	static uint8_t buf[PACKET_BUF_SIZE];
	static uint8_t index   = 0;
	static uint8_t len     = 0;
	static uint8_t len_all = 0;
	uint8_t* result = NULL;

    switch(state)
    {
        case S_WAIT:                           // Wait Protocal Header
			{
				// Find Header
				if(NULL != (result = ScanHeader_Pre(Data)))
				{
					index = 0;
					buf[index++] = *result++;
					buf[index++] = *result++;
					state = S_TYPE;

					result = NULL;
				}
			}
			break;

        case S_TYPE:                           // Wait Protocal Type
			{
				buf[index++] = Data;
				state = S_LEN;
			}
			break;

        case S_LEN:                           // Wait Protocal Length
			{

				buf[index++] = Data;
				len     = Data;
				len_all = 2 + 1 + 1 + len + 1;

				if(0 == len)           // No Data Payload
				{
					state = S_CHECKSUM;
				}
				else if(Data > 250)   // Invalid Len
				{
					state = S_WAIT;
					index = 0;
				}
				else
				{
					state = S_PAYLOAD;
				}
			}
			break;

        case S_PAYLOAD:                       // Find Header Payload
			{
				buf[index++] = Data;

				if(0 == --len)
				{
					state = S_CHECKSUM;
				}
			}
			break;

        case S_CHECKSUM:                          // Check Header via CRC8
			{
				uint8_t checksum = 0;
				uint8_t i        = 0;

				buf[index++] = Data;

				for(i = 0; i < len_all; i++)
				{
					checksum ^= buf[i];
				}

				if(!checksum) // Check Passed
				{
					result = buf;
				}

				state = S_WAIT;
				index = 0;
			}

        default:
			{
				state = S_WAIT;
				index = 0;
			}
    }

	return result;
}

uint8_t* FramePack(uint8_t* buf, uint8_t type, uint8_t* pdata, uint8_t len)
{
	Packet_t* pPacket = (Packet_t*)buf;
	uint8_t i   = 0;
	uint8_t xor = 0;

	// Check Parameters
	if(NULL == buf)
		return NULL;

	if((NULL == pdata) && (0 != len))
		return NULL;

	pPacket->header[0] = (uint8_t)PACKET_MEM_HEADER_0;
	pPacket->header[1] = (uint8_t)PACKET_MEM_HEADER_1;
	pPacket->type      = (uint8_t)type;
	pPacket->len       = (uint8_t)len;

	xor ^= pPacket->header[0];
	xor ^= pPacket->header[1];
	xor ^= pPacket->type;
	xor ^= pPacket->len;

	for(i = 0; i < len; i++)
	{
		pPacket->data[i] = pdata[i];
		xor ^= pdata[i];
	}

	pPacket->data[len] = xor;

    return buf;
}
