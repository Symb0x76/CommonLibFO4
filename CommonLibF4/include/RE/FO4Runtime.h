#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "REX/W32/KERNEL32.h"
#include "REL/Relocation.h"
#if defined(FALLOUT_POST_AE)
#include "REX/FModule.h"
#endif

struct ID3D11SamplerState;

namespace RE
{
	class BSRenderPass;
	class NiLight;
	class ShadowSceneNode;

	namespace BSGraphics
	{
		class RenderTargetManager;
		class State;
	}
}

namespace RE::FO4Runtime
{
	enum class RuntimeVariant : std::uint8_t
	{
		kUnknown,
		kPreNG,
		kPostNG,
		kPostAE
	};

#if defined(FALLOUT_POST_AE)
	inline constexpr RuntimeVariant RUNTIME_VARIANT = RuntimeVariant::kPostAE;
#elif defined(FALLOUT_POST_NG)
	inline constexpr RuntimeVariant RUNTIME_VARIANT = RuntimeVariant::kPostNG;
#elif defined(FALLOUT_PRE_NG)
	inline constexpr RuntimeVariant RUNTIME_VARIANT = RuntimeVariant::kPreNG;
#else
	inline constexpr RuntimeVariant RUNTIME_VARIANT = RuntimeVariant::kUnknown;
#endif

	inline constexpr std::uintptr_t PRE_NG_STATIC_IMAGE_BASE = 0x140000000ull;

	namespace Win32
	{
		using MemoryBasicInformation = REX::W32::MEMORY_BASIC_INFORMATION;

		inline constexpr std::uint32_t kMemCommit = 0x00001000u;
		inline constexpr std::uint32_t kPageNoAccess = 0x00000001u;
		inline constexpr std::uint32_t kPageGuard = 0x00000100u;
		inline constexpr std::uint32_t kPageReadWrite = 0x00000004u;
		inline constexpr std::uint32_t kPageWriteCopy = 0x00000008u;
		inline constexpr std::uint32_t kPageExecuteReadWrite = 0x00000040u;
		inline constexpr std::uint32_t kPageExecuteWriteCopy = 0x00000080u;

		[[nodiscard]] inline std::size_t VirtualQuery(
			const void* a_address,
			MemoryBasicInformation* a_buffer,
			std::size_t a_length) noexcept
		{
			return REX::W32::VirtualQuery(a_address, a_buffer, a_length);
		}
	}

	[[nodiscard]] inline std::uintptr_t ModuleBase()
	{
#if defined(FALLOUT_POST_AE)
		return REX::FModule::GetExecutingModule().GetBaseAddress();
#else
		return static_cast<std::uintptr_t>(REL::Module::get().base());
#endif
	}

	[[nodiscard]] inline std::uintptr_t RuntimeAddress(
		std::uintptr_t a_staticVA,
		std::uintptr_t a_staticImageBase = PRE_NG_STATIC_IMAGE_BASE)
	{
		return ModuleBase() + (a_staticVA - a_staticImageBase);
	}

	[[nodiscard]] inline bool IsReadableAddress(std::uintptr_t a_address, std::size_t a_size)
	{
		Win32::MemoryBasicInformation mbi{};
		if (Win32::VirtualQuery(reinterpret_cast<const void*>(a_address), &mbi, sizeof(mbi)) == 0) {
			return false;
		}

		const auto regionBegin = reinterpret_cast<std::uintptr_t>(mbi.baseAddress);
		const auto regionEnd = regionBegin + mbi.regionSize;
		const auto readEnd = a_address + a_size;
		if (readEnd < a_address || mbi.state != Win32::kMemCommit || a_address < regionBegin || readEnd > regionEnd) {
			return false;
		}

		return (mbi.protect & (Win32::kPageNoAccess | Win32::kPageGuard)) == 0;
	}

