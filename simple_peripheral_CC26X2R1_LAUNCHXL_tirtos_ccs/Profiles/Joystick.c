/**********************************************************************************************
 * Filename:       Joystick.c
 *
 * Description:    This file contains the implementation of the service.
 *
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *************************************************************************************************/


/*********************************************************************
 * INCLUDES
 */
#include <string.h>

#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "gapbondmgr.h"

#include "Joystick.h"

#include "icall_ble_api.h"
#include "icall.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
* GLOBAL VARIABLES
*/

// Joystick Service UUID
CONST uint8_t JoystickUUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(JOYSTICK_SERV_UUID), HI_UINT16(JOYSTICK_SERV_UUID)
};

// JoystickMesures UUID
CONST uint8_t Joystick_JoystickMesuresUUID[ATT_UUID_SIZE] =
{
  TI_BASE_UUID_128(JOYSTICK_JOYSTICKMESURES_UUID)
};

/*********************************************************************
 * LOCAL VARIABLES
 */
uint16 JXF;
uint16 JYF;

static JoystickCBs_t *pAppCBs = NULL;

/*********************************************************************
* Profile Attributes - variables
*/

// Service declaration
static CONST gattAttrType_t JoystickDecl = { ATT_BT_UUID_SIZE, JoystickUUID };

// Characteristic "JoystickMesures" Properties (for declaration)
static uint8_t Joystick_JoystickMesuresProps = GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_NOTIFY;

// Characteristic "JoystickMesures" Value variable
static uint8_t Joystick_JoystickMesuresVal[JOYSTICK_JOYSTICKMESURES_LEN] = {0};

// Characteristic "JoystickMesures" CCCD
static gattCharCfg_t *Joystick_JoystickMesuresConfig;

/*********************************************************************
* Profile Attributes - Table
*/

static gattAttribute_t JoystickAttrTbl[] =
{
  // Joystick Service Declaration
  {
    { ATT_BT_UUID_SIZE, primaryServiceUUID },
    GATT_PERMIT_READ,
    0,
    (uint8_t *)&JoystickDecl
  },
    // JoystickMesures Characteristic Declaration
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &Joystick_JoystickMesuresProps
    },
      // JoystickMesures Characteristic Value
      {
        { ATT_UUID_SIZE, Joystick_JoystickMesuresUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        Joystick_JoystickMesuresVal
      },
      // JoystickMesures CCCD
      {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)&Joystick_JoystickMesuresConfig
      },
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t Joystick_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                           uint8 *pValue, uint16 *pLen, uint16 offset,
                                           uint16 maxLen, uint8 method );
static bStatus_t Joystick_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                            uint8 *pValue, uint16 len, uint16 offset,
                                            uint8 method );

/*********************************************************************
 * PROFILE CALLBACKS
 */
// Simple Profile Service Callbacks
CONST gattServiceCBs_t JoystickCBs =
{
  Joystick_ReadAttrCB,  // Read callback function pointer
  Joystick_WriteAttrCB, // Write callback function pointer
  NULL                       // Authorization callback function pointer
};

/*********************************************************************
* PUBLIC FUNCTIONS
*/

/*
 * Joystick_AddService- Initializes the Joystick service by registering
 *          GATT attributes with the GATT server.
 *
 */
bStatus_t Joystick_AddService( void )
{
  uint8_t status;

  // Allocate Client Characteristic Configuration table
  Joystick_JoystickMesuresConfig = (gattCharCfg_t *)ICall_malloc( sizeof(gattCharCfg_t) * linkDBNumConns );
  if ( Joystick_JoystickMesuresConfig == NULL )
  {
    return ( bleMemAllocError );
  }

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( LINKDB_CONNHANDLE_INVALID, Joystick_JoystickMesuresConfig );
  // Register GATT attribute list and CBs with GATT Server App
  status = GATTServApp_RegisterService( JoystickAttrTbl,
                                        GATT_NUM_ATTRS( JoystickAttrTbl ),
                                        GATT_MAX_ENCRYPT_KEY_SIZE,
                                        &JoystickCBs );

  return ( status );
}

/*
 * Joystick_RegisterAppCBs - Registers the application callback function.
 *                    Only call this function once.
 *
 *    appCallbacks - pointer to application callbacks.
 */
bStatus_t Joystick_RegisterAppCBs( JoystickCBs_t *appCallbacks )
{
  if ( appCallbacks )
  {
    pAppCBs = appCallbacks;

    return ( SUCCESS );
  }
  else
  {
    return ( bleAlreadyInRequestedMode );
  }
}

/*
 * Joystick_SetParameter - Set a Joystick parameter.
 *
 *    param - Profile parameter ID
 *    len - length of data to right
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 */
bStatus_t Joystick_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case JOYSTICK_JOYSTICKMESURES:
      if ( len == JOYSTICK_JOYSTICKMESURES_LEN )
      {
        memcpy(Joystick_JoystickMesuresVal, value, len);

        // Try to send notification.
        GATTServApp_ProcessCharCfg( Joystick_JoystickMesuresConfig, (uint8_t *)&Joystick_JoystickMesuresVal, FALSE,
                                    JoystickAttrTbl, GATT_NUM_ATTRS( JoystickAttrTbl ),
                                    INVALID_TASK_ID,  Joystick_ReadAttrCB);
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
  return ret;
}


