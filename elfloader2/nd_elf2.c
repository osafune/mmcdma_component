/**************************************************************************
	PROCYON コンソールライブラリ「nd_Lib」 (Cineraria DE0 Edition)

		アブソリュート実行elfファイルをロード

	UPDATE	2011/12/28 : FatFS→PetitFatFSに変更 
 **************************************************************************/
/*
【　機能　】
	SDカードからelfファイルをメモリにロードして実行する

【必須リソース】
	・プログラムを外部RAMに展開して実行できるバス構成
	・SD/MMCにアクセスできるSPIペリフェラル(MMC_SPIまたはMMCDMAペリフェラルを想定)
	・ロードするelfファイルは外部RAMの最終8kバイトにデータを配置していないこと

【システムライブラリ設定】
	・RTOSなし（※必ずnone single-threadedにすること）
	・stdout/stderr/stdinはUARTまたはJTAG-UART
	・System clock timerなし（※必ずnoneにすること）
	・Timestamp timerなし（※必ずnoneにすること）
	・■ Program never exits
	・□ Clean exit (flush buffers)
	・□ Support C++
	・■ Reduced device drivers
	・■ Lightweight device driver API
	・■ Small C library
	・□ Link with profiling library
	・□ ModelSim only, no hardware support
	・□ Unimplemented instruction handler
	・□ Run time stack checking
	・全てのメモリ領域を該当するRAM(外部RAM)へ設定
	・□ Use a separate exception stack
	・最適化オプションを-Osに変更

	いったん上記設定でビルドを行い、スクリプトファイルを生成する

	～syslib -> Release -> system_description 以下の generated.x を編集
	プログラム配置メモリの _UNUSED ブロックを最終8kバイト領域までずらす

	再度ビルドを行い、メモリエリアが変更されている事を確認する
*/

#include <system.h>
#include <io.h>
#include <sys/alt_cache.h>
#include "pff.h"
#include "mmc_spi.h"


// elf_loaderが配置されるメモリアドレス 
//#define ELFLOADER_PROC_BEGIN		(0x0007e000 | (1<<31))
//#define ELFLOADER_PROC_END			(0x0007ffff | (1<<31))
#define ELFLOADER_PROC_BEGIN        (0x007fe000 | (1<<31))
#define ELFLOADER_PROC_END          (0x007fffff | (1<<31))


// ブートプログラムファイル名 
#define ELFLOADER_BOOT_FILE			"/DE0/boot.elf"


// デバッグ用表示 
//#define _DEBUG_


/***** ELFファイル型定義 **************************************************/

// ELFファイルの変数型宣言 

typedef void*			ELF32_Addr;
typedef unsigned short	ELF32_Half;
typedef unsigned long	ELF32_Off;
typedef long			ELF32_Sword;
typedef unsigned long	ELF32_Word;
typedef unsigned char	ELF32_Char;

// ELFファイルヘッダ構造体 
typedef struct {
	ELF32_Char		elf_id[16];		// ファイルID 
	ELF32_Half		elf_type;		// オブジェクトファイルタイプ 
	ELF32_Half		elf_machine;	// ターゲットアーキテクチャ 
	ELF32_Word		elf_version;	// ELFファイルバージョン(現在は1) 
	ELF32_Addr		elf_entry;		// エントリアドレス(エントリ無しなら0) 
	ELF32_Off		elf_phoff;		// Programヘッダテーブルのファイル先頭からのオフセット 
	ELF32_Off		elf_shoff;		// 実行時未使用
	ELF32_Word		elf_flags;		// プロセッサ固有のフラグ 
	ELF32_Half		elf_ehsize;		// ELFヘッダのサイズ 
	ELF32_Half		elf_phentsize;	// Programヘッダテーブルの1要素あたりのサイズ 
	ELF32_Half		elf_phnum;		// Programヘッダテーブルの要素数 
	ELF32_Half		elf_shentsize;	// 実行時未使用
	ELF32_Half		elf_shnum;		// 実行時未使用
	ELF32_Half		elf_shstrndx;	// 実行時未使用
} __attribute__ ((packed)) ELF32_HEADER;

// Programヘッダ構造体 
typedef struct {
	ELF32_Word		p_type;			// セグメントのエントリタイプ 
	ELF32_Off		p_offset;		// 対応するセグメントのファイル先頭からのオフセット 
	ELF32_Addr		p_vaddr;		// メモリ上でのセグメントの第一バイトの仮想アドレス 
	ELF32_Addr		p_paddr;		// 物理番地指定が適切なシステムの為に予約(p_vaddrと同値)
	ELF32_Word		p_filesz;		// 対応するセグメントのファイルでのサイズ(0も可)
	ELF32_Word		p_memsz;		// 対応するセグメントのメモリ上に展開された時のサイズ(0も可)
	ELF32_Word		p_flags;		// 対応するセグメントに適切なフラグ 
	ELF32_Word		p_align;		// アライメント(p_offsetとp_vaddrをこの値で割った余りは等しい)
} __attribute__ ((packed)) ELF32_PHEADER;

// ELFオブジェクトファイルタイプの定数宣言 
#define ELF_ET_EXEC		(2)			// 実行可能なオブジェクトファイル 
#define ELF_EM_NIOS2	(0x0071)	// Altera NiosII Processor
#define ELF_PT_LOAD		(1)			// 実行時にロードされるセグメント 


