/**********************************************************************************************
 * Filename:       accelerometre.c
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

#include "accelerometre.h"

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

// accelerometre Service UUID
CONST uint8_t accelerometreUUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(ACCELEROMETRE_SERV_UUID), HI_UINT16(ACCELEROMETRE_SERV_UUID)
};

// AccelerometreMesures UUID
CONST uint8_t accelerometre_AccelerometreMesuresUUID[ATT_UUID_SIZE] =
{
  TI_BASE_UUID_128(ACCELEROMETRE_ACCELEROMETREMESURES_UUID)
};

/*********************************************************************
 * LOCAL VARIABLES
 */
uint16 AXF;
uint16 AYF;
uint16 AZF;

static accelerometreCBs_t *pAppCBs = NULL;

/*********************************************************************
* Profile Attributes - variables
*/

// Service declaration
static CONST gattAttrType_t accelerometreDecl = { ATT_BT_UUID_SIZE, accelerometreUUID };

// Characteristic "AccelerometreMesures" Properties (for declaration)
static uint8_t accelerometre_AccelerometreMesuresProps = GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_NOTIFY;

// Characteristic "AccelerometreMesures" Value variable
static uint8_t Accelerometre_AccelerometreMesuresVal[ACCELEROMETRE_ACCELEROMETREMESURES_LEN] = {0};

// Characteristic "AccelerometreMesures" CCCD
static gattCharCfg_t *accelerometre_AccelerometreMesuresConfig;

/*********************************************************************
* Profile Attributes - Table
*/

static gattAttribute_t accelerometreAttrTbl[] =
{
  // accelerometre Service Declaration
  {
    { ATT_BT_UUID_SIZE, primaryServiceUUID },
    GATT_PERMIT_READ,
    0,
    (uint8_t *)&accelerometreDecl
  },
    // AccelerometreMesures Characteristic Declaration
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &accelerometre_AccelerometreMesuresProps
    },
      // AccelerometreMesures Characteristic Value
      {
        { ATT_UUID_SIZE, accelerometre_AccelerometreMesuresUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        Accelerometre_AccelerometreMesuresVal
      },
      // AccelerometreMesures CCCD
      {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)&accelerometre_AccelerometreMesuresConfig
      },
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t accelerometre_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                           uint8 *pValue, uint16 *pLen, uint16 offset,
                                           uint16 maxLen, uint8 method );
static bStatus_t accelerometre_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                            uint8 *pValue, uint16 len, uint16 offset,
                                            uint8 method );

/*********************************************************************
 * PROFILE CALLBACKS
 */
// Simple Profile Service Callbacks
CONST gattServiceCBs_t accelerometreCBs =
{
  accelerometre_ReadAttrCB,  // Read callback function pointer
  accelerometre_WriteAttrCB, // Write callback function pointer
  NULL                       // Authorization callback function pointer
};

/*********************************************************************
* PUBLIC FUNCTIONS
*/

/*
 * Accelerometre_AddService- Initializes the Accelerometre service by registering
 *          GATT attributes with the GATT server.
 *
 */
bStatus_t Accelerometre_AddService( void )
{
  uint8_t status;

  // Allocate Client Characteristic Configuration table
  accelerometre_AccelerometreMesuresConfig = (gattCharCfg_t *)ICall_malloc( sizeof(gattCharCfg_t) * linkDBNumConns );
  if ( accelerometre_AccelerometreMesuresConfig == NULL )
  {
    return ( bleMemAllocError );
  }

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( LINKDB_CONNHANDLE_INVALID, accelerometre_AccelerometreMesuresConfig );
  // Register GATT attribute list and CBs with GATT Server App
  status = GATTServApp_RegisterService( accelerometreAttrTbl,
                                        GATT_NUM_ATTRS( accelerometreAttrTbl ),
                                        GATT_MAX_ENCRYPT_KEY_SIZE,
                                        &accelerometreCBs );

  return ( status );
}

/*
 * Accelerometre_RegisterAppCBs - Registers the application callback function.
 *                    Only call this function once.
 *
 *    appCallbacks - pointer to application callbacks.
 */
bStatus_t Accelerometre_RegisterAppCBs( accelerometreCBs_t *appCallbacks )
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
 * Accelerometre_SetParameter - Set a Accelerometre parameter.
 *
 *    param - Profile parameter ID
 *    len - length of data to right
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 */
bStatus_t Accelerometre_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case ACCELEROMETRE_ACCELEROMETREMESURES:
      if ( len == ACCELEROMETRE_ACCELEROMETREMESURES_LEN )
      {
        memcpy(Accelerometre_AccelerometreMesuresVal, value, len);

        // Try to send notification.
        GATTServApp_ProcessCharCfg( accelerometre_AccelerometreMesuresConfig, (uint8_t *)&Accelerometre_AccelerometreMesuresVal, FALSE,
                                    accelerometreAttrTbl, GATT_NUM_ATTRS( accelerometreAttrTbl ),
                                    INVALID_TASK_ID,  accelerometre_ReadAttrCB);
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
 * Accelerometre_GetParameter - Get a Accelerometre parameter.
 *
 *    param - Profile parameter ID
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 */
bStatus_t Accelerometre_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case ACCELEROMETRE_ACCELEROMETREMESURES:
      memcpy(value, Accelerometre_AccelerometreMesuresVal, ACCELEROMETRE_ACCELEROMETREMESURES_LEN);
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  return ret;
}


