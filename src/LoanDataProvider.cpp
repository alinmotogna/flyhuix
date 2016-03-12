#include "LoanDataProvider.h"
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string.hpp>
#include <ctime>
#include <thread>
#include <time.h>
#include "PerfTimer.h"

#define DATA_SIZE_INCREASE_THRESHOLD (70 * 1024)


LoanDataProvider::LoanDataProvider(const std::string& server_addr
	, const std::string& server_port
	, const std::string& machine_name
	, const std::string& platform_id
	, int simulation_flag)
	:	mServerAdress(server_addr)
	,	mServerPort(server_port)
	,	mMachineName(machine_name)
	,	mPlatformId(platform_id)
	,	mServerConnection([this](const web::json::value& message){this->OnServerMessage(message); })
	,	mLDQueryExecutor(
	        [this](uint8_t* data, uint32_t size, bool useAllLoans, const utility::string_t& request_time, const utility::string_t& receive_time)
	        {
                return this->OnLendingClubData(data, size, useAllLoans, request_time, receive_time);
	        },
	        DATA_SIZE_INCREASE_THRESHOLD,
	        simulation_flag)
	,	mHasApiInfo(false)
	,	mSimulationFlag(simulation_flag)
	,	mStopRequested(false)
{
}

LoanDataProvider::~LoanDataProvider()
{
}

void LoanDataProvider::Run()
{
	while (true)
	{
		BOOST_LOG_TRIVIAL(warning) << "LoanDataProvider::Run -- start session";
		
		if (mServerConnection.Open(mServerAdress, mServerPort, mMachineName, mPlatformId))
			SendRegisterMessage();

		while (mServerConnection.IsOpen() && mStopRequested == false);
		mLDQueryExecutor.Stop();
		mServerConnection.Close();

		if (mStopRequested)
		{
			BOOST_LOG_TRIVIAL(warning) << "LoanDataProvider::Run -- stopping";
			break;
		}

		BOOST_LOG_TRIVIAL(warning) << "LoanDataProvider::Run -- end session, retry in 10s";
        std::this_thread::sleep_for(std::chrono::seconds(10));
	}
}

void LoanDataProvider::OnServerMessage(const web::json::value& message)
{
	//get message type
	web::json::value type = message.at(U("msgType"));

	utility::string_t type_str = type.as_string();
	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::onServerData -- msgtype:" << type_str.c_str();

	//interpret message based on type
	if (type_str == U("Register"))
		OnRegisterService(message);
	else if (type_str == U("Initialize"))
		OnInitialize();
	else if (type_str == U("Start"))
		OnStartQuerying(message);
	else if (type_str == U("Stop"))
		OnStopQuerying();
	else if (type_str == U("Ping"))
		OnPing();
	else if (type_str == U("RemoveClient"))
		OnRemoveClient();
	else
		BOOST_LOG_TRIVIAL(warning) << "LoanDataProvider::onServerData -- unrecognized message type:" << type_str.c_str();
}

void LoanDataProvider::OnRegisterService(const web::json::value& message)
{
	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::onRegisterService";

	//extract service info
	web::json::value loanServiceInfo = message.at(U("loanServiceInfo"));

	web::json::value api_token_val = loanServiceInfo.at(U("accountToken"));
	web::json::value serviceId = loanServiceInfo.at(U("serviceId"));
	web::json::value account_name_val = loanServiceInfo.at(U("accountName"));
	web::json::value message_val = loanServiceInfo.at(U("message"));
	web::json::value status_val = loanServiceInfo.at(U("status"));

	mApiAddr = U("https://api.lendingclub.com/api/investor/v1/");
	mInvestorId = U("10698236");
	mApiToken = api_token_val.as_string();
	mServiceId = serviceId.as_string();
	mAccountName = account_name_val.as_string();

	utility::string_t register_message = message_val.as_string();
	int status = status_val.as_integer();

	BOOST_LOG_TRIVIAL(trace) << " -ApiAddr:" << mApiAddr.c_str()
		<< " -Token:" << mApiToken.c_str()
		<< " -Guid:" << mServiceId.c_str()
		<< " -AccName:" << mAccountName.c_str()
		<< " -Message:" << register_message.c_str()
		<< " -Status:" << status;

	if (status == 0)
		mHasApiInfo = true;

	//initialize the query executor
	mLDQueryExecutor.Register(mApiAddr, mApiToken, mInvestorId, 60);

	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::onRegisterService -- done";
}