/**************************************************************************
	アブソリュートelfファイルのロード 
 **************************************************************************/

/* Petit-FatFs work area */
FATFS g_fatfs_work;

/* デバッグ用printf */
#ifdef _DEBUG_
 #include <stdio.h>
 #define dgb_printf printf
#else
 int dgb_printf(const char *format, ...) { return 0; }
#endif


/* elfファイルをメモリに展開 */
static ELF32_HEADER eh;		// elfファイルヘッダ	(※メモリ使用領域確認のためstackから外している) 
static ELF32_PHEADER ph;	// elfセクションヘッダ	(※メモリ使用領域確認のためstackから外している) 

static int nd_elfload(alt_u32 *entry_addr)
{
	int phnum;
	alt_u32 phy_addr,sec_size;
	WORD res_byte,load_byte;
	DWORD f_pos;


	/* elfヘッダファイルのチェック */

	if (pf_lseek(0) != FR_OK) return (-1);
	if (pf_read(&eh, sizeof(ELF32_HEADER), &res_byte) != FR_OK) return (-1);

	if (eh.elf_id[0] != 0x7f ||				// ELFヘッダのチェック 
			eh.elf_id[1] != 'E' ||
			eh.elf_id[2] != 'L' ||
			eh.elf_id[3] != 'F') {
		return(-2);
	}
	if (eh.elf_type != ELF_ET_EXEC) {		// オブジェクトタイプのチェック 
		return(-2);
	}
	if (eh.elf_machine != ELF_EM_NIOS2) {	// ターゲットCPUのチェック 
		return(-2);
	}

	*entry_addr = (alt_u32)eh.elf_entry;	// エントリアドレスの取得 


	/* セクションデータをロード */

	f_pos = (DWORD)eh.elf_ehsize;
	for (phnum=1 ; phnum<=eh.elf_phnum ; phnum++) {

		// Programヘッダを読み込む 
		if (pf_lseek(f_pos) != FR_OK) return (-1);
		if (pf_read(&ph, eh.elf_phentsize, &res_byte) != FR_OK) return (-1);
		f_pos += eh.elf_phentsize;

		// セクションデータをメモリに展開 
		if(ph.p_type == ELF_PT_LOAD && ph.p_filesz > 0) {
			dgb_printf("- Section %d -----\n",phnum);
			dgb_printf("  Mem address : 0x%08x\n",(unsigned int)ph.p_vaddr);
			dgb_printf("  Image size  : %d bytes(0x%08x)\n",ph.p_filesz, ph.p_filesz);
			dgb_printf("  File offset : 0x%08x\n",ph.p_offset);

			if (pf_lseek(ph.p_offset) != FR_OK) return (-1);
			phy_addr = (alt_u32)ph.p_vaddr | (1<<31);
			sec_size = ph.p_filesz;

			while(sec_size > 0) {
				if (sec_size >= 32768) {		// 32kバイト単位で読み込む 
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


/* elfファイルのロードと実行 */

int main(void)
{
	FRESULT res;
	void (*pProc)();
	alt_u32 entry_addr = 0,ledcode = 0;
	char *elf_fname;

	IOWR(LED_7SEG_BASE, 0, ~0x7c5c5c78);		// boot表示 


	/* ELFファイル名の取得 */

	elf_fname = ELFLOADER_BOOT_FILE;


	/* FatFsモジュール初期化 */

	dgb_printf("\n*** ELF LOADING ***\n");
	dgb_printf("Disk initialize... ");

	res = pf_mount(&g_fatfs_work);		/* Initialize file system */
	if (res != FR_OK) {
		dgb_printf("fail(%d)",(int)res);

		ledcode = ~0x7950d006;					// ディスク初期化エラー Err.1 
		goto BOOT_FAILED;
	}
	dgb_printf("done.\n");


	/* ファイルを開く */

	dgb_printf("Open \"%s\"\n",elf_fname);

	res = pf_open( elf_fname );
	if (res != FR_OK) {
		dgb_printf("[!] f_open fail(%d)\n", (int)res);

		ledcode = ~0x7950d05b;					// ファイルオープンエラー Err.2 
		goto BOOT_FAILED;
	}


	/* ELFファイルの読み込み */

	if (nd_elfload(&entry_addr) != 0) {
		dgb_printf("[!] elf-file read error.\n");

		ledcode = ~0x7950d04f;					// ファイルリードエラー Err.3 
		goto BOOT_FAILED;
	}


	/* elfファイル実行 */

	dgb_printf("Entry address : 0x%08x\n",entry_addr);
	dgb_printf("elf file execute.\n\n");

	IOWR(LED_7SEG_BASE, 0, ~0x501c5400);		// run表示 

	pProc = (void (*)())entry_addr;

	alt_dcache_flush_all();
	alt_icache_flush_all();
	(*pProc)();


	/* ブート失敗 */

  BOOT_FAILED:
	while(1) {
		IOWR(LED_7SEG_BASE, 0, ~0);				// エラーコードを点滅表示 
		mmc_spi_SetTimer(200);
		while( mmc_spi_CheckTimer() ) {}

		IOWR(LED_7SEG_BASE, 0, ledcode);
		mmc_spi_SetTimer(300);
		while( mmc_spi_CheckTimer() ) {}
	}

	return (0);
}


