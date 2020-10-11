
#include "pipesender.hpp"
#include <windows.h>
#include <stdio.h>
#include <string>
#include <chrono>

// EXCLUDED FROM BUILD!!!

using namespace std;

#pragma pack(push, 1)
struct DataPacket {
	int packetLength;
	int packetType;
	int64_t currentTimeMillis;
	float x, y, z;
	float a, b, c;
};
#pragma pack(pop) 

class PipeSenderImplementation : public PipeSender {

	// https://docs.microsoft.com/en-us/windows/win32/ipc/multithreaded-pipe-server
	// this pipe is output-only, so it is not multithreaded

private:


	volatile HANDLE hPipe;
	boolean opened;

	int64_t currentTimeMillis()
	{
		std::chrono::time_point<std::chrono::steady_clock> t = std::chrono::steady_clock::now();
		return std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()).count();
	}


	string provideErrorMessage(LPCSTR name, DWORD gle)
	{
		if (0 == gle)
		{
			return string("");
		}

		// https://stackoverflow.com/questions/1387064/how-to-get-the-error-message-from-the-error-code-returned-by-getlasterror
		
		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, gle, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		char b[4096];
		sprintf_s(b, sizeof(b), "%s failed, GLE=%d (%08X) : \"%s\"\n", name, gle, gle, messageBuffer);
		LocalFree(messageBuffer);

		return string(b);
	}

public:

	PipeSenderImplementation()
	{
		this->hPipe = 0;
		this->opened = false;
	}

	virtual optional<string> openPipe() 
	{

		hPipe = CreateNamedPipeA(
			"\\\\.\\pipe\\head_position_and_rotation.pipe",
			PIPE_ACCESS_OUTBOUND,
			PIPE_TYPE_MESSAGE,
			PIPE_UNLIMITED_INSTANCES,
			1024,
			1024,
			1,
			NULL
		);

		if (hPipe == INVALID_HANDLE_VALUE)
			return provideErrorMessage("CreateNamedPipe", ::GetLastError());

		return nullopt;
	}

	virtual void send(float x, float y, float z, float a, float b, float c) 
	{
		if (!hPipe)
			return;

		DataPacket dp;
		dp.packetType = 71;
		dp.packetLength = sizeof(dp);
		dp.currentTimeMillis = this->currentTimeMillis();
		dp.x = x;
		dp.y = y;
		dp.z = z;
		dp.a = a;
		dp.b = b;
		dp.c = c;

		DWORD cbWritten;
		BOOL fSuccess = WriteFile(hPipe, &dp, sizeof(dp), &cbWritten, NULL);
		FlushFileBuffers(hPipe);
	}

	virtual void close()
	{
		if (hPipe)
		{
			FlushFileBuffers(hPipe);
			DisconnectNamedPipe(hPipe);
			CloseHandle(hPipe);
			hPipe = FALSE;
		}
	}

	virtual ~PipeSenderImplementation() {
		this->close();
	}

};



std::unique_ptr<PipeSender> factory_PipeSender() {
	return std::unique_ptr<PipeSender>(new PipeSenderImplementation());
}


