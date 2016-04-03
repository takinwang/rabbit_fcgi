/*
 ============================================================================
 Name        : conjson.cpp
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C++,
 ============================================================================
 */

#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <boost/regex.hpp>

#include <pthread.h>

#include "Utils.hpp"

#include "fcgiapp.h"
#include "fcgi.hpp"

/************************/
#include "endp0_impl.hpp"
#include "endp1_impl.hpp"
/************************/

const char * pid_file = "/var/run/covjson.pid";

void write_pid() {
	std::ofstream outfile;
	outfile.open(pid_file);

	if (outfile.is_open() == false) {
		std::cerr << "Write pid_file failed:" << pid_file << std::endl;
		return;
	}
	std::string t = boost::lexical_cast<std::string, int>(getpid());
	outfile.write(t.c_str(), t.size());
	outfile.close();
}

void unlink_pid() {
	struct stat st;
	if (stat(pid_file, &st) == 0) unlink(pid_file);
}

void _signal_term(int code) {
	kill(0, SIGTERM); // kill all forks
	unlink_pid();
	exit(0);
}

int stop_app() {
	std::string line;

	std::ifstream infile;
	infile.open(pid_file);

	if (infile.is_open() == false) return -1;
	if (infile.eof() == false) getline(infile, line);
	infile.close();

	struct stat st;
	if (stat(pid_file, &st) == 0) unlink(pid_file);

	boost::trim<std::string>(line);
	if (line.length() == 0) return -1;

	int fork_pid = 0;
	try {
		fork_pid = boost::lexical_cast<int, std::string>(line);
	}
	catch (std::exception &e) {
	}

	if (fork_pid > 0) {
		std::cout << "Sending signal to pid: " << fork_pid << std::endl;

		kill(fork_pid, SIGTERM);
		kill(fork_pid, SIGINT);

		waitpid(fork_pid, NULL, 0);

		std::cout << "Terminated, pid: " << fork_pid << std::endl;
		return 0;
	}

	return -1;
}

static void * run_thread_fcgi(void *a) {
	std::vector<std::pair<boost::regex, fcgi::Endpoint*> > opt_endpoint;

	for (auto& endp : fcgi::Endpoints) {
		opt_endpoint.emplace_back(boost::regex(endp.first), &endp.second);
	}

	FCGX_Request cgi_request_;
	FCGX_InitRequest(&cgi_request_, FCGI_LISTENSOCK_FILENO, 0);

	while (FCGX_Accept_r(&cgi_request_) >= 0) {
		{
			fcgi::request req(cgi_request_);
			fcgi::response resp(cgi_request_, req);

			for (auto& endp : opt_endpoint) {
				boost::smatch path_match;

				if (boost::regex_match(req.REQUEST_URI, path_match, endp.first)) {
					if (endp.second->on_call) endp.second->on_call(req, resp);
					break;
				}
			}

			resp.status = 404;
		}

		FCGX_Finish_r(&cgi_request_);
	}
	return NULL;
}

int run_fcgi(int thrd_count) {

	FCGX_Init();

	pthread_t thread_id[thrd_count];

	for (int i = 0; i < thrd_count; i++)
		pthread_create(&thread_id[i], NULL, run_thread_fcgi, NULL);

	for (int i = 0; i < thrd_count; i++)
		pthread_join(thread_id[i], NULL);

	return 0;
}

