#ifndef UDPTESTBENCH_DETAIL_CONNECTION_H_
#define UDPTESTBENCH_DETAIL_CONNECTION_H_

/// @file
/// Connection
/// 6/22/20 19:27

// UDPTest includes
#include <UDPTest/Detail/Control.h>
#include <UDPTest/Detail/Transport.h>

// asio includes
#include <asio.hpp>

// STL includes
#include <memory>
#include <vector>

namespace UDPTest
{
	namespace Detail
	{
		class ConnectionManager;

		/// @brief Connection represents a client connection
		class Connection
			: public std::enable_shared_from_this<Connection>
		{
		public:
			using ErrorCode_t = asio::error_code;
			using TCPProto_t = asio::ip::tcp;
			using TCPSocket_t = TCPProto_t::socket;
			using UDPProto_t = asio::ip::udp;
			using UDPSocket_t = UDPProto_t::socket;

			/// @brief Creates a connection with a connected control socket
			/// @param connectionManager The connection manager
			/// @param socket The control socket
			Connection(ConnectionManager& connectionManager,
				TCPSocket_t socket) noexcept;

			/// @brief Starts the connection
			void Start() noexcept;
			/// @bried Stops the connection
			void Stop() noexcept;
		private:
			/// @brief Reads the request from the control socket
			void ReadControl() noexcept;
			/// @brief Writes the response to the control socket
			void WriteControl() noexcept;

			/// @brief Reads from the transport socket
			void ReadTransport() noexcept;
			/// @brief Writes to the transport socket
			void WriteTransport() noexcept;

			ConnectionManager& m_connectionManager;
			TCPSocket_t m_controlSocket;
			UDPSocket_t m_transportSocket;
			UDPProto_t::endpoint m_transportEndpoint;
			Detail::Request m_request;
			Detail::Response m_response;
			Detail::RandomPacket m_randomPacket;
			Detail::PacketAck m_packetAck;
		};
	}
}

#endif