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
#include <codecvt>

#define TRIPS		U("trips")
#define DATA		U("data")
#define AIRPORT		U("airport")
#define TRIPOPTION	U("tripOption")

const utility::string_t QPXService::Resources::URL=U("https://www.googleapis.com/qpxExpress/v1/trips/search?key=");
const utility::string_t QPXService::Resources::KEY=U("AIzaSyDLo2QRtu4g-k-hPUv3kLRY5c6dDMWOv_0");

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
    for (auto file = boost::filesystem::directory_iterator(p); file != boost::filesystem::directory_iterator(); ++file)
    {
        std::thread th( [=]{ query(file->path().string()); });
        thrs.push_back(std::move(th));
		break;
    }

    for (auto& t : thrs)
        t.join();
}

void QPXService::query(std::string filename)
{
	BOOST_LOG_TRIVIAL(trace) << "query from " << filename;
	try {
		auto request_msg = buildRequest(filename);
		auto client = web::http::client::http_client(Resources::URL + Resources::KEY);
		auto response = client.request(web::http::methods::POST, L"", request_msg).get();
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
	//file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
	std::locale::global(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
	utility::stringstream_t buffer;
	buffer << file.rdbuf();
	file.close();

	web::json::value message;
	buffer >> message;
	BOOST_LOG_TRIVIAL(trace) << " message is " << message.serialize();
	return message;
}

void QPXService::process(web::http::http_response response)
{
	if (response.status_code() != web::http::status_codes::OK) {
		BOOST_LOG_TRIVIAL(error) << " query unsuccesful: " << response.to_string();
		return;
	}

	auto json_value = response.extract_json().get();
	auto trips = json_value[TRIPS];
	auto data = trips[DATA];

	BOOST_LOG_TRIVIAL(trace) << data.serialize();

	utility::stringstream_t sstr;
	for(auto& airport : data[AIRPORT].as_array())
		BOOST_LOG_TRIVIAL(trace) << "-- airport: "<< airport[U("code")].as_string() << "/" << airport[U("name")].as_string();

	for (auto tripOpt : trips[TRIPOPTION].as_array()) {
		utility::stringstream_t sstr;
		sstr << tripOpt[U("saleTotal")] << "\n";
		for (auto slice : tripOpt[U("slice")].as_array()) {
			sstr << "duration:" << slice[U("duration")].as_integer() << "\n";		
			for (auto segment : slice[U("segment")].as_array()) {
				for (auto leg : segment[U("leg")].as_array())
					sstr << leg[U("origin")] << " [" << leg[U("departureTime")] << "]; "
						<< leg[U("destination")] << " [" << leg[U("arrivalTime")] << "]; "
						<< " (" << leg[U("duration")] << ")"
						<< " ++ " << leg[U("connectionDuration")];

				sstr << " ++ " << segment[U("connectionDuration")] << "\n";
			}
		}
		BOOST_LOG_TRIVIAL(trace) << "-- slice: " << sstr.str();
	}
}