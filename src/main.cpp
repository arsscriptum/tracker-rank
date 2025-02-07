
//==============================================================================
//
//     main.cpp
//
//============================================================================
//  Copyright (C) Guilaume Plante 2020 <cybercastor@icloud.com>
//==============================================================================



#include "stdafx.h"
#include "cmdline.h"
#include "Shlwapi.h"
#include "log.h"

#include <codecvt>
#include <locale>
#include <vector>
#include <unordered_map>
#include <iterator>
#include <regex>
#include <filesystem>
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <fstream>
#include "Shlwapi.h"
#include "log.h"
#include <ctime>
#include <iostream>
#include <string>
#include <algorithm>
//#include "jsmn.h"  // Include JSMN header

#include <libtorrent/settings_pack.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/error_code.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/error_code.hpp>



using namespace std;
using namespace std::chrono_literals;

#pragma message( "Compiling " __FILE__ )
#pragma message( "Last modified on " __TIMESTAMP__ )

int unique_id = 0;
int default_timeout_sec = 10;
int max_url_len = 48;
int download_latest();
bool downloadFile(string dwl_url, string fullUrl, string outFile, bool optVerbose);
bool testConnection(const char* host, int port, time_t conn_timeout_sec, time_t read_timeout_sec, time_t write_timeout_sec);
void logError(const char* msg);
void test_tracker(const std::string& tracker_url, bool is_json = false, int num_want = 10);
void rate_indexer(const std::string& magnet_uri, bool is_json = false, int num_want = 10);
void banner();
void usage();


bool isNumeric(const std::string& str) {
	return !str.empty() && std::all_of(str.begin(), str.end(), ::isdigit);
}


void test_tracker(const std::string& tracker_url, bool is_json, int num_want) {
	lt::settings_pack settings;
	settings.set_int(lt::settings_pack::alert_mask, lt::alert_category::tracker);
	settings.set_int(lt::settings_pack::num_want, num_want);

	lt::session session(settings);

	lt::add_torrent_params params;
	params.trackers.push_back(tracker_url);
	params.info_hashes.v1 = lt::sha1_hash("01234567890123456789"); // Fake info-hash for testing
	params.name = "Test Torrent";
	params.save_path = "./downloads";

	auto handle = session.add_torrent(std::move(params));

	bool tracker_responded = false;
	int num_peers = 0;
	std::string details = "n/a";

	auto start_time = std::chrono::high_resolution_clock::now();
	int iterations = default_timeout_sec * 2;
	for (int i = 0; i < iterations; ++i) { // 10 seconds timeout
		std::vector<lt::alert*> alerts;
		session.pop_alerts(&alerts);

		for (const auto* alert : alerts) {
			if (auto reply = lt::alert_cast<lt::tracker_reply_alert>(alert)) {
				tracker_responded = true;
				num_peers = reply->num_peers;
				details = "Valid response";
			}
			else if (auto error = lt::alert_cast<lt::tracker_error_alert>(alert)) {
				details = error->error.message();
				tracker_responded = false;
			}
		}

		if (tracker_responded) break;

		std::this_thread::sleep_for(0.5s);
	}

	auto end_time = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> response_time = end_time - start_time;

	// Get current timestamp
	auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::ostringstream timestamp;
	timestamp << std::put_time(std::localtime(&now), "%Y-%m-%dT%H:%M:%S");


	/*
	TODO:  return this instead
	{
	  "unique_id": 1,
	  "is_valid": true,
	  "num_peers": 3,
	  "response_time": 2.33,
	  "url": "udp://tracker.dler.org:6969/announce",
	  "last_tested": "2025-02-07T09:12:42"
	}
	*/
	
	if (is_json) {
		// Manually create a JSON string using JSMN-style formatting
		std::ostringstream json_output;
		json_output << "{"
			<< "\"unique_id\": " << unique_id++ << ", "
			<< "\"is_valid\": " << (tracker_responded ? "true" : "false") << ", "
			<< "\"num_peers\": " << num_peers << ", "
			<< "\"response_time\": " << response_time.count() << ", "
			<< "\"url\": \"" << tracker_url << "\", "
			<< "\"last_tested\": \"" << timestamp.str() << "\""
			<< "}" << std::flush;

		std::cout << json_output.str();
	}
	else {
		std::cout << std::left << std::setw(max_url_len) << tracker_url
			<< std::setw(10) << (tracker_responded ? "valid" : "invalid")
			<< std::setw(8) << (tracker_responded ? std::to_string(num_peers) : "0")
			<< std::setw(12) << (tracker_responded ? std::to_string(response_time.count()) : "n/a" )
			<< details << std::endl << std::flush;
	}

	session.remove_torrent(handle);
}

