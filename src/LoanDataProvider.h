#pragma once

#include <string>
#include "TulipServerConnection.h"
#include "LendingClubQueryExecutor.h"
#include <cpprest/json.h>

class LoanDataProvider
{
public:
	LoanDataProvider(const std::string& server_addr, const std::string& server_port, const std::string& machine_name, const std::string& platform_id, int simulation_flag = 0);
	~LoanDataProvider();
	
	void Run();	

protected:

	void OnServerMessage(const web::json::value& message);
	web::json::value OnLendingClubData(uint8_t* data, uint32_t size, bool useAllLoans, const utility::string_t& request_time, const utility::string_t& receive_time);

	void OnRegisterService(const web::json::value& message);
	void OnInitialize();
	void OnStartQuerying(const web::json::value& message);
	void OnStopQuerying();
	void OnRemoveClient();
	void OnPing();

	void SendRegisterMessage();
	void SendUnregisterMessage();

	std::string mServerAdress;
	std::string mServerPort;
	std::string mMachineName;
	std::string mPlatformId;

	//api info
	utility::string_t mApiAddr;
	utility::string_t mApiToken;
	utility::string_t mInvestorId;
	utility::string_t mAccountName;
	utility::string_t mServiceId;
	bool mHasApiInfo;

	TulipServerConnection mServerConnection;
	LendingClubQueryExecutor mLDQueryExecutor;
	int mSimulationFlag;
	bool mStopRequested;
};

