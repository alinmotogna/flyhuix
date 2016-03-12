#include "TulipServerConnection.h"
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/array.hpp>

TulipServerConnection::TulipServerConnection(TulipServerMessageCallback data_callback)
	: mDataCallback(data_callback)
	, mCurrentSocket(0x0)
{
}

TulipServerConnection::~TulipServerConnection()
{
}

bool TulipServerConnection::Open(const std::string& server_addr
	, const std::string& server_port
	, const std::string& machine_name
	, const std::string& platform_id
	)
{
	mServerAdress = server_addr;
	mServerPort = server_port;
	mMachineName = machine_name;
	mPlatformId = platform_id;

	try
	{
		BOOST_LOG_TRIVIAL(trace) << "TulipServerConnection::start connecting";

		mIoService.reset();
		tcp::resolver resolver(mIoService);

		//create resolved query && get endpoint
		tcp::resolver::query query(mServerAdress, mServerPort);
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

		//create a tcp socket and connect it to the endpoint
		mCurrentSocket = std::make_shared<tcp::socket>(mIoService);
		boost::asio::connect(*mCurrentSocket, endpoint_iterator);
	}
	catch (std::exception& ex)
	{
		mCurrentSocket->close();
		BOOST_LOG_TRIVIAL(error) << "EXCEPTION >> " << ex.what();
		return false;
	}

	BOOST_LOG_TRIVIAL(trace) << "TulipServerConnection::start running";

	mConnectionThread = std::thread([&]()
	{
		//create a buffer to store incoming data
		boost::array<char, 2048> buf;
		boost::system::error_code error;

		//while not stopping ...
		while (true)
		{
			//clear data
			memset(buf.data(), 0, 2048 * sizeof(char));

			//try to read from the socket
			size_t len = mCurrentSocket->read_some(boost::asio::buffer(buf), error);

			if (error == boost::asio::error::eof)
			{
				BOOST_LOG_TRIVIAL(trace) << "-- mServerListenerThread -- connection closed cleanly by peer";
				break;
			}
			else if (error)
			{
				BOOST_LOG_TRIVIAL(error) << "-- mServerListenerThread -- connection error";
				break;
			}
			else
			{
				//parse data received & handle messages
				OnSocketData(buf.data(), len);
			}
		}

		mCurrentSocket->close();
	});

	return true;
}

bool TulipServerConnection::IsOpen()
{
	if (mCurrentSocket)
		return mCurrentSocket->is_open();

	return false;
}

void TulipServerConnection::Close()
{
	if (mCurrentSocket && mCurrentSocket->is_open())
		mCurrentSocket->close();

	if (mConnectionThread.joinable())
		mConnectionThread.join();
}

void TulipServerConnection::SendMessage(const web::json::value& message)
{
	mCurrentSocket->write_some(boost::asio::buffer(utility::conversions::to_utf8string(message.serialize())));
}

void TulipServerConnection::OnSocketData(char* data, size_t len)
{
	//parse data into a json struct
	std::string data_str(data, len);
	BOOST_LOG_TRIVIAL(trace) << "Data:" << data_str;

	try
	{
		web::json::value jsonMsg = web::json::value::parse(utility::conversions::to_string_t(data_str));
		mDataCallback(jsonMsg);
	}
	catch (std::exception& e)
	{
		BOOST_LOG_TRIVIAL(error) << "TulipServerConnection::OnSocketData EXCEPTION:" << e.what();
	}
}