/*********************************************************************
 * @fn          accelerometre_ReadAttrCB
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
static bStatus_t accelerometre_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                       uint8 *pValue, uint16 *pLen, uint16 offset,
                                       uint16 maxLen, uint8 method )
{
  bStatus_t status = SUCCESS;

  // See if request is regarding the AccelerometreMesures Characteristic Value
if ( ! memcmp(pAttr->type.uuid, accelerometre_AccelerometreMesuresUUID, pAttr->type.len) )
  {
    if ( offset > ACCELEROMETRE_ACCELEROMETREMESURES_LEN )  // Prevent malicious ATT ReadBlob offsets.
    {
      status = ATT_ERR_INVALID_OFFSET;
    }
    else
    {
      *pLen = MIN(maxLen, ACCELEROMETRE_ACCELEROMETREMESURES_LEN - offset);  // Transmit as much as possible
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
 * @fn      accelerometre_WriteAttrCB
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
static bStatus_t accelerometre_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
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
  // See if request is regarding the AccelerometreMesures Characteristic Value
  else if ( ! memcmp(pAttr->type.uuid, accelerometre_AccelerometreMesuresUUID, pAttr->type.len) )
  {
    if ( offset + len > ACCELEROMETRE_ACCELEROMETREMESURES_LEN )
    {
      status = ATT_ERR_INVALID_OFFSET;
    }
    else
    {
      // Copy pValue into the variable we point to from the attribute table.
      memcpy(pAttr->pValue + offset, pValue, len);

      // Only notify application if entire expected value is written
      if ( offset + len == ACCELEROMETRE_ACCELEROMETREMESURES_LEN)
        paramID = ACCELEROMETRE_ACCELEROMETREMESURES;
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
uint8_t in=0;
void SendAccelerometreMesure(void){
    uint8_t indexArr=0;
    Accelerometre_AccelerometreMesuresVal[indexArr++] = 0xFE;
    Accelerometre_AccelerometreMesuresVal[indexArr++] = in++;
    Accelerometre_AccelerometreMesuresVal[indexArr++] = 0x70;
    Accelerometre_AccelerometreMesuresVal[indexArr++] = 0x00;
    Accelerometre_AccelerometreMesuresVal[indexArr++] = 0x0E;
    Accelerometre_AccelerometreMesuresVal[indexArr++] = 0x00;
    Accelerometre_AccelerometreMesuresVal[indexArr++] = 0x00;
    Accelerometre_AccelerometreMesuresVal[indexArr++] = (uint8_t)(AXF >>8);
    Accelerometre_AccelerometreMesuresVal[indexArr++] = (uint8_t)(AXF);
    Accelerometre_AccelerometreMesuresVal[indexArr++] = (uint8_t)(AYF >>8);
    Accelerometre_AccelerometreMesuresVal[indexArr++] = (uint8_t)(AYF);
    Accelerometre_AccelerometreMesuresVal[indexArr++] = (uint8_t)(AZF >>8);
    Accelerometre_AccelerometreMesuresVal[indexArr++] = (uint8_t)(AZF);
    Accelerometre_AccelerometreMesuresVal[indexArr++] = 0x00;
    Accelerometre_AccelerometreMesuresVal[indexArr++] = 0x00;
    Accelerometre_AccelerometreMesuresVal[indexArr++] = 0x00;
    Accelerometre_AccelerometreMesuresVal[indexArr++] = 0x00;
    Accelerometre_AccelerometreMesuresVal[indexArr++] = 0x00;
    Accelerometre_AccelerometreMesuresVal[indexArr++] = 0x00;
    Accelerometre_AccelerometreMesuresVal[indexArr++] = 0x00;
    Accelerometre_SetParameter(ACCELEROMETRE_ACCELEROMETREMESURES,
                               ACCELEROMETRE_ACCELEROMETREMESURES_LEN,
                               Accelerometre_AccelerometreMesuresVal);

}
void SaveDataToSend(float AxADC, float AyADC, float AzADC){
    AXF = (uint16)(AxADC*1000);
    AYF = (uint16)(AyADC*1000);
    AZF = (uint16)(AzADC*1000);
}
