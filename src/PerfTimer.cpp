#include "PerfTimer.h"
#include <iostream>
#include <boost/array.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <locale>
#include <chrono>
#include <iomanip>
#include <ctime>


PerfTimer::PerfTimer()
{}

void PerfTimer::start()
{
	_start_time = std::chrono::high_resolution_clock::now();
}

uint64_t PerfTimer::elapsedMilliseconds(bool restart)
{
	std::chrono::time_point<std::chrono::high_resolution_clock> end;
	end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> elapsed = end - _start_time;
	std::chrono::milliseconds elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

	if (restart)
		start();

	return elapsed_ms.count();
}

std::chrono::high_resolution_clock::time_point parseTime(utility::string_t time_str)
{
    std::time_t t = std::time(NULL);
    std::tm tm = *std::localtime(&t);

    std::vector<std::string> tokens;
    boost::split(tokens, time_str, boost::is_any_of(":."));

    tm.tm_hour = std::strtoul(tokens[0].c_str(), nullptr, 10);
    tm.tm_min = std::strtoul(tokens[1].c_str(), nullptr, 10);
    tm.tm_sec = std::strtoul(tokens[2].c_str(), nullptr, 10);

    int milliseconds = 0;
    if (tokens.size() >= 4)
        milliseconds = std::strtoul(tokens[3].c_str(), nullptr, 10);

    return std::chrono::high_resolution_clock::from_time_t(std::mktime(&tm)) + std::chrono::high_resolution_clock::duration(std::chrono::milliseconds(milliseconds));
}

utility::string_t getCurrentTimestamp()
{
    boost::posix_time::time_facet* facet = new boost::posix_time::time_facet("%H:%M:%S.%f");
    std::stringstream date_stream;
    date_stream.imbue(std::locale(date_stream.getloc(), facet));
    date_stream << boost::posix_time::microsec_clock::local_time();
    return utility::conversions::to_string_t(date_stream.str());
}

utility::string_t printTime(const std::chrono::high_resolution_clock::time_point timestamp, const std::string& format, bool milis /*= false*/)
{
    std::time_t t = std::chrono::high_resolution_clock::to_time_t(timestamp);
    char mbstr[32];
    std::strftime(mbstr, sizeof(mbstr), format.c_str(), std::localtime(&t));

    std::stringstream sstr;
    sstr << mbstr;

    if (milis)
    {
        // believe it || not: there's no 'modern' way to print timestamp w/ miliseconds in C++11
        auto milis = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch());
        std::size_t fractional_seconds = milis.count() % 1000;
        sstr << "." << std::setfill('0')<< std::setw(3) <<fractional_seconds;
    }
    return utility::string_t(sstr.str());
}

Perf::Perf()
{
}

void Perf::checkpoint(const std::string& name, Detail detail /* = {}*/)
{
    _checkpoints.push_back(Checkpoint(name, std::chrono::high_resolution_clock::now(), detail));
}

std::string Perf::stop()
{
    std::stringstream sstr;
    auto last_timestamp = HighResClock::time_point::min();

    for (auto checkpoint : _checkpoints)
    {
        sstr << std::get<chkName>(checkpoint);
        auto timestamp = std::get<chkTimestamp>(checkpoint);

        if (last_timestamp == HighResClock::time_point::min())
        {
            // believe it || not: there's no 'modern' way to print timestamp w/ miliseconds in C++11
            auto milis = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch());
            std::size_t fractional_seconds = milis.count() % 1000;
            sstr << ":" << printTime(timestamp, "%T") << "." << std::setfill('0')<< std::setw(3) <<fractional_seconds;
        }
        else
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(timestamp - last_timestamp);
            std::size_t milis = elapsed.count() / 1000;
            std::size_t micros = elapsed.count() % 1000;
            sstr<< ":" <<  milis << "." << std::setfill('0')<< std::setw(3) << micros;
        }
        sstr  <<", ";
        last_timestamp = timestamp;

        Detail detail = std::get<chkDetail>(checkpoint);
        auto name = std::get<dName>(detail);
        if (!name.empty())
        {
            sstr<< name<< ":" << std::get<dData>(detail)<< ", ";
        }
    }

    // clear the checckpoints
    _checkpoints.clear();

    return sstr.str();
}
