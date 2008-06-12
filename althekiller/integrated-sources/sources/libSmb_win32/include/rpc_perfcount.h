#ifndef _RPC_PERFCOUNT_H
#define _RPC_PERFCOUNT_H

typedef struct perf_counter_definition
{
	/* sizeof(PERF_COUNTER_DEFINITION) */
	uint32 ByteLength;
	uint32 CounterNameTitleIndex;
	uint32 CounterNameTitlePointer;
	uint32 CounterHelpTitleIndex;
	uint32 CounterHelpTitlePointer;
	uint32 DefaultScale;
	uint32 DetailLevel;
	uint32 CounterType;
	uint32 CounterSize;
	uint32 CounterOffset;
}
PERF_COUNTER_DEFINITION;

typedef struct perf_counter_block
{
	/* Total size of the data block, including all data plus this header */
	uint32 ByteLength;
	uint8 *data;
}
PERF_COUNTER_BLOCK;

typedef struct perf_instance_definition
{
	/* Total size of the instance definition, including the length of the terminated Name string */
	uint32 ByteLength;
	uint32 ParentObjectTitleIndex;
	uint32 ParentObjectTitlePointer;
	uint32 UniqueID;
	/* From the start of the PERF_INSTANCE_DEFINITION, the byte offset to the start of the Name string */
	uint32 NameOffset;
	uint32 NameLength;
	/* Unicode string containing the name for the instance */
	uint8 *data;
	PERF_COUNTER_BLOCK counter_data;
}
PERF_INSTANCE_DEFINITION;

typedef struct perf_object_type
{
	/* Total size of the object block, including all PERF_INSTANCE_DEFINITIONs,
	   PERF_COUNTER_DEFINITIONs and PERF_COUNTER_BLOCKs in bytes */
	uint32 TotalByteLength;
	/* Size of this PERF_OBJECT_TYPE plus all PERF_COUNTER_DEFINITIONs in bytes */
	uint32 DefinitionLength;
	/* Size of this PERF_OBJECT_TYPE */
	uint32 HeaderLength;
	uint32 ObjectNameTitleIndex;
	uint32 ObjectNameTitlePointer;
	uint32 ObjectHelpTitleIndex;
	uint32 ObjectHelpTitlePointer;
	uint32 DetailLevel;
	uint32 NumCounters;
	uint32 DefaultCounter;
	uint32 NumInstances;
	uint32 CodePage;
	UINT64_S PerfTime;
	UINT64_S PerfFreq;
	PERF_COUNTER_DEFINITION *counters;
	PERF_INSTANCE_DEFINITION *instances;
	PERF_COUNTER_BLOCK counter_data;
}
PERF_OBJECT_TYPE;

/* PerfCounter Inner Buffer structs */
typedef struct perf_data_block
{
	/* hardcoded to read "P.E.R.F" */
	uint16 Signature[4];
	uint32 LittleEndian;
	/* both currently hardcoded to 1 */
	uint32 Version;
	uint32 Revision;
	/* bytes of PERF_OBJECT_TYPE data, does NOT include the PERF_DATA_BLOCK */
	uint32 TotalByteLength;
	/* size of PERF_DATA_BLOCK including the uint8 *data */
	uint32 HeaderLength;
	/* number of PERF_OBJECT_TYPE structures encoded */
	uint32 NumObjectTypes;
	uint32 DefaultObject;
	SYSTEMTIME SystemTime;
	/* This will guarantee that we're on a 64-bit boundary before we encode
	   PerfTime, and having it there will make my offset math much easier. */
	uint32 Padding;
	/* Now when I'm marshalling this, I'll need to call prs_align_uint64() 
	   before I start encodint the UINT64_S structs */
	/* clock rate * seconds uptime */
	UINT64_S PerfTime;
	/* The clock rate of the CPU */
	UINT64_S PerfFreq; 
	/* used for high-res timers -- for now PerfTime * 10e7 */
	UINT64_S PerfTime100nSec;
	uint32 SystemNameLength;
	uint32 SystemNameOffset;
	/* The SystemName, in unicode, terminated */
	uint8* data;
	PERF_OBJECT_TYPE *objects;
} 
PERF_DATA_BLOCK;

#endif /* _RPC_PERFCOUNT_H */