void rate_indexer(const std::string& magnet_uri, bool is_json, int num_want) {
	// Create session parameters and a session
	lt::settings_pack settings;
	settings.set_int(lt::settings_pack::alert_mask, lt::alert::tracker_notification);
	settings.set_int(lt::settings_pack::num_want, num_want);
	lt::session session(settings);
	bool tracker_responded = false;
	// Add a magnet URI to the session
	lt::error_code ec;
	lt::add_torrent_params params = lt::parse_magnet_uri(magnet_uri, ec);

	if (ec) {
		std::cerr << "Failed to parse magnet URI: " << ec.message() << std::endl;
		return;
	}

	params.save_path = "./downloads"; // Path where files will be downloaded
	auto handle = session.add_torrent(std::move(params));

	std::cout << "Connecting to trackers..." << std::endl;

	// Trackers and quality rating
	std::map<std::string, double> tracker_ratings;
	std::map<std::string, int> tracker_ratings_peers;

	auto start_time = std::chrono::high_resolution_clock::now();
	int iterations = default_timeout_sec * 2;
	for (int i = 0; i < iterations; ++i) {
		std::vector<lt::alert*> alerts;
		session.pop_alerts(&alerts);

		for (auto alert : alerts) {
			if (auto t_alert = lt::alert_cast<lt::tracker_reply_alert>(alert)) {
				auto tracker = t_alert->url;
				tracker_responded = true;
				
				auto end_time = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> response_time = end_time - start_time;
				tracker_ratings[tracker] += (t_alert->num_peers * response_time.count());
				tracker_ratings_peers[tracker] += (t_alert->num_peers);
				std::cout << "Tracker: " << tracker << " responded with " << t_alert->num_peers << " peers in " << std::to_string(response_time.count()) + " seconds"  <<  std::endl;
			}
		}

		std::this_thread::sleep_for(0.5s);
	}



	std::cout << "\n--- Tracker Quality Ratings Peers ---\n";
	for (const auto& [tracker, rating] : tracker_ratings_peers) {
		std::cout << "Tracker: " << tracker << ", Quality Rating: " << rating << std::endl;
	}
	std::cout << "\n--- Tracker Quality Ratings Calibrated with Time ---\n";
	for (const auto& [tracker, rating] : tracker_ratings_peers) {
		std::cout << "Tracker: " << tracker << ", Quality Rating: " << rating << std::endl;
	}
	session.remove_torrent(handle);
}


