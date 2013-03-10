/**************************************************************************
	MMC/SD�J�[�hSPI�A�N�Z�X�y���t�F����
		�t�@�C���T�u�V�X�e���E�f�o�C�X�h���C�o�֐� (NiosII HAL version)
		�^�C���X�^���v�擾 

		UPDATE 2011/12/28
 **************************************************************************/
 
// ******************************************************************* //
//     Copyright (C) 2011, J-7SYSTEM Works.  All rights Reserved.      //
//                                                                     //
// * This module is a free sourcecode and there is NO WARRANTY.        //
// * No restriction on use. You can use, modify and redistribute it    //
//   for personal, non-profit or commercial products UNDER YOUR        //
//   RESPONSIBILITY.                                                   //
// * Redistributions of source code must retain the above copyright    //
//   notice.                                                           //
// ******************************************************************* //

#include <stdio.h>
#include <time.h>

#include "gettime.h"


/* RTC�������̂��ߎ��Ԃ̓n�[�h�R�[�h */

DWORD get_fattime (void)
{
	return
		 ( (2011UL - 1980) << 25 )	// Year = 2011
		|( 2UL << 21 )				// Month = Feb
		|( 25UL << 16 )				// Day = 25
		|( 0U << 11 )				// Hour = 0
		|( 0U << 5 )				// Min = 0
		|( 0U >> 1 )				// Sec = 0
		;
}

#if 0
DWORD get_fattime (void)
{
	time_t sec;
	struct tm *t_st;
	DWORD res;

	time(&sec);
	t_st = localtime(&sec);

	res = 
		 ( ((t_st->tm_year + 1900) - 1980) << 25 )	// �N�t�B�[���h 
		|( (t_st->tm_mon + 1) << 21 )				// ���t�B�[���h 
		|( t_st->tm_mday << 16 )					// ���t�B�[���h 
		|( t_st->tm_hour << 11 )					// ���t�B�[���h 
		|( t_st->tm_min << 5 )						// ���t�B�[���h 
		|( t_st->tm_sec >> 1 )						// �b�t�B�[���h 
		;

	return res;
}
#endif


