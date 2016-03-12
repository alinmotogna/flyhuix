/*
 * Loan.cpp
 *
 *  Created on: 5 Aug, 2015
 *      Author: alin
 */

#include "Loan.h"

#include <vector>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <string>

/* static */ const std::set<Loan::Fields> Loan::RelevantFields {Fields::LOAN_ID, Fields::LOAN_AMOUNT, Fields::TERM, Fields::INTEREST_RATE, Fields::GRADE};

Loan::Loan()
{
}

Loan::~Loan()
{
}

std::string Loan::toString() const
{
    std::stringstream sstr;
    sstr << loanId <<"," << loanAmount <<"," << term <<"," << intRate << "," << grade;
    return sstr.str();
}

LoanBuilder::Status LoanBuilder::addField(Loan::Fields fieldNo, const std::string& fieldStr)
{
    switch (fieldNo)
    {
        case Loan::Fields::LOAN_ID:
        {
            loan->loanId = fieldStr;
            return Status::OK;
        }
        case Loan::Fields::LOAN_AMOUNT:
        {
            loan->loanAmount = std::stod(fieldStr);
            return Status::OK;
        }
        case Loan::Fields::TERM:
        {
            loan->term = std::stoi(fieldStr);
            return checkTerm();
        }
        case Loan::Fields::INTEREST_RATE:
        {
            loan->intRate = std::stod(fieldStr);
            return checkIntRate();
        }
        case Loan::Fields::GRADE:
        {
            loan->grade = fieldStr;
            return checkGrade();
        }
        default:
        {
            // field not relevant, moving on
        }
    }

    return Status::OK;
}

std::unique_ptr<Loan> LoanBuilder::getLoan()
{
    return std::move(loan);
}

bool LoanBuilder::done(Loan::Fields fieldNo) const
{
    return fieldNo == Loan::Fields::GRADE;
}

LoanBuilder::Status LoanBuilder::checkTerm() const
{
    return (loan->term == 36 ? Status::OK : Status::REJECTED);
}

LoanBuilder::Status LoanBuilder::checkIntRate() const
{
    return (loan->intRate > 15.0 ? Status::OK : Status::REJECTED);
}

LoanBuilder::Status LoanBuilder::checkGrade() const
{
    return (loan->grade == "E" ? Status::FINISHED : Status::REJECTED);
}
