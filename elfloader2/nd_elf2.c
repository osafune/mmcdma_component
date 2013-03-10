/**************************************************************************
	PROCYON �R���\�[�����C�u�����und_Lib�v (Cineraria DE0 Edition)

		�A�u�\�����[�g���self�t�@�C�������[�h

	UPDATE	2011/12/28 : FatFS��PetitFatFS�ɕύX 
 **************************************************************************/
/*
�y�@�@�\�@�z
	SD�J�[�h����elf�t�@�C�����������Ƀ��[�h���Ď��s����

�y�K�{���\�[�X�z
	�E�v���O�������O��RAM�ɓW�J���Ď��s�ł���o�X�\��
	�ESD/MMC�ɃA�N�Z�X�ł���SPI�y���t�F����(MMC_SPI�܂���MMCDMA�y���t�F������z��)
	�E���[�h����elf�t�@�C���͊O��RAM�̍ŏI8k�o�C�g�Ƀf�[�^��z�u���Ă��Ȃ�����

�y�V�X�e�����C�u�����ݒ�z
	�ERTOS�Ȃ��i���K��none single-threaded�ɂ��邱�Ɓj
	�Estdout/stderr/stdin��UART�܂���JTAG-UART
	�ESystem clock timer�Ȃ��i���K��none�ɂ��邱�Ɓj
	�ETimestamp timer�Ȃ��i���K��none�ɂ��邱�Ɓj
	�E�� Program never exits
	�E�� Clean exit (flush buffers)
	�E�� Support C++
	�E�� Reduced device drivers
	�E�� Lightweight device driver API
	�E�� Small C library
	�E�� Link with profiling library
	�E�� ModelSim only, no hardware support
	�E�� Unimplemented instruction handler
	�E�� Run time stack checking
	�E�S�Ẵ������̈���Y������RAM(�O��RAM)�֐ݒ�
	�E�� Use a separate exception stack
	�E�œK���I�v�V������-Os�ɕύX

	���������L�ݒ�Ńr���h���s���A�X�N���v�g�t�@�C���𐶐�����

	�`syslib -> Release -> system_description �ȉ��� generated.x ��ҏW
	�v���O�����z�u�������� _UNUSED �u���b�N���ŏI8k�o�C�g�̈�܂ł��炷

	�ēx�r���h���s���A�������G���A���ύX����Ă��鎖���m�F����
*/

#include <system.h>
#include <io.h>
#include <sys/alt_cache.h>
#include "pff.h"
#include "mmc_spi.h"


// elf_loader���z�u����郁�����A�h���X 
//#define ELFLOADER_PROC_BEGIN		(0x0007e000 | (1<<31))
//#define ELFLOADER_PROC_END			(0x0007ffff | (1<<31))
#define ELFLOADER_PROC_BEGIN        (0x007fe000 | (1<<31))
#define ELFLOADER_PROC_END          (0x007fffff | (1<<31))


// �u�[�g�v���O�����t�@�C���� 
#define ELFLOADER_BOOT_FILE			"/DE0/boot.elf"


// �f�o�b�O�p�\�� 
//#define _DEBUG_


/***** ELF�t�@�C���^��` **************************************************/

// ELF�t�@�C���̕ϐ��^�錾 

typedef void*			ELF32_Addr;
typedef unsigned short	ELF32_Half;
typedef unsigned long	ELF32_Off;
typedef long			ELF32_Sword;
typedef unsigned long	ELF32_Word;
typedef unsigned char	ELF32_Char;

// ELF�t�@�C���w�b�_�\���� 
typedef struct {
	ELF32_Char		elf_id[16];		// �t�@�C��ID 
	ELF32_Half		elf_type;		// �I�u�W�F�N�g�t�@�C���^�C�v 
	ELF32_Half		elf_machine;	// �^�[�Q�b�g�A�[�L�e�N�`�� 
	ELF32_Word		elf_version;	// ELF�t�@�C���o�[�W����(���݂�1) 
	ELF32_Addr		elf_entry;		// �G���g���A�h���X(�G���g�������Ȃ�0) 
	ELF32_Off		elf_phoff;		// Program�w�b�_�e�[�u���̃t�@�C���擪����̃I�t�Z�b�g 
	ELF32_Off		elf_shoff;		// ���s�����g�p
	ELF32_Word		elf_flags;		// �v���Z�b�T�ŗL�̃t���O 
	ELF32_Half		elf_ehsize;		// ELF�w�b�_�̃T�C�Y 
	ELF32_Half		elf_phentsize;	// Program�w�b�_�e�[�u����1�v�f������̃T�C�Y 
	ELF32_Half		elf_phnum;		// Program�w�b�_�e�[�u���̗v�f�� 
	ELF32_Half		elf_shentsize;	// ���s�����g�p
	ELF32_Half		elf_shnum;		// ���s�����g�p
	ELF32_Half		elf_shstrndx;	// ���s�����g�p
} __attribute__ ((packed)) ELF32_HEADER;

