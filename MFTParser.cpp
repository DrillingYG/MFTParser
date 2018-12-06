//https://blog.naver.com/empty7792/221380762099
//https://github.com/DrillingYG/MFTParser
/*
1. StartClusterforMFT는 왜 자동으로 Little Endian으로 바뀌는가?
2. 여러개의 NTFS FileSystem 중에서 내 C:인 것은 어떻게 찾을 수 있는가?($Volume에서 확인 가능할 것이라고 추측)
3. static 변수를 사용해서 MFT Entry size를 하드코딩 하지 않고 사용할 수 있겠는가?
*/
#include "stdafx.h"

void errorMsg(string error);
void hexdump(void * buf, U32 size);
U32 getHDD(U8 * HDDdump, U64 offset, U32 size);
U32 BIOSUEFI(U8 * HDDdump, vector<U32> & PartitionType);
U64 setVBR(U8 * HDDdump, VBR &vbr);
void MFTParser(const VBR & vbr, MFTEntry & mEntry);

int main(U32 argc, char * argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage : MFTParser.exe [MFT Number]");
		return 1;
	}

	U8 * HDDdump = new U8[sizeof(U8) * SECTORSIZE];
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

	delete[] HDDdump;
	HDDdump = new U8[sizeof(U8) * SECTORSIZE * 2];
	
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
	delete[] HDDdump;

	return 0xFFFFFFFFFFFFFFFF;
}

void MFTParser (const VBR &vbr, MFTEntry & mEntry) {
	U64 lbaOffset;
	U8 tmpBuf[1024];
	lbaOffset = vbr.VBR_LBA * SECTORSIZE + vbr.StartClusterforMFT * CLUSTERSIZE;
	for (U32 i = 0; i < mEntry.getEntryNum(); i++) lbaOffset += 1024;
	getHDD(tmpBuf, lbaOffset, 1024);
	mEntry.setMFTEntry(tmpBuf, 1024);
	//hexdump(static_cast<void*>(mEntry.getBuf()), 1024);
	mEntry.printMftInfo();
	mEntry.printStdInfo();
	mEntry.printFileNameInfo();
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
	LONG addrHigh = (offset & 0xFFFFFFFF00000000) >> 32;
	LONG addrLow = (offset & 0xFFFFFFFF);
	handle = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	if (handle == INVALID_HANDLE_VALUE) {
		puts("invalid handle");
		return 0;
	}
	
	pointerRet = SetFilePointer(handle, addrLow, &addrHigh, FILE_BEGIN);
	if (pointerRet == INVALID_SET_FILE_POINTER) {
		puts("invalid handle");
		GetLastError();
		return 0;
	}

	if (!ReadFile(handle, HDDdump, size, (DWORD*)&nRead, NULL)) {
		return 0;
	}

	CloseHandle(handle);

	return 1;
}


