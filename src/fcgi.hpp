/*
 * fcgi.hpp
 *
 *  Created on: Apr 1, 2016
 *      Author: root
 */

#ifndef FCGI_HPP_
#define FCGI_HPP_

#include "Utils.hpp"
#include <fstream>
#include <map>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#include<sys/wait.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>

#define FCGI_LISTENSOCK_FILENO 0
#define nginx_context_path "^/covjson"

# include <sys/socket.h>
# include <sys/ioctl.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <sys/un.h>
# include <arpa/inet.h>

# include <netdb.h>

#include <fcgiapp.h>
//#include <fcgi_stdio.h>

namespace fcgi {

//#define  FCGI_printf(...) FCGX_FPrintF(request->out, __VA_ARGS__)

#define  FCGI_ENVP(VAR, KEY) { \
	const char * s = FCGX_GetParam(KEY, cgi_request_.envp);  \
	VAR = (s == NULL) ? std::string(): std::string(s); \
}

class request;
class response;

class request {
	private:
		std::unordered_map<std::string, std::string> querys;
		std::string null_string;
		FCGX_Request &cgi_request_;
	public:
		request(FCGX_Request &cgi_request_)
				: cgi_request_(cgi_request_) {

			FCGI_ENVP(DOCUMENT_ROOT, "DOCUMENT_ROOT");
			FCGI_ENVP(HTTP_COOKIE, "HTTP_COOKIE");
			FCGI_ENVP(HTTP_HOST, "HTTP_HOST");
			FCGI_ENVP(HTTP_REFERER, "HTTP_REFERER");
			FCGI_ENVP(HTTP_USER_AGENT, "HTTP_USER_AGENT");
			FCGI_ENVP(HTTPS, "HTTPS");
			FCGI_ENVP(PATH, "PATH");
			FCGI_ENVP(QUERY_STRING, "QUERY_STRING");
			FCGI_ENVP(REMOTE_ADDR, "REMOTE_ADDR");
			FCGI_ENVP(REMOTE_HOST, "REMOTE_HOST");
			FCGI_ENVP(REMOTE_PORT, "REMOTE_PORT");
			FCGI_ENVP(REMOTE_USER, "REMOTE_USER");
			FCGI_ENVP(REQUEST_METHOD, "REQUEST_METHOD");
			FCGI_ENVP(REQUEST_URI, "REQUEST_URI");
			FCGI_ENVP(SCRIPT_FILENAME, "SCRIPT_FILENAME");
			FCGI_ENVP(SCRIPT_NAME, "SCRIPT_NAME");
			FCGI_ENVP(SERVER_ADMIN, "SERVER_ADMIN");
			FCGI_ENVP(SERVER_NAME, "SERVER_NAME");
			FCGI_ENVP(SERVER_SOFTWARE, "SERVER_SOFTWARE");

			QUERY_STRING = Utils::uri_decode(QUERY_STRING);
			REQUEST_URI = Utils::uri_decode(REQUEST_URI);

			size_t pos = REQUEST_URI.find("?");
			if (pos > 0) REQUEST_URI = REQUEST_URI.substr(0, pos);

			Utils::parse_querys(querys, QUERY_STRING);
		}

		const std::string & get_parameter(const std::string & key) {
			return Utils::get_map_value(querys, key, null_string);
		}

	public:
		std::string DOCUMENT_ROOT;
		std::string HTTP_COOKIE;
		std::string HTTP_HOST;
		std::string HTTP_REFERER;
		std::string HTTP_USER_AGENT;
		std::string HTTPS;
		std::string PATH;
		std::string QUERY_STRING;
		std::string REMOTE_ADDR;
		std::string REMOTE_HOST;
		std::string REMOTE_PORT;
		std::string REMOTE_USER;
		std::string REQUEST_METHOD;
		std::string REQUEST_URI;
		std::string SCRIPT_FILENAME;
		std::string SCRIPT_NAME;
		std::string SERVER_ADMIN;
		std::string SERVER_NAME;
		std::string SERVER_SOFTWARE;

};

class response {
	private:
		std::unordered_map<int, std::string> status_lines;
		FCGX_Request &cgi_request_;
		request & request_;
		bool commited;
	public:
		response(FCGX_Request &cgi_request_, request & request)
				: cgi_request_(cgi_request_), request_(request), commited(false), status(200) {

			status_lines[200] = "OK";
			status_lines[201] = "Created";
			status_lines[202] = "Accepted";
			status_lines[204] = "No Content";
			status_lines[206] = "Partial Content";
			status_lines[301] = "Moved Permanently";
			status_lines[302] = "Moved Temporarily";
			status_lines[303] = "See Other";
			status_lines[304] = "Not Modified";
			status_lines[307] = "Temporary Redirect";
			status_lines[400] = "Bad Request";
			status_lines[401] = "Unauthorized";
			status_lines[402] = "Payment Required";
			status_lines[403] = "Forbidden";
			status_lines[404] = "Not Found";
			status_lines[405] = "Not Allowed";
			status_lines[406] = "Not Acceptable";
			status_lines[408] = "Request Time-out";
			status_lines[409] = "Conflict";
			status_lines[410] = "Gone";
			status_lines[411] = "Length Required";
			status_lines[412] = "Precondition Failed";
			status_lines[413] = "Request Entity Too Large";
			status_lines[414] = "Request-URI Too Large";
			status_lines[415] = "Unsupported Media Type";
			status_lines[416] = "Requested Range Not Satisfiable";
			status_lines[500] = "Internal Server Error";
			status_lines[501] = "Not Implemented";
			status_lines[502] = "Bad Gateway";
			status_lines[503] = "Service Temporarily Unavailable";
			status_lines[504] = "Gateway Time-out";
			status_lines[507] = "Insufficient Storage";

			headers["Content-type"] = "text/html;charset=UTF-8";
		}
		~response() {
			commit();
			if (fstream.is_open()) fstream.close();
		}

		void commit() {
			if (commited == true) return;
			commited = true;

			if (status_lines.find(status) == status_lines.end()) {
				status = 500;
			}

			std::string message = status_lines[status];
			std::string message_str = (boost::format("%s %s") % status % message.c_str()).str();
			headers["Status"] = message_str;

			for (auto iter : headers) {
				FCGX_FPrintF(cgi_request_.out, "%s: %s\r\n", iter.first.c_str(), iter.second.c_str());
			}

			FCGX_FPrintF(cgi_request_.out, "\r\n");
			if (status >= 300) {
				if (status >= 400) FCGX_FPrintF(cgi_request_.out, message_str.c_str());
				return;
			}

			if (fstream.is_open()) {
				char buf[8193];

				while (fstream.good() == true) {
					fstream.read(&buf[0], 8192);
					size_t ncount = fstream.gcount();
					if (ncount == 0) {
						break;
					}
					FCGX_FPrintF(cgi_request_.out, &buf[0], ncount);
				}
				return;
			}

			std::string data = sstream.str();
			if (data.size() > 0) {
				FCGX_FPrintF(cgi_request_.out, data.c_str());
			}
		}

	public:
		std::unordered_map<std::string, std::string> headers;

		size_t status;
		std::string message;

		std::stringstream sstream;
		std::ifstream fstream;
};

class Endpoint {
	public:
		std::function<void(request &, response &)> on_call;
};

static std::map<std::string, Endpoint> Endpoints;

}

#endif /* FCGI_HPP_ */
