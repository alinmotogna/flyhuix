#ifndef TIMER_H
#define TIMER_H

#include <iostream>
#include <chrono>
#include <cstdint>
#include <cpprest/json.h>

class PerfTimer
{
public:
	PerfTimer();

	void start();

	uint64_t elapsedMilliseconds(bool restart);

	std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;
};

class Perf
{
public:
    // support for extra info of type int
    // quite restrictive, but all I need for now, can be extended with variadic arguments or variadic templates)
    using Detail = std::tuple<std::string,  //detail name
                              int> ;        // detail data
    enum DetailFields { dName,
                        dData };

    Perf();

    // registering a checkpoint always resets internal clock
    void checkpoint(const std::string& name, Detail detail = {});
    // prints registered checkpoints and then clears them
    std::string stop();

private:
    using HighResClock = std::chrono::high_resolution_clock;
    using TimeStamp = std::chrono::time_point<HighResClock>;

    using Checkpoint = std::tuple<std::string,  // name
                                  TimeStamp,    // high resolution timestamp
                                  Detail> ;     // extra information
    enum CheckpointFields { chkName,
                            chkTimestamp,
                            chkDetail };

    std::string _name;
    std::vector<Checkpoint> _checkpoints;
};

std::chrono::high_resolution_clock::time_point parseTime(utility::string_t time_str);
utility::string_t printTime(const std::chrono::high_resolution_clock::time_point timestamp, const std::string& format, bool milis = false);
utility::string_t getCurrentTimestamp();

#endif // TIMER_H