int fcgi_spawn_connection(int fork_count, int thrd_count) {
	int rc = 0;
	struct timeval tv = { 0, 100 * 1000 };
	pid_t child;
	int children = 0;

	for (;;) {
		switch (child = fork()) {
		case 0: {
			/* in child */

//			setsid();
//			chdir("/");

			int max_fd = open("/dev/null", O_RDWR);
			if (-1 != max_fd) {
				if (max_fd != STDOUT_FILENO) dup2(max_fd, STDOUT_FILENO);
				if (max_fd != STDERR_FILENO) dup2(max_fd, STDERR_FILENO);
				if (max_fd != STDOUT_FILENO && max_fd != STDERR_FILENO) close(max_fd);
			}
			else {
				std::cerr << "Couldn't open and redirect stdout/stderr to '/dev/null': " << strerror(errno)
						<< std::endl;
			}

			/* we don't need the client socket */
			for (int i = 3; i < max_fd; i++) {
				if (i != FCGI_LISTENSOCK_FILENO) close(i);
			}

			run_fcgi(thrd_count);

			exit(0);
		}
		case -1: {
			/* error */
			std::cerr << "Fork failed: " << strerror(errno) << std::endl;
			exit(0);
		}
		default: {
			/* in parent */

			select(0, NULL, NULL, NULL, &tv);

			int status;
			children++;
			while (children >= fork_count) {
				switch (child = waitpid(WAIT_ANY, &status, 0)) {
				case -1:
					std::cerr << "Waitpid failed: " << strerror(errno) << std::endl;
					exit(0);
				default:
//					fprintf(stdout, "Child spawned waitpid PID: %d\n", child);

					if (WIFEXITED(status)) {
						std::cerr << "Child exited with: " << WEXITSTATUS(status) << std::endl;
						children--;
					}
					else if (WIFSIGNALED(status)) {
						std::cerr << "Child signaled: " << WTERMSIG(status) << std::endl;
						int status0;
						pid_t child0;
						switch (child0 = waitpid(child, &status0, WNOHANG)) {
						case -1: {
							std::cerr << "Waitpid [" << child << "] failed: " << strerror(errno) << std::endl;
							children--;
							break;
						}
						default:
							if (WIFEXITED(status0)) {
								std::cerr << "Child exited with: " << WEXITSTATUS(status0) << std::endl;
								children--;
							}
						}
					}
					else {
						std::cerr << "Child died somehow, exit status: " << status << std::endl;
						children--;
					}
				}

			}
		}
		}
	}

	return rc;
}

int main(int argc, char **argv) {

	if (argc == 2) {
		if (boost::equal<std::string, std::string>(argv[1], "stop")) return stop_app();
	}

	if (stop_app() == 0) {
		sleep(1);
	}

	unsigned short port = 9000;

	int fork_count = 0;
	int thrd_count = 1;
	int daemonized = 0;

	for (int i = 1; i < argc; i++) {
		std::string a_arg = argv[i];
		if (boost::equal<std::string, std::string>(a_arg, "-d")) {
			daemonized = 1;
		}
		else if (boost::equal<std::string, std::string>(a_arg, "-p") && (i < argc - 1)) {
			port = boost::lexical_cast<int, std::string>(argv[++i]);
		}
		else if (boost::equal<std::string, std::string>(a_arg, "-F") && (i < argc - 1)) {
			fork_count = boost::lexical_cast<int, std::string>(argv[++i]);
		}
		else if (boost::equal<std::string, std::string>(a_arg, "-T") && (i < argc - 1)) {
			thrd_count = boost::lexical_cast<int, std::string>(argv[++i]);
		}
		else if (boost::equal<std::string, std::string>(a_arg, "-h")) {
			std::cout << "Example: " << argv[0] << " <-p port> <-F forks> <-T threads> <-d>" << std::endl;
			return -1;
		}
	}

	std::cout << "Start with " << fork_count << " forks and " << thrd_count << " threads each fork, listen on port: "
			<< port << std::endl;

	signal(SIGTERM, _signal_term);
	signal(SIGINT, _signal_term);

	if (daemonized > 0) {
		daemon(0, 0);
	}

	std::string socket_port = (boost::format(":%d") % port).str();
	int fcgi_fd = FCGX_OpenSocket(socket_port.c_str(), 1024);
	if (-1 == fcgi_fd) {
		return -1;
	}

	write_pid();

	if (fcgi_fd != FCGI_LISTENSOCK_FILENO) {
		close(FCGI_LISTENSOCK_FILENO);
		dup2(fcgi_fd, FCGI_LISTENSOCK_FILENO);
		close(fcgi_fd);
	}

	int val = 1;
	if (setsockopt(FCGI_LISTENSOCK_FILENO, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
		std::cerr << "Couldn't set SO_REUSEADDR: " << strerror(errno) << std::endl;
		return -1;
	}

	if (fork_count <= 0) return run_fcgi(thrd_count);

	return fcgi_spawn_connection(fork_count, thrd_count);
}
