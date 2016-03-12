#include "LendingClubQueryExecutor.h"
#include <boost/log/trivial.hpp>
#include "PerfTimer.h"

using namespace utility;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;

#define MAX_DATA_SIZE (1024 * 1024 * 10)

http_request LendingClubQueryExecutor::BuildGetLoansRequest(bool get_all_loans)
{
    http_request loan_request(methods::GET);
    loan_request.headers().add(U("Authorization"), mApiToken);
    loan_request.headers().set_content_type(U("application/json"));
    loan_request.headers().add(U("Accept"), U("text/plain"));

    //build paths and query
    uri_builder request_uri_builder;
    request_uri_builder.set_path(U("loans/listing"));

    //add useAllLoans to query
    if (get_all_loans == false)
        request_uri_builder.set_query(U("showAll=false"));
    else
        request_uri_builder.set_query(U("showAll=true"));

    loan_request.set_request_uri(request_uri_builder.to_uri());

    BOOST_LOG_TRIVIAL(trace) << "LendingClubQueryExecutor::BuildGetLoansRequest " << loan_request.to_string();

    //TODO: check if it has move c'tor
    return loan_request;
}

http_request LendingClubQueryExecutor::BuildSubmitOrdersRequest(const web::json::value& orders)
{
    //create HTTP client
   http_client client("https://api-sandbox.lendingclub.com/api/investor/v1/");

   //create request
   http_request loan_request(methods::POST);
   loan_request.headers().add(U("Authorization"), mApiToken);
   loan_request.headers().set_content_type(U("application/json"));

   uri_builder request_uri_builder;
   request_uri_builder.set_path(U("accounts/" + mInvestorId + "/orders"));
   loan_request.set_request_uri(request_uri_builder.to_uri());

   //create data message to send
   web::json::value message;
   message[U("aid")] = web::json::value(mInvestorId);
   message[U("orders")] = orders;
   loan_request.set_body(message);

   BOOST_LOG_TRIVIAL(trace) << "LendingClubQueryExecutor::BuildSubmitOrdersRequest " << loan_request.to_string();

    //TODO: check if it has move c'tor
    return loan_request;
}


LendingClubQueryExecutor::LendingClubQueryExecutor(LendingClubQueryExecutorCallback val, uint32_t datasize_increase_threshold, int simulation_flag)
	:	mState(QS_NOT_RUNNING)
	,	mCallback(val)
	,	mReceivedDataSize(0)
	,	mDataSizeIncreaseThreshold(datasize_increase_threshold)
	,	mSimulationFlag(simulation_flag)
	,	mQueryCount(0)
{
	//allocate receive data
	mReceivedData = std::unique_ptr<uint8_t[]>(new uint8_t[MAX_DATA_SIZE]);
	BOOST_LOG_TRIVIAL(trace) <<"TIME:" << getCurrentTimestamp();
}

LendingClubQueryExecutor::~LendingClubQueryExecutor()
{
	Stop();
}

void LendingClubQueryExecutor::Register(const utility::string_t& api_adr,
                                        const utility::string_t& api_token,
                                        const utility::string_t& api_investor_id,
                                        uint32_t query_limit)
{
    //init properties
    mApiAddr = api_adr;
    mApiToken = api_token;
    mInvestorId = api_investor_id;
    mQueryLimitCount = query_limit;
}

void LendingClubQueryExecutor::Initialize()
{
    Stop();

	//create an executor thread
	mThread = std::thread([&]()
	{
        //create HTTP client
        http_client client(mApiAddr);

        BOOST_LOG_TRIVIAL(trace) << "QueryThread -- Initializing";
        //do warmup query
        auto loan_request = BuildGetLoansRequest(true);
        DoQuery(client, loan_request);

		while (true)
		{
			BOOST_LOG_TRIVIAL(trace) << "QueryThread -- loop";
			SetState(QS_NOT_RUNNING);

			try
			{
				//wait for the start message
				if (!WaitForStartMsg())
					break;

				//reinit loan request with received used loans
				loan_request = BuildGetLoansRequest(mUseAllLoans);

				//wait for the start time
				if (!WaitForStartTime())
					break;

				bool found_data = false;
				mQueryCount = 0;
				mReceivedDataSize = 0;

				//while thread was not asked to stop and not found data to send and below max query count
				while (mState == QS_RUNNING && found_data == false && mQueryCount < mQueryLimitCount)
				{
					mQueryCount++;
					found_data = DoQuery(client, loan_request);
				}

				BOOST_LOG_TRIVIAL(trace) << "QueryThread -- done, found_data:" << found_data << " queries:" << mQueryCount;

				//notify callbacks
				if (found_data)
				{
				    web::json::value orders = mCallback(mReceivedData.get(), mReceivedDataSize, mUseAllLoans, mLastRequestTimestamp, mLastResponseTimestamp);
				    if ( !orders.is_null() )
				    {
				        loan_request = BuildSubmitOrdersRequest(orders);
				    }
				}
				else
					mCallback(NULL, 0, mUseAllLoans, mLastRequestTimestamp, mLastResponseTimestamp);
			}
			catch (std::exception& ex)
			{
				BOOST_LOG_TRIVIAL(error) << "EXCEPTION:" << ex.what();
			}

			if (mState == QS_STOPPING)
			{
				BOOST_LOG_TRIVIAL(error) << "QueryThread -- ending loop:";
				break;
			}
		}
	});
}