	[[nodiscard]] inline bool IsWritableAddress(std::uintptr_t a_address, std::size_t a_size)
	{
		Win32::MemoryBasicInformation mbi{};
		if (Win32::VirtualQuery(reinterpret_cast<const void*>(a_address), &mbi, sizeof(mbi)) == 0) {
			return false;
		}

		const auto regionBegin = reinterpret_cast<std::uintptr_t>(mbi.baseAddress);
		const auto regionEnd = regionBegin + mbi.regionSize;
		const auto writeEnd = a_address + a_size;
		if (writeEnd < a_address || mbi.state != Win32::kMemCommit || a_address < regionBegin || writeEnd > regionEnd) {
			return false;
		}
		if ((mbi.protect & (Win32::kPageNoAccess | Win32::kPageGuard)) != 0) {
			return false;
		}

		const auto protect = mbi.protect & 0xFF;
		return protect == Win32::kPageReadWrite ||
		       protect == Win32::kPageWriteCopy ||
		       protect == Win32::kPageExecuteReadWrite ||
		       protect == Win32::kPageExecuteWriteCopy;
	}

	template <class T>
	[[nodiscard]] inline bool ReadValue(std::uintptr_t a_address, T& a_value)
	{
		static_assert(std::is_trivially_copyable_v<T>);
		if (!IsReadableAddress(a_address, sizeof(T))) {
			return false;
		}

		std::memcpy(&a_value, reinterpret_cast<const void*>(a_address), sizeof(T));
		return true;
	}

	template <class T>
	[[nodiscard]] inline T ReadValueOr(std::uintptr_t a_address, T a_fallback = {})
	{
		T value{};
		return ReadValue(a_address, value) ? value : a_fallback;
	}

	template <class T>
	[[nodiscard]] inline bool WriteValue(std::uintptr_t a_address, const T& a_value)
	{
		static_assert(std::is_trivially_copyable_v<T>);
		if (!IsWritableAddress(a_address, sizeof(T))) {
			return false;
		}

		std::memcpy(reinterpret_cast<void*>(a_address), &a_value, sizeof(T));
		return true;
	}

	[[nodiscard]] inline std::uintptr_t ReadPointer(std::uintptr_t a_address)
	{
		return ReadValueOr<std::uintptr_t>(a_address, 0);
	}

	struct RuntimeAddressValue
	{
		std::uintptr_t staticVA;

		[[nodiscard]] std::uintptr_t address() const
		{
			return RuntimeAddress(staticVA);
		}

		template <class T>
		[[nodiscard]] bool read(T& a_value) const
		{
			return ReadValue(address(), a_value);
		}

		template <class T>
		[[nodiscard]] bool write(const T& a_value) const
		{
			return WriteValue(address(), a_value);
		}
	};

	struct RuntimeRVAValue
	{
		std::uintptr_t rva;

		[[nodiscard]] std::uintptr_t address() const
		{
			return ModuleBase() + rva;
		}
	};

	struct RelocationID
	{
		REL::ID id;
		std::ptrdiff_t offset = 0;

		[[nodiscard]] std::uintptr_t address() const
		{
			return id.address() + offset;
		}
	};

	struct NamedRelocationID
	{
		const char* name;
		REL::ID id;
	};

	struct NamedRuntimeRVA
	{
		const char* name;
		RuntimeRVAValue location;
	};

	struct RuntimeField
	{
		std::uintptr_t offset;

		[[nodiscard]] std::uintptr_t address(std::uintptr_t a_base) const
		{
			return a_base + offset;
		}

		template <class T>
		[[nodiscard]] bool read(std::uintptr_t a_base, T& a_value) const
		{
			return ReadValue(address(a_base), a_value);
		}

		template <class T>
		[[nodiscard]] bool write(std::uintptr_t a_base, const T& a_value) const
		{
			return WriteValue(address(a_base), a_value);
		}
	};

	namespace IDs
	{
#if defined(FALLOUT_POST_NG)
		inline constexpr REL::ID GRAPHICS_STATE{ 2704621 };
		inline constexpr REL::ID RENDER_TARGET_MANAGER{ 2666735 };
		inline constexpr REL::ID SAMPLER_STATE_ARRAY{ 2704455 };
		inline constexpr REL::ID TAA_ENABLE_FLAG{ 2704658 };
		inline constexpr REL::ID CAMERA_NEAR{ 2712882 };
		inline constexpr REL::ID CAMERA_FAR{ 2712883 };
#else
		inline constexpr REL::ID GRAPHICS_STATE{ 600795 };
		inline constexpr REL::ID RENDER_TARGET_MANAGER{ 1508457 };
		inline constexpr REL::ID SAMPLER_STATE_ARRAY{ 44312 };
		inline constexpr REL::ID TAA_ENABLE_FLAG{ 460417 };
		inline constexpr REL::ID CAMERA_NEAR{ 57985 };
		inline constexpr REL::ID CAMERA_FAR{ 958877 };
#endif
	}

