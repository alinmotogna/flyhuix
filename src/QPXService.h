/*
 * QPXService.h
 *
 *  Created on: Mar 14, 2016
 *      Author: alin.motogna
 */

#pragma once
#include <string>
#include <cpprest/http_msg.h>

class QPXService
{
public:
    struct Resources
    {
        static const utility::string_t URL;
        static const utility::string_t KEY;
    };

    QPXService();
    ~QPXService();

    void run();

private:
    void query(std::string req_file);
};