// Program�w�b�_�\���� 
typedef struct {
	ELF32_Word		p_type;			// �Z�O�����g�̃G���g���^�C�v 
	ELF32_Off		p_offset;		// �Ή�����Z�O�����g�̃t�@�C���擪����̃I�t�Z�b�g 
	ELF32_Addr		p_vaddr;		// ��������ł̃Z�O�����g�̑��o�C�g�̉��z�A�h���X 
	ELF32_Addr		p_paddr;		// �����Ԓn�w�肪�K�؂ȃV�X�e���ׂ̈ɗ\��(p_vaddr�Ɠ��l)
	ELF32_Word		p_filesz;		// �Ή�����Z�O�����g�̃t�@�C���ł̃T�C�Y(0����)
	ELF32_Word		p_memsz;		// �Ή�����Z�O�����g�̃�������ɓW�J���ꂽ���̃T�C�Y(0����)
	ELF32_Word		p_flags;		// �Ή�����Z�O�����g�ɓK�؂ȃt���O 
	ELF32_Word		p_align;		// �A���C�����g(p_offset��p_vaddr�����̒l�Ŋ������]��͓�����)
} __attribute__ ((packed)) ELF32_PHEADER;

// ELF�I�u�W�F�N�g�t�@�C���^�C�v�̒萔�錾 
#define ELF_ET_EXEC		(2)			// ���s�\�ȃI�u�W�F�N�g�t�@�C�� 
#define ELF_EM_NIOS2	(0x0071)	// Altera NiosII Processor
#define ELF_PT_LOAD		(1)			// ���s���Ƀ��[�h�����Z�O�����g 


/**************************************************************************
	�A�u�\�����[�gelf�t�@�C���̃��[�h 
 **************************************************************************/

/* Petit-FatFs work area */
FATFS g_fatfs_work;

/* �f�o�b�O�pprintf */
#ifdef _DEBUG_
 #include <stdio.h>
 #define dgb_printf printf
#else
 int dgb_printf(const char *format, ...) { return 0; }
#endif


/* elf�t�@�C�����������ɓW�J */
static ELF32_HEADER eh;		// elf�t�@�C���w�b�_	(���������g�p�̈�m�F�̂���stack����O���Ă���) 
static ELF32_PHEADER ph;	// elf�Z�N�V�����w�b�_	(���������g�p�̈�m�F�̂���stack����O���Ă���) 