void LendingClubQueryExecutor::Start(const std::chrono::high_resolution_clock::time_point& startTime, bool use_all_loans)
{
    BOOST_LOG_TRIVIAL(trace) << "LendingClubQueryExecutor::start when state was " << mState;
	if (mState == QS_STOPPING)
	{
		BOOST_LOG_TRIVIAL(error) << "LendingClubQueryExecutor::start -- ERROR: in stopping state";
		return;
	}

	mStartTime = startTime;
	mUseAllLoans = use_all_loans;

	SetState(QS_RUNNING);
}

void LendingClubQueryExecutor::Stop()
{
	BOOST_LOG_TRIVIAL(trace) << "LendingClubQueryExecutor::stop";

	//mark as stopping, wait for thread to end
	SetState(QS_STOPPING);

	if (mThread.joinable())
		mThread.join();

	SetState(QS_NOT_RUNNING);

	BOOST_LOG_TRIVIAL(trace) << "LendingClubQueryExecutor::stop -- done";
}

void LendingClubQueryExecutor::StopQuerying()
{
	BOOST_LOG_TRIVIAL(trace) << "LendingClubQueryExecutor::StopQuerying";

	SetState(QS_NOT_RUNNING);

	BOOST_LOG_TRIVIAL(trace) << "LendingClubQueryExecutor::StopQuerying -- done";
}

bool LendingClubQueryExecutor::DoQuery(web::http::client::http_client& client, web::http::http_request& loan_request)
{
	BOOST_LOG_TRIVIAL(trace) << "QueryThread -- querying";

	rawptr_buffer<uint8_t> buffer(mReceivedData.get(), MAX_DATA_SIZE);
	PerfTimer query_timer;

	query_timer.start();
	bool found_data = false;

	//fire request
	mLastRequestTimestamp = getCurrentTimestamp();
	client.request(loan_request).then([&](http_response response) -> pplx::task<size_t>
	{
		//read response to buffer
		BOOST_LOG_TRIVIAL(trace) << "Response code:" << response.status_code();
		return response.body().read_to_end(buffer);
	}).then([&](size_t data_size)
	{
		BOOST_LOG_TRIVIAL(trace) << "QueryThread -- received:"
			<< data_size << " time:"
			<< query_timer.elapsedMilliseconds(false);

		//if previous data size was less than the current one, we got fresh data, found_data=true
		if (mReceivedDataSize != 0 && (data_size > mReceivedDataSize + mDataSizeIncreaseThreshold || SimulationFlagReleaseCheck()))
		{
			BOOST_LOG_TRIVIAL(trace) << "QueryThread -- found data!";
			found_data = true;
		}

		mLastResponseTimestamp = getCurrentTimestamp();
		mReceivedDataSize = data_size;
	}).wait();

	return found_data;
}

bool LendingClubQueryExecutor::WaitForStartMsg()
{
	BOOST_LOG_TRIVIAL(trace) << "QueryExecutor::waitForStartMsg when state was " << mState;

	WaitFor([&](){return mState == QS_RUNNING || mState == QS_STOPPING; });

	//if status is set to stopping, someone requested stop
	if (mState == QS_STOPPING)
	{
		BOOST_LOG_TRIVIAL(trace) << "QueryExecutor::waitForStartMsg -- interrupt!";
		return false;
	}

	BOOST_LOG_TRIVIAL(trace) << "QueryExecutor::waitForStartMsg -- done: ";
	return true;
}

bool LendingClubQueryExecutor::WaitForStartTime()
{
	//print wait time
	std::unique_lock<std::mutex> lk(mCondVariableMutex);
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	BOOST_LOG_TRIVIAL(trace) << "waitForStartTime -- wait: "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(mStartTime - now).count()
		<< "ms";

	//wait until specified time, interrupt if STOP request
	if (mStateConditionVariable.wait_until(lk, mStartTime, [&](){return mState == QS_STOPPING; }))
	{
		BOOST_LOG_TRIVIAL(trace) << "QueryExecutor::waitForStartMsg -- interrupt!";
		return false;
	}

	BOOST_LOG_TRIVIAL(trace) << "waitForStartTime -- done: ";
	return true;
}

void LendingClubQueryExecutor::WaitFor(std::function<bool()> func)
{
	BOOST_LOG_TRIVIAL(trace) << "QueryExecutor::WaitFor";

	std::unique_lock<std::mutex> lk(mCondVariableMutex);

	//wait until specified time, interrupt if STOP request
	mStateConditionVariable.wait(lk, func);
}

bool LendingClubQueryExecutor::SimulationFlagReleaseCheck()
{
	if (mSimulationFlag == 1 && mQueryCount > 0)
	{
		BOOST_LOG_TRIVIAL(trace) << "LendingClubQueryExecutor::SimulationFlagReleaseCheck TRIGGER";
		return true;
	}

	return false;
}
