/*
 * QPXService.cpp
 *
 *  Created on: Mar 14, 2016
 *      Author: alin.motogna
 */

#include "QPXService.h"
#include <thread>
#include <cpprest/http_client.h>
#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/locale.hpp>
#include <locale>
#include <iomanip>

namespace bgr = boost::gregorian;
namespace bfs = boost::filesystem;

const utility::string_t QPXService::Resources::URL=U("https://www.googleapis.com/qpxExpress/v1/trips/search?key=");
const utility::string_t QPXService::Resources::KEY=U("AIzaSyDLo2QRtu4g-k-hPUv3kLRY5c6dDMWOv_0");

utility::string_t printDuration(web::json::value duration)
{
    using namespace std::chrono;
    auto total_mins = minutes(duration.as_integer());
    
    auto hrs = duration_cast<hours>(total_mins);
    total_mins -= hrs;
    auto mins = duration_cast<minutes>(total_mins);
    
    utility::stringstream_t ss;
    ss << std::setw(2) /*<< std::setfill('0')*/ << hrs.count()
            << ":" << std::setw(2) /*<< std::setfill('0')*/ << mins.count();
    return ss.str();
}

std::tuple<bgr::date, bgr::date> getNextWeekend(bgr::date refDate)
{
	auto nextFriday = refDate;
	while (nextFriday.day_of_week() != bgr::Friday)
		nextFriday = nextFriday + bgr::days(1);
	auto nextSunday = nextFriday + bgr::days(2);

	return std::make_tuple(nextFriday, nextSunday);
}

std::vector<std::string> getDestinations(std::string file)
{
	std::vector<std::string> destinations;
	std::string line;
	std::ifstream infile(file, std::ios_base::in);
	while (getline(infile, line, '\n'))
		destinations.push_back(line);

	return destinations;
}

QPXService::QPXService()
{
}

QPXService::~QPXService()
{
}

void QPXService::run()
{
    BOOST_LOG_TRIVIAL(trace) << "QPXService::run ";

    bfs::path p("../requests");

    std::vector<std::thread> thrs;
    for(auto file = bfs::directory_iterator(p); file != bfs::directory_iterator(); ++file)
	{
        BOOST_LOG_TRIVIAL(trace) << "probing " << file->path();
		if (file->path().extension() != ".json")
			continue;
		
        if (file->path().filename() == "base.json") {
            BOOST_LOG_TRIVIAL(trace) << "base " << file->path().string();
			auto jsonRequest = buildRequest(file->path().string());
			auto currDate = bgr::day_clock::local_day();
			auto destinations = getDestinations(bfs::path(p / "destinations.txt").string());
			BOOST_LOG_TRIVIAL(trace) << "today is " << currDate << "; destinations: " << boost::algorithm::join(destinations, ", ");

	        for (auto& dest : destinations)
	        {
				bgr::date friday, sunday;
				std::tie(friday, sunday) = getNextWeekend(currDate);

				auto& slices = jsonRequest[U("request")][U("slice")];
				auto fridaystr = bgr::to_iso_extended_string(friday);
				slices[0][U("date")] = web::json::value::string(utility::conversions::to_string_t(fridaystr));
				slices[0][U("destination")] = web::json::value::string(utility::conversions::to_string_t(dest));

				auto sundaystr = bgr::to_iso_extended_string(sunday);
				slices[1][U("date")] = web::json::value::string(utility::conversions::to_string_t(sundaystr));
				slices[1][U("origin")] = web::json::value::string(utility::conversions::to_string_t(dest));
				std::thread th([jsonRequest, this] {query(jsonRequest); });
				thrs.push_back(std::move(th));
	        }
        }
        else {
            //std::thread th( &QPXService::query, this, file->path().string() );
            // DISCUSSION:  capturing iterator object 'file' and calling query(file->path().string()) is not correct - race condition
            //  http://stackoverflow.com/questions/36325039/starting-c11-thread-with-a-lambda-capturing-local-variable
			auto filename = file->path().string();
			BOOST_LOG_TRIVIAL(trace) << "query from " << filename;
			auto request_msg = buildRequest(filename);
            std::thread th([request_msg, this] {query(request_msg); });
            thrs.push_back(std::move(th));
        }
    }

    for (auto& t : thrs)
        t.join();
}