static int nd_elfload(alt_u32 *entry_addr)
{
	int phnum;
	alt_u32 phy_addr,sec_size;
	WORD res_byte,load_byte;
	DWORD f_pos;


	/* elf�w�b�_�t�@�C���̃`�F�b�N */

	if (pf_lseek(0) != FR_OK) return (-1);
	if (pf_read(&eh, sizeof(ELF32_HEADER), &res_byte) != FR_OK) return (-1);

	if (eh.elf_id[0] != 0x7f ||				// ELF�w�b�_�̃`�F�b�N 
			eh.elf_id[1] != 'E' ||
			eh.elf_id[2] != 'L' ||
			eh.elf_id[3] != 'F') {
		return(-2);
	}
	if (eh.elf_type != ELF_ET_EXEC) {		// �I�u�W�F�N�g�^�C�v�̃`�F�b�N 
		return(-2);
	}
	if (eh.elf_machine != ELF_EM_NIOS2) {	// �^�[�Q�b�gCPU�̃`�F�b�N 
		return(-2);
	}

	*entry_addr = (alt_u32)eh.elf_entry;	// �G���g���A�h���X�̎擾 


	/* �Z�N�V�����f�[�^�����[�h */

	f_pos = (DWORD)eh.elf_ehsize;
	for (phnum=1 ; phnum<=eh.elf_phnum ; phnum++) {

		// Program�w�b�_��ǂݍ��� 
		if (pf_lseek(f_pos) != FR_OK) return (-1);
		if (pf_read(&ph, eh.elf_phentsize, &res_byte) != FR_OK) return (-1);
		f_pos += eh.elf_phentsize;

		// �Z�N�V�����f�[�^���������ɓW�J 
		if(ph.p_type == ELF_PT_LOAD && ph.p_filesz > 0) {
			dgb_printf("- Section %d -----\n",phnum);
			dgb_printf("  Mem address : 0x%08x\n",(unsigned int)ph.p_vaddr);
			dgb_printf("  Image size  : %d bytes(0x%08x)\n",ph.p_filesz, ph.p_filesz);
			dgb_printf("  File offset : 0x%08x\n",ph.p_offset);

			if (pf_lseek(ph.p_offset) != FR_OK) return (-1);
			phy_addr = (alt_u32)ph.p_vaddr | (1<<31);
			sec_size = ph.p_filesz;

			while(sec_size > 0) {
				if (sec_size >= 32768) {		// 32k�o�C�g�P�ʂœǂݍ��� 
					load_byte = 32768;
					sec_size -= 32768;
				} else {
					load_byte = sec_size;
					sec_size = 0;
				}

				if ( (ELFLOADER_PROC_BEGIN <= phy_addr+load_byte) &&
						(ELFLOADER_PROC_END >= phy_addr+load_byte) ) return (-2);

				if (pf_read((void*)phy_addr, load_byte, &res_byte) != FR_OK) return (-1);
				phy_addr += load_byte;
			}
		}
	}

	return(0);
}


/* elf�t�@�C���̃��[�h�Ǝ��s */

int main(void)
{
	FRESULT res;
	void (*pProc)();
	alt_u32 entry_addr = 0,ledcode = 0;
	char *elf_fname;

	IOWR(LED_7SEG_BASE, 0, ~0x7c5c5c78);		// boot�\�� 


	/* ELF�t�@�C�����̎擾 */

	elf_fname = ELFLOADER_BOOT_FILE;


	/* FatFs���W���[�������� */

	dgb_printf("\n*** ELF LOADING ***\n");
	dgb_printf("Disk initialize... ");

	res = pf_mount(&g_fatfs_work);		/* Initialize file system */
	if (res != FR_OK) {
		dgb_printf("fail(%d)",(int)res);

		ledcode = ~0x7950d006;					// �f�B�X�N�������G���[ Err.1 
		goto BOOT_FAILED;
	}
	dgb_printf("done.\n");


	/* �t�@�C�����J�� */

	dgb_printf("Open \"%s\"\n",elf_fname);

	res = pf_open( elf_fname );
	if (res != FR_OK) {
		dgb_printf("[!] f_open fail(%d)\n", (int)res);

		ledcode = ~0x7950d05b;					// �t�@�C���I�[�v���G���[ Err.2 
		goto BOOT_FAILED;
	}


	/* ELF�t�@�C���̓ǂݍ��� */

	if (nd_elfload(&entry_addr) != 0) {
		dgb_printf("[!] elf-file read error.\n");

		ledcode = ~0x7950d04f;					// �t�@�C�����[�h�G���[ Err.3 
		goto BOOT_FAILED;
	}


	/* elf�t�@�C�����s */

	dgb_printf("Entry address : 0x%08x\n",entry_addr);
	dgb_printf("elf file execute.\n\n");

	IOWR(LED_7SEG_BASE, 0, ~0x501c5400);		// run�\�� 

	pProc = (void (*)())entry_addr;

	alt_dcache_flush_all();
	alt_icache_flush_all();
	(*pProc)();


	/* �u�[�g���s */

  BOOT_FAILED:
	while(1) {
		IOWR(LED_7SEG_BASE, 0, ~0);				// �G���[�R�[�h��_�ŕ\�� 
		mmc_spi_SetTimer(200);
		while( mmc_spi_CheckTimer() ) {}

		IOWR(LED_7SEG_BASE, 0, ledcode);
		mmc_spi_SetTimer(300);
		while( mmc_spi_CheckTimer() ) {}
	}

	return (0);
}


