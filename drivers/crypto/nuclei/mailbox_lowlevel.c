/* Includes ------------------------------------------------------------------*/
#include "mailbox_lowlevel.h"
#include <string.h>
#include <linux/delay.h>

/**
  * \brief  Host link to specified mailbox.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  * \param  mbx_num the number of mailbox, range from 0 to (MAILBOX_AVALIABLE_MAX_NUM - 1)
  *
  * \retval SUCCESS successfully linked 
  * \retval ERROR faild to link
  */
ErrStatus MAILBOX_HostLinkToMbx(MAILBOX_TypeDef* mailbox, uint8_t mbx_num)
{
    uint8_t host_id = 0, master_id = 0;

    master_id = MAILBOX_GetMasterId(mailbox);
    host_id = MAILBOX_GetHostId(mailbox);

    if (master_id == host_id) {
        /* unlock, ensure has right to access */
        MAILBOX_LockOutCtrl(mailbox, host_id, mbx_num, DISABLE);
    } else {
        /* check whether has unlocked for accessing the specified mailbox */
        if (MAILBOX_GetUnlockStatus(mailbox, host_id, mbx_num) == RESET) {
            return ERROR;
        }
    }
    
    /* Check the specified mailbox whether is avaliable to link */
    if (RESET == MAILBOX_GetMbxUnlinkAlbStatus(mailbox, mbx_num)) {      
        return ERROR;
    }

    /* Set to link */
    MAILBOX_MbxLink(mailbox, mbx_num);
    /* Ensure whether has linked */
    if (RESET == MAILBOX_GetMbxLinkStatus(mailbox, mbx_num)) {
        return ERROR;
    } else if (host_id != MAILBOX_GetMbxLinkedId(mailbox, mbx_num)) {
        return ERROR; 
    } else {
        return SUCCESS;
    }  
}

/**
  * \brief  Host writes data to input mailbox.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  * \param  mbx_num the number of mailbox, range from 0 to (MAILBOX_AVALIABLE_MAX_NUM - 1)
  * \param  addr point to the input mailbox start address
  * \param  buf point to the buffer address
  * \param  len the length of word data
  */
void MAILBOX_HostWriteDataToMailboxIn(MAILBOX_TypeDef* mailbox, uint8_t mbx_num, uint32_t *addr, uint32_t *buf, uint8_t len)
{
    uint8_t i = 0;
    //printf("addr:%p,buf:%p\n",addr,buf);
    //delay_u(200);
    //memcpy( addr, buf, len);
    
#if 1
    /* Write data to input mailbox*/
    for (i = 0; i < len; i++) {
        addr[i] = buf[i];  
        //mdelay(1);
         //printf("i=%d\n",i);
    }
#endif
	asm volatile ("fence");
    //printf("buf to addr finish.\n");
    /* Write 1 to set status */
    MAILBOX_SetMbxInFull(mailbox, mbx_num);
    //printf("MAILBOX_SetMbxInFull finish.\n");
	asm volatile ("fence");
    //printf("fence finish.\n");
}

/**
  * \brief  Host read data from output mailbox.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  * \param  mbx_num the number of mailbox, range from 0 to (MAILBOX_AVALIABLE_MAX_NUM - 1)
  * \param  addr point to the output mailbox start address
  * \param  buf point to the buffer address
  * \param  len the length of word data
  */
void MAILBOX_HostReadDataFromMailboxOut(MAILBOX_TypeDef* mailbox, uint8_t mbx_num, uint32_t *addr, uint32_t *buf, uint8_t len)
{
    uint8_t i = 0; 

    /* Write data to input mailbox*/
    for (i = 0; i < len; i++) {
        buf[i] = addr[i];    
    }
    /* Write 1 to clear */
    MAILBOX_SetMbxOutFull(mailbox, mbx_num);
}


/**
  * \brief  Return the status about which host is blocked from accessing specified mailbox.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  * \param  host_id host id
  * \param  mbx_num the number of mailbox, range from 0 to (MAILBOX_AVALIABLE_MAX_NUM - 1)
  * \param  NewState lock or unlock the mailbox for host.
  *
  * \retval SET the host has right to access the specified mailbox
  * \retval RESET the host has not right to access the specified mailbox
  */