	[[nodiscard]] inline BSGraphics::State* GetGraphicsState()
	{
		static REL::Relocation<BSGraphics::State*> ptr{ IDs::GRAPHICS_STATE };
		return ptr.get();
	}

	[[nodiscard]] inline BSGraphics::RenderTargetManager* GetRenderTargetManager()
	{
		static REL::Relocation<BSGraphics::RenderTargetManager*> ptr{ IDs::RENDER_TARGET_MANAGER };
		return ptr.get();
	}

	[[nodiscard]] inline ID3D11SamplerState** GetSamplerStateArray()
	{
		static REL::Relocation<ID3D11SamplerState**> ptr{ IDs::SAMPLER_STATE_ARRAY };
		return ptr.get();
	}

	[[nodiscard]] inline bool* GetTAAEnableFlag()
	{
		static REL::Relocation<bool*> ptr{ IDs::TAA_ENABLE_FLAG };
		return ptr.get();
	}

	[[nodiscard]] inline float* GetCameraNearPtr()
	{
		static REL::Relocation<float*> ptr{ IDs::CAMERA_NEAR };
		return ptr.get();
	}

	[[nodiscard]] inline float* GetCameraFarPtr()
	{
		static REL::Relocation<float*> ptr{ IDs::CAMERA_FAR };
		return ptr.get();
	}

	[[nodiscard]] inline float GetCameraNear()
	{
		return *GetCameraNearPtr();
	}

	[[nodiscard]] inline float GetCameraFar()
	{
		return *GetCameraFarPtr();
	}

	namespace PreNG
	{
		// TODO(PostNG LLF): This is the verified PreNG 1.10.163 layout.
		// Keep it for PreNG source compatibility only; do not use these raw
		// addresses or field offsets as PostNG LLF runtime data.
		inline constexpr RuntimeAddressValue BS_LIGHTING_SHADER_SETUP_GEOMETRY{ 0x14289DD10ull };
		inline constexpr RuntimeAddressValue POINT_LIGHT_CALL{ 0x14289E0BFull };
		inline constexpr RuntimeAddressValue POINT_LIGHT_TARGET{ 0x14289F550ull };
		inline constexpr RuntimeAddressValue BS_LIGHTING_SHADER_VTABLE{ 0x14309AAB8ull };
		inline constexpr RuntimeAddressValue BS_LIGHTING_SHADER_VFUNC_7{ 0x14309AAF0ull };
		inline constexpr RuntimeAddressValue BS_LIGHTING_SHADER_SETUP_TECHNIQUE{ 0x14289CEC0ull };
		inline constexpr RuntimeAddressValue BS_SHADER_LOOKUP{ 0x142891DB0ull };
		inline constexpr RuntimeAddressValue BIND_SHADERS{ 0x141D10400ull };
		inline constexpr RuntimeAddressValue RENDERER_STATE{ 0x1461E0900ull };
		inline constexpr RuntimeAddressValue SHADOW_SCENE_NODE_ARRAY{ 0x146721B70ull };
		inline constexpr RuntimeAddressValue SHADOW_SCENE_NODE_CURRENT_INDEX{ 0x146721C19ull };
		inline constexpr RuntimeAddressValue CURRENT_VERTEX_SHADER_ENTRY{ 0x146732E10ull };
		inline constexpr RuntimeAddressValue CURRENT_HULL_SHADER_ENTRY{ 0x146732E18ull };
		inline constexpr RuntimeAddressValue CURRENT_DOMAIN_SHADER_ENTRY{ 0x146732E20ull };
		inline constexpr RuntimeAddressValue CURRENT_PIXEL_SHADER_ENTRY{ 0x146732E28ull };

		inline constexpr std::array<std::uint8_t, 13> POINT_LIGHT_CALL_CONTEXT{
			0xC7, 0x44, 0x24, 0x20, 0x00, 0x00, 0x00, 0x00,
			0xE8, 0x8C, 0x14, 0x00, 0x00
		};

