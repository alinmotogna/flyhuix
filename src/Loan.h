/*
 * Loan.h
 *
 *  Created on: 5 Aug, 2015
 *      Author: alin
 */

#pragma once

#include <inttypes.h>
#include <string>
#include <set>
#include <memory>

class LoanBuilder;


class Loan {
    friend class LoanBuilder;

 public:
    enum class Fields : int
    {
        LOAN_ID = 0, MEMBER_ID = 1, LOAN_AMOUNT, FUNDED_AMOUNT, TERM, INTEREST_RATE, EXPECTED_DEFAULT_RATE,
        SERVICE_FEE_RATE, INSTALLMENT, GRADE, SUBGRADE, EMPLOYMENT_LENGTH, HOME_OWNERSHIP, ANNUAL_INCOME,
        ANNUAL_INCOME_VERIFIED, ACCEPTED_DATE, EXPIRATION_DATE, LISTING_DATE, CREDIT_PULL_DATE, REVIEW_STATUS_DATE,
        REVIEW_STATUS, DESCRIPTION, PURPOSE, ADDRESS_ZIP, ADDRESS_STATE, INVESTOR_COUNT, INITIAL_LIST_STATUS,
        EMP_TITLE, INITIAL_LIST_STATUS_EXPIRE_DATE, ACCOUNTS_NOW_DELINQUENT, ACCOUNTS_OPEN_IN_PAST_24_MTHS,
        BANK_CARDS_AMOUNT_OPEN_TO_BUY, BANK_CARDS_UTILIZATION_RATIO, DEBT_TO_INCOME_RATIO, DELINQUENCIES_IN_LAST_2_YEARS,
        DELINQUENT_AMOUNT, EARLIEST_CREDIT_LINE_DATE, FICO_RANGE_LOW, FICO_RANGE_HIGH, INQUIRIES_IN_LAST_6_MONTHS,
        MONTHS_SINCE_LAST_DELINQUENCY, MONTHS_SINCE_LAST_RECORD, MONTHS_SINCE_LAST_INQUIRY, MONTHS_SINCE_RECENT_REVOLVING_DELINQUENCY,
        MONTHS_SINCE_RECENT_BANK_CARD, MORTGAGE_ACCOUNTS,OPEN_ACCOUNTS, PUBLIC_RECORDS, TOTAL_BALANCE_EXCLUDING_MORTGAGE,
        REVOLVING_BALANCE, REVOLVING_UTILIZATION, TOTAL_BANK_CARD_LIMIT, TOTAL_ACCOUNTS, TOTAL_IL_HIGH_CREDIT_LIMIT,
        NUM_REV_ACCTS, MTHS_SINCE_RECENT_BC_DLQ, PUB_REC_BANKRUPTCIES, NUM_ACCTS_EVER_120_PPD, CHARGEOFF_WITHIN_12_MTHS,
        COLLECTIONS_12_MTHS_EXMED, TAX_LIENS, MTHS_SINCE_LAST_MAJOR_DEROG, NUM_STATS, NUM_TL_OP_PAST_12_M,
        MO_SIN_RCNT_TL, TO_HI_CRED_LIM, TO_CUR_BAL, AVG_CUR_BAL, NUM_BC_TL, NUM_ACTV_BC_TL, NUM_BC_STATS,
        PCT_TL_NVR_DLQ, NUM_TL_90_G_DPD_24_M, NUM_TL_30_DPD, NUM_TL_120_DPD_2_M, NUM_IL_TL, MO_SIN_OLD_IL_ACCT,
        NUM_ACTV_REV_TL, MO_SIN_OLD_REV_TL_OP, MO_SIN_RCNT_REV_TL_OP, TOTAL_REV_HI_LIM, NUM_REV_TL_BAL_GT0,
        NUM_OP_REV_TL, TOT_COLL_AMT, PERCENT_OF_BANK_CARDS_OVER_75_PERCENT_UTIL
    };

    static const std::set<Fields> RelevantFields;

 public:
    virtual ~Loan();

    std::string getId() const { return loanId; }
    int getTerm() const { return term; }
    double getAmount() const { return loanAmount; }
    double getIntRate() const { return intRate; }

    std::string toString() const;

 private:
    Loan();

    std::string loanId {""};
    int term {0};
    double loanAmount {0.0};
    double intRate {0.0};
    std::string grade {""};
};

class LoanBuilder
{
 public:
    enum class Status : int { OK, REJECTED, FINISHED };

    Status addField(Loan::Fields fieldNo, const std::string& fieldStr);
    std::unique_ptr<Loan> getLoan();

 private:
    std::unique_ptr<Loan> loan { new Loan() };

    // signal that this is the last field / token that we're interested in
    bool done(Loan::Fields fieldNo) const;

    Status checkTerm() const;
    Status checkIntRate() const;
    Status checkGrade() const;
};
