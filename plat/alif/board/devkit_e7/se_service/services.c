#include <platform_def.h>
#include <mhu.h>
#include <lib/mmio.h>
#include <common/debug.h>
#include "services_lib_protocol.h"
#include "services_lib_ids.h"

#define AES_ENC_KEY_LEN                 16
/**
 * OSPI write key commands
 */
#define OSPI_WRITE_OTP_KEY_OSPI0        0
#define OSPI_WRITE_OTP_KEY_OSPI1        1
#define OSPI_WRITE_EXTERNAL_KEY_OSPI0   2
#define OSPI_WRITE_EXTERNAL_KEY_OSPI1   3
#define CH_INT_ST0                      0xFA0
#define CH_CLR                          0x8
#define DELAY                           0xFFF00
#define CH_ID                           0

service_header_t heartbeat_req;
ospi_write_key_svc_t ospi1_write_key;
/* ASCII values of key '0123456789ABCDEF' or
 * '0', '1', '2', '3', etc. */
uint8_t aes_enc_key[AES_ENC_KEY_LEN] = AES_ENC_KEY;

int service_ospi_write_aes_key(void)
{
	uint8_t temp;

	/* Send heartbeat for synchronization */
	memset(&heartbeat_req, 0x0, sizeof(heartbeat_req));
	heartbeat_req.hdr_service_id = SERVICE_MAINTENANCE_HEARTBEAT_ID;
	INFO("Sending heartbeat messages for synchronization\n");
	mhu_secure_message_start(PLAT_SDK700_MHU0_SEND, CH_ID);
	do {
		INFO("heartbeat\n");
		mhu_secure_message_send(PLAT_SDK700_MHU0_SEND, CH_ID,
					(uint32_t)&heartbeat_req);
		__asm__ __volatile__("dsb" ::: "memory");
	} while(!(mmio_read_32(PLAT_SDK700_MHU0_RECV + CH_INT_ST0) &
		 (1 << CH_ID)));
	/* channel 0 is cleared */
	mmio_write_32(PLAT_SDK700_MHU0_RECV + CH_CLR, 0xFFFFFFFF);

	/* Write external key into OSPI1 AES decoder */
	for(int i = 0; i < (AES_ENC_KEY_LEN>>1) ; ++i)
	{
		temp = aes_enc_key[i];
		aes_enc_key[i] = aes_enc_key[AES_ENC_KEY_LEN -i -1];
		aes_enc_key[AES_ENC_KEY_LEN -i -1] = temp;
	}
	memset(&ospi1_write_key, 0x0, sizeof(ospi1_write_key));
	ospi1_write_key.send_command = OSPI_WRITE_EXTERNAL_KEY_OSPI1;
	memcpy((void *)ospi1_write_key.send_key, aes_enc_key, AES_ENC_KEY_LEN);
	ospi1_write_key.header.hdr_service_id = SERVICE_APPLICATION_OSPI_WRITE_KEY_ID;
	mhu_secure_message_send(PLAT_SDK700_MHU0_SEND, CH_ID,
				(uint32_t) &ospi1_write_key);
	__asm__ __volatile__("dsb" ::: "memory");
	while(!(mmio_read_32(PLAT_SDK700_MHU0_RECV + CH_INT_ST0) &
	       (1 << CH_ID)));
	/* channel 0 is cleared */
	mmio_write_32(PLAT_SDK700_MHU0_RECV + CH_CLR, 0xFFFFFFFF);
	mhu_secure_message_end(PLAT_SDK700_MHU0_SEND, 0);

	INFO("AES encryption key wrote to decryption register\n");
	return 0;
}
