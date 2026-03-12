#include "stdafx.h"
#include <chrono>

#ifdef __PS3__
#include <sys/sys_time.h>
#endif
#include "System.h"

// High-precision monotonic timer
int64_t System::nanoTime()
{
	auto now = std::chrono::steady_clock::now();
	return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}

// Wall-clock time in milliseconds
int64_t System::currentTimeMillis()
{
#ifdef __PS3__
	sys_time_sec_t sec;
	sys_time_nsec_t nsec;
	sys_time_get_current_time(&sec, &nsec);
	return (sec * 1000) + (nsec / 1000000);

#elif defined __ORBIS__
	SceRtcTick tick;
	sceRtcGetCurrentTick(&tick);
	return static_cast<int64_t>(tick.tick / 1000);

#elif defined __PSVITA__
	// AP - TRC states we can't use the RTC for measuring elapsed game time
	return sceKernelGetProcessTimeWide() / 1000;

#else
	auto now = std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
#endif 
}

// 4J Stu - Added this so that we can use real-world timestamps in PSVita saves. Particularly required for the save transfers to be smooth
int64_t System::currentRealTimeMillis()
{
#ifdef __PSVITA__
	SceFiosDate fileTime = sceFiosDateGetCurrent();
	return fileTime / 1000000;
#else
	return currentTimeMillis();
#endif
}

// Hardware-accelerated byte swapping
#if defined(_MSC_VER)
	#include <stdlib.h>
	#define BSWAP16(x) _byteswap_ushort(x)
	#define BSWAP32(x) _byteswap_ulong(x)
	#define BSWAP64(x) _byteswap_uint64(x)
#elif defined(__GNUC__) || defined(__clang__) || defined(__ORBIS__) || defined(__PSVITA__) || defined(__PS3__)
	#define BSWAP16(x) __builtin_bswap16(x)
	#define BSWAP32(x) __builtin_bswap32(x)
	#define BSWAP64(x) __builtin_bswap64(x)
#else
	static inline uint16_t BSWAP16(uint16_t x) { return (x << 8) | (x >> 8); }
	static inline uint32_t BSWAP32(uint32_t x) { return ((x & 0xFF000000) >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) | ((x & 0x000000FF) << 24); }
	static inline uint64_t BSWAP64(uint64_t x) { return ((x & 0xFF00000000000000ULL) >> 56) | ((x & 0x00FF000000000000ULL) >> 40) | ((x & 0x0000FF0000000000ULL) >> 24) | ((x & 0x000000FF00000000ULL) >> 8) | ((x & 0x00000000FF000000ULL) << 8) | ((x & 0x0000000000FF0000ULL) << 24) | ((x & 0x000000000000FF00ULL) << 40) | ((x & 0x00000000000000FFULL) << 56); }
#endif

void System::ReverseUSHORT(unsigned short *pusVal)   { *pusVal = BSWAP16(*pusVal); }
void System::ReverseSHORT(short *pusVal)             { *pusVal = static_cast<short>(BSWAP16(static_cast<uint16_t>(*pusVal))); }
void System::ReverseULONG(unsigned long *pulVal)     { *pulVal = static_cast<unsigned long>(BSWAP32(static_cast<uint32_t>(*pulVal))); }
void System::ReverseULONG(unsigned int *pulVal)      { *pulVal = BSWAP32(*pulVal); }
void System::ReverseINT(int *piVal)                  { *piVal = static_cast<int>(BSWAP32(static_cast<uint32_t>(*piVal))); }
void System::ReverseULONGLONG(int64_t *pullVal)      { *pullVal = static_cast<int64_t>(BSWAP64(static_cast<uint64_t>(*pullVal))); }

void System::ReverseWCHARA(WCHAR *pwch, int iLen)
{
	for(int i = 0; i < iLen; ++i)
	{
		pwch[i] = static_cast<WCHAR>(BSWAP16(static_cast<uint16_t>(pwch[i])));
	}
}