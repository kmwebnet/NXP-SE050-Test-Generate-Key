#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"


#include "sdkconfig.h" // generated by "make menuconfig"

#include "mbedtls/esp_config.h"
#define MBEDTLS_CONFIG_FILE "mbedtls/esp_config.h"


#define SDA2_PIN GPIO_NUM_18
#define SCL2_PIN GPIO_NUM_19

#define TAG "maintask"

#define I2C_MASTER_ACK 0
#define I2C_MASTER_NACK 1





void i2c_master_init()
{
	i2c_config_t i2c_config = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = SDA2_PIN,
		.scl_io_num = SCL2_PIN,
		.sda_pullup_en = GPIO_PULLUP_DISABLE,
		.scl_pullup_en = GPIO_PULLUP_DISABLE,
		.master.clk_speed = 100000
		};
			
	i2c_param_config(I2C_NUM_0 , &i2c_config);
	i2c_driver_install(I2C_NUM_0 , I2C_MODE_MASTER, 0, 0, 0);


}
/* clang-format off */

#if defined(SSS_USE_FTR_FILE)
#include "fsl_sss_ftr.h"
#else
#include "fsl_sss_ftr_default.h"
#endif




#define mbedtls_time time
#define mbedtls_time_t time_t
#define mbedtls_printf printf
#define mbedtls_fprintf fprintf
#define mbedtls_snprintf snprintf

#include <assert.h>


#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/timing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* +S */

#ifdef TGT_A71CH
#   include "sm_printf.h"
#endif

#if SSS_HAVE_ALT_SSS
#include "sss_mbedtls.h"
#endif

#include <ex_sss.h>
#include <ex_sss_boot.h>
#include <nxLog_App.h>
#include <se05x_APDU.h>

static ex_sss_boot_ctx_t gex_sss_demo_boot_ctx;
ex_sss_boot_ctx_t *pex_sss_demo_boot_ctx = &gex_sss_demo_boot_ctx;
static ex_sss_cloud_ctx_t gex_sss_demo_tls_ctx;
ex_sss_cloud_ctx_t *pex_sss_demo_tls_ctx = &gex_sss_demo_tls_ctx;

#define EX_SSS_BOOT_PCONTEXT (&gex_sss_demo_boot_ctx)

#define EX_SSS_BOOT_DO_ERASE 0
#define EX_SSS_BOOT_EXPOSE_ARGC_ARGV 1

#include <ex_sss_main_inc.h>

#define OBJID_usr_key (EX_SSS_OBJID_CUST_START + 0x10000002u)
#define OBJID_usr_cert (EX_SSS_OBJID_CUST_START + 0x10000003u)

/*The size of the client certificate should be checked when script is used to store it in GP storage and updated here */
#define SIZE_CLIENT_CERTIFICATE 2048

#define SIZE_PUBKEY 300
#define ECC_KEY_BIT_LEN 256

#include "ex_sss_objid.h"

sss_status_t base64encode(
    const uint8_t*  data,         /**< [in] The input byte array that will be converted to base 64 encoded characters */
    size_t          data_size,    /**< [in] The length of the byte array */
    char*           encoded,      /**< [in] The output converted to base 64 encoded characters. */
    size_t*         encoded_size /**< [inout] Input: The size of the encoded buffer, Output: The length of the encoded base 64 character string */
    );



