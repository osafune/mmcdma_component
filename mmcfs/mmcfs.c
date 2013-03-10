/**************************************************************************
	MMC/SD�J�[�hSPI�A�N�Z�X�y���t�F����
		�t�@�C���T�u�V�X�e���E�f�o�C�X�h���C�o�֐� (NiosII HAL version)

		UPDATE	2011/12/10 : �f�o�b�O��
				2011/12/28 : 1st�����[�X 
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
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/alt_warning.h>
#include <sys/alt_dev.h>
#include <sys/alt_llist.h>

#include <system.h>
#include "fatfs/ff.h"
#include "mmcfs.h"


/* �t�@�C���f�X�N���v�^�e�[�u�� */

#ifndef MMCFS_FD_MAXNUM
  #define MMCFS_FD_MAXNUM	ALT_MAX_FD
#endif

static int dev_mmcfs_fd_num;					// �t�@�C���f�X�N���v�^�X�^�b�N 
static int dev_mmcfs_fd[ MMCFS_FD_MAXNUM ];
static FIL dev_mmcfs_table[ MMCFS_FD_MAXNUM ];	// �t�@�C���I�u�W�F�N�g�e�[�u�� 



/* �t�@�C���I�[�v�� */

int dev_mmcfs_open(
		alt_fd *fd,
		const char *name,
		int flags,			// �t�@�C���I�[�v���t���O 
		int mode)			// �g�p���Ȃ� 
{
	FIL fsobj;
	FRESULT fatfs_res;
	BYTE fatfs_mode=0;
	int fd_num;
	int str_offset;

	str_offset = strlen(fd->dev->name);

	// �t�@�C���f�X�N���v�^�̊m�F 
	if( dev_mmcfs_fd_num == 0 ) return -ENFILE;

	// �I�[�v�����[�h�̕ϊ� 
	switch( flags & 3 ) {
	default:
		return -EINVAL;

	case O_RDONLY:
		fatfs_mode = FA_OPEN_EXISTING | FA_READ;
		break;

	case O_WRONLY:
		if(flags & O_APPEND) {
			fatfs_mode = FA_OPEN_ALWAYS | FA_WRITE;
		} else {
			fatfs_mode = FA_CREATE_ALWAYS | FA_WRITE;
		}
		break;

	case O_RDWR:
		if(flags & O_APPEND) {
			fatfs_mode = FA_OPEN_ALWAYS | FA_READ | FA_WRITE;
		} else if(flags & O_CREAT) {
			fatfs_mode = FA_CREATE_ALWAYS | FA_READ | FA_WRITE;
		} else {
			fatfs_mode = FA_OPEN_EXISTING | FA_READ | FA_WRITE;
		}
		break;
	}

	// �t�@�C���I�[�v�� 
	fatfs_res = f_open(&fsobj, name+str_offset, fatfs_mode);
	if( fatfs_res != FR_OK ) return -ENOENT;

	// �t�@�C���f�X�N���v�^�擾 
	dev_mmcfs_fd_num--;
	fd_num = dev_mmcfs_fd[dev_mmcfs_fd_num];
	dev_mmcfs_table[fd_num] = fsobj;

	fd->priv = (void *)&dev_mmcfs_table[fd_num];
	fd->fd_flags = fd_num;


	return fd_num;
}



/* �t�@�C���N���[�Y */

int dev_mmcfs_close(
		alt_fd *fd)
{
	FIL *fp;
	FRESULT fatfs_res;
	int fd_num;

	// �t�@�C���N���[�Y 
	fp = (FIL *)fd->priv;
	fd_num = fd->fd_flags;

	fatfs_res = f_close(fp);
	if( fatfs_res != FR_OK ) return -EIO;

	// �t�@�C���f�X�N���v�^�ԋp 
	dev_mmcfs_fd[dev_mmcfs_fd_num] = fd_num;
	dev_mmcfs_fd_num++;

	fd->priv = NULL;
	fd->fd_flags = -1;

	return 0;
}



