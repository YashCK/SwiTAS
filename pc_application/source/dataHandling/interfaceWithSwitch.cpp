#include "interfaceWithSwitch.hpp"

// Decided upon using https://github.com/DFHack/clsocket

bool CommunicateWithSwitch::readData(uint8_t* data, uint16_t sizeToRead) {
	// Info about pointers here: https://stackoverflow.com/a/4318446
	// Will return true on error
	uint16_t numOfBytesSoFar = 0;
	while(numOfBytesSoFar != sizeToRead) {
		// Have to read at the right index with the right num of bytes
		int res = serverConnection.Receive(sizeToRead - numOfBytesSoFar, &data[numOfBytesSoFar]);
		if(res == 0 || res == -1) {
			// Socket has either been shutdown on other side
			// Or has encountered an error
			handleSocketError(res);
			return true;
		} else {
			// Just num of bytes
			// Now have to read this less bytes next time
			numOfBytesSoFar += res;
		}
	}
	// If here, success! Operation has encountered no error
	return false;
}

CommunicateWithSwitch::CommunicateWithSwitch() {
	// Should keep reading network at the beginning
	keepReading = true;

	// Start the thread, this means that this class goes on the main thread
	networkThread = std::make_shared<std::thread>(&CommunicateWithSwitch::initNetwork, this);
}

void CommunicateWithSwitch::initNetwork() {
	connectedToServer = false;
	serverConnection.Initialize();

	// Set to blocking for all data
	serverConnection.SetBlocking();

	// Block for 3 seconds to recieve a byte
	// Within 3 seconds
	serverConnection.SetReceiveTimeout(3);

	// Wait until string is good
	std::unique_lock<std::mutex> lk(ipMutex);
	/* To set IP, use
		std::lock_guard<std::mutex> lk(ipMutex);
		ipAddressServer = "[WHATEVER IP]";
		lk.unlock();
		cv.notify_one();
	*/
	// Wait for IP address of server to be set
	cv.wait(lk, [this] { return connectedToServer.load(); });
	// Unlock lock
	lk.unlock();

	// This will loop forever until keepReading is set to false
	listenForSwitchCommands();
}

void CommunicateWithSwitch::endNetwork() {
	// Stop reading
	keepReading = false;
	// Wait for thread to end
	networkThread->join();
}

bool CommunicateWithSwitch::handleSocketError(int res) {
	// Return true if it's an error, fatal or wouldblock
	//   false if there is no error
	if(res == -1) {
		serverConnection.TranslateSocketError();
		CSimpleSocket::CSocketError error = serverConnection.GetSocketError();
		// It's okay if it would have blocked, just means there is no data
		if(error != CSimpleSocket::SocketEwouldblock) {
			// Would block is harmless
			puts(serverConnection.DescribeError());
		}
		return true;
	} else {
		return false;
	}
}

void CommunicateWithSwitch::attemptConnectionToServer(const char* ip) {
	if(!serverConnection.Open(ip, SERVER_PORT)) {
		// There was an error
		connectedToServer = false;
	} else {
		connectedToServer = true;
		// We good, let the network thread know
		cv.notify_one();
	}
}

void CommunicateWithSwitch::listenForSwitchCommands() {
	// Socket connected, do things
	// Some info: MSG_WAITALL is needed to make sure the socket waits for the specified amount of
	// 	data, so this is changed in zed_net
	// The format works by preceding each message with a uint16_t with the size of the message, then the message right after it
	while(keepReading.load()) {
		// First, check over every outgoing queue to detect outgoing data
		SEND_QUEUE_DATA(SetProjectName)
		SEND_QUEUE_DATA(SetCurrentFrame)
		SEND_QUEUE_DATA(ModifyFrame)

		// Block for all this because it's in a main loop in a thread anyway

		if(readData((uint8_t*)&dataSize, sizeof(dataSize))) {
			break;
		}

		// Get the number back to the correct representation
		// https://linux.die.net/man/3/ntohl
		dataSize = ntohs(dataSize);

		// Get the flag now, just a uint8_t, no endian conversion, I think
		if(readData((uint8_t*)&currentFlag, sizeof(currentFlag))) {
			break;
		}
		// Flag now tells us the data we expect to recieve

		// The message worked, so get the data
		if(readData(dataToRead, dataSize)) {
			break;
		}

		// Now, check over incoming queues, they will absorb the data if they correspond with the flag
		// Keep in mind, this is not the main thread, so can't act upon the data instantly
		RECIEVE_QUEUE_DATA(IsPaused)
		// That's it, wxWidgets will take care of it on idle
	}
}

template <typename T> void CommunicateWithSwitch::addDataToQueue(T data, DataFlag dataType) { }

CommunicateWithSwitch::~CommunicateWithSwitch() {
	serverConnection.Close();
}