		inline constexpr std::uint32_t BS_LIGHTING_SHADER_TYPE = 8;
		inline constexpr std::uint32_t DF_LIGHTING_SHADER_TYPE = 4;
		inline constexpr std::uint32_t DF_LIGHT_FULL_SHADOWED_PIXEL_DESCRIPTOR_920 = 0x09200202;
		inline constexpr std::uint32_t DF_LIGHT_FULL_SHADOWED_PIXEL_DESCRIPTOR_922 = 0x09220202;

		[[nodiscard]] inline constexpr bool IsDFLightFullShadowedPixelDescriptor(std::uint32_t a_descriptor)
		{
			return a_descriptor == DF_LIGHT_FULL_SHADOWED_PIXEL_DESCRIPTOR_920 ||
			       a_descriptor == DF_LIGHT_FULL_SHADOWED_PIXEL_DESCRIPTOR_922;
		}

		inline constexpr RuntimeField BS_RENDER_PASS_SCENE_LIGHTS{ 0x30 };
		inline constexpr RuntimeField BS_RENDER_PASS_RAW_LIGHT_COUNT{ 0x50 };
		inline constexpr std::uint32_t BS_RENDER_PASS_SCENE_LIGHT_FIRST_INDEX = 1;

		inline constexpr RuntimeField BS_LIGHT_WRAPPER_FADE{ 0x10 };
		inline constexpr RuntimeField BS_LIGHT_WRAPPER_NI_LIGHT{ 0xB8 };
		inline constexpr RuntimeField BS_SHADOW_LIGHT_MASK_INDEX{ 0x1B0 };
		inline constexpr std::uint32_t INVALID_SHADOW_LIGHT_MASK_INDEX = 255;
		inline constexpr std::uint32_t MAX_SHADOW_LIGHT_MASK_BITS = 32;

		inline constexpr RuntimeField NI_LIGHT_WORLD_TRANSLATE{ 0xA0 };
		inline constexpr RuntimeField NI_LIGHT_DIFFUSE{ 0x12C };
		inline constexpr RuntimeField NI_LIGHT_RADIUS{ 0x138 };
		inline constexpr RuntimeField NI_LIGHT_DIMMER{ 0x144 };

		inline constexpr std::uint32_t SHADOW_SCENE_NODE_ARRAY_SLOTS = 21;
		inline constexpr RuntimeField SHADOW_SCENE_NODE_ACTIVE_LIGHTS{ 0x158 };
		inline constexpr RuntimeField SHADOW_SCENE_NODE_ACTIVE_LIGHTS_COUNT{ 0x168 };
		inline constexpr RuntimeField SHADOW_SCENE_NODE_ACTIVE_SHADOW_LIGHTS{ 0x170 };
		inline constexpr RuntimeField SHADOW_SCENE_NODE_ACTIVE_SHADOW_LIGHTS_COUNT{ 0x180 };
		inline constexpr RuntimeField SHADOW_SCENE_NODE_ACTIVE_EXTRA_LIGHTS{ 0x188 };
		inline constexpr RuntimeField SHADOW_SCENE_NODE_ACTIVE_EXTRA_LIGHTS_COUNT{ 0x198 };

		inline constexpr RuntimeField SHADER_ENTRY_D3D_OBJECT{ 0x8 };
		inline constexpr RuntimeField PIXEL_SHADER_SLOT_88{ 88 };
		inline constexpr RuntimeField PIXEL_SHADER_SLOT_89{ 89 };
		inline constexpr RuntimeField PIXEL_SHADER_SLOT_94{ 94 };
		inline constexpr RuntimeField PIXEL_SHADER_SLOT_96{ 96 };

		[[nodiscard]] inline std::uint32_t NormalizeLightingVertexDescriptor(std::uint32_t a_descriptor)
		{
			return a_descriptor & 0x3F0F;
		}

		[[nodiscard]] inline std::uint32_t NormalizeLightingPixelDescriptor(std::uint32_t a_descriptor)
		{
			if ((a_descriptor & 4u) == 0) {
				a_descriptor &= ~2u;
			}
			return a_descriptor | 1u;
		}

