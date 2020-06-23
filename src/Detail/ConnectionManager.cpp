#include <UDPTest/Detail/ConnectionManager.h>

using UDPTest::Detail::ConnectionManager;

void ConnectionManager::Start(ConnectionPtr_t conn) noexcept
{
	m_connections.insert(conn);
	conn->Start();
}

void ConnectionManager::Stop(const ConnectionPtr_t& conn) noexcept
{
	conn->Stop();
	m_connections.erase(conn);
}

void ConnectionManager::StopAll() noexcept
{
	for (const ConnectionPtr_t& conn : m_connections)
		conn->Stop();
	m_connections.clear();
}