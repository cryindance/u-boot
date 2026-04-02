#include "mailbox.h"
#include <string.h>
#include <linux/delay.h>

uint8_t CheckMailboxInNotFullStatus(MAILBOX_TypeDef* pMailbox, uint8_t mbx_num, uint32_t timeout)
{
    uint32_t cnt = 0;
    while ((MAILBOX_GetMbxInFullStatus(pMailbox, mbx_num) == SET) && (cnt++ < timeout));
    if (cnt >= timeout) {
        return 0;
    }
    return 1;
}

uint8_t CheckMailboxOutFullStatus(MAILBOX_TypeDef* pMailbox, uint8_t mbx_num, uint32_t timeout)
{
    uint32_t cnt = 0;
    while ((MAILBOX_GetMbxOutFullStatus(pMailbox, mbx_num) == RESET) && (cnt++ < timeout));
    if (cnt >= timeout) {
        return 0;
    }
    return 1;
}

uint8_t CheckMailboxOutFreeStatus(MAILBOX_TypeDef* pMailbox, uint8_t mbx_num, uint32_t timeout)
{
    uint32_t cnt = 0;
    while ((MAILBOX_GetMbxOutFullStatus(pMailbox, mbx_num) == SET) && (cnt++ < timeout));
    if (cnt >= timeout) {
        return 0;
    }
    return 1;
}

/**
  * \brief  Initialize the cryp in token command to accomplish the symmetric encryption or decryption
  *
  * \param  cmd_t point to mailbox_cryp_cmd_in_token struct
  * \param  iv point to IV buffer
  * \param  key point to KEY buffer
  * \param  keySel 0, using soft configuration key, 1~6 using efuse group0~5 key
  * \param  encrypt encrypt or decrypt
  *     \arg SECURE_SERVICE_CRYP_ENCRYPT: encrypt
  *     \arg SECURE_SERVICE_CRYP_DECRYPT: decrypt
  * \param  nonceLen default 0 
  * \param  keyLen key size 
  *     \arg SECURE_SERVICE_CRYP_KEY_128BITS: 128bit
  *     \arg SECURE_SERVICE_CRYP_KEY_192BITS: 192bit
  *     \arg SECURE_SERVICE_CRYP_KEY_256BITS: 256bit
  * \param  mode algorithm mode select 
  *     \arg SECURE_SERVICE_CRYP_ECB: ECB
  *     \arg SECURE_SERVICE_CRYP_CBC: CBC
  *     \arg SECURE_SERVICE_CRYP_CTR: CTR
  * \param  algo algorithm select 
  *     \arg SECURE_SERVICE_CRYP_AES: AES
  *     \arg SECURE_SERVICE_CRYP_SM4: SM4
  *     \arg SECURE_SERVICE_CRYP_CTR: CTR
  * \param  dataLen total input plaintext or ciphertext byte length, default equal to inLen 
  * \param  inAddrLow the lower 32bit start address storing the input plaintext or ciphertext 
  * \param  inAddrHi the high 32bit start address storing the input plaintext or ciphertext
  * \param  inLen input plaintext or ciphertext byte length
  * \param  outAddrLow the lower 32bit start address storing the output ciphertext or plaintext 
  * \param  outAddrHi the high 32bit start address storing the output ciphertext or plaintext 
  */
