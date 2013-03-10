/**************************************************************************
	MMC/SD�J�[�hSPI�A�N�Z�X�y���t�F����
		�f�o�C�X�T�|�[�g�֐� (Cineraria DE0 version)

	UPDATE	2010/12/11
			2011/12/24 100us�҂��ǉ�(Petit FatFs�Ή�) 
			2011/12/28 1st�����[�X�� 
 **************************************************************************/
#ifndef __mmc_spi_h_
#define __mmc_spi_h_

#include <system.h>
#include <alt_types.h>


// MMC_SPI�y���t�F�����쓮�N���b�N(���[�U�[��`) 
#define mmc_clock_freq          (40000000)


/***** �萔�E�}�N����` ***************************************************/

#ifdef MMCDMA_BASE
// #define _USE_MMCDMA
 #define MMC_REGBASE		MMCDMA_BASE
#else
 #define MMC_REGBASE		MMC_SPI_BASE
#endif

#define mmcreg_status			(0)
#define mmcreg_clkdiv			(1)
#define mmcreg_timer			(2)

#define mmc_irq_enable			(1<<15)
#define mmc_zf_bitmask			(1<<12)
#define mmc_wp_bitmask			(1<<11)
#define mmc_cd_bitmask			(1<<10)
#define mmc_commstart			(0<<9)
#define mmc_commexit			(1<<9)
#define mmc_selassert			(0<<8)
#define mmc_selnegete			(1<<8)

#ifdef _USE_MMCDMA
#define mmcreg_dmastatus		(4)
#define mmcreg_readbuff			(128)
#define mmc_dmairq_enable		(1<<15)
#define mmc_dmadone_bitmask		(1<<14)
#define mmc_dmade_bitmask		(1<<13)
#define mmc_dmato_bitmask		(1<<12)
#define mmc_dmastart			(1<<10)
#endif

#ifndef mmc_clock_freq
#define mmc_clock_freq			ALT_CPU_FREQ
#endif
#define mmc_timecount_1ms		(mmc_clock_freq / 1000)
#define mmc_timecount_100us		(mmc_clock_freq / 10000)
#define mmc_ppmode_freq			(400 * 1000)		// PPmode = 400kHz 
#define mmc_ddmode_freq			(20 * 1000 * 1000)	// DDmode = 20MHz 


/***** �\���̒�` *********************************************************/



/***** �v���g�^�C�v�錾 ***************************************************/

/* MMC�\�P�b�g�C���^�[�t�F�[�X������ */
void mmc_spi_InitSocket(void);

/* MMC SPI�N���b�N�ݒ� */
void mmc_spi_SetIdentClock(void);
void mmc_spi_SetTransClock(void);

/* MMC �^�C���A�E�g�J�E���^�ݒ� */
void mmc_spi_Wait100us(void);
void mmc_spi_SetTimer(const alt_u32);
int mmc_spi_CheckTimer(void);

/* �J�[�h�}����Ԍ��o */
int mmc_spi_CheckCardDetect(void);

/* �J�[�h���C�g�v���e�N�g�X�C�b�`��Ԍ��o */
int mmc_spi_CheckWritePortect(void);

/* MMC CS���� */
void mmc_spi_SetCardSelect(void);
void mmc_spi_SetCardDeselect(void);

/* MMC��1�o�C�g���M */
void mmc_spi_Sendbyte(alt_u8);

/* MMC����1�o�C�g��M */
alt_u8 mmc_spi_Recvbyte(void);

/* MMC����w��o�C�g��M(2�`512�o�C�g) */
#ifdef _USE_MMCDMA
int mmc_spi_DmaRecv(alt_u8, alt_u8 *);
#endif


#endif
/**************************************************************************/