/*
 * Joystick_GetParameter - Get a Joystick parameter.
 *
 *    param - Profile parameter ID
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 */
bStatus_t Joystick_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case JOYSTICK_JOYSTICKMESURES:
      memcpy(value, Joystick_JoystickMesuresVal, JOYSTICK_JOYSTICKMESURES_LEN);
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  return ret;
}


/*********************************************************************
 * @fn          Joystick_ReadAttrCB
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
static bStatus_t Joystick_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                       uint8 *pValue, uint16 *pLen, uint16 offset,
                                       uint16 maxLen, uint8 method )
{
  bStatus_t status = SUCCESS;

  // See if request is regarding the JoystickMesures Characteristic Value
if ( ! memcmp(pAttr->type.uuid, Joystick_JoystickMesuresUUID, pAttr->type.len) )
  {
    if ( offset > JOYSTICK_JOYSTICKMESURES_LEN )  // Prevent malicious ATT ReadBlob offsets.
    {
      status = ATT_ERR_INVALID_OFFSET;
    }
    else
    {
      *pLen = MIN(maxLen, JOYSTICK_JOYSTICKMESURES_LEN - offset);  // Transmit as much as possible
      memcpy(pValue, pAttr->pValue + offset, *pLen);
    }
  }
  else
  {
    // If we get here, that means you've forgotten to add an if clause for a
    // characteristic value attribute in the attribute table that has READ permissions.
    *pLen = 0;
    status = ATT_ERR_ATTR_NOT_FOUND;
  }

  return status;
}


/*********************************************************************
 * @fn      Joystick_WriteAttrCB
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
static bStatus_t Joystick_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                        uint8 *pValue, uint16 len, uint16 offset,
                                        uint8 method )
{
  bStatus_t status  = SUCCESS;
  uint8_t   paramID = 0xFF;

  // See if request is regarding a Client Characterisic Configuration
  if ( ! memcmp(pAttr->type.uuid, clientCharCfgUUID, pAttr->type.len) )
  {
    // Allow only notifications.
    status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                             offset, GATT_CLIENT_CFG_NOTIFY);
  }
  // See if request is regarding the JoystickMesures Characteristic Value
  else if ( ! memcmp(pAttr->type.uuid, Joystick_JoystickMesuresUUID, pAttr->type.len) )
  {
    if ( offset + len > JOYSTICK_JOYSTICKMESURES_LEN )
    {
      status = ATT_ERR_INVALID_OFFSET;
    }
    else
    {
      // Copy pValue into the variable we point to from the attribute table.
      memcpy(pAttr->pValue + offset, pValue, len);

      // Only notify application if entire expected value is written
      if ( offset + len == JOYSTICK_JOYSTICKMESURES_LEN)
        paramID = JOYSTICK_JOYSTICKMESURES;
    }
  }
  else
  {
    // If we get here, that means you've forgotten to add an if clause for a
    // characteristic value attribute in the attribute table that has WRITE permissions.
    status = ATT_ERR_ATTR_NOT_FOUND;
  }

  // Let the application know something changed (if it did) by using the
  // callback it registered earlier (if it did).
  if (paramID != 0xFF)
    if ( pAppCBs && pAppCBs->pfnChangeCb )
      pAppCBs->pfnChangeCb( paramID ); // Call app function from stack task context.

  return status;
}


void SendJoystickMesure(void){
    uint8_t indexArr=0;
    Joystick_JoystickMesuresVal[indexArr++] = 0xFE;
    Joystick_JoystickMesuresVal[indexArr++] = 0X00;
    Joystick_JoystickMesuresVal[indexArr++] = 0x70;
    Joystick_JoystickMesuresVal[indexArr++] = 0x00;
    Joystick_JoystickMesuresVal[indexArr++] = 0x0E;
    Joystick_JoystickMesuresVal[indexArr++] = 0x00;
    Joystick_JoystickMesuresVal[indexArr++] = 0x00;
    Joystick_JoystickMesuresVal[indexArr++] = (uint8_t)(JXF >>8);
    Joystick_JoystickMesuresVal[indexArr++] = (uint8_t)(JXF);
    Joystick_JoystickMesuresVal[indexArr++] = (uint8_t)(JYF >>8);
    Joystick_JoystickMesuresVal[indexArr++] = (uint8_t)(JYF);
    Joystick_JoystickMesuresVal[indexArr++] = (0x00);
    Joystick_JoystickMesuresVal[indexArr++] = (0x00);
    Joystick_JoystickMesuresVal[indexArr++] = 0x00;
    Joystick_JoystickMesuresVal[indexArr++] = 0x00;
    Joystick_JoystickMesuresVal[indexArr++] = 0x00;
    Joystick_JoystickMesuresVal[indexArr++] = 0x00;
    Joystick_JoystickMesuresVal[indexArr++] = 0x00;
    Joystick_JoystickMesuresVal[indexArr++] = 0x00;
    Joystick_JoystickMesuresVal[indexArr++] = 0x00;
    Joystick_SetParameter(JOYSTICK_JOYSTICKMESURES,
                          JOYSTICK_JOYSTICKMESURES_LEN,
                          Joystick_JoystickMesuresVal);

}
void SaveDataToSendj(float JxADC, float JyADC){
    JXF = (uint16)(JxADC*1000);
    JYF = (uint16)(JyADC*1000);
}
