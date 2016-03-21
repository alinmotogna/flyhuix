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
    }

    for (auto& t : thrs)
        t.join();
}

void QPXService::query(std::string filename)
{
    BOOST_LOG_TRIVIAL(trace) << "query from " << filename;
    web::http::client::http_client client(Resources::URL + Resources::KEY);

    std::ifstream file(filename);
    if ( !file )
    {
        BOOST_LOG_TRIVIAL(error) << "error opening " << filename;
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    web::json::value message;
    buffer >> message;
    BOOST_LOG_TRIVIAL(trace) << " message is " << message.serialize();

    BOOST_LOG_TRIVIAL(trace) << " running ";
    client.request(web::http::methods::POST, "", message)
            .then([&](web::http::http_response response)
            {
                BOOST_LOG_TRIVIAL(trace) << " -> response code:" << response.status_code();
                if(response.status_code() == web::http::status_codes::OK)
                    return response.extract_json();

                return pplx::create_task([] { return web::json::value(); });
            })
            .then([](web::json::value json_value)
            {
                if(json_value.is_null())
                  return;

                BOOST_LOG_TRIVIAL(trace) << json_value.serialize();
            })
            .wait();
}