sss_status_t ex_sss_entry(ex_sss_boot_ctx_t *pCtx)
{

    int ret = 0;
    uint8_t pubkey[SIZE_PUBKEY] = {0};

    mbedtls_x509_crt cacert;
    mbedtls_x509_crt clicert;
    mbedtls_x509_crt_init( &cacert );
    mbedtls_x509_crt_init( &clicert );

    uint8_t buf[4000];
    size_t buf_len = sizeof(buf);


    uint32_t keyId = OBJID_usr_key;
    size_t keyBitLen = ECC_KEY_BIT_LEN;
    size_t keyLen = keyBitLen * 8;
    sss_key_part_t keyPart = kSSS_KeyPart_Pair;
    sss_cipher_type_t cipherType = kSSS_CipherType_EC_NIST_P;


        sss_status_t status;
        smStatus_t retStatus = SM_NOT_OK;

    sss_se05x_session_t *pSession = (sss_se05x_session_t *)&pCtx->session;

        /* doc+:initialize-key-objs */

        /* pex_sss_demo_tls_ctx->obj will have the private key handle */
        status = sss_key_object_init(&pex_sss_demo_tls_ctx->obj, &pCtx->ks);
        if (status != kStatus_SSS_Success) {
            printf(" sss_key_object_init for keyPair Failed...\n");
            return kStatus_SSS_Fail;
        }

        retStatus = Se05x_API_DeleteSecureObject(&pSession->s_ctx, keyId);
        if (retStatus != SM_OK) {
            LOG_W("Error in erasing ObjId=0x%08X (Others)", keyId);
        }


        status = sss_key_object_allocate_handle(
            &pex_sss_demo_tls_ctx->obj, keyId,
            keyPart,
            cipherType, keyLen,
            kKeyObject_Mode_Persistent);
        if (status != kStatus_SSS_Success) {
            printf(" sss_key_object_allocate_handle  for keyPair Failed...\ntrying to get_handle\n");
            return kStatus_SSS_Fail;
        }

        status = sss_key_store_generate_key(
            &pCtx->ks,&pex_sss_demo_tls_ctx->obj,
            keyBitLen,
            NULL);
        if (status != kStatus_SSS_Success) {
            printf(" sss_key_store_generate_keypair Failed...skipping\n");
        }

        status = sss_key_store_save(
            &pCtx->ks);
        if (status != kStatus_SSS_Success) {
            printf(" sss_key_store_save Failed...skipping\n");
        }

        /* free */
        sss_key_object_free(&pex_sss_demo_tls_ctx->pub_obj);



        /* pex_sss_demo_tls_ctx->pub_obj will have the private key handle */
        status = sss_key_object_init(&pex_sss_demo_tls_ctx->pub_obj, &pCtx->ks);
        if (status != kStatus_SSS_Success) {
            printf(" sss_key_object_init for Pub key Failed...\n");
            return kStatus_SSS_Fail;
        }

        status = sss_key_object_get_handle(
            &pex_sss_demo_tls_ctx->pub_obj, keyId);
        if (status != kStatus_SSS_Success) {
            printf(" sss_key_object_get_handle  for keyPair Failed...\ntrying to get_handle\n");
            return kStatus_SSS_Fail;
        }


            /* doc+:load-certificate-from-se */
            size_t KeyBitLen = SIZE_PUBKEY * 8;
            size_t KeyByteLen = SIZE_PUBKEY;

        status = sss_key_store_get_key(
            &pCtx->ks, &pex_sss_demo_tls_ctx->pub_obj, pubkey, &KeyByteLen, &KeyBitLen);
        if (status != kStatus_SSS_Success) {
            printf(" sss_key_store_get_key for extPubkey Failed...%d\n",status);
            return kStatus_SSS_Fail;
        }



    buf_len = sizeof(buf);
    memset (buf ,0 , buf_len);



    /* Convert to base 64 */
    base64encode(pubkey, (pubkey[1] + 2), (char *)buf, &buf_len);


    /* Add a null terminator */
    buf[buf_len] = 0;

    /* Print out the pubkey */
    printf("-----BEGIN PUBLIC KEY-----\n%s\n-----END PUBLIC KEY-----\n", buf);


    if (ret < 0)
        ret = kStatus_SSS_Fail;

    smStatus_t sw_status;
    SE05x_Result_t result = kSE05x_Result_NA;

    sw_status = Se05x_API_CheckObjectExists(
        &pSession->s_ctx, kSE05x_AppletResID_UNIQUE_ID, &result);
    if (SM_OK != sw_status) {
        LOG_E("Failed Se05x_API_CheckObjectExists");
        return kStatus_SSS_Fail;
    }
    uint8_t uid[SE050_MODULE_UNIQUE_ID_LEN];
    size_t uidLen = sizeof(uid);
    sw_status = Se05x_API_ReadObject(&pSession->s_ctx,
        kSE05x_AppletResID_UNIQUE_ID,
        0,
        (uint16_t)uidLen,
        uid,
        &uidLen);
    if (SM_OK != sw_status) {
        LOG_E("Failed Se05x_API_CheckObjectExists");
        return kStatus_SSS_Fail;
    }
    status = kStatus_SSS_Success;



    buf_len = sizeof(buf);
    memset (buf ,0 , buf_len);



    /* Convert to base 64 */
    base64encode(uid, uidLen, (char *)buf, &buf_len);


    /* Add a null terminator */
    buf[buf_len] = 0;

    /* Print out the pubkey */
    printf("-----BEGIN UID-----\n%s\n-----END UID-----\n", buf);





    return kStatus_SSS_Success;
}



void app_main(void)
{
	i2c_master_init();
    ESP_ERROR_CHECK( nvs_flash_init() );    
    main( 1 ,NULL);
}