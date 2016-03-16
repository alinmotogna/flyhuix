/*
 * LendingClubAgent.cpp
 *
 *  Created on: Aug 4, 2015
 *      Author: alin.motogna
 */

#include "LendingClubAgent.h"
#include <boost/log/trivial.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <ostream>
#include "PerfTimer.h"

#include <cpprest/http_client.h>
#include "cpprest/filestream.h"
#include "cpprest/containerstream.h"
#include "cpprest/producerconsumerstream.h"

using namespace utility;
using namespace concurrency::streams;

LendingClubAgent::LendingClubAgent(
        const std::string& api_addr,
        const std::string& api_token,
        const std::string& api_investor_id,
        const std::string& show_all_loans,
        const std::string& start_time,
        const std::string& frequencies)
    : mApiAddr(api_addr)
    , mApiToken(api_token)
    , mInvestorId(api_investor_id)
    , mShowAllLoans(show_all_loans)
    , mStartTime(parseTime(start_time))
{
    std::vector<std::string> tokens;
    boost::split(tokens, frequencies, boost::is_any_of(","));
    for(int i = 1; i< tokens.size(); i=i+2)
    {
        int frequency = std::stol(tokens[i-1]);
        std::istringstream istr(tokens[i]);
        bool continuous;
        istr >> std::boolalpha >> continuous;
        mQueries.push_back(QueryInfo(std::chrono::milliseconds(frequency), continuous));
    }
}

LendingClubAgent::~LendingClubAgent()
{
}

void LendingClubAgent::run()
{
    BOOST_LOG_TRIVIAL(trace) << "LendingClubAgent::BuildGetLoansRequest " << buildGetLoansRequest().to_string();

    std::vector<std::thread> ths;
    for (int index = 0; index < mQueries.size(); ++index)
    {
        ths.push_back( std::thread([=]{query(index);}) );
    }

    for (auto& th : ths)
    {
        th.join();
    }
}

void LendingClubAgent::query(int index)
{
    auto log_prefix = "Query #" + std::to_string(index);
    web::http::client::http_client client(mApiAddr);
    auto loan_request = buildGetLoansRequest();

    // preallocate a large buffer and set it as reponse output strean
    size_t size = 10 * 1024 * 1024;
    concurrency::streams::container_buffer<std::vector<uint8_t>> buffer;
    buffer.collection().reserve(size);
    auto out_stream = buffer.create_ostream();
    loan_request.set_response_stream(out_stream);

    Perf perf;

    // we capture everything by-reference because we use captured vars in the same scope
    auto get_response = [&](web::http::http_response response)
    {
        BOOST_LOG_TRIVIAL(trace) << log_prefix << " -> response code:" << response.status_code()
                << " content_length:" << response.headers().content_length()
                << " cache control:" << response.headers().cache_control();
        perf.checkpoint("response ready");
        return response.content_ready();
    };

    auto process_data = [&](web::http::http_response response)
    {
        std::vector<uint8_t> result = std::move(buffer.collection());
        perf.checkpoint("content ready", Perf::Detail("buffer size", result.size()));

        int total_loans_count = processLoans(&(result[0]), result.size());

        perf.checkpoint("processing ready", Perf::Detail("loans count", total_loans_count));
        BOOST_LOG_TRIVIAL(trace) << perf.stop();

        //dump query results in a file named based on the start time
        std::string filename = printTime(mStartTime, "%d%m_%H%M") + "_" + std::to_string(index) + "_loans.csv";
        std::ofstream ofstr(filename, std::ios_base::app);
        ofstr << utility::string_t(&(result[0]), &(result[0]) + result.size());
        return result.size();
    };

    auto delayed_start = mStartTime + std::chrono::milliseconds(mQueries[index].first);
    BOOST_LOG_TRIVIAL(trace) << log_prefix << (mQueries[index].second ? " [continuous]" : " [once]")
                             << ", waiting for start time: " << printTime(delayed_start, "%T", true);
    std::this_thread::sleep_until(delayed_start);
    BOOST_LOG_TRIVIAL(trace) << log_prefix << " running ";

    bool done = false;
    std::size_t previous_downloaded = 0;
    do
    {
        perf.checkpoint(log_prefix + " start");
        client.request(loan_request)
                .then(get_response)
                .then(process_data)
                .then([&](std::size_t downloaded_size)
                {
                    // we're done when we got a different download size than the previous call
                    // an early query ('warmup') will stop when the loans decrease before actual release
                    // regular queries will start by getting no loans and will stop when they get a first batch of loans
                    done = (previous_downloaded != 0) && (downloaded_size != previous_downloaded);
                    previous_downloaded = downloaded_size;
                })
                .wait();

        // reset the output stream for the next iteration
        std::fill(buffer.collection().begin(), buffer.collection().end(), 0);
        out_stream.seek(0);
    }
    while (mQueries[index].second && !done);
}

