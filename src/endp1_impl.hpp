/*
 * covjson_impl.hpp
 *
 *  Created on: Apr 2, 2016
 *      Author: root
 */

#ifndef ENDP1_IMPL_HPP_
#define ENDP1_IMPL_HPP_

#include "fcgi.hpp"
#include <pthread.h>

namespace endpoint1 {

class endp {
	private:
		static endp obj;
	public:
		endp() {
			auto & t0 = fcgi::Endpoints[nginx_context_path "/t1.*"];
			t0.on_call =
					[]( fcgi:: request & req, fcgi::response & resp) {

						std::string a = req.get_parameter("testid");
						if (a == "q") {
							resp.status = 404;
							return;
						}

						resp.sstream << "<html><head><title>FastCGI Hello!</title></head>";
						resp.sstream << "<body>";
						resp.sstream << "<h1>FastCGI Hello!</h1>";
						resp.sstream << req.REQUEST_URI << "<br>";
						resp.sstream << boost::format(
								"pgid:%s, pid:%d, threadid:%d\n<br><i>%s</i>\n") % getpgrp() % getpid() % pthread_self() % a.c_str();
						resp.sstream << "</body><html>";
						resp.commit();
					};
		}
};

endp endp::obj; //auto register fcgi::Endpoints callback

}

#endif /* ENDP0_IMPL_HPP_ */
