#include "hal_uart.h"
#include "serialInterface.h"
#include "BLE_Bridge.h"
#include "bcomdef.h"
#include "FrameParser.h"
#include "OnBoard.h"
#include "ll.h"
#include "hci.h"
#include "peripheral.h"

//local function
static void SerialInterface_ProcessOSALMsg( osal_event_hdr_t *pMsg );

uint8 serialInterface_TaskID;   // Task ID for internal task/event processing

uint8 serialBuffer[RX_BUFF_SIZE];

uint16 serialBufferOffset = 0;
static serial_state_t  serialRxState = SERIAL_STATE_LEN;
uint8 packet_length = 0;

uint8 SystemState = 0;
uint8 FlushTxReq = 0;
uint8 FlushRxReq = 0;

static void SerialPacketParser( uint8 port, uint8 events );

void SerialInterface_Init( uint8 task_id )
{
  serialInterface_TaskID = task_id;

  NPI_InitTransport(SerialPacketParser);
}

uint16 SerialInterface_ProcessEvent( uint8 task_id, uint16 events )
{

  VOID task_id; // OSAL required parameter that isn't used in this function

  if ( events & SYS_EVENT_MSG )
  {
    uint8 *pMsg;

    if ( (pMsg = osal_msg_receive( serialInterface_TaskID )) != NULL )
    {
      SerialInterface_ProcessOSALMsg( (osal_event_hdr_t *)pMsg );

      // Release the OSAL message
      VOID osal_msg_deallocate( pMsg );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  // Discard unknown events
  return 0;
}

static void SerialInterface_MsgDispatch( SerialMsg_t* pMsg )
{
    Packet_t* Packet = (Packet_t*)pMsg->data;

    // Up Stream
    switch(pMsg->type)
    {
        case CMD_REQ_SEND_DATA:
            {
                for (uint8 i = 0; i < Packet->len; i++)
                {
                    //copy one byte to data buffer
                    serialBuffer[serialBufferOffset] = Packet->data[i];
                    //update offset
                    serialBufferOffset = circular_add(serialBufferOffset,1);
                }
            }
            break;
        case CMD_REQ_DEVICE_RESET:
            {
                HAL_SYSTEM_RESET();
            }
            break;
        case CMD_REQ_DEVICE_VERSION:
            {
                uint8 len = strlen(DEVICE_VERSION) + 1;
                Packet = (Packet_t*) osal_mem_alloc(5 + len);
                if (Packet)
                {
                    if(NULL != FramePack((uint8_t*) Packet, CMD_ACK_DEVICE_VERSION, DEVICE_VERSION, len))
                    {
                        HalUARTWrite(NPI_UART_PORT, (uint8*)Packet, len + 5);
                    }
                    osal_mem_free(Packet);
                }
                else
                {
                    // Error Frame Ack
                    FrameErrorAck(CMD_ERR_DEVICE_VERSION);
                }
            }
            break;
        case CMD_REQ_CHANGE_NAME:
            {
                uint8 len = strlen(DEVICE_VERSION) + 1;

                // Change Name
                if(13 != Packet->len)
                {
                    FrameErrorAck(CMD_ERR_CHANGE_NAME);
                }
                else
                {
                    extern uint8 scanRspData[31];
                    uint8* name = Packet->data;

                    // Change Name
                    for(uint8 i = ADV_NAME_INDEX_START; i <= ADV_NAME_INDEX_END; i++)
                    {
                        scanRspData[i] = *name++;
                    }

                    GAPRole_SetParameter( GAPROLE_SCAN_RSP_DATA, sizeof ( scanRspData ), scanRspData );

                    // Send Ack
                    Packet = (Packet_t*) osal_mem_alloc(5 + 20);
                    if (Packet)
                    {
                        if(NULL != FramePack((uint8_t*) Packet, CMD_ACK_CHANGE_NAME, &scanRspData[ADV_NAME_INDEX_START], 20))
                        {
                            HalUARTWrite(NPI_UART_PORT, (uint8*)Packet, 5 + 20);
                        }
                        osal_mem_free(Packet);
                    }
                }
            }
            break;
        case CMD_REQ_FLUSH_TX:
            {
                FlushTxReq = 1;
            }
            break;
        case CMD_REQ_FLUSH_RX:
            {
                FlushTxReq = 0;
                FrameErrorAck(CMD_ACK_FLUSH_RX);
            }
            break;
        case CMD_REQ_DEVICE_STATE:
            {
                SendMessage(CMD_ACK_DEVICE_STATE, SystemState);
            }
            break;
        case CMD_REQ_DEVICE_POWER:
            {
                uint8 power[] = {
                    HCI_EXT_TX_POWER_MINUS_23_DBM,
                    HCI_EXT_TX_POWER_MINUS_6_DBM,
                    HCI_EXT_TX_POWER_0_DBM,
                    HCI_EXT_TX_POWER_4_DBM
                };
                uint8 power_index = Packet->data[0];

                if(power_index <= 3)
                {
                    HCI_EXT_SetTxPowerCmd(power[power_index]);
                    SendMessage(CMD_ACK_DEVICE_POWER, power[power_index]);
                }
                else
                {
                    FrameErrorAck(CMD_ERR_DEVICE_POWER);
                }
            }
            break;
        default:
            {
                FrameErrorAck(CMD_ERR_SEND_DATA);
            }
            break;
    }
}

static void SerialInterface_ProcessOSALMsg( osal_event_hdr_t *pMsg )
{
    switch ( pMsg->event )
    {
        case SER_NEW_MSG_EVT:
            {
                SerialMsg_t* Msg = (SerialMsg_t*)pMsg;

                SerialInterface_MsgDispatch(Msg);
                // Dispatch Message
                osal_mem_free(Msg->data);
            }
            break;

        case SER_APP_MSG_EVT:
            {
                // App Data
                // Down Stream
                SerialMsg_t* Msg = (SerialMsg_t*)pMsg;

                HalUARTWrite(NPI_UART_PORT, (uint8*)Msg->data, Msg->len);

                // Dispatch Message
                osal_mem_free(Msg->data);
            }
            break;

        default:
            // do nothing
            break;
    }
}

void SerialPacketParser( uint8 port, uint8 events )
{
    //unused input parameters
    (void)port;
    (void)events;

    uint8* result = NULL;
    uint8  tmp;
    uint8 numBytes;

   // get the number of available bytes to process
   numBytes = NPI_RxBufLen();

   for(uint8 i = 0; i < numBytes; i++)
   {
       (void)NPI_ReadTransport((uint8 *)&tmp, 1);
       result = FrameUnpack(tmp);
       if(NULL != result)
       {
            SerialMsg_t* Msg;
            Packet_t* Packet = (Packet_t*)result;

            // TODO: Send Message to App layer
            Msg = (SerialMsg_t*)osal_msg_allocate( sizeof(SerialMsg_t));
            if(NULL != Msg)
            {
                Msg->data = osal_mem_alloc( 2 + 1 + 1+ Packet->len + 1);
                if(NULL != Msg->data)
                {
                    Msg->hdr.event = SER_NEW_MSG_EVT;
                    Msg->len = 2 + 1 + 1 + Packet->len + 1;
                    Msg->type = Packet->type;
                    memcpy(Msg->data, result, Msg->len);

                    osal_msg_send( serialInterface_TaskID, (uint8 *)Msg );
                }
            }
       }
   }
}

void cSerialPacketParser( uint8 port, uint8 events )
{
  //unused input parameters
  (void)port;
  (void)events;

   uint8 done = FALSE;
   uint8 numBytes;

   // get the number of available bytes to process
   numBytes = NPI_RxBufLen();
   // check if there's any serial port data to process
   while ( (numBytes > 0) && (!done) )
   {
     switch (serialRxState)
     {
     case SERIAL_STATE_LEN:
       //read the length
       (void)NPI_ReadTransport((uint8 *)&packet_length, 1);
       // decrement the number of available bytes
       numBytes -= 1;

       // next state
       serialRxState = SERIAL_STATE_DATA;

       //DROP THROUGH

     case SERIAL_STATE_DATA:
       //check if we've dma'd the entire packet
       if (numBytes >= packet_length)
       {
         // alloc temporary buffer
         uint8 *temp_buf;
         temp_buf = osal_mem_alloc( packet_length );

         //store dma buffer into temp buffer
         (void)NPI_ReadTransport(temp_buf, packet_length);

         // copy data from temp buffer into rx circular buffer
         for (uint8 i = 0; i < packet_length; i++)
         {
           //copy one byte to data buffer
           serialBuffer[serialBufferOffset] = temp_buf[i];
           //update offset
           serialBufferOffset = circular_add(serialBufferOffset,1);
         }
         //free temp buffer
         osal_mem_free(temp_buf);

         //decrement number of available bytes
         numBytes -= packet_length;

         //reset state machine
         serialRxState = SERIAL_STATE_LEN;
       }
       else
       {
         //not enough data to progress, so leave it in driver buffer
         done = TRUE;
       }
       break;
     }
   }
}

uint8 SendMessage(uint8 cmd, uint8 info_sent)
{
    Packet_t Packet;

    if(NULL != FramePack((uint8_t*) &Packet, cmd, &info_sent, 1))
    {
        if(6 != HalUARTWrite(NPI_UART_PORT, (uint8*)&Packet, 6))
        {
            return SUCCESS;
        }
        else
        {
            return FAILURE;
        }
    }

    return FAILURE;
}

uint8 sendAckMessage(uint8 bytes_sent)
{
    return SendMessage(CMD_ACK_SEND_DATA, bytes_sent);
}

uint8 sendDataToHost(uint8* data, uint8 len)
{
    SerialMsg_t* Msg;
    Packet_t*    Packet;

    Msg = (SerialMsg_t*)osal_msg_allocate( sizeof(SerialMsg_t));
    if(NULL != Msg)
    {
        Msg->data = osal_mem_alloc( 2 + 1 + 1+ len + 1);
        if(NULL != Msg->data)
        {
            Packet = (Packet_t*)Msg->data;

            Msg->hdr.event = SER_APP_MSG_EVT;
            Msg->len = 2 + 1 + 1 + len + 1;
            Msg->type = CMD_REQ_APP_DATA;
            FramePack(Msg->data, CMD_REQ_APP_DATA, data, len);

            osal_msg_send( serialInterface_TaskID, (uint8 *)Msg );
        }
    }

    return 0;
}

uint8 FrameErrorAck(uint8 err)
{
    Packet_t Packet;
    uint8 bytes_sent = 0;

    if(NULL != FramePack((uint8_t*) &Packet, err, NULL, 0))
    {
        bytes_sent = HalUARTWrite(NPI_UART_PORT, (uint8*)&Packet, 5);
    }

    return bytes_sent;
}

uint16 circular_diff(uint16 offset, uint16 tail)
{
  if (offset > tail)
  {
    return (offset - tail);
  }
  else
  {
    return (RX_BUFF_SIZE - tail) + offset;
  }
}

uint16 circular_add(uint16 x, uint16 y)
{
  uint16 sum = x + y;
  if (sum != RX_BUFF_SIZE)
  {
    sum = sum % RX_BUFF_SIZE;
  }
  else
  {
    sum = 0;
  }
  return sum;
}