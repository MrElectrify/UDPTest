#ifndef UDPTEST_DETAIL_CONNECTIONMANAGET_H_
#define UDPTEST_DETAIL_CONNECTIONMANAGET_H_

/// @file
/// Connection Manager
/// 6/18/20 13:50

// UDPTest includes
#include <UDPTest/Detail/Connection.h>

// STL includes
#include <memory>
#include <unordered_set>

namespace UDPTest
{
	namespace Detail
	{
		/// @brief ConnectionManager manages connections
		class ConnectionManager
		{
		public:
			using ConnectionPtr_t = std::shared_ptr<Connection>;
			using ConnectionSet_t = std::unordered_set<ConnectionPtr_t>;

			ConnectionManager() = default;
			ConnectionManager(const ConnectionManager&) = delete;
			ConnectionManager& operator=(const ConnectionManager&) = delete;

			/// @brief Starts tracking a connection and begins request handling
			/// @param conn The connection to track
			void Start(ConnectionPtr_t conn) noexcept;

			/// @brief Stops tracking a connection and shuts it down
			/// @param conn The connection to stop tracking
			void Stop(const ConnectionPtr_t& conn) noexcept;

			/// @brief Stops all connections and stop tracking them
			void StopAll() noexcept;
		private:
			ConnectionSet_t m_connections;
		};
	}
}

#endif