void mailbox_cryp_in_token_set(mailbox_cryp_cmd_in_token *cmd_t, 
                               uint8_t *iv, 
                               uint8_t *key, 
                               uint8_t keySel, uint8_t encrypt, uint8_t nonceLen, uint8_t keyLen, uint8_t mode, uint8_t algo,
                               uint32_t update_mode, uint32_t inAddrLow, uint32_t inAddrHi, uint32_t inLen,
                               uint32_t outAddrLow, uint32_t outAddrHi)
{
    cmd_t->cryp.header.opcode = SECURE_SERVICE_OPCODE_CRYP;
    cmd_t->cryp.identity = (SECURE_SERVICE_OPCODE_CRYP << 28) | (algo << 20) | (mode << 16) | (keySel << 8) | (keyLen << 4) | (encrypt);
    cmd_t->cryp.length = inLen;
    cmd_t->cryp.inputdata_addr_low = inAddrLow;
    cmd_t->cryp.inputdata_addr_hig = inAddrHi;
    cmd_t->cryp.inputdata_length = inLen;
    cmd_t->cryp.outputdata_addr_low = outAddrLow;
    cmd_t->cryp.outputdata_addr_hig = outAddrHi;
    cmd_t->cryp.outputdata_length = inLen;

    cmd_t->cryp.cmd_cfg.encryp = encrypt;
    cmd_t->cryp.cmd_cfg.algo = algo;
    cmd_t->cryp.cmd_cfg.mode = mode;
    cmd_t->cryp.cmd_cfg.NonceLength = nonceLen;
    cmd_t->cryp.cmd_cfg.key_sel = keySel;
    cmd_t->cryp.cmd_cfg.key_length = keyLen;
    cmd_t->cryp.cmd_cfg.in_ctrl = update_mode;

    if ((mode == SECURE_SERVICE_CRYP_CBC) || (mode == SECURE_SERVICE_CRYP_CTR)) {
        memcpy( ADDR8P(cmd_t->iv), ADDR8P(iv), 16);
    }
    if (keySel == 0) {
        memcpy( ADDR8P(cmd_t->key), ADDR8P(key), 16 + (keyLen << 3));
    }
}

/**
  * \brief  Initialize the acryp in token command to accomplish the signature verify algorithm 
  *
  * \param  cmd_t point to acryp_in_token_t struct
  * \param  mode algorithm mode select, now only support verify,default is SECURE_SERVICE_ACRYP_VERIFY 
  *     \arg SECURE_SERVICE_ACRYP_SIGNATURE: signature
  *     \arg SECURE_SERVICE_ACRYP_VERIFY: verify
  * \param  algo algorithm select 
  *     \arg SECURE_SERVICE_ACRYP_ED25519: ED25519
  *     \arg SECURE_SERVICE_ACRYP_SM2: SM2
  *     \arg SECURE_SERVICE_ACRYP_RSA2048: CTR
  *     \arg SECURE_SERVICE_ACRYP_RSA4096: CTR
  * \param  dataLen total input message byte length, default equal to inLen 
  * \param  inAddrLow the lower 32bit start address storing the input message 
  * \param  inAddrHi the high 32bit start address storing the input message
  * \param  inLen input message byte length
  * \param  signAddrLow the lower 32bit start address storing the signature 
  * \param  signAddrHi the high 32bit start address storing the signature 
  * \param  pubKeyAddrLow the lower 32bit start address storing the public key 
  * \param  pubKeyAddrHi the high 32bit start address storing the public key
  */
void mailbox_acryp_in_token_set(acryp_in_token_t *cmd_t,
                                uint8_t mode, uint8_t algo,
                                uint32_t dataLen, 
                                uint32_t inAddrLow, uint32_t inAddrHi, uint32_t inLen, 
                                uint32_t signAddrLow, uint32_t signAddrHi, 
                                uint32_t pubKeyAddrLow, uint32_t pubKeyAddrHi)
{
    cmd_t->header.opcode = SECURE_SERVICE_OPCODE_ACRYP;
    cmd_t->identity = (SECURE_SERVICE_OPCODE_ACRYP << 28) | (algo << 16) | (mode);
    cmd_t->length = dataLen;
    cmd_t->input_data_addr_low = inAddrLow;
    cmd_t->input_data_addr_hig = inAddrHi;
    cmd_t->input_data_length = inLen;
    cmd_t->input_signdata_addr_low = signAddrLow;
    cmd_t->input_signdata_addr_hig = signAddrHi;
    cmd_t->input_publickey_addr_low = pubKeyAddrLow;
    cmd_t->input_PublicKey_addr_hig = pubKeyAddrHi;

    cmd_t->cmd_cfg.algo = algo;
    cmd_t->cmd_cfg.mode = mode;
}

