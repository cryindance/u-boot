#ifndef __MAILBOX_H
#define __MAILBOX_H

#include "mailbox_lowlevel.h"

#define CORE0_PPI_RAM_BASE                        (0x8800000UL)

#define CORE0_PPI_RAM                             ((MAILBOX_TypeDef *) CORE0_PPI_RAM_BASE)		

#define TIMEOUT_CYCLE           0x1fffffff

/* Macro define about Communication Protocal */
// opcode
#define SECURE_SERVICE_OPCODE_HASH                          1 
#define SECURE_SERVICE_OPCODE_CRYP                          2
#define SECURE_SERVICE_OPCODE_ACRYP                         3 
#define SECURE_SERVICE_OPCODE_TRNG                          4
#define SECURE_SERVICE_OPCODE_EFUSE                         5
#define SECURE_SERVICE_OPCODE_BOOT                          6
// hash
#define SECURE_SERVICE_HASH_SHA1                            1
#define SECURE_SERVICE_HASH_MD5                             2
#define SECURE_SERVICE_HASH_SHA224                          3
#define SECURE_SERVICE_HASH_SHA256                          4
#define SECURE_SERVICE_HASH_SHA512                          5
#define SECURE_SERVICE_HASH_SM3                             6
#define SECURE_SERVICE_HASH_SHA384                          7
#define HASH_TABLE_MAX                                      8
#define SECURE_SERVICE_HASH_DIGEST_OFFSET                   8
#define SECURE_SERVICE_HASH_KEY_OFFSET                      24
/* hash mode */
#define SECURE_SERVICE_HASH_MODE                            0
#define SECURE_SERVICE_HMAC_MODE                            1
// cryp
#define SECURE_SERVICE_CRYP_KEY_SEL_CFG                     0
#define SECURE_SERVICE_CRYP_KEY_SEL_GRP0                    1
#define SECURE_SERVICE_CRYP_KEY_SEL_GRP1                    2
#define SECURE_SERVICE_CRYP_KEY_SEL_GRP2                    3
#define SECURE_SERVICE_CRYP_KEY_SEL_GRP3                    4
#define SECURE_SERVICE_CRYP_KEY_SEL_GRP4                    5
#define SECURE_SERVICE_CRYP_KEY_SEL_GRP5                    6
#define SECURE_SERVICE_CRYP_ENCRYPT                         0
#define SECURE_SERVICE_CRYP_DECRYPT                         1
#define SECURE_SERVICE_CRYP_NONCELEN_7                      0
#define SECURE_SERVICE_CRYP_NONCELEN_8                      1
#define SECURE_SERVICE_CRYP_NONCELEN_9                      2
#define SECURE_SERVICE_CRYP_NONCELEN_10                     3
#define SECURE_SERVICE_CRYP_NONCELEN_11                     4
#define SECURE_SERVICE_CRYP_NONCELEN_12                     5
#define SECURE_SERVICE_CRYP_NONCELEN_13                     6
#define SECURE_SERVICE_CRYP_KEY_128BITS                     0
#define SECURE_SERVICE_CRYP_KEY_192BITS                     1
#define SECURE_SERVICE_CRYP_KEY_256BITS                     2
#define SECURE_SERVICE_CRYP_ECB                             0
#define SECURE_SERVICE_CRYP_CBC                             1
#define SECURE_SERVICE_CRYP_CTR                             2

#define SECURE_SERVICE_CRYP_AES                             0
#define SECURE_SERVICE_CRYP_SM4                             1

#define CRYP_TABLE_MAX                                      2

#define SECURE_SERVICE_CRYP_IV_OFFSET                       10
#define SECURE_SERVICE_CRYP_KEY_OFFSET                      14

// acryp 
#define SECURE_SERVICE_ACRYP_ED25519                         1 
#define SECURE_SERVICE_ACRYP_SM2                             2
#define SECURE_SERVICE_ACRYP_RSA2048                         3
#define SECURE_SERVICE_ACRYP_RSA3072                         4
#define SECURE_SERVICE_ACRYP_RSA4096                         5
#define SECURE_SERVICE_ACRYP_MOD_EXP                         6
#define ACRYP_SUPPORT_SIGN_ALGO_MAX_NUM                      6
#define SECURE_SERVICE_ACRYP_VERIFY                          0
#define SECURE_SERVICE_ACRYP_SIGNATURE                       1
#define SECURE_SERVICE_ACRYP_MOD_EXP_RSA2048                 0
#define SECURE_SERVICE_ACRYP_MOD_EXP_RSA4096                 1

#define SECURE_SERVICE_IN_ALL                                0
#define SECURE_SERVICE_IN_INIT                               1
#define SECURE_SERVICE_IN_UPDATE                             2
#define SECURE_SERVICE_IN_END                                3

