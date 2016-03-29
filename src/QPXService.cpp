/*
 * QPXService.cpp
 *
 *  Created on: Mar 14, 2016
 *      Author: alin.motogna
 */

#include "QPXService.h"
#include <boost/log/trivial.hpp>
#include <thread>
#include <cpprest/http_client.h>
#include <boost/filesystem.hpp>
#include <locale>
#include <boost/locale.hpp>
#include <iomanip>

const utility::string_t QPXService::Resources::URL=U("https://www.googleapis.com/qpxExpress/v1/trips/search?key=");
const utility::string_t QPXService::Resources::KEY=U("AIzaSyDLo2QRtu4g-k-hPUv3kLRY5c6dDMWOv_0");

std::string printDuration(web::json::value duration)
{
    using namespace std::chrono;
    auto total_mins = minutes(duration.as_integer());
    
    auto hrs = duration_cast<hours>(total_mins);
    total_mins -= hrs;
    auto mins = duration_cast<minutes>(total_mins);
    
    utility::stringstream_t ss;
    ss << std::setw(2) << std::setfill('0') << hrs.count() << ":" << mins.count();
    return ss.str();
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

    boost::filesystem::path p("../requests");

    std::vector<std::thread> thrs;
    for(auto file = boost::filesystem::directory_iterator(p); file != boost::filesystem::directory_iterator(); file++)
    {
        BOOST_LOG_TRIVIAL(trace) << "probing " << file->path();
        if (file->path().extension() == ".json"){
            std::thread th( &QPXService::query, this, file->path().string() );
            //std::thread th( [this, file]() {query(file->path().string());} );
            thrs.push_back(std::move(th));
        }
    }

    for (auto& t : thrs)
        t.join();
}

void QPXService::query(const std::string filename)
{
    BOOST_LOG_TRIVIAL(trace) << "query from " << filename;
    try {
        auto request_msg = buildRequest(filename);
        auto client = web::http::client::http_client(Resources::URL + Resources::KEY);
        auto response = client.request(web::http::methods::POST, U(""), request_msg).get();
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
    BOOST_LOG_TRIVIAL(trace) << " message is " << message.serialize();
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
        for (auto& leg : data[U("leg")].as_array())
            ss << Leg(leg).print() << "\n";
        if (!data[U("connectionDuration")].is_null())
            ss << " layover: " << printDuration(data[U("connectionDuration")]) << "\n";
        
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
            ss << Segment(s).print() << "\n";

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
    auto trips = json_value[U("trips")];
    
    auto data = trips[U("data")];
    BOOST_LOG_TRIVIAL(trace) << data.serialize();

    utility::stringstream_t sstr;
    for (auto &a : data[U("airport")].as_array())
            sstr << a[U("code")] << "/" << a[U("name")] << "; ";
    BOOST_LOG_TRIVIAL(trace) << "-- airports: "<< sstr.str();

    for (auto tripOpt : trips[U("tripOption")].as_array()) {
        utility::stringstream_t sstr;
        sstr << tripOpt[U("saleTotal")] << "\n";
        for (auto slice : tripOpt[U("slice")].as_array()) {
            sstr << Slice(slice).print() << "\n";
        }
        BOOST_LOG_TRIVIAL(trace) << "-- slice: " << sstr.str();
    }
}
