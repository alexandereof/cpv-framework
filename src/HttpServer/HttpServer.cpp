#include <seastar/core/reactor.hh>
#include <CPVFramework/HttpServer/HttpServer.hpp>
#include <CPVFramework/Utility/NetworkUtils.hpp>
#include <CPVFramework/Exceptions/LogicException.hpp>
#include <CPVFramework/Exceptions/NotImplementedException.hpp>
#include "./Connections/Http11ServerConnection.hpp"
#include "./HttpServerData.hpp"

namespace cpv {
	/** Start accept http connections */
	seastar::future<> HttpServer::start() {
		// check state
		if (data_->stopping) {
			return seastar::make_exception_future(LogicException(
				CPV_CODEINFO, "can't start http server while stopping"));
		}
		// cleanup (if last time this function interrupt by exception)
		for (auto& listener : data_->listeners) {
			listener->first.abort_accept();
		}
		data_->listeners.clear();
		data_->listenerStoppedFutures.clear();
		// parse listen addresses and create listeners
		seastar::listen_options listenOptions;
		listenOptions.reuse_address = true;
		for (const auto& listenAddressStr : data_->sharedData->configuration.getListenAddresses()) {
			auto listenAddress = parseListenAddress(listenAddressStr);
			auto listener = seastar::listen(listenAddress, listenOptions);
			data_->listeners.emplace_back(seastar::make_lw_shared<
				std::pair<seastar::server_socket, seastar::socket_address>>(
				std::move(listener), std::move(listenAddress)));
		}
		// start listeners
		for (const auto& listener : data_->listeners) {
			data_->sharedData->logger->log(LogLevel::Info,
				"start listen http connections from:", listener->second);
			data_->listenerStoppedFutures.emplace_back(seastar::keep_doing(
				[listener, connectionsWrapper=data_->connectionsWrapper, sharedData=data_->sharedData] {
				return listener->first.accept().then(
					[connectionsWrapper, sharedData]
					(seastar::connected_socket fd, seastar::socket_address addr) {
					sharedData->logger->log(LogLevel::Info,
						"accepted http connection from:", addr);
					// currently only support http 1.0/1.1
					auto connection = seastar::make_shared<Http11ServerConnection>(
						sharedData, std::move(fd));
					connection->start();
					connectionsWrapper->value.emplace(std::move(connection));
				});
			}).handle_exception([listener, sharedData=data_->sharedData] (std::exception_ptr ex) {
				sharedData->logger->log(LogLevel::Info,
					"stop listen http connections from:", listener->second, "because", ex);
			}));
		}
		return seastar::make_ready_future<>();
	}
	
	/** Stop accept http connection and close all exists connections  */
	seastar::future<> HttpServer::stop() {
		// abort all listeners
		for (const auto& listener : data_->listeners) {
			listener->first.abort_accept();
		}
		return seastar::when_all(
			data_->listenerStoppedFutures.begin(),
			data_->listenerStoppedFutures.end()).then([this] (auto&&) {
			// clean all listeners
			data_->listeners.clear();
			data_->listenerStoppedFutures.clear();
			// close all connections
			auto futures = std::make_unique<std::vector<seastar::future<>>>();
			auto connectionsCopy = data_->connectionsWrapper->value;
			for (const auto& connection : connectionsCopy) {
				// connections may remove themself from collection while iterating
				// so iterate a copied collection
				futures->emplace_back(connection->stop());
			}
			return seastar::when_all(futures->begin(), futures->end())
				.then([this, futures=std::move(futures)] (auto&&) {
				// clean all connections
				data_->connectionsWrapper->value.clear();
			});
		});
	}
	
	/** Constructor */
	HttpServer::HttpServer(
		const HttpServerConfiguration& configuration,
		const seastar::shared_ptr<Logger>& logger,
		std::vector<std::unique_ptr<HttpServerRequestHandlerBase>>&& handlers) :
		data_(std::make_unique<HttpServerData>(configuration, logger, std::move(handlers))) { }
}