		namespace Hooks
		{
			inline constexpr REL::ID UPSCALER_WINDOW_SIZE_CHANGED{ 212827 };
			inline constexpr RelocationID UPSCALER_SET_DEFAULT_VIEWPORT_CALL{ REL::ID(587723), 0xE1 };
			inline constexpr REL::ID UPSCALER_DRAW_WORLD_FORWARD{ 656535 };
			inline constexpr RelocationID UPSCALER_DRAW_WORLD_RETICLE_CALL{ REL::ID(338205), 0x253 };

			inline constexpr RelocationID UPSCALER_RENDER_BACKEND_DYNAMIC_RESOLUTION_CALL{ REL::ID(984743), 0x14B };
			inline constexpr RelocationID UPSCALER_RENDER_BACKEND_DEFERRED_PREPASS_CALL{ REL::ID(984743), 0x17F };
			inline constexpr RelocationID UPSCALER_RENDER_BACKEND_FORWARD_CALL{ REL::ID(984743), 0x1C9 };

			inline constexpr std::array<NamedRelocationID, 4> DEFERRED_PIPELINE{
				NamedRelocationID{ "Main_RenderShadowMaps", REL::ID(620025) },
				NamedRelocationID{ "Main_RenderWorld_Start", REL::ID(1108521) },
				NamedRelocationID{ "Main_RenderWorld_BlendedDecals", REL::ID(465756) },
				NamedRelocationID{ "Renderer_ResetState", REL::ID(153957) }
			};

			inline constexpr std::array<NamedRuntimeRVA, 4> DEFERRED_SCAN_TARGETS{
				NamedRuntimeRVA{ "ShadowMaps", RuntimeRVAValue{ 0x2850B1Bull } },
				NamedRuntimeRVA{ "BlendedDecals", RuntimeRVAValue{ 0x2851BBCull } },
				NamedRuntimeRVA{ "World_Start", RuntimeRVAValue{ 0x28529B0ull } },
				NamedRuntimeRVA{ "ResetState", RuntimeRVAValue{ 0x1EB7BA0ull } }
			};
		}
	}

	namespace PostNG
	{
		// TODO(PostNG LLF): Add typed PostNG runtime data for BSRenderPass light
		// tables, BSShadowLight maskIndex, ShadowSceneNode active buckets,
		// BSShaderLookup/BindShaders globals, and shader entry D3D object fields.
		// The entries below only cover hooks already consumed by FO4CS.
		namespace Hooks
		{
			inline constexpr REL::ID UPSCALER_WINDOW_SIZE_CHANGED{ 2276824 };
			inline constexpr RelocationID UPSCALER_SET_DEFAULT_VIEWPORT_CALL{ REL::ID(2318322), 0xC5 };
			inline constexpr REL::ID UPSCALER_DRAW_WORLD_FORWARD{ 2318315 };
			inline constexpr RelocationID UPSCALER_DRAW_WORLD_RETICLE_CALL{ REL::ID(2318315), 0x53D };

			inline constexpr RelocationID UPSCALER_RENDER_BACKEND_DYNAMIC_RESOLUTION_CALL{ REL::ID(2318321), 0x29F };
			inline constexpr RelocationID UPSCALER_RENDER_BACKEND_DEFERRED_PREPASS_CALL{ REL::ID(2318321), 0x2E3 };
			inline constexpr RelocationID UPSCALER_RENDER_BACKEND_FORWARD_CALL{ REL::ID(2318321), 0x3A6 };

			inline constexpr REL::ID DEFERRED_MAIN_RENDER_WORLD{ 2318315 };
			inline constexpr REL::ID DEFERRED_MAIN_RENDER_SHADOW_MAPS{ 2318298 };
			inline constexpr REL::ID DEFERRED_MAIN_RENDER_WORLD_START{ 2318312 };
			inline constexpr REL::ID DEFERRED_MAIN_RENDER_WORLD_BLENDED_DECALS{ 2318306 };
			inline constexpr REL::ID DEFERRED_RENDERER_RESET_STATE{ 2276833 };

