#include <UDPTest/Client.h>

#include <UDPTest/Common.h>

#include <charconv>
#include <stdexcept>

using UDPTest::Client;

Client::Client(const std::string& address, const std::string& port,
	const std::string& bitRate, uint32_t packetRate, uint32_t time) : m_worker(),
	m_controlSocket(m_worker), m_transportSocket(m_worker),
	m_signals(m_worker), m_endTimer(m_worker), m_printTimer(m_worker), 
	m_sendTimer(m_worker), m_totalRecvTime(), m_maxRecvTime(),
	m_pending(0), m_seq(0), m_ack(0), m_time(time), m_totalBytes(0), 
	m_bytesSinceLastCheck(0), m_packetsSinceLastCheck(0)
{
	spdlog::set_level(spdlog::level::debug);
	ErrorCode_t ec;
	// resolve local address
	TCPProto_t::resolver resolver(m_worker);
	TCPProto_t::endpoint remoteEndpoint = 
		*resolver.resolve(address, port, ec);
	if (ec)
		throw ec;
	// register signals
	m_signals.add(SIGINT);
	m_signals.add(SIGTERM);
	Connect(remoteEndpoint);
	WaitSignals();
	// we subtract 4 because the seq sent with every packet takes 4 bytes
	m_packetSize = static_cast<uint32_t>((ParseBitrate(bitRate) / 8) / packetRate) - 4;
	if (m_packetSize > 65507)
		throw std::runtime_error("Packets would be too large with the given bitrate and packetrate");
	m_timeBetweenSend = std::chrono::microseconds(1000000) / packetRate;
	SPDLOG_DEBUG("Specified packet size of {} bytes, sending every {} ms",
		m_packetSize, static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(
			m_timeBetweenSend).count()) / 1000.f);
	SPDLOG_INFO("Started client");
}

void Client::Run()
{
	SPDLOG_INFO("Running client");
	m_worker.run();
}

void Client::Stop() noexcept
{
	ErrorCode_t ignored;
	m_controlSocket.shutdown(TCPSocket_t::shutdown_both, ignored);
	m_controlSocket.close(ignored);
	m_signals.cancel(ignored);
	CloseTransportLayer();
	SPDLOG_DEBUG("Client stopped");
}

void Client::Connect(const TCPEndpoint_t& endpoint) noexcept
{
	m_controlSocket.async_connect(endpoint,
		[this, endpoint](const ErrorCode_t& ec)
		{
			if (!ec)
			{
				SPDLOG_INFO("Connected to server {}:{}",
					endpoint.address().to_string(),
					endpoint.port());
				m_request = Detail::Request(
					Detail::Request::Command::Open,
					m_packetSize);
				WriteControl();
			}
			else if (ec != asio::error::operation_aborted)
			{
				SPDLOG_ERROR("Failed to connect to {}:{}: {}",
					endpoint.address().to_string(),
					endpoint.port(), ec.message());
				Stop();
			}
		});
}

void Client::WaitSignals() noexcept
{
	m_signals.async_wait(
		[this](const ErrorCode_t& ec, int signo)
		{
			if (ec)
				return;
			SPDLOG_DEBUG("Intercepted signo {}", signo);
			PrintEndStats();
			Stop();
		});
}

uint64_t Client::ParseBitrate(const std::string& bitrate)
{
	uint64_t coefficient;
	auto res = std::from_chars(bitrate.data(), 
		bitrate.data() + bitrate.size(), coefficient, 10);
	if (res.ec == std::errc::invalid_argument)
		throw std::runtime_error("Failed to parse bitrate");
	if (res.ptr == bitrate.data() + bitrate.size())
		return coefficient;
	switch (*res.ptr)
	{
	case 'k':
	case 'K':
		return coefficient * 1000;
	case 'm':
	case 'M':
		return coefficient * 1000 * 1000;
	case 'g':
	case 'G':
		return coefficient * 1000 * 1000 * 1000;
	default:
		throw std::runtime_error(std::string("Failed to parse multiplier: ") + *res.ptr);
	}
}