FlagStatus MAILBOX_GetUnlockStatus(MAILBOX_TypeDef* mailbox, uint8_t host_id, uint8_t mbx_num)
{
    uint32_t temp = 0;
    temp = MAILBOX_LOCKOUT_REG(mailbox, mbx_num);
    if (temp & (BIT(0) << host_id)) {
        return RESET;    
    } else {
        return SET; 
    }
}

/**
  * \brief  Enable or disable which host is blocked from accessing specified mailbox.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  * \param  host_id host id
  * \param  mbx_num the number of mailbox, range from 0 to (MAILBOX_AVALIABLE_MAX_NUM - 1)
  * \param  NewState lock or unlock the mailbox for host.
  *           \arg ENABLE: The host is blocked from accessing specified mailbox.
  *           \arg DISABLE: The host can link to the specified mailbox.
  */
void MAILBOX_LockOutCtrl(MAILBOX_TypeDef* mailbox, uint8_t host_id, uint8_t mbx_num, FunctionalState NewState)
{
    uint32_t temp = 0;
    temp = MAILBOX_LOCKOUT_REG(mailbox, mbx_num);
    if (NewState != DISABLE) {
        temp |= (BIT(0) << host_id);    
    } else {
        temp &= ~(BIT(0) << host_id);
    }
    switch (mbx_num) {
        case 0:
            mailbox->LOCKOUT0 = temp;
            break;
        default :
            break;
    }
}

/**
  * \brief  To link the specified mailbox.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  * \param  mbx_num the number of mailbox, range from 0 to (MAILBOX_AVALIABLE_MAX_NUM - 1)
  */
void MAILBOX_MbxLink(MAILBOX_TypeDef* mailbox, uint8_t mbx_num)
{
    mailbox->CSR |= (MAILBOX_RF_CSR_MBX0_LINK << (4 * mbx_num));
}

/**
  * \brief  Checks whether the specified mailbox has linked.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  * \param  mbx_num the number of mailbox, range from 0 to (MAILBOX_AVALIABLE_MAX_NUM - 1)
  *
  * \retval SET The specified mailbox has linked
  * \retval RESET The specified mailbox has not linked
  */
FlagStatus MAILBOX_GetMbxLinkStatus(MAILBOX_TypeDef* mailbox, uint8_t mbx_num)
{
    FlagStatus bitstatus = RESET;

    if ((mailbox->CSR & (MAILBOX_RF_CSR_MBX0_LINK << (4 * mbx_num))) != RESET) {
        /* mailbox is linked */
        bitstatus = SET;
    } else {
        /* mailbox is unlinked */
        bitstatus = RESET;
    }

    /* Return the mailbox link status */
    return bitstatus;
}

/**
  * \brief  Return the number of mailbox that is linked to link 
  *
  * \param  mailbox the struct of MAILBOX peripheral
  *
  * \return mailbox number that is avaliable to link
  */
uint8_t MAILBOX_GetLinkedMbxNum(MAILBOX_TypeDef* mailbox)
{
    for (int i = 0; i < MAILBOX_AVALIABLE_MAX_NUM; i++) {
        if (MAILBOX_GetMbxLinkStatus(mailbox, i) == SET) {
            return i;
        }
    }

    return -1;
}

/**
  * \brief  To unlink the specified mailbox.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  * \param  mbx_num the number of mailbox, range from 0 to (MAILBOX_AVALIABLE_MAX_NUM - 1)
  */
void MAILBOX_HostUnlinkMbx(MAILBOX_TypeDef* mailbox, uint8_t mbx_num)
{
    mailbox->CSR |= (MAILBOX_RF_CSR_MBX0_UNLINK << (4 * mbx_num));
}