void LoanDataProvider::OnInitialize()
{
	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::onInitialize";

	//do warmup
	mLDQueryExecutor.Initialize();

	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::onInitialize -- done";
}

void LoanDataProvider::OnStartQuerying(const web::json::value& message)
{
	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::onStartQueryInfo";

	//can't start with no api info
	if (!mHasApiInfo)
	{
		BOOST_LOG_TRIVIAL(error) << "LoanDataProvider::onStartQueryInfo -- NO API INFO!";
		return;
	}

	web::json::value service_process_data = message.at(U("serviceProcessData"));

	//read starttime and loans flag
	web::json::value start_time_val = service_process_data.at(U("startTime"));
	web::json::value use_all_loans_val = service_process_data.at(U("useAllLoans"));

	utility::string_t start_time_str = start_time_val.as_string();

	//create a timepoint where to start querying
	std::chrono::high_resolution_clock::time_point start_time = parseTime(start_time_str);
	bool use_all_loans = use_all_loans_val.as_bool();

	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::onStartQueryInfo -- start_time:" << start_time_str.c_str()
		<< " use_all_loans:" << use_all_loans;

	//kick the executor
	mLDQueryExecutor.Start(start_time, use_all_loans);
}

void LoanDataProvider::OnStopQuerying()
{
	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::OnStopQuerying";

	mLDQueryExecutor.StopQuerying();

	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::stop -- OnStopQuerying";
}

void LoanDataProvider::OnPing()
{
	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::onPing";
	
	web::json::value message;
	message[U("msgType")] = web::json::value(U("Ping"));
	mServerConnection.SendMessage(message);

	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::onPing -- done";
}

void LoanDataProvider::SendRegisterMessage()
{
	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::sendProviderInfo";

	web::json::value message;
	message[U("msgType")] = web::json::value(U("Register"));

	web::json::value register_info;

	register_info[U("platformId")] = web::json::value(atoi(mPlatformId.c_str()));
	register_info[U("machineName")] = web::json::value(utility::conversions::to_string_t(mMachineName));
	register_info[U("serviceType")] = web::json::value(U("CPP"));
	register_info[U("previousServiceId")] = web::json::value(mServiceId);

	message[U("registerInfo")] = register_info;

	mServerConnection.SendMessage(message);
	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::sendProviderInfo -- done";
}

void LoanDataProvider::SendUnregisterMessage()
{
	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::sendUnregister";

	web::json::value message;
	message[U("msgType")] = web::json::value(U("Unregister"));
	message[U("serviceId")] = web::json::value(mServiceId);

	mServerConnection.SendMessage(message);
	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::sendUnregister -- done";
}

web::json::value LoanDataProvider::OnLendingClubData(uint8_t* data, uint32_t size, bool useAllLoans, const utility::string_t& request_time, const utility::string_t& receive_time)
{
	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::OnLendingClubData";

	if (data != NULL)
	{
		//create data message to send
		web::json::value message;
		message[U("msgType")] = web::json::value(U("LoanData"));

		web::json::value release_data;

		//inject data
		utility::string_t data_str(data, data + size);
		release_data[U("newLoans")] = web::json::value(data_str);
		release_data[U("serviceId")] = web::json::value(mServiceId);
		release_data[U("useAllLoans")] = web::json::value(useAllLoans);
		release_data[U("loansRequestTime")] = web::json::value(request_time);
		release_data[U("loansResponseTime")] = web::json::value(receive_time);

		message[U("releaseData")] = release_data;
	
		//write to socket
		mServerConnection.SendMessage(message);
	}

	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::OnQuerySuccess -- done";

	// we don't want here to send any orders
	return web::json::value();
}

void LoanDataProvider::OnRemoveClient()
{
	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::OnRemoveClient";
	mStopRequested = true;
	BOOST_LOG_TRIVIAL(trace) << "LoanDataProvider::OnRemoveClient -- done";
}
