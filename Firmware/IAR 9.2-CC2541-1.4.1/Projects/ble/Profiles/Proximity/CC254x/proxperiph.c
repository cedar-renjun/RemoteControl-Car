/**************************************************************************************************
  Filename:       proxperiph.c
  Revised:        $Date: 2015-03-24 09:19:15 -0700 (Tue, 24 Mar 2015) $
  Revision:       $Revision: 43274 $

  Description:    Proximity Profile - Peripheral Role


  Copyright 2009 - 2015 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE, 
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com. 
**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"

#include "proxperiph.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#define PP_DEFAULT_TX_POWER               4
#define PP_DEFAULT_PATH_LOSS              0x7F

#define SERVAPP_NUM_ATTR_SUPPORTED        5

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Link Loss Service UUID: 0x1500
CONST uint8 linkLossServUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16( LINK_LOSS_SERVICE_UUID ), HI_UINT16( LINK_LOSS_SERVICE_UUID )
};

// Immediate Alert Service UUID: 0x1501
CONST uint8 imAlertServUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16( IMMEDIATE_ALERT_SERVICE_UUID ), HI_UINT16( IMMEDIATE_ALERT_SERVICE_UUID )
};

// Tx Power Level Service UUID: 0x1502
CONST uint8 txPwrLevelServUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16( TX_PWR_LEVEL_SERVICE_UUID ), HI_UINT16( TX_PWR_LEVEL_SERVICE_UUID )
};

// Alert Level Attribute UUID: 0x1505
CONST uint8 alertLevelUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16( PROXIMITY_ALERT_LEVEL_UUID ), HI_UINT16( PROXIMITY_ALERT_LEVEL_UUID )
};

// Tx Power Level Attribute UUID: 0x1506
CONST uint8 txPwrLevelUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16( PROXIMITY_TX_PWR_LEVEL_UUID ), HI_UINT16( PROXIMITY_TX_PWR_LEVEL_UUID )
};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static proxPeriphCBs_t *pp_AppCBs = NULL;

/*********************************************************************
 * Profile Attributes - variables
 */

// Link Loss Service
static CONST gattAttrType_t linkLossService = { ATT_BT_UUID_SIZE, linkLossServUUID };

// Alert Level Characteristic Properties
static uint8 llAlertLevelCharProps = GATT_PROP_READ | GATT_PROP_WRITE;

// Alert Level attribute
// This attribute enumerates the level of alert.
static uint8 llAlertLevel = PP_ALERT_LEVEL_NO;

// Immediate Alert Service
static CONST gattAttrType_t imAlertService = { ATT_BT_UUID_SIZE, imAlertServUUID };

// Alert Level Characteristic Properties
static uint8 imAlertLevelCharProps = GATT_PROP_WRITE;

// Alert Level attribute
// This attribute enumerates the level of alert.
static uint8 imAlertLevel = PP_ALERT_LEVEL_NO;

// Tx Power Level Service
static CONST gattAttrType_t txPwrLevelService = { ATT_BT_UUID_SIZE, txPwrLevelServUUID };

// Tx Power Level Characteristic Properties
static uint8 txPwrLevelCharProps = GATT_PROP_READ;

// Tx Power Level attribute
// This attribute represents the range of transmit power levels in dBm with
// a range from -20 to +20 to a resolution of 1 dBm.
static int8 txPwrLevel = PP_DEFAULT_TX_POWER;

/*********************************************************************
 * Profile Attributes - Table
 */
// Link Loss Service Atttribute Table
static gattAttribute_t linkLossAttrTbl[] = 
{
  // Link Loss service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&linkLossService                 /* pValue */
  },

    // Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &llAlertLevelCharProps 
    },

      // Alert Level attribute
      { 
        { ATT_BT_UUID_SIZE, alertLevelUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        &llAlertLevel 
      },
};

// Immediate Alert Service Atttribute Table
static gattAttribute_t imAlertAttrTbl[] = 
{
  // Immediate Alert service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&imAlertService                  /* pValue */
  },

    // Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &imAlertLevelCharProps 
    },

      // Alert Level attribute
      { 
        { ATT_BT_UUID_SIZE, alertLevelUUID },
        GATT_PERMIT_WRITE, 
        0, 
        &imAlertLevel 
      },
};

// Tx Power Level Service Atttribute Table
static gattAttribute_t txPwrLevelAttrTbl[] = 
{
  // Tx Power Level service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&txPwrLevelService               /* pValue */
  },

    // Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &txPwrLevelCharProps 
    },

      // Tx Power Level attribute
      { 
        { ATT_BT_UUID_SIZE, txPwrLevelUUID },
        GATT_PERMIT_READ, 
        0, 
        (uint8 *)&txPwrLevel 
      },
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t proxPeriph_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                                        uint8 *pValue, uint8 *pLen, uint16 offset,
                                        uint8 maxLen, uint8 method );
static bStatus_t proxPeriph_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                         uint8 *pValue, uint8 len, uint16 offset,
                                         uint8 method );

/*********************************************************************
 * PROFILE CALLBACKS
 */
