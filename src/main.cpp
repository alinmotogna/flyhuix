#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "QPXService.h"

void usage()
{
    BOOST_LOG_TRIVIAL(trace) << "-- Missing arguments. Usage is: ";
}


int main(int argc, char* argv[])
{
    boost::log::add_file_log(
        boost::log::keywords::file_name = "flyhuix.log",
        boost::log::keywords::format = "[%TimeStamp%]: %Message%",
        boost::log::keywords::auto_flush = true,
        boost::log::keywords::open_mode = (std::ios::out | std::ios::app));
    boost::log::add_console_log(std::cout,
        boost::log::keywords::format = "[%TimeStamp%]: %Message%",
        boost::log::keywords::auto_flush = true);
    boost::log::add_common_attributes();

    BOOST_LOG_TRIVIAL(trace) << std::endl << std::endl << "--- FlyHuiX---" << std::endl;

    if (argc < 0)
    {
        usage();
        return 1;
    }

    QPXService qpx_service;
    qpx_service.run();

    return 0;
}
