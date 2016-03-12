#pragma once

#include <string>
#include <thread>
#include <cpprest/json.h>
#include <boost/asio.hpp>

typedef std::function<void(const web::json::value& message)> TulipServerMessageCallback;

using boost::asio::ip::tcp;

class TulipServerConnection
{
public:
	TulipServerConnection(TulipServerMessageCallback data_callback);
	~TulipServerConnection();

	bool Open(const std::string& server_addr
		, const std::string& server_port
		, const std::string& machine_name
		, const std::string& platform_id);

	bool IsOpen();
	void Close();

	void SendMessage(const web::json::value& message);

protected:

	void OnSocketData(char* data, size_t len);

	std::string mServerAdress;
	std::string mServerPort;
	std::string mMachineName;
	std::string mPlatformId;

	std::thread mConnectionThread;
	
	boost::asio::io_service mIoService;
	std::shared_ptr<tcp::socket> mCurrentSocket;

	TulipServerMessageCallback mDataCallback;
};