// Proximity Profile - Peripheral Role Service Callbacks
CONST gattServiceCBs_t proxPeriphCBs =
{
  proxPeriph_ReadAttrCB,  // Read callback function pointer
  proxPeriph_WriteAttrCB, // Write callback function pointer
  NULL                    // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      ProxPeriph_AddService
 *
 * @brief   Initializes the Proximity Peripheral service by
 *          registering GATT attributes with the GATT server.
 *          Only call this function once.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return   Success or Failure
 */
bStatus_t ProxPeriph_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  if ( services & PP_LINK_LOSS_SERVICE )
  {
    // Register Link Loss attribute list and CBs with GATT Server App  
    status = GATTServApp_RegisterService( linkLossAttrTbl, GATT_NUM_ATTRS( linkLossAttrTbl ),
                                          GATT_MAX_ENCRYPT_KEY_SIZE, &proxPeriphCBs );
  }

  if ( ( status == SUCCESS ) && ( services & PP_IM_ALETR_SERVICE ) )
  {
    // Register Link Loss attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( imAlertAttrTbl, GATT_NUM_ATTRS( imAlertAttrTbl ),
                                          GATT_MAX_ENCRYPT_KEY_SIZE, &proxPeriphCBs );
  }
  
  if ( ( status == SUCCESS )  && ( services & PP_TX_PWR_LEVEL_SERVICE ) )
  {
    // Register Tx Power Level attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( txPwrLevelAttrTbl, GATT_NUM_ATTRS( txPwrLevelAttrTbl ),
                                          GATT_MAX_ENCRYPT_KEY_SIZE, &proxPeriphCBs );
  }

  return ( status );
}


/*********************************************************************
 * @fn      ProxPeriph_RegisterAppCBs
 *
 * @brief   Registers the application callback function. Only call 
 *          this function once.
 *
 * @param   callbacks - pointer to application callbacks.
 *
 * @return  SUCCESS or bleAlreadyInRequestedMode
 */
bStatus_t ProxPeriph_RegisterAppCBs( proxPeriphCBs_t *appCallbacks )
{
  if ( appCallbacks )
  {
    pp_AppCBs = appCallbacks;
    
    return ( SUCCESS );
  }
  else
  {
    return ( bleAlreadyInRequestedMode );
  }
}
   

/*********************************************************************
 * @fn      ProxPeriph_SetParameter
 *
 * @brief   Set a Proximity Peripheral parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to right
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t ProxPeriph_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {

    case PP_LINK_LOSS_ALERT_LEVEL:
      if ( (len == sizeof ( uint8 )) && ((*((uint8*)value) <= PP_ALERT_LEVEL_HIGH)) ) 
      {
        llAlertLevel = *((uint8*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
      
    case PP_IM_ALERT_LEVEL:
      if ( len == sizeof ( uint8 ) ) 
      {
        imAlertLevel = *((uint8*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    case PP_TX_POWER_LEVEL:
      if ( len == sizeof ( int8 ) ) 
      {
        txPwrLevel = *((int8*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn      ProxPeriph_GetParameter
 *
 * @brief   Get a Proximity Peripheral parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to put.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t ProxPeriph_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case PP_LINK_LOSS_ALERT_LEVEL:
      *((uint8*)value) = llAlertLevel;
      break;
      
    case PP_IM_ALERT_LEVEL:
      *((uint8*)value) = imAlertLevel;
      break;
      
    case PP_TX_POWER_LEVEL:
      *((int8*)value) = txPwrLevel;
      break;
 
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}


/*********************************************************************
 * @fn          proxPeriph_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 * @param       method - type of read message 
 *
 * @return      SUCCESS, blePending or Failure
 */
static uint8 proxPeriph_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                                    uint8 *pValue, uint8 *pLen, uint16 offset,
                                    uint8 maxLen, uint8 method )
{
  uint16 uuid;
  bStatus_t status = SUCCESS;

  // Make sure it's not a blob operation
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }  

  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
    // 16-bit UUID
    uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
    
    switch ( uuid )
    {
      // No need for "GATT_SERVICE_UUID" case;
      // gattserverapp handles those types for reads   

      case PROXIMITY_ALERT_LEVEL_UUID:
      case PROXIMITY_TX_PWR_LEVEL_UUID:
        *pLen = 1;
        pValue[0] = *pAttr->pValue;
        break;
      
      default:
        // Should never get here!
        *pLen = 0;
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  else
  {
    //128-bit UUID
    *pLen = 0;
    status = ATT_ERR_INVALID_HANDLE;
  }

  return ( status );
}

/*********************************************************************
 * @fn      proxPeriph_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 * @param   method - type of write message 
 *
 * @return  SUCCESS, blePending or Failure
 */
static bStatus_t proxPeriph_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                         uint8 *pValue, uint8 len, uint16 offset,
                                         uint8 method )
{
  bStatus_t status = SUCCESS;
  uint8 notify = 0xFF;

  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  { 
    // 16-bit UUID
    uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
    switch ( uuid )
    { 
      case PROXIMITY_ALERT_LEVEL_UUID:
        // Validate the value
        // Make sure it's not a blob operation
        if ( offset == 0 )
        {
          if ( len > 1 )
            status = ATT_ERR_INVALID_VALUE_SIZE;
          else
          {
            if ( pValue[0] > PP_ALERT_LEVEL_HIGH )
              status = ATT_ERR_INVALID_VALUE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
        
        //Write the value
        if ( status == SUCCESS )
        {
          uint8 *pCurValue = (uint8 *)pAttr->pValue;
          
          *pCurValue = pValue[0];
          if( pAttr->pValue == &llAlertLevel )
            notify = PP_LINK_LOSS_ALERT_LEVEL;        
          else
            notify = PP_IM_ALERT_LEVEL;                      
        }
        
        break;
        
      default:
        // Should never get here!
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  else
  {
    // 128-bit UUID
    status = ATT_ERR_INVALID_HANDLE;
  }    
  
  // If an attribute changed then callback function to notify application of change
  if ( (notify != 0xFF) && pp_AppCBs && pp_AppCBs->pfnAttrChange )
    pp_AppCBs->pfnAttrChange( notify );

  return ( status );
}

/*********************************************************************
*********************************************************************/
