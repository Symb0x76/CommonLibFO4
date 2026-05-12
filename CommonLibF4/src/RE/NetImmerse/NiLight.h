#pragma once

#include "RE/NetImmerse/NiAVObject.h"
#include "RE/NetImmerse/NiBound.h"
#include "RE/NetImmerse/NiColor.h"

namespace RE
{
	#pragma warning(push)
	#pragma warning(disable: 4324)  // structure was padded due to alignas

	class __declspec(novtable) NiLight :
		public NiAVObject  // 000
	{
	public:
		// members
		NiColor amb;                     // 120
		NiColor diff;                    // 12C
		NiColor spec;                    // 138
		float   dimmer;                  // 144
		alignas(16) NiBound modelBound;  // 150
		void* rendererData;              // 160
	};
	#pragma warning(pop)
	static_assert(sizeof(NiLight) == 0x170);
	static_assert(offsetof(NiLight, diff) == 0x12C);
	static_assert(offsetof(NiLight, modelBound) == 0x150);
}