/**
  * \brief  Checks whether the specified mailbox is avaliable to link.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  * \param  mbx_num the number of mailbox, range from 0 to (MAILBOX_AVALIABLE_MAX_NUM - 1)
  *
  * \retval SET The specified mailbox can link to
  * \retval RESET The specified mailbox can not link to
  */
FlagStatus MAILBOX_GetMbxUnlinkAlbStatus(MAILBOX_TypeDef* mailbox, uint8_t mbx_num)
{
    FlagStatus bitstatus = RESET;

    if ((mailbox->CSR & (MAILBOX_RF_CSR_MBX0_UNLINK_AVAILABLE << (4 * mbx_num))) != RESET) {
        /* mailbox is avaliable to link */
        bitstatus = SET;
    } else {
        /* mailbox is not avaliable to link */
        bitstatus = RESET;
    }

    /* Return the mailbox available to link status */
    return bitstatus;
}

/**
  * \brief  Return the number of mailbox that is avaliable to link 
  *
  * \param  mailbox the struct of MAILBOX peripheral
  *
  * \return mailbox number that is avaliable to link
  */
uint8_t MAILBOX_GetAlbToLinkMbxNum(MAILBOX_TypeDef* mailbox)
{
    for (int i = 0; i < MAILBOX_AVALIABLE_MAX_NUM; i++) {
        if (MAILBOX_GetMbxUnlinkAlbStatus(mailbox, i) == SET) {
            return i;
        }
    }

    return -1;
}

/**
  * \brief  To unlink the specified mailbox.
  *
  * \note   Only master host can unlink the mailbox.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  * \param  mbx_num the number of mailbox, range from 0 to (MAILBOX_AVALIABLE_MAX_NUM - 1)
  */
void MAILBOX_MastHostUnlinkMbx(MAILBOX_TypeDef* mailbox, uint8_t mbx_num)
{
    mailbox->RESET |= (MAILBOX_RF_RESET_MBX0_UNLINK << (4 * mbx_num));
}

/**
  * \brief  Set mbx_in_full bit in CSR register to 1.
  *
  * \note   After finish to write to mailbox in, means setting full status, or finish to read means clearing full status.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  * \param  mbx_num the number of mailbox, range from 0 to (MAILBOX_AVALIABLE_MAX_NUM - 1)
  */
void MAILBOX_SetMbxInFull(MAILBOX_TypeDef* mailbox, uint8_t mbx_num)
{
    mailbox->CSR |= (MAILBOX_RF_CSR_MBX0_IN_FULL << (4 * mbx_num));
}

/**
  * \brief  Set mbx_out_full bit in CSR register to 1.
  *
  * \note   After finish to write to mailbox out, means setting full status, or finish to read means clearing full status.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  * \param  mbx_num the number of mailbox, range from 0 to (MAILBOX_AVALIABLE_MAX_NUM - 1)
  */
void MAILBOX_SetMbxOutFull(MAILBOX_TypeDef* mailbox, uint8_t mbx_num)
{
    mailbox->CSR |= (MAILBOX_RF_CSR_MBX0_OUT_FULL << (4 * mbx_num));
}

/**
  * \brief  Checks whether the specified input mailbox is full.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  * \param  mbx_num the number of mailbox, range from 0 to (MAILBOX_AVALIABLE_MAX_NUM - 1)
  *
  * \retval SET The specified input mailbox is full
  * \retval RESET The specified input mailbox is not full
  */
FlagStatus MAILBOX_GetMbxInFullStatus(MAILBOX_TypeDef* mailbox, uint8_t mbx_num)
{
    FlagStatus bitstatus = RESET;

    if ((mailbox->CSR & (MAILBOX_RF_CSR_MBX0_IN_FULL << (4 * mbx_num))) != RESET) {
        /* input mailbox is full */
        bitstatus = SET;
    } else {
        /* input mailbox is not full */
        bitstatus = RESET;
    }

    /* Return the input mailbox full status */
    return bitstatus;
}

/**
  * \brief  Clear output mailbox full status
  *
  * \note   Only master host can clear it.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  * \param  mbx_num the number of mailbox, range from 0 to (MAILBOX_AVALIABLE_MAX_NUM - 1)
  */
