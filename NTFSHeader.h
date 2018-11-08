#pragma once
#include <cstdint>
#include <cstring>

#define SECTORSIZE 512
#define CLUSTERSIZE 4096
#define PARTITIONTABLESIZE 16
#define PARTITIONTABLEOFFSET 446
#define PARTITION_ENTRY_SIZE 128

#define NTFS 0x07
#define GPT 0xEE
#define NO_FS 0x00

typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t U8;

#pragma pack(1)
typedef struct __GPTHeader {
	U8 Signature[8];
	U8 Revision[4];
	U32 HeaderSize;
	U32 HEaderChksum;
	U32 Reserved;
	U64 LBAofGPTHeader;
	U64 LBAofbkpGPTHeader;
	U64 StartingLBAforPartitions;
	U64 EndingLBAforPratitions;
	U8 GUID[16];
	U64 StartingLBAParitionTable;
	U32 NumOfPartitionEntries;
	U32 SizeOfEntries;
	U32 PartitionTableChksum;
	U8 Padding[420];
} GPTHeader;

typedef struct __GPTable {
	U8 PartitonTypeGUID[16];
	U8 PartitionGUID[16];
	U64 PartitionStartingLBA;
	U64 PartitionEndeingLBA;
	U64 PartitionAtts;
	U8 PartitonName[72];
} GPTable;

typedef struct __BootRecord {
	U8 JumpBootCode[3];
	U8 OEM_ID[8];
	U16 BytesPerSector;
	U8 SecPerClus;
	U16 ReservedSectors;
	U8 Unused1[5];
	U8 Media;
	U8 Unused2[18];
	U64 TotalSectors;
	U64 StartClusterforMFT;
	U64 StartClusterforMFTMirr;
	U32 ClusPerEntry;
	U32 ClusPerIndex;
	U64 VolumeSerialNumber;
	U8 Unused[430];
	U16 Siganture;
	U64 BytesPerClus;
	U64 VBR_LBA;
} VBR;

struct MFTHeader{
	U8 Signature[4];
	U16 FixupArrOffset;
	U16 FixupArrEntries;
	U64 LSN;
	U16 SeqNum;
	U16 HardlinkCnt;
	U16 FileAttrOffset;
	U16 Flags;
	U32 RealSizeofMFTEntry;
	U32 AllocatedSizeofMFTEntry;
	U64 FileReference;
	U16 NextAttrID;
	U8 Unused[6];
};

struct FixupArr {
	U16 arrEntries[4];
};

struct attrCommonHeader {
	U32 AttrtypeID;
	U32 lenOfAttr;
	U8 Nregflag;
	U8 nameLen;
	U16 OffsettoName;
	U16 Flags;
	U16 attrID;
};

class MFTEntry{
public:
	MFTEntry(void) = default;
	MFTEntry(U32 targetNum) :MyNum(targetNum) {}
	
	U32 getEntryNum(void) const { return MyNum; }

	void setMFTEntry(void * buf, uint32_t MFTSize) {
		setBuf(MFTSize-sizeof(struct MFTHeader) - sizeof(struct FixupArr));
		memset(this, 0, MFTSize);
		memcpy_s(this, MFTSize, buf, MFTSize);
	}

	void setBuf(U32 others) {
		
	}

private:
	struct MFTHeader;
	struct FixupArr;
	U8 * Buf;
	U32 MyNum;
};

#pragma pack()

