#pragma once

#include <cpprest/json.h>
#include <string.h>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>
#include <memory>
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/rawptrstream.h>
#include <condition_variable>

typedef std::function<web::json::value(uint8_t* data,
        uint32_t size,
        bool useAllLoans,
        const utility::string_t& request_time,
        const utility::string_t& receive_time)> LendingClubQueryExecutorCallback;

class LendingClubQueryExecutor
{
public:
	LendingClubQueryExecutor(LendingClubQueryExecutorCallback val, uint32_t datasize_increase_threshold, int simulation_flag);
	~LendingClubQueryExecutor();

	void Register(const utility::string_t& api_adr,
                  const utility::string_t& api_token,
                  const utility::string_t& api_investor_id,
                  uint32_t query_limi);
	void Initialize();

	void StopQuerying();

	void Start(const std::chrono::high_resolution_clock::time_point& startTime, bool use_all_loans);
	void Stop();

    web::http::http_request BuildGetLoansRequest(bool get_all_loans);


protected:

	enum QueryState
	{
		QS_NOT_RUNNING,
		QS_RUNNING,
		QS_STOPPING
	};

	void SetState(QueryState val)
	{
		mState = val;
		mStateConditionVariable.notify_one();
	}

	web::http::http_request BuildSubmitOrdersRequest(const web::json::value& get_all_loans);

	bool DoQuery(web::http::client::http_client& client, web::http::http_request& loan_request);

    void WaitFor(std::function<bool()> func);
	bool WaitForStartMsg();
	bool WaitForStartTime();

	bool SimulationFlagReleaseCheck();

	QueryState mState;

	std::thread	mThread;
	std::condition_variable	mStateConditionVariable;
	std::mutex	mCondVariableMutex;

	utility::string_t	mApiAddr;
    utility::string_t   mInvestorId;
	utility::string_t	mApiToken;
	std::chrono::high_resolution_clock::time_point	mStartTime;
	bool	mUseAllLoans;
	uint32_t	mQueryLimitCount;

	std::unique_ptr<uint8_t[]>	mReceivedData;
	uint32_t	mReceivedDataSize;
	uint32_t	mQueryCount;
	utility::string_t	mLastRequestTimestamp;
	utility::string_t	mLastResponseTimestamp;
	LendingClubQueryExecutorCallback	mCallback;

	uint32_t mDataSizeIncreaseThreshold;
	int mSimulationFlag;
};