struct common_head_t
{
    uint32_t TokenID:16;
    uint32_t reserved:8;
    uint32_t subcode:4;
    uint32_t opcode :4;
};

struct cmd_cfg_t
{
    uint32_t algo :4;
    uint32_t mode:4;
    uint32_t w_r:1;
    uint32_t keyLen:8;
    uint32_t in_ctrl :4;
    uint32_t reserved:11;
};

struct cryp_cmd_cfg{
    uint32_t algo :4;
    uint32_t mode:4;
    uint32_t key_length:4;
    uint32_t NonceLength:4;
    uint32_t encryp:1;
    uint32_t key_sel:4;
    uint32_t in_ctrl :4;
    uint32_t reserved :7;
};

typedef struct {
    struct common_head_t header;
    uint32_t identity;
    uint32_t length;
    uint32_t inputdata_addr_low;
    uint32_t inputdata_addr_hig;
    uint32_t inputdata_length;
    uint32_t outputdata_addr_low;
    uint32_t outputdata_addr_hig;
    uint32_t outputdata_length;
    struct cryp_cmd_cfg cmd_cfg;
} cryp_in_token_t;

typedef struct {
    struct common_head_t header;
    uint32_t identity;
    uint32_t length;
    uint32_t input_data_addr_low;
    uint32_t input_data_addr_hig;
    uint32_t input_data_length;
    uint32_t input_signdata_addr_low;
    uint32_t input_signdata_addr_hig;
    uint32_t input_publickey_addr_low;
    uint32_t input_PublicKey_addr_hig;
    struct cmd_cfg_t cmd_cfg;
} acryp_in_token_t;

typedef struct {
    struct common_head_t header;
    uint32_t identity;
    struct cmd_cfg_t cmd_cfg;
    uint32_t length;
    uint32_t efuse_word_index;
    uint32_t input_data_addr_low;
    uint32_t input_data_addr_hig;
    uint32_t input_data_length;
} efuse_in_token_t;

typedef struct {
    struct common_head_t header;
    uint32_t identity;
    uint32_t length;
    uint32_t inputdata_addr_low;
    uint32_t inputdata_addr_hig;
    uint32_t inputdata_length;
    uint32_t key_data_addr_low;
    uint32_t key_data_addr_hig;
    struct cmd_cfg_t cmd_cfg;
} hash_in_token_t;



typedef struct {
    uint32_t tokenid:16;
    uint32_t reserved:8;
    uint32_t result:5;
    uint32_t resultsrc:2;
    uint32_t error:1;
} common_out_head;

typedef struct {
    cryp_in_token_t cryp;           /* cryp secure service struct */  
    uint32_t iv[4];                 /* IV data buffer */    
    uint32_t key[8];                /* key data buffer */ 
} mailbox_cryp_cmd_in_token;

typedef struct {
    hash_in_token_t hash;           /* hash secure service struct */
    uint32_t rescv;  
    uint32_t digest[16];            /* digest data buffer */
    uint8_t key[128];            /* digest data buffer */
} mailbox_hash_cmd_in_token;

/**
  * \brief  Get the avaliable linked mailbox number.
  *
  * \retval  -1 has no avaliable linked mailbox
  * \retval others the avaliable linked mailbox number.
  */
int8_t mailbox_avaliable_linked_num(void);

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
void mailbox_secure_service_host_send(uint32_t *data, uint8_t opcode, uint8_t mailbox_num);

/**
  * \brief  Receive secure service back-command using the specified mailbox.
  *
  * \param  data point to receive buf, the detail reference to communication protocol
  * \param  mailbox_num mailbox number
  */
void mailbox_secure_service_host_receive(uint32_t *data, int8_t mailbox_num);



void mailbox_hash_in_token_set(mailbox_hash_cmd_in_token *cmd_t,
                               uint8_t mode, uint8_t algo,
                               uint32_t update_mode, 
                               uint32_t inAddrLow, uint32_t inAddrHi, uint32_t inLen);
							   
void mailbox_acryp_in_token_set(acryp_in_token_t *cmd_t,
                                uint8_t mode, uint8_t algo,
                                uint32_t dataLen, 
                                uint32_t inAddrLow, uint32_t inAddrHi, uint32_t inLen, 
                                uint32_t signAddrLow, uint32_t signAddrHi, 
                                uint32_t pubKeyAddrLow, uint32_t pubKeyAddrHi);

void mailbox_cryp_in_token_set(mailbox_cryp_cmd_in_token *cmd_t, 
                               uint8_t *iv, 
                               uint8_t *key, 
                               uint8_t keySel, uint8_t encrypt, uint8_t nonceLen, uint8_t keyLen, uint8_t mode, uint8_t algo,
                               uint32_t update_mode, uint32_t inAddrLow, uint32_t inAddrHi, uint32_t inLen,
                               uint32_t outAddrLow, uint32_t outAddrHi);

#endif