void QPXService::query(const web::json::value request)
{
	BOOST_LOG_TRIVIAL(trace) << " request is " << request.serialize();

    try {
        auto client = web::http::client::http_client(Resources::URL + Resources::KEY);
        auto response = client.request(web::http::methods::POST, U(""), request).get();
        process(response);
    }
    catch (const web::json::json_exception &je) {
        BOOST_LOG_TRIVIAL(error) << "json error " << je.what();
    }
    catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "query error " << e.what();
    }
}

web::json::value QPXService::buildRequest(std::string filename)
{
    utility::ifstream_t file(filename);
    //std::locale::global(std::locale("", new std::codecvt_utf8<wchar_t>));
    //file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
    std::locale::global(boost::locale::generator().generate(""));
    file.imbue(std::locale());
    utility::stringstream_t buffer;
    buffer << file.rdbuf();
    file.close();

    web::json::value message;
    buffer >> message;
    return message;
}

class Leg
{
public:
    Leg(web::json::value val) : data(val) {}
    friend utility::ostream_t& operator<< (utility::ostream_t& stream, Leg const& leg);

    utility::string_t print() {
        utility::stringstream_t ss;
        ss << data[U("origin")] << " (" << data[U("departureTime")] << "); -> "
                << data[U("destination")] << " (" << data[U("arrivalTime")] << ");"
                << " (" << printDuration(data[U("duration")]) << ")";
        if (!data[U("connectionDuration")].is_null())
            ss << " layover: " << printDuration(data[U("connectionDuration")]);

        return ss.str();
    }
    
private:
    web::json::value data;
};

class Segment
{
public:
    Segment(web::json::value val) : data(val) {}

    utility::string_t print() { 
        utility::stringstream_t ss;
        ss << " carrier:" << data[U("flight")][U("carrier")];
        for (auto& leg : data[U("leg")].as_array())
            ss << "\n" << Leg(leg).print();
        if (!data[U("connectionDuration")].is_null())
            ss << " layover: " << printDuration(data[U("connectionDuration")]);
        
        return ss.str();
    }
    
private:
    web::json::value data;
};

class Slice
{
public:
    Slice(web::json::value val) : data(val) {}

    utility::string_t print() { 
        utility::stringstream_t ss;
        ss << " duration: " << printDuration(data[U("duration")]) << "\n";
        for (auto& s : data[U("segment")].as_array())
            ss << Segment(s).print();

        return ss.str();
    }
    
private:
    web::json::value data;
};


void QPXService::process(web::http::http_response response)
{
    if (response.status_code() != web::http::status_codes::OK) {
        BOOST_LOG_TRIVIAL(error) << " query unsuccesful: " << response.to_string();
        return;
    }

    auto json_value = response.extract_json().get();
    
    //BOOST_LOG_TRIVIAL(trace) << json_value.serialize();
    auto trips = json_value[U("trips")];
    
    auto data = trips[U("data")];
    if (data[U("tripOption")].is_null() )
    {
        BOOST_LOG_TRIVIAL(trace) << "No results!";
        return;
    }

    utility::stringstream_t sstr;
    for (auto &airport : data[U("airport")].as_array())
            sstr << airport[U("code")] << "/" << airport[U("name")] << "; ";
    BOOST_LOG_TRIVIAL(trace) << "-- airports: "<< sstr.str();

    sstr.str(utility::string_t());
    sstr.clear();
    for (auto &carrier : data[U("carrier")].as_array())
            sstr << carrier[U("code")] << "/" << carrier[U("name")] << "; ";
    BOOST_LOG_TRIVIAL(trace) << "-- carriers: "<< sstr.str();

    for (auto tripOpt : trips[U("tripOption")].as_array()) {
        utility::stringstream_t sstr;
        sstr << tripOpt[U("saleTotal")] << "\n";
        for (auto slice : tripOpt[U("slice")].as_array()) {
            sstr << Slice(slice).print() << "\n" ;
        }
        BOOST_LOG_TRIVIAL(trace) << "-- slice: " << sstr.str();
    }
}
