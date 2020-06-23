#ifndef UDPTEST_UDPTEST_H_
#define UDPTEST_UDPTEST_H_

/// @file
/// UDPTest
/// 6/22/20 18:35

// UDPTest includes
#include <UDPTest/Common.h>
#include <UDPTest/Detail/ConnectionManager.h>

// asio includes
#include <asio.hpp>

// STL includes
#include <cstdint>
#include <iostream>
#include <optional>

namespace UDPTest
{
	/// @brief Server is a UDP testbench server
	class Server
	{
	public:
		using ErrorCode_t = asio::error_code;
		using Proto_t = asio::ip::tcp;
		using Socket_t = Proto_t::socket;

		/// @brief Creates a UDP test server
		/// @param address The address to use
		/// @param port The port to use
		/// @throws ErrorCode_t
		Server(const std::string& address, const std::string& port);

		/// @brief Runs the UDP test bench server
		void Run() noexcept;
	private:
		/// @brief Accepts a new connection
		void Accept() noexcept;
		/// @brief Closes all resources and shuts down the test bench
		void Stop() noexcept;
		/// @brief Waits for a signal
		void WaitSignals() noexcept;

		asio::io_context m_worker;
		Proto_t::acceptor m_acceptor;
		Detail::ConnectionManager m_connectionManager;
		asio::signal_set m_signals;
	};
}

#endif