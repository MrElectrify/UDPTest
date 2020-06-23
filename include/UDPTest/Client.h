#ifndef UDPTEST_CLIENT_H_
#define UDPTEST_CLIENT_H_

/// @file
/// Client
/// 6/22/20 21:37

// USPTest includes
#include <UDPTest/Detail/Control.h>
#include <UDPTest/Detail/Transport.h>

// asio includes
#include <asio.hpp>

// STL includes
#include <chrono>
#include <string>
#include <unordered_map>

namespace UDPTest
{
	/// @brief The UDPTest client
	class Client
	{
	public:
		using ErrorCode_t = asio::error_code;
		using TCPProto_t = asio::ip::tcp;
		using TCPEndpoint_t = TCPProto_t::endpoint;
		using TCPSocket_t = TCPProto_t::socket;
		using UDPProto_t = asio::ip::udp;
		using UDPSocket_t = UDPProto_t::socket;

		/// @brief Creates a client and starts it
		/// @param address The address of the server
		/// @param port The port of the server
		/// @param bitRate The bitrate to transfer at
		/// @param packetRate The packet rate to transfer with. Requires: nonzero
		/// @throws ErrorCode_t
		/// @throws std::runtime_error
		Client(const std::string& address, const std::string& port,
			const std::string& bitRate, uint32_t packetRate, uint32_t time);

		/// @brief Runs the client
		/// @throws ErrorCode_t
		void Run();
	private:
		/// @brief Stops the client
		void Stop() noexcept;

		/// @brief Attempts to connect to the server
		/// @param endpoint The endpoint of the server
		void Connect(const TCPEndpoint_t& endpoint) noexcept;
		/// @brief Waits for signals
		void WaitSignals() noexcept;

		/// @brief Parses a bitrate
		/// @param bitrate The bitrate string
		/// @throws std::runtime_error
		/// @return The bitrate in bits per second
		uint64_t ParseBitrate(const std::string& bitrate);

		/// @brief Reads a response from the control socket
		void ReadControl() noexcept;
		/// @brief Writes a request to the control socket
		void WriteControl() noexcept;

		/// @brief Reads an ack from the transport socket
		void ReadTransport() noexcept;
		/// @brief Writes the random packed to the socket
		void WriteTransport() noexcept;
		/// @brief Closes the transport layer
		void CloseTransportLayer() noexcept;
		/// @brief Processes the transport queue
		void ProcessTransportQueue() noexcept;

		/// @brief Awaits the packet finish
		void AwaitFinish() noexcept;
		/// @brief Awaits the socket print
		void AwaitPrint() noexcept;
		/// @brief Awaits the next send
		void AwaitNextSend() noexcept;

		/// @brief Prints end stats
		void PrintEndStats() noexcept;
		
		/// @brief Converts a bit count to string, compressing as necessary
		/// @param bits The number of bits
		/// @return A string representing the bit count
		static std::string BitsToString(uint64_t bits) noexcept;

		asio::io_context m_worker;
		TCPSocket_t m_controlSocket;
		UDPSocket_t m_transportSocket;
		UDPProto_t::endpoint m_transportEndpoint;
		asio::signal_set m_signals;
		Detail::Request m_request;
		Detail::Response m_response;
		Detail::RandomPacket m_randomPacket;
		Detail::PacketAck m_packetAck;
		asio::steady_timer m_endTimer;
		asio::steady_timer m_printTimer;
		asio::high_resolution_timer m_sendTimer;
		std::chrono::high_resolution_clock::time_point m_start;
		std::chrono::high_resolution_clock::duration m_timeBetweenSend;
		std::unordered_map<uint32_t, std::chrono::high_resolution_clock::time_point> m_sendTimes;
		std::chrono::high_resolution_clock::duration m_totalRecvTime;
		std::chrono::high_resolution_clock::duration m_maxRecvTime;
		std::chrono::high_resolution_clock::duration m_recvTimeSinceLastCheck;
		std::chrono::high_resolution_clock::duration m_maxRecvTimeSinceLastCheck;
		uint32_t m_pending;
		uint32_t m_packetSize;
		uint32_t m_seq;
		uint32_t m_ack;
		uint32_t m_time;
		uint64_t m_totalBytes;
		uint64_t m_bytesSinceLastCheck;
		uint32_t m_packetsSinceLastCheck;
	};
}

#endif