/**
  * \brief  Initialize the hash in token command to accomplish the hash digest computation 
  *
  * \param  cmd_t point to mailbox_hash_cmd_in_token struct
  * \param  mode algorithm mode select, now only support hash 
  *     \arg SECURE_SERVICE_HASH_MODE: hash
  *     \arg SECURE_SERVICE_HMAC_MODE: hmac
  * \param  algo algorithm select 
  *     \arg SECURE_SERVICE_HASH_SHA1: HASH SHA1
  *     \arg SECURE_SERVICE_HASH_SHA224: HASH SHA224
  *     \arg SECURE_SERVICE_HASH_SHA256: HASH SHA256
  *     \arg SECURE_SERVICE_HASH_SHA384: HASH SHA384
  *     \arg SECURE_SERVICE_HASH_SHA512: HASH SHA512
  *     \arg SECURE_SERVICE_HASH_SM3: HASH SM3
  * \param  dataLen total input message byte length, default equal to inLen 
  * \param  inAddrLow the lower 32bit start address storing the input message 
  * \param  inAddrHi the high 32bit start address storing the input message
  * \param  inLen input message byte length
  * \param  digest point to digest buffer storing the intermediate-result to continue computation if need multiple times computation
  */
void mailbox_hash_in_token_set(mailbox_hash_cmd_in_token *cmd_t,
                               uint8_t mode, uint8_t algo,
                               uint32_t update_mode, 
                               uint32_t inAddrLow, uint32_t inAddrHi, uint32_t inLen)
{
    cmd_t->hash.header.opcode = SECURE_SERVICE_OPCODE_HASH;
    cmd_t->hash.identity = (SECURE_SERVICE_OPCODE_HASH << 28) | (algo << 16) | (mode);
    cmd_t->hash.length = inLen;
    cmd_t->hash.inputdata_addr_low = inAddrLow;
    cmd_t->hash.inputdata_addr_hig = inAddrHi;
    cmd_t->hash.inputdata_length = inLen;

    cmd_t->hash.cmd_cfg.algo = algo;
    cmd_t->hash.cmd_cfg.mode = mode;
    cmd_t->hash.cmd_cfg.in_ctrl = update_mode;
    cmd_t->hash.key_data_addr_low = *(uint32_t *)(&cmd_t->hash.cmd_cfg);
}

/**
  * \brief  Get the avaliable linked mailbox number.
  *
  * \retval  -1 has no avaliable linked mailbox
  * \retval others the avaliable linked mailbox number.
  */
int8_t mailbox_avaliable_linked_num(void)
{
    int8_t mailbox_num = -1;
    /* Get mailbox number that is avaliable to link */
    mailbox_num = MAILBOX_GetAlbToLinkMbxNum(CORE0_PPI_RAM);
    if (-1 == mailbox_num) {
        debug(">>>>>>>>>>host has no unlinked mailbox!<<<<<<<<<<\r\n");
        return -1;
    }
    return mailbox_num;
}

/**
  * \brief  Send secure service command using the specified mailbox.
  *
  * \param  data point to send buf, according the opcode to chose
  *     \arg the address of acryp_in_token_t struct variable 
  *     \arg the address of mailbox_cryp_cmd_in_token struct variable
  *     \arg the address of boot_in_token_t struct variable
  *     \arg the address of mailbox_hash_cmd_in_token struct variable
  *     \arg the address of efuse_in_token_t struct variable
  * \param  opcode select the secure service items
  *     \arg SECURE_SERVICE_OPCODE_HASH: hash
  *     \arg SECURE_SERVICE_OPCODE_CRYP: cryp
  *     \arg SECURE_SERVICE_OPCODE_ACRYP: acryp
  *     \arg SECURE_SERVICE_OPCODE_EFUSE: efuse
  *     \arg SECURE_SERVICE_OPCODE_BOOT: boot firmware
  * \param  mailbox_num mailbox number
  */
