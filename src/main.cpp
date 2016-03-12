#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "LoanDataProvider.h"
#include "LendingClubAgent.h"

void usage()
{
    BOOST_LOG_TRIVIAL(trace) << "-- Missing arguments. Usage is: [loan_provider|investment_agent] {args}" << std::endl
        << "for 'loan_provider' args are [server_ip] [server_port] [machine_name] [platform_name] [simulation_flag, optional]" << std::endl
        << "for 'investment_agent' args are [api_address] [api_token] [api_investor_id] [show_all_loans] [start_time] [query_frequencies]";
}

int main(int argc, char* argv[])
{
	boost::log::add_file_log(
		boost::log::keywords::file_name = "TulipLoanDataProvider.log",
		boost::log::keywords::format = "[%TimeStamp%]: %Message%",
		boost::log::keywords::auto_flush = true,
		boost::log::keywords::open_mode = (std::ios::out | std::ios::app)
		);
	boost::log::add_console_log(std::cout
		, boost::log::keywords::format = "[%TimeStamp%]: %Message%"
		, boost::log::keywords::auto_flush = true);
	boost::log::add_common_attributes();

	BOOST_LOG_TRIVIAL(trace) << std::endl << std::endl << "--- TulipLoanDataProvider---" << std::endl;

    if (argc < 2)
    {
        usage();
        return 1;
    }

    std::string mode(argv[1]);
    if (mode == "loan_provider")
    {
        if (argc < 6)
        {
            usage();
            return 1;
        }

        std::string server_ip(argv[2]);
        std::string server_port(argv[3]);
        std::string machine_name(argv[4]);
        std::string platform_name(argv[5]);

        int simulation_flag = 0;
        if (argc == 7)
            simulation_flag = atoi(argv[6]);


        BOOST_LOG_TRIVIAL(trace) << "Simulation Flag:" << simulation_flag;

        LoanDataProvider data_provider(server_ip, server_port, machine_name, platform_name, simulation_flag);
        data_provider.Run();
    }
    else if (mode == "investment_agent")
    {
        if (argc < 8)
        {
            usage();
            return 1;
        }

        std::string api_address(argv[2]);
        std::string api_token(argv[3]);
        std::string api_investor_id(argv[4]);
        std::string show_all_loans(argv[5]);
        std::string start_time(argv[6]);
        std::string query_frequency(argv[7]);

        LendingClubAgent investment_agent(
                api_address,
                api_token,
                api_investor_id,
                show_all_loans,
                start_time,
                query_frequency);
        investment_agent.run();
    }
    else
    {
        usage();
    }



	return 0;
}