void MAILBOX_MasterHostClearMbxOutFull(MAILBOX_TypeDef* mailbox, uint8_t mbx_num)
{
    mailbox->RESET |= (MAILBOX_RF_RESET_MBX0_OUT_EMPTY << (4 * mbx_num));
}

/**
  * \brief  Checks whether the specified output mailbox is full.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  * \param  mbx_num the number of mailbox, range from 0 to (MAILBOX_AVALIABLE_MAX_NUM - 1)
  *
  * \retval SET The specified output mailbox is full
  * \retval RESET The specified output mailbox is not full
  */
FlagStatus MAILBOX_GetMbxOutFullStatus(MAILBOX_TypeDef* mailbox, uint8_t mbx_num)
{
    FlagStatus bitstatus = RESET;

    if ((mailbox->CSR & (MAILBOX_RF_CSR_MBX0_OUT_FULL << (4 * mbx_num))) != RESET) {
        /* output mailbox is full */
        bitstatus = SET;
    } else {
        /* output mailbox is not full */
        bitstatus = RESET;
    }

    /* Return the output mailbox full status */
    return bitstatus;
}

/**
  * \brief  Read the host id that links to the specified mailbox.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  * \param  mbx_num the number of mailbox, range from 0 to (MAILBOX_AVALIABLE_MAX_NUM - 1)
  *
  * \return Host ID, range from 0 to (1-1)
  */
uint8_t MAILBOX_GetMbxLinkedId(MAILBOX_TypeDef* mailbox, uint8_t mbx_num)
{
    uint32_t temp = 0;
    uint32_t offset = 0;

    offset = (8 * (mbx_num % 4));    
    temp = MAILBOX_RF_LINKID0_MBX0_LINK_ID << offset;  
    temp &= MAILBOX_LINKID_REG(mailbox, mbx_num/4);
    /* Return the host ID */
    return (temp >> offset);
}

/**
  * \brief  Read host ID.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  *
  * \return Host ID, range from 0 to (1-1)
  */
uint8_t MAILBOX_GetHostId(MAILBOX_TypeDef* mailbox)
{
    uint32_t temp = 0;
    
    temp = mailbox->OPT & MAILBOX_RF_OPT_MY_ID;
    /* Return the host ID */
    return (temp >> MAILBOX_RF_OPT_MY_ID_OFS);
}

/**
  * \brief  Return the number of actived host.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  *
  * \return the number of actived host
  */
uint8_t MAILBOX_GetActivedHostNum(MAILBOX_TypeDef* mailbox)
{
    uint32_t temp = 0;
    
    temp = mailbox->OPT & MAILBOX_RF_OPT_ACTIVE_HOST_N;
    /* Return the number of actived host */
    return (temp >> MAILBOX_RF_OPT_ACTIVE_HOST_N_OFS);
}

/**
  * \brief  Read master ID.
  *
  * \param  mailbox the struct of MAILBOX peripheral
  *
  * \return master ID, range from 1~Max master number
  */
uint8_t MAILBOX_GetMasterId(MAILBOX_TypeDef* mailbox)
{
    uint32_t temp = 0;
    
    temp = mailbox->OPT & MAILBOX_RF_OPT_MASTER_ID;
    /* Return the master ID */
    return (temp >> MAILBOX_RF_OPT_MASTER_ID_OFS);
}

/**
  * \brief  Get MAILBOX RTL version
  *
  * \param  mailbox the struct of MAILBOX peripheral
  *
  * \return The RTL version of the MAILBOX
  */
uint32_t MAILBOX_GetIpVersion(MAILBOX_TypeDef* mailbox)
{
    return mailbox->IP_VER;
}

/**
  * \brief  Get MAILBOX git version
  *
  * \param  mailbox the struct of MAILBOX peripheral
  *
  * \return The git version of the MAILBOX
  */
uint32_t MAILBOX_GetGitVersion(MAILBOX_TypeDef* mailbox)
{
    return mailbox->GIT_VER;
}