web::http::http_request LendingClubAgent::buildGetLoansRequest()
{
    web::http::http_request loan_request(web::http::methods::GET);
    loan_request.headers().add(U("Authorization"), mApiToken);
    loan_request.headers().set_content_type(U("application/json"));
    //loan_request.headers().set_cache_control(U("no-cache"));
    loan_request.headers().add(U("Accept"), U("text/plain"));
    //loan_request.headers().add(U("Cache"), U("no-cache"));

    //build paths and query
    web::http::uri_builder request_uri_builder;
    request_uri_builder.set_path(U("loans/listing"));
    request_uri_builder.set_query(U("showAll=") + mShowAllLoans);

    loan_request.set_request_uri(request_uri_builder.to_uri());

    return loan_request;
}



uint32_t getLoan(const uint8_t* const ptr, const uint8_t* const end)
{
    uint32_t loanSize = 0;
    while (ptr + loanSize < end && ptr[loanSize] != '\n')
    {
        loanSize++;
    }

    return loanSize;
}

uint32_t getField(const uint8_t* const ptr, const uint8_t* const end)
{
    uint32_t fieldSize = 0;
    while (ptr + fieldSize < end && ptr[fieldSize] != ',' && ptr[fieldSize] != '\n')
    {
        fieldSize++;
    }

    return fieldSize;
}

std::unique_ptr<Loan> processLoan(const uint8_t* const loanStartPtr, uint8_t loanSize)
{
    auto fieldStr = std::string();
    auto fieldNo = 0;
    auto ptr = loanStartPtr;
    auto end = loanStartPtr + loanSize;
    LoanBuilder loanBuilder;
    auto status = LoanBuilder::Status::OK;

    while (ptr < end && status == LoanBuilder::Status::OK)
    {
        auto fieldSize = getField(ptr, end);

        // extract & add only if the field is relevant
        auto field = static_cast<Loan::Fields>(fieldNo);
        if (Loan::RelevantFields.find(field) != Loan::RelevantFields.end())
        {
            // skip the quotes (") at first position and last position in the field
            fieldStr = std::string(reinterpret_cast<const char*>(ptr + 1), fieldSize - 1 - 1);
            status = loanBuilder.addField(field, fieldStr);
        }

        // advance skipping also over comma ',' char
        ptr = ptr + fieldSize + 1;
        fieldNo++;
    }

    if (status == LoanBuilder::Status::FINISHED)
    {
        // this is an rvalue so it will be std::move'd automatically
        return loanBuilder.getLoan();
    }
    else
    {
        return nullptr;
    }
}

int LendingClubAgent::processLoans(const uint8_t* const dataPtr, size_t size)
{
    std::multimap<double, Loan> available_loans;

    // explicitly and deliberately cast away const'ness
    uint8_t* ptr = const_cast<uint8_t*>(dataPtr);
    uint8_t* end = ptr + size;

    //first line is CSV header; skip to the first loan
    ptr = ptr + getLoan(ptr, end);
    int loans_count = 0;

    while (ptr < end)
    {
        auto loanSize = getLoan(ptr, end);
        loans_count++;
        std::unique_ptr<Loan> loan = processLoan(ptr, loanSize);
        if (loan != nullptr)
        {
            available_loans.insert( {loan->getIntRate(), *loan.get()} );
        }

        // advance skipping also over end-of-line '\n' char
        ptr = ptr + loanSize + 1;
    }

    return loans_count;
}
