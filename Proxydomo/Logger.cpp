/**
	@file	Logger.cpp
*/

#include "stdafx.h"
#include "Logger.h"

#ifdef _DEBUG
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sinks/debug_output_backend.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/empty_deleter.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/trivial.hpp>


///////////////////////////////////////////////////////
// CLogger

using namespace boost;
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

void CLogger::Init()
{
	{ // for Visual Studio
		shared_ptr<sinks::debug_output_backend> backend =
			make_shared<sinks::debug_output_backend>();

		typedef sinks::synchronous_sink<sinks::debug_output_backend> sink_t;
		shared_ptr<sink_t> sink(new sink_t(backend));

		sink->set_formatter(
			expr::format("%1%\t%2%\t%3%\n")
			% expr::format_date_time<posix_time::ptime>(
			"TimeStamp", "%H:%M:%S")
			% logging::trivial::severity
			% expr::message
			);

		logging::core::get()->add_sink(sink);
	}


	BOOST_LOG_TRIVIAL(trace) << "A trace severity message";
	BOOST_LOG_TRIVIAL(debug) << "A debug severity message";
	BOOST_LOG_TRIVIAL(info) << "An informational severity message";
	BOOST_LOG_TRIVIAL(warning) << "A warning severity message";
	BOOST_LOG_TRIVIAL(error) << "An error severity message";
	BOOST_LOG_TRIVIAL(fatal) << "A fatal severity message";

}

#endif
