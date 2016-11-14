/*
 * QPXService.h
 *
 *  Created on: Mar 14, 2016
 *      Author: alin.motogna
 */

#pragma once
#include <cpprest/http_msg.h>

class QPXService
{
public:
    struct Resources
    {
        static const utility::string_t URL;
        static const utility::string_t KEY;
		static const utility::string_t BASE_JSON_QUERY;
    };

    QPXService();
    ~QPXService();

    void run();

private:
	web::json::value buildRequest(std::string filename);
    void query(const web::json::value request_msg);
	void process(web::http::http_response response);
};