int main(int argc, TCHAR** argv, TCHAR envp)
{

#ifdef UNICODE
	const char** argn = (const char**)C::Convert::allocate_argn(argc, argv);
#else
	char** argn = argv;
#endif // UNICODE

	CmdLineUtil::getInstance()->initializeCmdlineParser(argc, argn);

	CmdlineParser* inputParser = CmdLineUtil::getInstance()->getInputParser();

	CmdlineOption cmdlineOptionHelp({		"-h", "--help" }, "display this help");
	CmdlineOption cmdlineOptionVerbose({	"-v", "--verbose" }, "verbose output");
	CmdlineOption cmdlineOptionTimeout({	"-t", "--timeout" }, "timeout in seconds for each tracker requests");
	CmdlineOption cmdlineOptionPath({		"-p", "--path" }, "path");
	CmdlineOption cmdlineOptionJson({		"-j", "--json" }, "json  format");
	CmdlineOption cmdlineOptionIndexer({	"-i", "--indexer" }, "indexer");
	CmdlineOption cmdlineOptionDownload({	"-d", "--download" }, "download latest");
	CmdlineOption cmdlineOptionNoBanner({	"-n", "--nobanner" }, "no banner");
	CmdlineOption cmdlineOptionTrackerUrl({ "-u", "--url" }, "tracker url (i.e. udp://mytrack:1337/announce)");
	CmdlineOption cmdlineOptionWhatIf({		"-w", "--whatif" }, "whatif");

	inputParser->addOption(cmdlineOptionHelp);
	inputParser->addOption(cmdlineOptionVerbose);
	inputParser->addOption(cmdlineOptionDownload);
	inputParser->addOption(cmdlineOptionNoBanner);
	inputParser->addOption(cmdlineOptionWhatIf);
	inputParser->addOption(cmdlineOptionTrackerUrl);
	inputParser->addOption(cmdlineOptionPath);
	inputParser->addOption(cmdlineOptionJson);
	inputParser->addOption(cmdlineOptionIndexer);
	inputParser->addOption(cmdlineOptionTimeout);

	


	bool optHelp = inputParser->isSet(cmdlineOptionHelp);
	bool optVerbose = inputParser->isSet(cmdlineOptionVerbose);
	bool optTracker = inputParser->isSet(cmdlineOptionTrackerUrl);
	bool optDownload = inputParser->isSet(cmdlineOptionDownload);
	bool optPath = inputParser->isSet(cmdlineOptionPath);
	bool optJson = inputParser->isSet(cmdlineOptionJson);
	bool optIndexer = inputParser->isSet(cmdlineOptionIndexer);
	bool optNoBanner = inputParser->isSet(cmdlineOptionNoBanner);
	bool optWhatIf = inputParser->isSet(cmdlineOptionWhatIf);
	bool optTimeout = inputParser->isSet(cmdlineOptionTimeout);
	

	if (optNoBanner == false) {
		banner();
	}
	if (optHelp) {
		usage();
		return 0;
	}
	std::string file_path = "";
	std::string tracker_url = "";
	

	if (optTimeout) {
		std::string str_timeout_sec = inputParser->getCmdOption("-t");
		if (isNumeric(str_timeout_sec)) {
			default_timeout_sec = std::stoi(str_timeout_sec);
		}
	}

	if (optTracker) {
		tracker_url = inputParser->getCmdOption("-u");
		test_tracker(tracker_url, optJson);
	}
	else if (optIndexer) {
		std::string magnet_url = inputParser->getCmdOption("-i");
		rate_indexer(magnet_url, optJson);
	}
	else if (optPath) {
		file_path = inputParser->getCmdOption("-p");

		std::string filename(file_path);

		std::ifstream file(filename); // Ensure this matches the correct type
		if (!file.is_open()) {
			std::cerr << "Failed to open file: " << filename << std::endl;
			return 1;
		}

		if (optJson) {
			std::cout << "[" << std::endl << std::flush;
		} else {
			std::cout << std::endl << std::endl << std::left << std::setw(max_url_len) << "Tracker Address"
				<< std::setw(10) << "Status"
				<< std::setw(8) << "Peers"
				<< std::setw(12) << "Latency"
				<< "Details" << std::endl;
			std::cout << std::endl << std::flush;
		}

		bool first = true;
		while (std::getline(file, tracker_url)) {
			if (tracker_url.empty()) {
				continue;
			}

			if (!first) {
				if (optJson) {
					std::cout << "," << std::endl;
				}
			}else {
				first = false;
			}
			test_tracker(tracker_url, optJson);
			
		}		
		if (optJson) {
			std::cout << std::endl << "]" << std::endl << std::flush;
		}

		file.close();
	}
	


	return 0;
}


void banner() {

	COUTC("===============================================");
	COUTC("               TRACKERS RATING                 ");
	COUTC("===============================================");


	std::wcout << std::endl;
	COUTC("tracker-rank v2.1 - benchmarking tool for torrent trackers and indexers\n");
	COUTC("Built on %s\n", __TIMESTAMP__);
	COUTC("Copyright (C) 2000-2021 Guillaume Plante\n");
	std::wcout << std::endl;
}
void usage() {
	COUTCS("Usage: trackers-ratings [-h][-v][-n][-p] path [-t] tracker \n");
	COUTCS("   -v				Verbose mode\n");
	COUTCS("   -h				Help\n");
	COUTCS("   -n				No banner\n");
	COUTCS("   -t				Timeout in seconds\n");
	COUTCS("   -j, --json		Json  format\n");
	COUTCS("   -i, --indexer    Rate Indexer\n");
	COUTCS("   -u, --url	    test a single tracker url\n");
	COUTCS("   -p, --path		path of the file containing all trackers\n");
	std::wcout << std::endl;
}
