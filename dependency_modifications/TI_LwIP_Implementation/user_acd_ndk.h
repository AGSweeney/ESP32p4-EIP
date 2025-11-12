/**
 * @file user_acd_ndk.h
 * @brief Include file for ACD functionalities of EIP application
 *
 */

/*
 * Copyright (c) 2015, Texas Instruments Incorporated
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
 **/
#ifndef _USER_ACD_NDK_H_
#define _USER_ACD_NDK_H_

#define ACD_API

#include <third_party/protocols/ethernetip_adapter/include/app_api.h>
#include <third_party/protocols/ethernetip_adapter/include/acd_api.h>
#include <ti/drv/icss_emac/icss_emacDrv.h>
#include <third_party/protocols/ethernetip_adapter/include/user_acd.h>
#include "eip_main.h"

#define ACD_LINKDOWN -10
#define ACD_LED_CONFLICT 1

#define lswap(x) ((((x) & 0xff000000) >> 24) | \
                  (((x) & 0x00ff0000) >>  8) | \
                  (((x) & 0x0000ff00) <<  8) | \
                  (((x) & 0x000000ff) << 24))

#define htonl(l) (lswap(l))
#define ntohl(l) (lswap(l))

/**
 *  @brief  Function to init the ACD monitor mechanism
 *
 *          Creates the holder to receive the ARP frame.Sets the NDK hooks required for ACD
 *
 *  @param   none
 *  @retval  none
 *
 */
void EIPACD_init(void);

/**
 *  @brief  Function to exit the ACD monitor mechanism
 *
 *
 *  @param   none
 *  @retval  none
 *
 */
void EIPACD_exit(void);


/**
 *  @brief  Starts the ACD Monitor mechanism
 *
 *          Creates ACD monitor task, semaphore and acdobj mailbox
 *
 *  @param   none
 *  @retval  Success - TRUE
 *           Fail   -  FALSE
 *
 */
Bool EIPACD_start(void);
/**
 *  @brief  Stops the ACD Monitor Task
 *
 *          Deletes the task,mailbox and semaphore used by the ACD monitor mechanism
 *
 *  @param   none
 *  @retval  none
 *
 */
void EIPACD_stop(void);
/**
 *  @brief  Function to store the Conflict details to non volatile memory
 *
 *          This Function needs to be called by the ACD Stack whenever an ACD conflict occurs
 *          The Conflict details obtained from stack is stored to Non volatile memory (SPI Flash)
 *
 *
 *  @param   stAcdConflictDetected  [in]  Structure containing ACD Conflict details
 *  @retval  Success  -   0
 *           Error   -   <0
 */
int EIPACD_conflictDetected(ST_ACD_CONFLICT_DETECTED stAcdConflictDetected);
/**
 *  @brief  Function to send out the ARP packet framed by ACD Stack
 *
 *          This Function needs to be called by the ACD Stack to send out ARP frames.This function also count the ARP frames
 *          which is used for Gratitous ARP blocking workaorund
 *
 *
 *  @param   wFrameSize  [in]  size of the frame to be sent
 *  @param   byFrame     [in]  ARP packet pointer
 *  @retval  Success  -   0
 *           Error   -   <0
 */
int EIPACD_sendArpFrame(unsigned short wFrameSize, unsigned char *byFrame);
/**
 *  @brief  Function to send out the unicast ARP packet framed by ACD Stack
 *
 *          This Function needs to be called by the ACD Stack to send out ARP frames.This function also count the ARP frames
 *          which is used for Gratitous ARP blocking workaorund
 *
 *
 *  @param   wFrameSize  [in]  size of the frame to be sent
 *  @param   byFrame     [in]  ARP packet pointer
 *  @retval  Success  -   0
 *           Error   -   <0
 */
int EIPACD_sendArpFrameUnicast(EIP_WORD wFrameSize, EIP_BYTE *byFrame);
/**
 *  @brief  Function to be called by the ACD stack whenever an event occurs
 *
 *          The ACD stack informs the application about the ACD events using this Function.
 *
 *
 *  @param   stAcdConflictDetected  [in]  Structure containing ACD Conflict details
 *  @retval  Success  -   0
 *           Error   -   <0
 */
int EIPACD_event(unsigned long dwEvent);
/**
 *  @brief  Function to register the Link change callback
 *
 *          This function internally calls the EMAC LLD API to register the Link change callback
 *
 *
 *  @param   icssEmacHandle [in] ICSS EMAC handle
 *  @retval  none
 *
 */
void EIPACD_registerLinkCallBack(ICSSEMAC_Handle pruIcssHandle);
/**
 *  @brief  Function to de register the ACD specific Link change callback
 *
 *
 *
 *  @param   icssEmacHandle [in] ICSS EMAC handle
 *  @retval  none
 *
 */
void EIPACD_deregisterLinkCallBack(ICSSEMAC_Handle pruIcssHandle);
/**
 *  @brief  Function to get the current ACD status.
 *
 *          Used by the LED Task to know whether Conflict has occured or not
 *
 *  @param   none
 *  @retval  1 (ACD_LED_CONFLICT) - In case conflict occured
 *           0  No conflict occured
 */
EIP_WORD EIPACD_getACDLEDStat();

void EIPNDK_getTCPIPConfigDetails(ST_TCPIP_CONFIGURATION *tcpIPDetails);

#endif
