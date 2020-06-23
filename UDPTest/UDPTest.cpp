#include <UDPTest/Client.h>
#include <UDPTest/Server.h>

#include <cxxopts.hpp>

using UDPTest::Client;
using UDPTest::Server;

int main(int argc, char* argv[])
{
	cxxopts::Options opt("UDPTest launcher");
	opt.add_options()
		("s,server", "Server mode", cxxopts::value<bool>()->implicit_value("true"))
		("c,client", "Client mode", cxxopts::value<bool>()->implicit_value("true"))
		("a,address", "Server address", cxxopts::value<std::string>()->default_value("127.0.0.1"))
		("p,port", "Server port", cxxopts::value<std::string>()->default_value("5601"))
		("b,bitrate", "The bitrate per second to send", cxxopts::value<std::string>()->default_value("1M"))
		("r,packetrate", "The rate of packets per second to send", cxxopts::value<uint32_t>()->default_value("100"))
		("t,time", "The total time to test in seconds", cxxopts::value<uint32_t>()->default_value("10"))
		("h,help", "Display this help message");
	try
	{
		auto res = opt.parse(argc, argv);
		if (res.count("help") != 0 ||
			res.arguments().empty() == true)
		{
			std::cout << opt.help() << '\n';
			return 0;
		}
		if (res["server"].as<bool>() == true)
		{
			Server server(res["address"].as<std::string>(),
				res["port"].as<std::string>());
			server.Run();
		}
		else if (res["client"].as<bool>() == true)
		{
			if (res["packetrate"].as<uint32_t>() == 0)
			{
				std::cerr << "Packet rate must be nonzero\n";
				return 1;
			}
			Client client(res["address"].as<std::string>(),
				res["port"].as<std::string>(), 
				res["bitrate"].as<std::string>(),
				res["packetrate"].as<uint32_t>(),
				res["time"].as<uint32_t>());
			client.Run();
		}
		else
		{
			std::cerr << "Neither server nor client were specified\n";
			return 1;
		}
	}
	catch (const cxxopts::OptionException& ex)
	{
		std::cerr << "Exception parsing command line: " << ex.what() << '\n';
		return 1;
	}
	catch (const Server::ErrorCode_t& ec)
	{
		std::cerr << "UDP Test failed: " << ec.message() << '\n';
		return 1;
	}
	catch (const std::runtime_error& re)
	{
		std::cerr << "UDP Test failed: " << re.what() << '\n';
		return 1;
	}

	return 0;
}