void Client::ReadControl() noexcept
{
	asio::async_read(m_controlSocket, m_response.GetBuffers(),
		[this](const ErrorCode_t& ec, size_t)
		{
			if (!ec)
			{
				SPDLOG_DEBUG("Got control response: {}",
					m_response.GetStatus());
				switch (m_request.GetCommand())
				{
				case Detail::Request::Open:
				{
					if (m_response.GetStatus() !=
						Detail::Response::Status::OK)
					{
						SPDLOG_ERROR("Server failed to open transport socket: {}",
							m_response.GetStatus());
						return Stop();
					}
					// open local transport
					ErrorCode_t ec;
					if (m_transportSocket.open(UDPProto_t::v4(), ec), ec)
					{
						SPDLOG_ERROR("Failed to open local transport socket: {}",
							ec.message());
						return Stop();
					}
					m_transportEndpoint = m_response.GetEndpoint();
					SPDLOG_DEBUG("Server opened transport socket on {}:{}. Beginning sequence",
						m_transportEndpoint.address().to_string(), m_transportEndpoint.port());
					// send the first packet
					++m_pending;
					ProcessTransportQueue();
					// set the timers
					m_printTimer.expires_at(std::chrono::steady_clock::now());
					m_sendTimer.expires_at(std::chrono::high_resolution_clock::now());
					m_start = std::chrono::high_resolution_clock::now();
					ReadTransport();
					AwaitNextSend();
					AwaitPrint();
					AwaitFinish();
					SPDLOG_INFO("Started transport");
					break;
				}
				case Detail::Request::Close:
				{
					if (m_response.GetStatus() !=
						Detail::Response::Status::OK)
					{
						SPDLOG_ERROR("Error on close: {}",
							m_response.GetStatus());
					}
					return Stop();
				}
				}
			}
			else if (ec != asio::error::operation_aborted)
			{
				SPDLOG_ERROR("Control disconnected on read: {}",
					ec.message());
				Stop();
			}
		});
}

void Client::WriteControl() noexcept
{
	asio::async_write(m_controlSocket, m_request.GetBuffers(),
		[this](const ErrorCode_t& ec, size_t)
		{
			if (!ec)
			{
				SPDLOG_DEBUG("Sent request with cmd {}", 
					m_request.GetCommand());
				ReadControl();
			}
			else if (ec != asio::error::operation_aborted)
			{
				SPDLOG_ERROR("Control disconnected on write: {}",
					ec.message());
				Stop();
			}
		});
}

void Client::ReadTransport() noexcept
{
	m_transportSocket.async_receive_from(m_packetAck.GetBuffers(),
		m_transportEndpoint, [this](const ErrorCode_t& ec, size_t)
		{
			if (!ec)
			{
				SPDLOG_TRACE("Received ack for seq {}",
					m_packetAck.GetSeq());
				const auto seqIt = m_sendTimes.find(m_packetAck.GetSeq());
				if (seqIt != m_sendTimes.end())
				{
					const auto recvTime = 
						std::chrono::high_resolution_clock::now() - seqIt->second;
					if (recvTime > m_maxRecvTime)
						m_maxRecvTime = recvTime;
					m_totalRecvTime += recvTime;
					m_sendTimes.erase(seqIt);
				}
				else
					SPDLOG_WARN("Untracked seq: {}",
						m_packetAck.GetSeq());
				++m_ack;
				if (m_transportSocket.is_open() == true)
					ReadTransport();
			}
			else if (ec != asio::error::operation_aborted)
			{
				SPDLOG_DEBUG("Transport disconnected on read: {}",
					ec.message());
				return Stop();
			}
		});
}

