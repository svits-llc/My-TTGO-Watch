#pragma once

typedef struct {
	uint16_t event: 4; //2^4 == 16 events
	uint16_t timeoffset : 12; //2^12 = 4096 0.1 ms intervals
} AWProfilerRecord;


typedef struct {
	AWProfilerRecord* records;
	uint16_t filled;
	uint16_t total;
	size_t last_timestamp;
} AWProfiler;

inline int aw_profiler_init(AWProfiler* profiler, size_t packet_size)
{
	if (packet_size)
	{
		profiler->records = (AWProfilerRecord*)malloc(packet_size);
		memset(profiler->records, 0, packet_size);
		profiler->total = packet_size / sizeof(AWProfilerRecord);
	}
	profiler->filled = 0;
	profiler->last_timestamp = 0;
	return 0;
}

inline int aw_profiler_sample(AWProfiler* profiler, uint8_t event, size_t microseconds)
{
	if (profiler->filled == profiler->total)
		return -1;
	AWProfilerRecord* record = &profiler->records[profiler->filled];
	record->event = event;
	size_t tenms = profiler->last_timestamp ? (microseconds - profiler->last_timestamp) / 100 : 0;
	record->timeoffset = tenms < (1 << 12) ? tenms : (1 << 12) - 1;
	++profiler->filled;
	profiler->last_timestamp = microseconds;
	return 0;
}

inline int aw_profiler_fini(AWProfiler* profiler)
{
	free(profiler->records);
	memset(profiler, 0, sizeof(AWProfiler));
	return 0;
}
