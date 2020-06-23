#include <UDPTest/Server.h>

using UDPTest::Server;

Server::Server(const std::string& address, const std::string& port)
	: m_worker(), m_acceptor(m_worker), m_signals(m_worker)
{
	spdlog::set_level(spdlog::level::debug);
	ErrorCode_t ec;
	// resolve local address
	Proto_t::resolver resolver(m_worker);
	Proto_t::endpoint localEndpoint = *resolver.resolve(address, port, ec);
	if (ec)
		throw ec;
	// try to open the acceptor
	if (m_acceptor.open(Proto_t::v4(), ec), ec ||
		m_acceptor.bind(localEndpoint, ec), ec ||
		m_acceptor.listen(Proto_t::acceptor::max_listen_connections, ec))
		throw ec;
	// register signals
	m_signals.add(SIGINT);
	m_signals.add(SIGTERM);
	WaitSignals();
	Accept();
	SPDLOG_INFO("Started server");
}

void Server::Run() noexcept
{
	SPDLOG_INFO("Running server");
	m_worker.run();
}

void Server::Accept() noexcept
{
	m_acceptor.async_accept(
		[this](const ErrorCode_t& ec, Proto_t::socket socket)
		{
			// check if it was closed
			if (m_acceptor.is_open() == false)
				return;
			if (!ec)
			{
				SPDLOG_INFO("Accepted connection from {}:{}",
					socket.remote_endpoint().address().to_string(),
					socket.remote_endpoint().port());
				m_connectionManager.Start(
					std::make_shared<Detail::Connection>(
						m_connectionManager, std::move(socket)));
			}
			else
				SPDLOG_ERROR("Error accepting connection: {}", ec.message());
			Accept();
		});
}

void Server::Stop() noexcept
{
	ErrorCode_t ignored;
	m_acceptor.close(ignored);
	m_connectionManager.StopAll();
	m_signals.cancel(ignored);
}

void Server::WaitSignals() noexcept
{
	m_signals.async_wait(
		[this](const ErrorCode_t& ec, int signo)
		{
			if (ec)
				return;
			SPDLOG_DEBUG("Intercepted {}. Closing", signo);
			Stop();
		});
}