void mailbox_secure_service_host_send(uint32_t *data, uint8_t opcode, uint8_t mailbox_num)
{
    uint32_t wBuf[60] = {0};
    uint32_t mailbox_addr = 0;

    mailbox_addr = (CORE0_PPI_RAM_BASE + MAILBOX_BASE_ADDR_OFFSET + mailbox_num*MAILBOX_SIZE_IN_BYTE);

    /* Link to mailbox */
    if (ERROR == MAILBOX_HostLinkToMbx(CORE0_PPI_RAM, mailbox_num)) {
        debug(">>>>>>>>>>host link to mailbox fail!<<<<<<<<<<\r\n");
        return;
    }

    //printf("mailbox_num:%d,mailbox_addr:0x%x,wBuf:0x%lx\n",mailbox_num,mailbox_addr,&wBuf);

    switch (opcode) {
        case SECURE_SERVICE_OPCODE_HASH:
            memcpy( ADDR8P(wBuf), ADDR8P(data), sizeof(mailbox_hash_cmd_in_token));
            break;
        case SECURE_SERVICE_OPCODE_CRYP:
            memcpy( ADDR8P(wBuf), ADDR8P(data), sizeof(mailbox_cryp_cmd_in_token));
            break;
        case SECURE_SERVICE_OPCODE_ACRYP:
            memcpy( ADDR8P(wBuf), ADDR8P(data), sizeof(acryp_in_token_t));         
            break;
        case SECURE_SERVICE_OPCODE_EFUSE:
            memcpy( ADDR8P(wBuf), ADDR8P(data), sizeof(efuse_in_token_t));
            break;
        default:
            break;
    }

    /* Write command to mailbox */
    MAILBOX_HostWriteDataToMailboxIn(CORE0_PPI_RAM, mailbox_num, ADDR32P(mailbox_addr), wBuf, 60);
    if (0 == CheckMailboxInNotFullStatus(CORE0_PPI_RAM, mailbox_num, TIMEOUT_CYCLE)) {
        debug(">>>>>>>>>>wait kernel to read command out timeout!<<<<<<<<<<\r\n");
        return;
    }
    debug("Host write done!\r\n");
}

/**
  * \brief  Receive secure service back-command using the specified mailbox.
  *
  * \param  data point to receive buf, the detail reference to communication protocol
  * \param  mailbox_num mailbox number
  */
void mailbox_secure_service_host_receive(uint32_t *data, int8_t mailbox_num)
{
    uint32_t mailbox_addr = 0;

    mailbox_addr = (CORE0_PPI_RAM_BASE + MAILBOX_BASE_ADDR_OFFSET + mailbox_num*MAILBOX_SIZE_IN_BYTE);

    /* host read from mailbox */
    if (0 == CheckMailboxOutFullStatus(CORE0_PPI_RAM, mailbox_num, TIMEOUT_CYCLE)) {
        debug(">>>>>>>>>>wait kernel to write command timeout!<<<<<<<<<<\r\n");
        return;
    }
    MAILBOX_HostReadDataFromMailboxOut(CORE0_PPI_RAM, mailbox_num, ADDR32P(mailbox_addr), data, 32);
    if (0 == CheckMailboxOutFreeStatus(CORE0_PPI_RAM, mailbox_num, TIMEOUT_CYCLE)) {
        debug(">>>>>>>>>>host read command fail!<<<<<<<<<<\r\n");
        return;
    }
    /* unlink mailbox */
    MAILBOX_HostUnlinkMbx(CORE0_PPI_RAM, mailbox_num);
    while (MAILBOX_GetMbxLinkStatus(CORE0_PPI_RAM, mailbox_num) == SET); 
}