/* �t�@�C�����[�h */

int dev_mmcfs_read(
		alt_fd *fd,
		char *ptr,
		int len)
{
	FIL *fp;
	FRESULT fatfs_res;
	UINT readsize;

	fp = (FIL *)fd->priv;

	fatfs_res = f_read(fp, ptr, len, &readsize);
	if( fatfs_res != FR_OK ) return -EIO;

	return (int)readsize;
}



/* �t�@�C�����C�g */

int dev_mmcfs_write(
		alt_fd *fd,
		const char *ptr,
		int len)
{
	FIL *fp;
	FRESULT fatfs_res;
	UINT writesize;

	fp = (FIL *)fd->priv;

	fatfs_res = f_write(fp, ptr, len, &writesize);
	if( fatfs_res != FR_OK ) return -EIO;

	return (int)writesize;
}



/* �t�@�C���V�[�N */

int dev_mmcfs_lseek(
		alt_fd *fd,
		int ptr,
		int dir)
{
	FIL *fp;
	FRESULT fatfs_res;
	int fpos;

	fp = (FIL *)fd->priv;

	switch( dir ) {
	default:
		return -EINVAL;

	case SEEK_SET:
		fpos = ptr;
		break;

	case SEEK_CUR:
		fpos = (int)fp->fptr + ptr;
		break;

	case SEEK_END:
		fpos = (int)fp->fsize + ptr;
		break;
	}

	// �t�@�C���V�[�N���s 
	fatfs_res = f_lseek(fp, fpos);
	if( fatfs_res != FR_OK ) return -EINVAL;

	return (int)fp->fptr;
}



/* �t�@�C���X�e�[�^�X */

int dev_mmcfs_fstat(
		alt_fd *fd,
		struct stat *buf)
{
	FIL *fp;

	fp = (FIL *)fd->priv;

	buf->st_mode = S_IFREG;
	buf->st_rdev = 0;

	if( fp != NULL) {
		buf->st_size = (off_t)fp->fsize;
	} else {
		buf->st_size = 0;
	}

	return 0;
}



/* �f�o�C�X�C���X�^���X */

static FATFS dev_mmc_fatfsobj;					// FatFS�V�X�e���I�u�W�F�N�g 

static alt_dev dev_mmcfs_inst = {
	ALT_LLIST_ENTRY,	// �����g�p 
	"mmcfs:",			// �f�o�C�X�� 
	dev_mmcfs_open,		// �t�@�C���I�[�v�� 
	dev_mmcfs_close,	// �t�@�C���N���[�Y 
	dev_mmcfs_read,		// �t�@�C�����[�h 
	dev_mmcfs_write,	// �t�@�C�����C�g 
	dev_mmcfs_lseek,	// �t�@�C���V�[�N 
	dev_mmcfs_fstat,	// �t�@�C���f�o�C�X�X�e�[�^�X 
	NULL				// �t�@�C��I/O�R���g���[�� 
};

int mmcfs_setup(void)
{
	int ret_code;

	// FatFs������ 
	memset(&dev_mmc_fatfsobj, 0, sizeof(FATFS));	/* Invalidate file system */
	f_mount(0, &dev_mmc_fatfsobj);					/* Assign object memory to the FatFs module */

	// �t�@�C���f�X�N���v�^�Ǘ��e�[�u�������� 
	dev_mmcfs_fd_num = 0;
	do {
		dev_mmcfs_fd[dev_mmcfs_fd_num] = dev_mmcfs_fd_num;
		dev_mmcfs_fd_num++;
	} while( dev_mmcfs_fd_num < MMCFS_FD_MAXNUM );

	// �t�@�C���V�X�e���f�o�C�X�o�^ 
	ret_code = alt_fs_reg(&dev_mmcfs_inst);

	return ret_code;
}


