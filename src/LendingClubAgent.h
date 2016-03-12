/*
 * LendingClubAgent.h
 *
 *  Created on: Aug 4, 2015
 *      Author: alin.motogna
 */

#pragma once

#include "LendingClubQueryExecutor.h"
#include "Loan.h"

class LendingClubAgent
{
public:
    LendingClubAgent(
            const std::string& api_addr,
            const std::string& api_token,
            const std::string& api_investor_id,
            const std::string& show_all_loans,
            const std::string& start_time,
            const std::string& frequencies);
    ~LendingClubAgent();

    void run();

private:
    web::http::http_request buildGetLoansRequest();
    void query(int index);
    int processLoans(const uint8_t* const dataPtr, size_t size);

    //api info
    utility::string_t mApiAddr;
    utility::string_t mApiToken;
    utility::string_t mInvestorId;
    utility::string_t mShowAllLoans;
    std::chrono::high_resolution_clock::time_point mStartTime;

    using QueryInfo = std::pair<std::chrono::milliseconds,  // frequency
                                bool>;                      // continuous
    std::vector<QueryInfo>   mQueries;
};
