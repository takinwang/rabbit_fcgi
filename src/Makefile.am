bin_PROGRAMS=rabbit_fcgi

rabbit_fcgi_SOURCES=main.cpp
 
rabbit_fcgi_LDFLAGS=-lcrypto -lfcgi -lssl -lboost_random -lboost_system -lboost_thread -lboost_regex -lz -lrt -lpthread
	 
rabbit_fcgi_CPPFLAGS=-std=c++11 -Wall -g -ggdb

#-I$(top_srcdir)/include 