			inline constexpr std::array<NamedRelocationID, 5> DEFERRED_PIPELINE{
				NamedRelocationID{ "Main_RenderWorld", DEFERRED_MAIN_RENDER_WORLD },
				NamedRelocationID{ "Main_RenderShadowMaps", DEFERRED_MAIN_RENDER_SHADOW_MAPS },
				NamedRelocationID{ "Main_RenderWorld_Start", DEFERRED_MAIN_RENDER_WORLD_START },
				NamedRelocationID{ "Main_RenderWorld_BlendedDecals", DEFERRED_MAIN_RENDER_WORLD_BLENDED_DECALS },
				NamedRelocationID{ "Renderer_ResetState", DEFERRED_RENDERER_RESET_STATE }
			};
		}
	}

	struct PreNGShaderEntryState
	{
		std::uintptr_t entry = 0;
		std::uintptr_t d3dObject = 0;
		std::uint32_t id = 0;
		std::uint8_t slot88 = 0xFF;
		std::uint8_t slot89 = 0xFF;
		std::uint8_t slot94 = 0xFF;
		std::uint8_t slot96 = 0xFF;
		bool entryReadable = false;
	};

	[[nodiscard]] inline std::uintptr_t ReadPreNGShaderEntryD3DObject(std::uintptr_t a_entry)
	{
		if (a_entry == 0) {
			return 0;
		}

		return ReadPointer(PreNG::SHADER_ENTRY_D3D_OBJECT.address(a_entry));
	}

	[[nodiscard]] inline PreNGShaderEntryState ReadPreNGPixelShaderEntryState()
	{
		PreNGShaderEntryState result{};
		if (!PreNG::CURRENT_PIXEL_SHADER_ENTRY.read(result.entry) || result.entry == 0) {
			return result;
		}

		bool readable = true;
		readable &= ReadValue(result.entry, result.id);
		readable &= PreNG::SHADER_ENTRY_D3D_OBJECT.read(result.entry, result.d3dObject);
		readable &= PreNG::PIXEL_SHADER_SLOT_88.read(result.entry, result.slot88);
		readable &= PreNG::PIXEL_SHADER_SLOT_89.read(result.entry, result.slot89);
		readable &= PreNG::PIXEL_SHADER_SLOT_94.read(result.entry, result.slot94);
		readable &= PreNG::PIXEL_SHADER_SLOT_96.read(result.entry, result.slot96);
		result.entryReadable = readable;
		return result;
	}

	struct PreNGBSRenderPassView
	{
		std::uintptr_t address = 0;

		explicit PreNGBSRenderPassView(const BSRenderPass* a_pass) :
			address(reinterpret_cast<std::uintptr_t>(a_pass))
		{}

		[[nodiscard]] bool ReadSceneLights(std::uintptr_t& a_sceneLights) const
		{
			return PreNG::BS_RENDER_PASS_SCENE_LIGHTS.read(address, a_sceneLights);
		}

		[[nodiscard]] bool ReadRawLightCount(std::uint8_t& a_rawLightCount) const
		{
			return PreNG::BS_RENDER_PASS_RAW_LIGHT_COUNT.read(address, a_rawLightCount);
		}

		[[nodiscard]] bool ReadSceneLightWrapper(std::uint32_t a_logicalIndex, std::uintptr_t& a_wrapper) const
		{
			std::uintptr_t sceneLights = 0;
			if (!ReadSceneLights(sceneLights) || sceneLights == 0) {
				return false;
			}

			const auto physicalIndex = a_logicalIndex + PreNG::BS_RENDER_PASS_SCENE_LIGHT_FIRST_INDEX;
			return ReadValue(sceneLights + (physicalIndex * sizeof(std::uintptr_t)), a_wrapper);
		}
	};

	struct PreNGLightWrapperView
	{
		std::uintptr_t address = 0;

		[[nodiscard]] bool ReadFade(float& a_fade) const
		{
			return PreNG::BS_LIGHT_WRAPPER_FADE.read(address, a_fade);
		}

		[[nodiscard]] bool ReadNiLight(std::uintptr_t& a_niLight) const
		{
			return PreNG::BS_LIGHT_WRAPPER_NI_LIGHT.read(address, a_niLight);
		}

		[[nodiscard]] bool ReadShadowMaskIndex(std::uint32_t& a_maskIndex) const
		{
			return PreNG::BS_SHADOW_LIGHT_MASK_INDEX.read(address, a_maskIndex);
		}
	};

	struct PreNGNiLightData
	{
		float position[3]{};
		float diffuse[3]{};
		float radius = 0.0f;
		float dimmer = 0.0f;
	};

