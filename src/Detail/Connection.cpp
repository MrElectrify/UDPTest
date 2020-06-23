#include <UDPTest/Detail/Connection.h>

#include <UDPTest/Common.h>
#include <UDPTest/Detail/ConnectionManager.h>
#include <UDPTest/Detail/Control.h>

using UDPTest::Detail::Connection;

Connection::Connection(ConnectionManager& connectionManager,
	TCPSocket_t socket) noexcept
	: m_connectionManager(connectionManager),
		m_controlSocket(std::move(socket)),
		m_transportSocket(socket.get_executor()),
		m_randomPacket(0) {}

void Connection::Start() noexcept
{
	ReadControl();
}

void Connection::Stop() noexcept
{
	ErrorCode_t ignored;
	m_controlSocket.shutdown(TCPSocket_t::shutdown_both, ignored);
	m_controlSocket.close(ignored);
	m_transportSocket.shutdown(UDPSocket_t::shutdown_both, ignored);
	m_transportSocket.close(ignored);
	SPDLOG_INFO("Stopped connection");
}

void Connection::ReadControl() noexcept
{
	auto self = shared_from_this();
	asio::async_read(m_controlSocket, m_request.GetBuffers(), 
		[this, self](const ErrorCode_t& ec, size_t) 
		{
			if (!ec)
			{
				SPDLOG_DEBUG("Received request: {}", m_request.GetCommand());
				ErrorCode_t ec;
				switch (m_request.GetCommand())
				{
				case Request::Command::Open:
				{
					if (m_transportSocket.is_open() == true)
					{
						m_response = Response(Response::Status::AlreadyOpen,
							UDPProto_t::endpoint());
					}
					else if (m_transportSocket.open(UDPProto_t::v4(), ec), ec ||
						m_transportSocket.bind(UDPProto_t::endpoint(
							m_controlSocket.local_endpoint().address(),
							m_controlSocket.local_endpoint().port()), ec), ec)
					{
						m_transportSocket.close(ec);
						m_response = Response(Response::Status::FailedToOpen, 
							UDPProto_t::endpoint());
					}
					else
					{
						SPDLOG_DEBUG("Opened local UDP socket on {}:{}",
							m_transportSocket.local_endpoint().address().to_string(),
							m_transportSocket.local_endpoint().port());
						m_response = Response(Response::Status::OK,
							m_transportSocket.local_endpoint());
						// resize packet
						m_randomPacket = RandomPacket(m_request.GetPayloadSize());
						SPDLOG_DEBUG("Reading for payloads of size {}",
							m_randomPacket.GetPayloadSize());
						ReadTransport();
					}
					break;
				case Request::Command::Close:
				{
					if (m_transportSocket.close(ec), ec)
					{
						m_response = Response(Response::Status::FailedToClose, 
							UDPProto_t::endpoint());
						return WriteControl();
					}
					else
					{
						m_response = Response(Response::Status::OK,
							UDPProto_t::endpoint());
					}
					break;
				}
				}
				}
				WriteControl();
			}
			else if (ec != asio::error::operation_aborted)
			{
				SPDLOG_ERROR("Control disconnect on read: {}", ec.message());
				m_connectionManager.Stop(self);
			}
		});
}

void Connection::WriteControl() noexcept
{
	auto self = shared_from_this();
	asio::async_write(m_controlSocket, m_response.GetBuffers(),
		[this, self](const ErrorCode_t& ec, size_t)
		{
			if (!ec)
			{
				SPDLOG_DEBUG("Wrote response");
				ReadControl();
			}
			else if (ec == asio::error::operation_aborted)
			{
				SPDLOG_ERROR("Control disconnect on write: {}", ec.message());
				m_connectionManager.Stop(self);
			}
		});
}

void Connection::ReadTransport() noexcept
{
	auto self = shared_from_this();
	m_transportSocket.async_receive_from(m_randomPacket.GetBuffers(), 
		m_transportEndpoint, [this, self](const ErrorCode_t& ec, size_t)
		{
			if (!ec)
			{
				SPDLOG_TRACE("Received random packet of seq {}",
					m_randomPacket.GetSeq());
				m_packetAck = PacketAck(m_randomPacket.GetSeq());
				WriteTransport();
			}
			else if (ec != asio::error::operation_aborted)
			{
				SPDLOG_ERROR("Transport error on read: {}",
					ec.message());
				m_connectionManager.Stop(self);
			}
		});
}

void Connection::WriteTransport() noexcept
{
	auto self = shared_from_this();
	m_transportSocket.async_send_to(m_packetAck.GetBuffers(),
		m_transportEndpoint, [this, self](const ErrorCode_t& ec, size_t)
		{
			if (!ec)
			{
				SPDLOG_TRACE("Wrote ack {}",
					m_packetAck.GetSeq());
				ReadTransport();
			}
			else if (ec != asio::error::operation_aborted)
			{
				SPDLOG_ERROR("Transport error on write: {}",
					ec.message());
				m_connectionManager.Stop(self);
			}
		});
}