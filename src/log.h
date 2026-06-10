#pragma once
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>
#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/core/record_view.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/json/object.hpp>
#include "boost/json/serialize.hpp"
#include <boost/log/utility/value_ref.hpp>
#include <utility>
#include <string>

namespace json = boost::json;
namespace logging = boost::log;

inline void MyFormatter(logging::record_view const& req, logging::formatting_ostream& osrm) {
	json::object res;
	boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
	res["timestampt"] = boost::posix_time::to_iso_extended_string(now);
	logging::value_ref<json::object> data = logging::extract<json::object>("data", req);
	res["data"] = std::move(data.get());
	logging::value_ref<std::string> msg = logging::extract<std::string>("msg", req);
	res["message"] = std::move(msg.get());
	osrm << res;
}