	struct PreNGNiLightView
	{
		std::uintptr_t address = 0;

		[[nodiscard]] bool Read(PreNGNiLightData& a_data) const
		{
			return PreNG::NI_LIGHT_DIFFUSE.read(address, a_data.diffuse[0]) &&
			       ReadValue(PreNG::NI_LIGHT_DIFFUSE.address(address) + sizeof(float), a_data.diffuse[1]) &&
			       ReadValue(PreNG::NI_LIGHT_DIFFUSE.address(address) + (2 * sizeof(float)), a_data.diffuse[2]) &&
			       PreNG::NI_LIGHT_RADIUS.read(address, a_data.radius) &&
			       PreNG::NI_LIGHT_DIMMER.read(address, a_data.dimmer) &&
			       PreNG::NI_LIGHT_WORLD_TRANSLATE.read(address, a_data.position[0]) &&
			       ReadValue(PreNG::NI_LIGHT_WORLD_TRANSLATE.address(address) + sizeof(float), a_data.position[1]) &&
			       ReadValue(PreNG::NI_LIGHT_WORLD_TRANSLATE.address(address) + (2 * sizeof(float)), a_data.position[2]);
		}
	};

	struct PreNGShadowSceneNodeRef
	{
		std::uintptr_t node = 0;
		std::uint8_t selectedIndex = 0;
		std::uint8_t currentIndex = 0;
		bool currentIndexRead = false;
		bool usedFallback = false;
	};

	struct PreNGShadowSceneBucket
	{
		std::uintptr_t entries = 0;
		std::uint32_t count = 0;
	};

	struct PreNGShadowSceneBuckets
	{
		PreNGShadowSceneBucket active;
		PreNGShadowSceneBucket shadow;
		PreNGShadowSceneBucket extra;
	};

	[[nodiscard]] inline PreNGShadowSceneNodeRef GetPreNGWorldShadowSceneNode()
	{
		PreNGShadowSceneNodeRef result{};
		const auto arrayBase = PreNG::SHADOW_SCENE_NODE_ARRAY.address();
		if (arrayBase == 0) {
			return result;
		}

		if (PreNG::SHADOW_SCENE_NODE_CURRENT_INDEX.read(result.currentIndex)) {
			result.currentIndexRead = true;
			if (result.currentIndex < PreNG::SHADOW_SCENE_NODE_ARRAY_SLOTS) {
				std::uintptr_t indexedNode = 0;
				const auto indexedSlot = arrayBase + (static_cast<std::uintptr_t>(result.currentIndex) * sizeof(std::uintptr_t));
				if (ReadValue(indexedSlot, indexedNode) && indexedNode != 0) {
					result.node = indexedNode;
					result.selectedIndex = result.currentIndex;
					return result;
				}
			}
		}

		std::uintptr_t fallbackNode = 0;
		if (ReadValue(arrayBase, fallbackNode)) {
			result.node = fallbackNode;
			result.selectedIndex = 0;
			result.usedFallback = result.currentIndexRead && result.currentIndex != 0;
		}

		return result;
	}

	struct PreNGShadowSceneNodeView
	{
		std::uintptr_t address = 0;

		[[nodiscard]] bool ReadBuckets(PreNGShadowSceneBuckets& a_buckets) const
		{
			return PreNG::SHADOW_SCENE_NODE_ACTIVE_LIGHTS.read(address, a_buckets.active.entries) &&
			       PreNG::SHADOW_SCENE_NODE_ACTIVE_LIGHTS_COUNT.read(address, a_buckets.active.count) &&
			       PreNG::SHADOW_SCENE_NODE_ACTIVE_SHADOW_LIGHTS.read(address, a_buckets.shadow.entries) &&
			       PreNG::SHADOW_SCENE_NODE_ACTIVE_SHADOW_LIGHTS_COUNT.read(address, a_buckets.shadow.count) &&
			       PreNG::SHADOW_SCENE_NODE_ACTIVE_EXTRA_LIGHTS.read(address, a_buckets.extra.entries) &&
			       PreNG::SHADOW_SCENE_NODE_ACTIVE_EXTRA_LIGHTS_COUNT.read(address, a_buckets.extra.count);
		}
	};
}
