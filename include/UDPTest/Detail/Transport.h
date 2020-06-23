#ifndef UDPTEST_DETAIL_TRANSPORT_H_
#define UDPTEST_DETAIL_TRANSPORT_H_

/// @file
/// Transport
/// 6/22/20 21:06

// asio includes
#include <asio.hpp>

// STL includes
#include <array>
#include <cstdint>
#include <random>

namespace UDPTest
{
	namespace Detail
	{
		class RandomPacket
		{
		public:
			RandomPacket() = default;
			RandomPacket(uint32_t size) noexcept : m_payload(size) {}
			RandomPacket(uint32_t seq, uint32_t size) noexcept 
				: m_seq(htonl(seq)) 
			{
				static std::random_device rd;
				static std::mt19937 mt(rd());
				static std::uniform_int_distribution<uint16_t> d(0, UCHAR_MAX);
				for (uint32_t i = 0; i < size; ++i)
					m_payload.push_back(static_cast<uint8_t>(d(mt)));
			}

			uint32_t GetSeq() const noexcept { return ntohl(m_seq); }

			size_t GetPayloadSize() const noexcept { return m_payload.size(); }

			std::array<asio::mutable_buffer, 2> GetBuffers() noexcept
			{
				return 
				{
					asio::buffer(&m_seq, 4),
					asio::buffer(m_payload)
				};
			}
		private:
			uint32_t m_seq = 0;
			std::vector<uint8_t> m_payload;
		};

		class PacketAck
		{
		public:
			PacketAck() = default;
			PacketAck(uint32_t seq) noexcept : m_seq(htonl(seq)) {}

			uint32_t GetSeq() const noexcept { return ntohl(m_seq); }

			std::array<asio::mutable_buffer, 1> GetBuffers() noexcept
			{
				return { asio::buffer(&m_seq, 4) };
			}
		private:
			uint32_t m_seq = 0;
		};
	}
}

#endif