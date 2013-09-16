/**************************************************************************
	MMC/SD�J�[�hSPI�A�N�Z�X�y���t�F����
		�f�o�C�X�T�|�[�g�֐� (Cineraria DE0/DE0-nano version)

	UPDATE	2010/12/11
			2011/12/24 100us�҂��ǉ�(Petit FatFs�Ή�) 
			2011/12/28 1st�����[�X�� 

			2013/04/03 �����������}�N���ŏC�� 
 **************************************************************************/

// ---------------------------------------------------------------------- //
//     Copyright (C) 2010-2013, J-7SYSTEM Works.  All rights Reserved.    //
//                                                                        //
//  * This module is a free sourcecode and there is NO WARRANTY.          //
//  * No restriction on use. You can use, modify and redistribute it for  //
//    personal, non-profit or commercial products UNDER YOUR              //
//    RESPONSIBILITY.                                                     //
//  * Redistributions of source code must retain the above copyright      //
//    notice.                                                             //
// ---------------------------------------------------------------------- //

#include <system.h>
#include <io.h>
#include "mmc_spi.h"

static alt_u32 mmc_spi_reg;		// MMC_SPI�y���t�F�������W�X�^�A�h���X 
static alt_u32 mmc_spi_ncs;		// nCS�R���g���[�� 


/* MMC CS�A�T�[�g */
void mmc_spi_SetCardSelect(void)
{
	mmc_spi_ncs = mmc_selassert;
	IOWR(mmc_spi_reg, mmcreg_status, (mmc_commexit | mmc_spi_ncs | 0xff));
}


/* MMC CS�l�Q�[�g */
void mmc_spi_SetCardDeselect(void)
{
	mmc_spi_ncs = mmc_selnegete;
	IOWR(mmc_spi_reg, mmcreg_status, (mmc_commexit | mmc_spi_ncs | 0xff));
}


/* MMC��1�o�C�g���M */
void mmc_spi_Sendbyte(alt_u8 data)
{
	while( !(IORD(mmc_spi_reg, mmcreg_status) & mmc_commexit) ) {}
	IOWR(mmc_spi_reg, mmcreg_status, (mmc_commstart | mmc_spi_ncs | data));
}


/* MMC����1�o�C�g��M */
alt_u8 mmc_spi_Recvbyte(void)
{
	alt_u32 res;

	while( !(IORD(mmc_spi_reg, mmcreg_status) & mmc_commexit) ) {}
	IOWR(mmc_spi_reg, mmcreg_status, (mmc_commstart | mmc_spi_ncs | 0xff));

	do {
		res = IORD(mmc_spi_reg, mmcreg_status);
	} while( !(res & mmc_commexit) );

	return (alt_u8)(res & 0xff);
}


/* MMC����w��o�C�g��M(2�`512�o�C�g) */
#ifdef _USE_MMCDMA
int mmc_spi_DmaRecv(alt_u8 wc, alt_u8 *buff)
{
	alt_u32 cycle,*pdst32,data32;
	int dnum;

	if (wc != 0) {
		cycle = wc << 1;
	} else {
		cycle = 256 << 1;
	}

	while( !(IORD(mmc_spi_reg, mmcreg_status) & mmc_commexit) ) {}
    mmc_spi_SetTimer(100);
	IOWR(mmc_spi_reg, mmcreg_dmastatus, (mmc_dmastart |(cycle - 1)));

	while( !(IORD(mmc_spi_reg, mmcreg_dmastatus) & mmc_dmadone_bitmask) ) {}

	if ( (IORD(mmc_spi_reg, mmcreg_dmastatus) & (mmc_dmade_bitmask|mmc_dmato_bitmask))!= 0 ) {
		return 1;
	}


	dnum = 0;

	if ( ((alt_u32)buff & 3) == 0) {		// �S�o�C�g�A���C���̓]�� 
		pdst32 = (alt_u32 *)buff;

		while(cycle > 3) {
			*pdst32 = IORD(mmc_spi_reg, mmcreg_readbuff+dnum);
			pdst32++;
			dnum++;
			cycle-=4;
			buff+=4;
		}
	}

	while(cycle > 0) {						// �c�肠�邢�͔�A���C���̏ꍇ�̓]�� 
		if ( (cycle & 3)== 0 ) {
			data32 = IORD(mmc_spi_reg, mmcreg_readbuff+dnum);
			dnum++;
		}

		*buff++ = (alt_u8)(data32 & 0xff);
		data32 >>= 8;
		cycle--;
	}

	return 0;
}
#endif


/* �J�[�h�}����Ԍ��o */
/* ���J�[�h�}����Ԃ����o�ł��Ȃ��n�[�h�E�F�A�̏ꍇ�́A�펞 return 1 �Ƃ��� */
int mmc_spi_CheckCardDetect(void)
{
	if ( !(IORD(mmc_spi_reg, mmcreg_status) & mmc_cd_bitmask) ) {
		return 1;	/* �J�[�h�}����� */
	} else {
		return 0;	/* �J�[�h�Ȃ� */
	}
}


/* �J�[�h���C�g�v���e�N�g�X�C�b�`��Ԍ��o */
/* ���J�[�h�}����Ԃ����o�ł��Ȃ��n�[�h�E�F�A�̏ꍇ�́A�펞 return 0 �Ƃ��� */
int mmc_spi_CheckWritePortect(void)
{
	if( mmc_spi_CheckCardDetect() && (IORD(mmc_spi_reg, mmcreg_status) & mmc_wp_bitmask) ) {
		return 0;	/* �������݉� */
	} else {
		return 1;	/* �J�[�h�������ĂȂ����A���C�g�v���e�N�g�X�C�b�`��ON */
	}
}


/* MMC SPI�N���b�N���o�o�i�J�[�h�F�؃��[�h�j�ɐݒ� */
void mmc_spi_SetIdentClock(void)
{
	IOWR(mmc_spi_reg, mmcreg_clkdiv, (mmc_clock_freq /(mmc_ppmode_freq * 2)));	/* 400kHz�ɂ��� */
}


/* MMC SPI�N���b�N���c�c�i�f�[�^�]�����[�h�j�ɐݒ� */
void mmc_spi_SetTransClock(void)
{
	IOWR(mmc_spi_reg, mmcreg_clkdiv, (mmc_clock_freq /(mmc_ddmode_freq * 2)));	/* �ő呬�x��20MHz */
}


/* �^�C���A�E�g�^�C�}�ݒ�i1ms�P�ʁj*/
void mmc_spi_SetTimer(const alt_u32 timeout)
{
	IOWR(mmc_spi_reg, mmcreg_timer, mmc_timecount_1ms * timeout);
}


/* �^�C���A�E�g�`�F�b�N */
int mmc_spi_CheckTimer(void)
{
	if( IORD(mmc_spi_reg, mmcreg_timer) ) {
		return 1;		/* �^�C�}���쒆 */
	} else {
		return 0;		/* �^�C���A�E�g */
	}
}


/* 100us�^�C�} */
void mmc_spi_Wait100us(void)
{
	IOWR(mmc_spi_reg, mmcreg_timer, mmc_timecount_100us);

	while( IORD(mmc_spi_reg, mmcreg_timer) ) {}
}


/* MMC�\�P�b�g�C���^�[�t�F�[�X������ */
void mmc_spi_InitSocket(void)
{
	mmc_spi_reg = (alt_u32)MMC_REGBASE;
	while( !(IORD(mmc_spi_reg, mmcreg_status) & mmc_commexit) ) {}

	mmc_spi_SetCardDeselect();
	mmc_spi_SetIdentClock();
}



/**************************************************************************/
