#include "hal_uart.h"
#include "OSAL.h"
#include "npi.h"

#define MAX_PKT_SIZE    128
#define RX_BUFF_SIZE    512


// BLE_Bridge Task Events
#define SER_NEW_MSG_EVT                              0x0001
#define SER_APP_MSG_EVT                              0x0002

#define DEVICE_VERSION                  "FanDao SLBM04 V4.0.0 2015-09-05"

// Frame Command List
/*--- Mask ---*/
#define FRAME_COMMAD_MASK_TYPE          0x80
#define FRAME_COMMAD_MASK_DIR           0x40
#define FRAME_COMMAD_MASK_RESULT        0x30
#define FRAME_COMMAD_MASK_CMD           0x0F

/*--- Value Section ---*/
#define FRAME_COMMAD_VALUE_REQ          0x00
#define FRAME_COMMAD_VALUE_ACK          0x80
#define FRAME_COMMAD_VALUE_UP           0x00
#define FRAME_COMMAD_VALUE_DOWN         0x40
#define FRAME_COMMAD_VALUE_SUCCESS      0x00
#define FRAME_COMMAD_VALUE_FAILURE      0x10

/*--- Cmd Section ---*/
#define FRAME_COMMAD_CMD_DATA           0x00
#define FRAME_COMMAD_CMD_RESET          0x01
#define FRAME_COMMAD_CMD_VERSION        0x02
#define FRAME_COMMAD_CMD_NAME           0x03
#define FRAME_COMMAD_CMD_FLUSH_TX       0x04
#define FRAME_COMMAD_CMD_FLUSH_RX       0x05
#define FRAME_COMMAD_CMD_STATE          0x06
#define FRAME_COMMAD_CMD_POWER          0x07

/*--- Cmd Lists ---*/

// Up
#define CMD_REQ_SEND_DATA               FRAME_COMMAD_VALUE_REQ | FRAME_COMMAD_VALUE_UP |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_DATA

#define CMD_REQ_DEVICE_RESET            FRAME_COMMAD_VALUE_REQ | FRAME_COMMAD_VALUE_UP |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_RESET

#define CMD_REQ_DEVICE_VERSION          FRAME_COMMAD_VALUE_REQ | FRAME_COMMAD_VALUE_UP |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_VERSION

#define CMD_REQ_CHANGE_NAME             FRAME_COMMAD_VALUE_REQ | FRAME_COMMAD_VALUE_UP |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_NAME

#define CMD_REQ_FLUSH_TX                FRAME_COMMAD_VALUE_REQ | FRAME_COMMAD_VALUE_UP |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_FLUSH_TX

#define CMD_REQ_FLUSH_RX                FRAME_COMMAD_VALUE_REQ | FRAME_COMMAD_VALUE_UP |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_FLUSH_RX

#define CMD_REQ_DEVICE_STATE            FRAME_COMMAD_VALUE_REQ | FRAME_COMMAD_VALUE_UP |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_STATE

#define CMD_REQ_DEVICE_POWER            FRAME_COMMAD_VALUE_REQ | FRAME_COMMAD_VALUE_UP |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_POWER

// Down
#define CMD_REQ_APP_DATA                FRAME_COMMAD_VALUE_REQ | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_DATA

#define CMD_ACK_SEND_DATA               FRAME_COMMAD_VALUE_ACK | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_DATA

#define CMD_ACK_DEVICE_RESET            FRAME_COMMAD_VALUE_ACK | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_RESET

#define CMD_ACK_DEVICE_VERSION          FRAME_COMMAD_VALUE_ACK | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_VERSION

#define CMD_ACK_CHANGE_NAME             FRAME_COMMAD_VALUE_ACK | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_NAME

#define CMD_ACK_FLUSH_TX                FRAME_COMMAD_VALUE_ACK | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_FLUSH_TX

#define CMD_ACK_FLUSH_RX                FRAME_COMMAD_VALUE_ACK | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_FLUSH_RX

#define CMD_ACK_DEVICE_STATE            FRAME_COMMAD_VALUE_ACK | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_STATE

#define CMD_ACK_DEVICE_POWER            FRAME_COMMAD_VALUE_ACK | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_SUCCESS | FRAME_COMMAD_CMD_POWER

// Error
#define CMD_ERR_SEND_DATA               FRAME_COMMAD_VALUE_ACK | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_FAILURE | FRAME_COMMAD_CMD_DATA

#define CMD_ERR_DEVICE_RESET            FRAME_COMMAD_VALUE_ACK | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_FAILURE | FRAME_COMMAD_CMD_RESET

#define CMD_ERR_DEVICE_VERSION          FRAME_COMMAD_VALUE_ACK | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_FAILURE | FRAME_COMMAD_CMD_VERSION

#define CMD_ERR_CHANGE_NAME             FRAME_COMMAD_VALUE_ACK | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_FAILURE | FRAME_COMMAD_CMD_NAME

#define CMD_ERR_FLUSH_TX                FRAME_COMMAD_VALUE_ACK | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_FAILURE | FRAME_COMMAD_CMD_FLUSH_TX

#define CMD_ERR_FLUSH_RX                FRAME_COMMAD_VALUE_ACK | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_FAILURE | FRAME_COMMAD_CMD_FLUSH_RX

#define CMD_ERR_DEVICE_STATE            FRAME_COMMAD_VALUE_ACK | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_FAILURE | FRAME_COMMAD_CMD_STATE

#define CMD_ERR_DEVICE_POWER            FRAME_COMMAD_VALUE_ACK | FRAME_COMMAD_VALUE_DOWN |\
                                        FRAME_COMMAD_VALUE_FAILURE | FRAME_COMMAD_CMD_POWER

//===================================================

/* States for CRC parser */
typedef enum {
  SERIAL_STATE_START = 0,
  SERIAL_STATE_TYPE,
  SERIAL_STATE_LEN,
  SERIAL_STATE_DATA,
  SERIAL_STATE_COMPLETE //received complete serial message
} serial_state_t;

extern uint8 SystemState;
extern uint8 FlushTxReq;
extern uint8 FlushRxReq;

void parseCmd(void);
void sendSerialEvt(void);

extern uint8 SendMessage(uint8 cmd, uint8 info_sent);
extern uint8 FrameErrorAck(uint8 err);

/*******************************************************************************
 * MACROS
 */

//global

typedef struct
{
  osal_event_hdr_t hdr;
  uint8            type;
  uint8            len;
  uint8*           data;
} SerialMsg_t;

extern uint16 serialBufferOffset;
extern uint8 serialBuffer[RX_BUFF_SIZE];


/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the BLE Application
 */
extern void SerialInterface_Init( uint8 task_id );

/*
 * Task Event Processor for the BLE Application
 */
extern uint16 SerialInterface_ProcessEvent( uint8 task_id, uint16 events );

extern uint8 sendAckMessage(uint8 bytes_sent);

extern uint8 sendDataToHost(uint8* data, uint8 len);

extern uint16 circular_add(uint16 x, uint16 y);

extern uint16 circular_diff(uint16 offset, uint16 tail);