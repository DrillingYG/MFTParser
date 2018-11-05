//https://blog.naver.com/empty7792/221380762099

#include "stdafx.h"
#include "NTFSHeader.h"

using namespace std;

void errorMsg(string error);
void hexdump(void * buf, U32 size);
U32 getHDD(U8 * HDDdump, U64 offset, U32 size);
U32 BIOSUEFI(U8 * HDDdump, vector<U32> & PartitionType);
U64 setVBR(U8 * HDDdump, VBR &vbr);
U32 betole32(U32 num);
U64 betole64(U64 num);
void MFTParser(VBR & vbr, MFTEntry & mEntry);

int main(U32 argc, char * argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage : MFTParser.exe [MFT Number]");
		return 1;
	}
	
	U8 * HDDdump = (U8*)malloc(sizeof(U8) * SECTORSIZE);
	VBR vbr;
	vector<U32> PartitionType(4, 0);
	MFTEntry mEntry(atoi(argv[1]));
	string filename;

	if (!getHDD(HDDdump, 0, SECTORSIZE)) {
		return 1;
	}

	if (BIOSUEFI(HDDdump, PartitionType) == GPT) {
		getHDD(HDDdump, SECTORSIZE, SECTORSIZE);
		vbr.VBR_LBA = setVBR(HDDdump, vbr);
		if (vbr.VBR_LBA == -1) errorMsg("Cannot find NTFS VBR");
	}

	hexdump((U8*)&vbr, SECTORSIZE);

	MFTParser(vbr, mEntry);

	return 0;
}

void errorMsg(string error) {
	cerr << error << endl;
	exit(1);
}

U64 setVBR(U8 * HDDdump, VBR &vbr) {
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
#define MY_CDRIVE "\x00\xb0\x11\x00"

	while (true) {
		memcpy(&PT, HDDdump + GUIDoffset, sizeof(U8) * Gheader.SizeOfEntries);
		if (!memcmp(&(PT.PartitionStartingLBA), MY_CDRIVE, sizeof(U8) * 4)) {
			getHDD((U8*)(&vbr), PT.PartitionStartingLBA * SECTORSIZE, SECTORSIZE);
			return PT.PartitionStartingLBA;
		}
		
		
		GUIDoffset += Gheader.SizeOfEntries;
	}

	return 0xFFFFFFFFFFFFFFFF;
}

U64 betole64(U64 num) {
	U64 ret;
	ret = ((num & 0x00000000000000FF) << 56)
		| ((num & 0x000000000000FF00) << 40)
		| ((num & 0x0000000000FF0000) << 24)
		| ((num & 0x00000000FF000000) << 8)
		| ((num & 0x000000FF00000000) >> 8)
		| ((num & 0x0000FF0000000000) >> 24)
		| ((num & 0x00FF000000000000) >> 40)
		| ((num & 0xFF00000000000000) >> 56);
	return ret;
}

U32 betole32(U32 num) {
	U32 ret;
	ret = ((num & 0x000000FF) << 24) 
		| ((num & 0x0000FF00) << 8) 
		| ((num & 0x00FF0000) >> 8) 
		| ((num & 0xFF000000) >> 24);
	return ret;
}

void testFunc(U64 num, int cnt) {
	if (cnt == 65) return;

	testFunc(num >> 1, cnt + 1);
	printf("%d", num & 1);
}

void MFTParser (VBR &vbr, MFTEntry & mEntry) {
	cout << "target MFT entry : " << mEntry.getEntryNum() << endl;

	U64 lbaOffset;
	MFTEntry tempEntry;
	
	lbaOffset = betole64(vbr.StartClusterforMFT) * (CLUSTERSIZE / SECTORSIZE);
	getHDD((U8*)&tempEntry, lbaOffset, sizeof(MFTEntry));
	hexdump(&tempEntry, sizeof(MFTEntry));
}

U32 BIOSUEFI(U8 * HDDdump, vector<U32> & PartitionType) {
	if (memcmp(HDDdump + SECTORSIZE - 2, "\x55\xAA", sizeof(U8) * 2)) {
		cerr << "error : FileSystem Signature" << endl;
		exit(1);
	}
	
#define BOOTABLEFLAG_OFFSET 0
#define UNBOOTABLE 0

#define FSTYPE_OFFSET 4

	for (U32 a = 0; a < 4; a = a + 1) {
		U32 offset = PARTITIONTABLEOFFSET + a * PARTITIONTABLESIZE;
		PartitionType[a] = HDDdump[offset + FSTYPE_OFFSET];
		if (HDDdump[offset + FSTYPE_OFFSET] == NO_FS) {
			continue;
		}

		if (HDDdump[offset + BOOTABLEFLAG_OFFSET] == UNBOOTABLE) {
			printf("Unbootable file system #%d\n", a + 1);
		}
		
		switch (HDDdump[offset + FSTYPE_OFFSET]) {
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

U32 getHDD(U8 * HDDdump, U64 offset, U32 size) {
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

void hexdump(void * buf, U32 size) {
	printf("        "); 
	for (U32 a = 0; a < 16; a = a + 1) {
		printf("%02X ", a);
	}
	puts("");

	for (U32 a = 0; a < 55; a = a + 1) putchar((a < 8 ? ' ' : '-'));
	puts("");

	for (U32 a = 0; a < size; a = a + 1) {
		if (a % 16 == 0) printf("0x%04X  ", a);
		printf("%02X ", *((U8*)buf + a));
		if ((a + 1) % 16 == 0 || a == size - 1) puts("");
	}
	puts("");
}