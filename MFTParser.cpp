//https://blog.naver.com/empty7792/221380762099

#include "stdafx.h"
#include "NTFSHeader.h"

using namespace std;

void errorMsg(string error);
void hexdump(U8 * buf, U32 size);
U32 getHDD(U8 * HDDdump, U32 offset, U32 size);
U32 BIOSUEFI(U8 * HDDdump, vector<U32> & PartitionType);
void setVBR(U8 * HDDdump, U8 * VBR);
U32 betole32(U32 num);

int main(U32 argc, char * argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage : MFTParser.exe [MFT Number]");
		return 1;
	}
	
	U8 * HDDdump = (U8*)malloc(sizeof(U8) * SECTORSIZE);
	U8 * VBR = (U8*)malloc(sizeof(U8) * SECTORSIZE);
	vector<U32> PartitionType(4, 0);
	string filename;
	if (!getHDD(HDDdump, 0, SECTORSIZE)) {
		return 0;
	}

	if (BIOSUEFI(HDDdump, PartitionType) == GPT) {
		getHDD(HDDdump, SECTORSIZE, SECTORSIZE);
		setVBR(HDDdump, VBR);
	}
	
	free(VBR);

	return 0;
}

void errorMsg(string error) {
	cerr << error << endl;
}

void setVBR(U8 * HDDdump, U8 * VBR) {
	GPTHeader Gheader;
	GPTable PT;
	U64 PToffset;
	U32 GUIDoffset;
	memset(&Gheader, 0, sizeof(GPTHeader));
	memcpy(&Gheader, HDDdump, sizeof(U8) * SECTORSIZE);

	PToffset = Gheader.StartingLBAParitionTable * SECTORSIZE;

	free(HDDdump);
	HDDdump = (U8*)malloc(sizeof(U8) * SECTORSIZE * 2);
	
	getHDD(HDDdump, PToffset, SECTORSIZE * 2);

	GUIDoffset = 0;

#define FIRST_PARTITION_LBA 32
#define GUID_LEN 16
#define NO_GUID "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"


	while(true) {
		memcpy(&PT, HDDdump + GUIDoffset, sizeof(U8) * Gheader.SizeOfEntries);
		if (!memcmp(&PT, NO_GUID, sizeof(U8) * GUID_LEN)) break;
		printf("Address : %016llX\n", PT.PartitionStartingLBA * SECTORSIZE);
		getHDD(VBR, PT.PartitionStartingLBA * SECTORSIZE, SECTORSIZE);	
		hexdump(VBR, SECTORSIZE);

		GUIDoffset += Gheader.SizeOfEntries;
	}

	//hexdump(VBR, Gheader.SizeOfEntries);
}

U32 betole32(U32 num) {
	U32 ret;
	ret = ((num & 0x000000FF) << 24) 
		| ((num & 0x0000FF00) << 8) 
		| ((num & 0x00FF0000) >> 8) 
		| ((num & 0xFF000000) >> 24);
	return ret;
}

void MFTParser(void) {
	//U8 * 
}

U32 BIOSUEFI(U8 * HDDdump, vector<U32> & PartitionType) {
	if (memcmp(HDDdump + SECTORSIZE - 2, "\x55\xAA", sizeof(U8) * 2)) {
		cerr << "error : FileSystem Signature" << endl;
		exit(1);
	}
	
#define BOOTABLEFLAG 0
#define UNBOOTABLE 0

#define FSTYPE 4

#define LBA 8
#define FSSIZE 12

	for (U32 a = 0; a < 4; a = a + 1) {
		U32 offset = PARTITIONTABLEOFFSET + a * PARTITIONTABLESIZE;
		PartitionType[a] = HDDdump[offset + FSTYPE];
		if (HDDdump[offset + FSTYPE] == NO_FS) {
			continue;
		}

		if (HDDdump[offset + BOOTABLEFLAG] == UNBOOTABLE) {
			printf("Unbootable file system #%d\n", a + 1);
		}
		
		switch (HDDdump[offset + FSTYPE]) {
		case NTFS:
			printf("Partition table #%d : NTFS\n", a + 1);
			return NTFS;
		case GPT:
			printf("Partition table #%d : GUID Partition Table\n", a + 1);
			return GPT;
		default:
			printf("Partition table #%d : UNKNOWN\n", a + 1);
			break;
		}
	}
}

U32 getHDD(U8 * HDDdump, U32 offset, U32 size) {
	HANDLE handle;
	string filename("\\\\.\\PhysicalDrive0");
	U32 nRead;
	U32 pointerRet;

	handle = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	if (handle == INVALID_HANDLE_VALUE) {
		puts("invalid handle");
		return 0;
	}
	
	pointerRet = SetFilePointer(handle, offset, NULL, FILE_BEGIN);
	if (pointerRet == INVALID_SET_FILE_POINTER) {
		puts("invalid handle");
		return 0;
	}

	if (!ReadFile(handle, HDDdump, size, (DWORD*)&nRead, NULL)) {
		return 0;
	}

	CloseHandle(handle);

	return 1;
}

void hexdump(U8 * buf, U32 size) {
	printf("        "); 
	for (U32 a = 0; a < 16; a = a + 1) {
		printf("%02X ", a);
	}
	puts("");

	for (U32 a = 0; a < 55; a = a + 1) putchar((a < 8 ? ' ' : '-'));
	puts("");

	for (U32 a = 0; a < size; a = a + 1) {
		if (a % 16 == 0) printf("0x%04X  ", a);
		printf("%02X ", buf[a]);
		if ((a + 1) % 16 == 0 || a == size - 1) puts("");
	}
	puts("");
}