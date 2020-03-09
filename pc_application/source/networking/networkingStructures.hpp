#pragma once

#include <cstdint>
#include <zpp.hpp>

#include "../dataHandling/buttonData.hpp"

// clang-format off
#define DEFINE_STRUCT(Flag, body, ...) \
	struct Struct_##Flag : public zpp::serializer::polymorphic { \
		DataFlag flag = DataFlag::Flag; \
		body \
		friend zpp::serializer::access; \
		template <typename Archive, typename Self> static void serialize(Archive& archive, Self& self) { \
			archive(__VA_ARGS__); \
		} \
	};
// clang-format on

enum DataFlag : uint8_t {
	SendRunFrame,
	RecieveGameFramebuffer,
	RecieveGameInfo,
	SendStart,
	RecieveDone,
	NUM_OF_FLAGS,
};

enum DoneFlag : uint8_t {
	RUN_FRAME_DONE,
	FRAMEBUFFER_DONE,
};

enum StartFlag : uint8_t {
	GET_FRAMEBUFFER,
	GET_GAME_INFO,
};

// clang-format off
namespace Protocol {
	// Run a single frame and return when done
	DEFINE_STRUCT(SendRunFrame,
		ControllerData controllerData;
	, self.controllerData)

	// Recieve part of the game's framebuffer
	DEFINE_STRUCT(RecieveGameFramebuffer,
		std::vector<uint8_t> buf;
	, self.buf)

	// Recieve a ton of game and user info
	DEFINE_STRUCT(RecieveGameInfo,
		std::string userNickname;
	, self.userNickname)

	// Send start, with mostly everything as an enum value
	DEFINE_STRUCT(SendStart,
		StartFlag actFlag;
	, self.flag)

	// Recieve done, with mostly everything as an enum value
	DEFINE_STRUCT(RecieveDone,
		DoneFlag actFlag;
	, self.flag)
};
// clang-format on