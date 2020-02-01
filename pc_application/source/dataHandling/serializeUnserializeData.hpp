#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <zpp.hpp>

// Structs for the
struct Protocol_SetProjectName {
	std::string name;
};

class SerializeProtocol {
private:
	// clang-format on
	// Will be converted to bytes
	std::vector<unsigned char> serializingData;

public:
	// Both of these functions are deliberately designed to deal with any kind of struct
	template <typename T> std::shared_ptr<T> binaryToFrame(uint8_t* data, uint16_t size);

	template <typename T> void frameToBinary(std::shared_ptr<T> inputData, uint8_t* data, uint16_t* size);
};