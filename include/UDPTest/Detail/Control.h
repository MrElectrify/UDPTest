#ifndef UDPTEST_DETAIL_CONTROL_H_
#define UDPTEST_DETAIL_CONTROL_H_

/// @file
/// Control
/// 6/22/20 20:03

// asio includes
#include <asio.hpp>

// STL includes
#include <array>
#include <cstdint>

namespace UDPTest
{
	namespace Detail
	{
		class Request 
		{
		public:
			enum Command : uint8_t
			{
				Open = 0x01,
				Close = 0x02
			};

			Request() = default;
			Request(Command command, uint32_t payloadSize) 
				: m_command(command), m_payloadSize(htonl(payloadSize)) {}

			Command GetCommand() const noexcept { return m_command; }

			uint32_t GetPayloadSize() const noexcept { return ntohl(m_payloadSize); }

			std::array<asio::mutable_buffer, 2> GetBuffers()
			{
				return { 
					asio::buffer(&m_command, 1),
					asio::buffer(&m_payloadSize, 4) 
				};
			}
		private:
			Command m_command;
			uint32_t m_payloadSize;
		};

		class Response
		{
		public:
			enum Status : uint8_t
			{
				OK,
				AlreadyOpen,
				FailedToOpen,
				FailedToClose
			};

			Response() = default;
			Response(Status status,
				const asio::ip::udp::endpoint& endpoint) noexcept
				: m_status(status), m_address(endpoint.address().to_v4().to_bytes()),
				m_port(htons(endpoint.port())) {}

			Status GetStatus() const noexcept { return m_status; }

			asio::ip::udp::endpoint GetEndpoint() const noexcept
			{
				const asio::ip::address_v4 addr(m_address);
				const uint16_t port(ntohs(m_port));
				return asio::ip::udp::endpoint(addr, port);
			}

			std::array<asio::mutable_buffer, 3> GetBuffers()
			{
				return { asio::buffer(&m_status, 1), asio::buffer(m_address), asio::buffer(&m_port, 2) };
			}
		private:
			Status m_status;
			asio::ip::address_v4::bytes_type m_address;
			uint16_t m_port;
		};
	}
}

#endif