#include <unordered_map>
#include <CPVFramework/Http/HttpResponseHeaders.hpp>
#include <CPVFramework/Utility/Macros.hpp>

namespace cpv {
	class HttpResponseHeaders::Internal {
	public:
		using FixedMembersType = std::unordered_map<std::string_view, std::string_view(HttpResponseHeaders::*)>;
		static FixedMembersType FixedMembers;
	};
	
	HttpResponseHeaders::Internal::FixedMembersType HttpResponseHeaders::Internal::FixedMembers({
		{ constants::Date, &HttpResponseHeaders::date_ },
		{ constants::ContentType, &HttpResponseHeaders::contentType_ },
		{ constants::ContentLength, &HttpResponseHeaders::contentLength_ },
		{ constants::ContentEncoding, &HttpResponseHeaders::contentEncoding_ },
		{ constants::TransferEncoding, &HttpResponseHeaders::transferEncoding_ },
		{ constants::Connection, &HttpResponseHeaders::connection_ },
		{ constants::Server, &HttpResponseHeaders::server_ },
		{ constants::Vary, &HttpResponseHeaders::vary_ },
		{ constants::ETag, &HttpResponseHeaders::etag_ },
		{ constants::CacheControl, &HttpResponseHeaders::cacheControl_ },
		{ constants::Expires, &HttpResponseHeaders::expires_ },
		{ constants::LastModified, &HttpResponseHeaders::lastModified_ },
	});
	
	/** Set header value */
	void HttpResponseHeaders::setHeader(std::string_view key, std::string_view value) {
		auto it = Internal::FixedMembers.find(key);
		if (CPV_LIKELY(it != Internal::FixedMembers.end())) {
			this->*(it->second) = value;
		} else {
			remainHeaders_.insert_or_assign(key, value);
		}
	}
	
	/** Get header value */
	std::string_view HttpResponseHeaders::getHeader(std::string_view key) const {
		auto it = Internal::FixedMembers.find(key);
		if (CPV_LIKELY(it != Internal::FixedMembers.end())) {
			return this->*(it->second);
		} else {
			auto rit = remainHeaders_.find(key);
			if (rit != remainHeaders_.end()) {
				return rit->second;
			}
			return { };
		}
	}
	
	/** Remove header */
	void HttpResponseHeaders::removeHeader(std::string_view key) {
		auto it = Internal::FixedMembers.find(key);
		if (CPV_LIKELY(it != Internal::FixedMembers.end())) {
			this->*(it->second) = { };
		} else {
			remainHeaders_.erase(key);
		}
	}
	
	/** Add header that may occurs multiple times */
	void HttpResponseHeaders::addAdditionHeader(
		std::string_view key, std::string_view value) {
		additionHeaders_.emplace_back(key, value);
	}
	
	/** Get headers that may occurs multiple times */
	HttpResponseHeaders::AdditionHeadersType&
	HttpResponseHeaders::getAdditionHeaders() & {
		return additionHeaders_;
	}
	
	/** Get headers that may occurs multiple times */
	const HttpResponseHeaders::AdditionHeadersType&
	HttpResponseHeaders::getAdditionHeaders() const& {
		return additionHeaders_;
	}
	
	/** Get maximum count of headers, may greater than actual count */
	std::size_t HttpResponseHeaders::maxSize() const {
		return (Internal::FixedMembers.size() +
			remainHeaders_.size() +
			additionHeaders_.size());
	}
	
	/** Clear headers in this collection */
	void HttpResponseHeaders::clear() {
		remainHeaders_.clear();
		additionHeaders_.clear();
		date_ = {};
		contentType_ = {};
		contentLength_ = {};
		contentEncoding_ = {};
		transferEncoding_ = {};
		connection_ = {};
		server_ = {};
		vary_ = {};
		etag_ = {};
		cacheControl_ = {};
		expires_ = {};
		lastModified_ = {};
	}
	
	/** Constructor */
	HttpResponseHeaders::HttpResponseHeaders() :
		remainHeaders_(),
		additionHeaders_(),
		date_(),
		contentType_(),
		contentLength_(),
		contentEncoding_(),
		transferEncoding_(),
		connection_(),
		server_(),
		vary_(),
		etag_(),
		cacheControl_(),
		expires_(),
		lastModified_() { }
}