void Client::WriteTransport() noexcept
{
	m_transportSocket.async_send_to(m_randomPacket.GetBuffers(),
		m_transportEndpoint, [this](const ErrorCode_t& ec, size_t bytes)
		{
			if (!ec)
			{
				SPDLOG_TRACE("Wrote random packet with seq {}",
					m_randomPacket.GetSeq());
				m_sendTimes.emplace(m_randomPacket.GetSeq(), std::chrono::high_resolution_clock::now());
				m_bytesSinceLastCheck += bytes;
				m_totalBytes += bytes;
				++m_packetsSinceLastCheck;
				if (m_transportSocket.is_open() == true &&
					m_pending != 0)
					ProcessTransportQueue();
			}
			else if (ec != asio::error::operation_aborted)
			{
				SPDLOG_DEBUG("Transport disconnected on read: {}",
					ec.message());
				return Stop();
			}
		});
}

void Client::CloseTransportLayer() noexcept
{
	ErrorCode_t ignored;
	m_transportSocket.close(ignored);
	m_sendTimer.cancel(ignored);
	m_endTimer.cancel(ignored);
	m_printTimer.cancel(ignored);
}

void Client::ProcessTransportQueue() noexcept
{
	--m_pending;
	m_randomPacket = Detail::RandomPacket(m_seq++, m_packetSize);
	WriteTransport();
}

void Client::AwaitFinish() noexcept
{
	m_endTimer.expires_after(std::chrono::seconds(m_time));
	m_endTimer.async_wait([this](const ErrorCode_t& ec)
		{
			if (ec)
				return;
			SPDLOG_DEBUG("Finished");
			--m_seq;
			CloseTransportLayer();
			PrintEndStats();
			ErrorCode_t ignored;
			// cancel any outgoing packets
			m_request = Detail::Request(
				Detail::Request::Command::Close, 0);
			WriteControl();
		});
}

void Client::AwaitPrint() noexcept
{
	m_printTimer.expires_at(m_printTimer.expiry() + std::chrono::seconds(1));
	m_printTimer.async_wait([this](const ErrorCode_t& ec)
		{
			if (ec)
				return;
			AwaitPrint();
			SPDLOG_INFO("-------- Info --------");
			SPDLOG_INFO("Bits sent: {}\tPackets sent: {}",
				BitsToString(m_bytesSinceLastCheck * 8), m_packetsSinceLastCheck);
			m_bytesSinceLastCheck = 0;
			m_packetsSinceLastCheck = 0;
		});
}

void Client::AwaitNextSend() noexcept
{
	m_sendTimer.expires_at(m_sendTimer.expiry() + m_timeBetweenSend);
	m_sendTimer.async_wait([this](const ErrorCode_t& ec)
		{
			if (ec)
				return;
			AwaitNextSend();
			bool sendInProgress = (m_pending != 0);
			++m_pending;
			if (sendInProgress == false)
				ProcessTransportQueue();
		});
}

void Client::PrintEndStats() noexcept
{
	SPDLOG_INFO("End stats:");
	SPDLOG_INFO("Total packets sent: {}\tTotal packets received: {}",
		m_seq, m_ack);
	SPDLOG_INFO("Packets lost: {} ({}%)\tPackets unsent: {} ({}%)",
		m_seq - m_ack, (static_cast<float>(m_seq - m_ack) / m_seq) * 100,
		m_pending, static_cast<float>(m_pending) / (m_pending + m_seq) * 100);
	SPDLOG_INFO("Total bits sent: {}\tEnding bitrate: {}",
		BitsToString(m_totalBytes * 8), BitsToString(m_totalBytes / m_time * 8));
	SPDLOG_INFO("Average latency: {} ms\tMax latency: {} ms",
		static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(
			m_totalRecvTime / m_ack).count()) / 1000.f,
		static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(
			m_maxRecvTime).count()) / 1000.f);
}

std::string Client::BitsToString(uint64_t bits) noexcept
{
	constexpr const char postfixes[] = { ' ', 'K', 'M', 'G' };
	uint8_t i = 0;
	while (bits > 1000 &&
		i < 3)
	{
		bits /= 1000;
		++i;
	}
	return std::to_string(bits) + postfixes[i];
}