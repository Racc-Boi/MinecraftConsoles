#pragma once

#include "ArrayWithLength.h"
#include <cstdint>
#include <type_traits>
#include <cstring>
#include <algorithm>
#include <cassert>

class System
{
public:
	// Replaces multiple overloads to safely copy arrays while natively handling overlapping memory
	template <class T>
	static void arraycopy(arrayWithLength<T> src, unsigned int srcPos, arrayWithLength<T> *dst, unsigned int dstPos, unsigned int length)
	{
		assert(srcPos <= src.length);
		assert(srcPos + length <= src.length);
		assert(dstPos <= dst->length);
		assert(dstPos + length <= dst->length);

		if (length == 0) return;

		if constexpr (std::is_trivially_copyable_v<T>) 
		{
			std::memmove(dst->data + dstPos, src.data + srcPos, length * sizeof(T));
		} 
		else 
		{
			if (src.data == dst->data && srcPos < dstPos) {
				std::copy_backward(src.data + srcPos, src.data + srcPos + length, dst->data + dstPos + length);
			} else {
				std::copy(src.data + srcPos, src.data + srcPos + length, dst->data + dstPos);
			}
		}
	}

	static int64_t nanoTime();
	static int64_t currentTimeMillis();
	static int64_t currentRealTimeMillis();

	static void ReverseUSHORT(unsigned short *pusVal);
	static void ReverseSHORT(short *psVal);
	static void ReverseULONG(unsigned long *pulVal);
	static void ReverseULONG(unsigned int *pulVal);
	static void ReverseINT(int *piVal);
	static void ReverseULONGLONG(int64_t *pullVal);
	static void ReverseWCHARA(WCHAR *pwch, int iLen);
};

#define MAKE_FOURCC(ch0, ch1, ch2, ch3) \
	((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
	((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))