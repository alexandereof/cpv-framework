#include <CPVFramework/Utility/Packet.hpp>
#include <CPVFramework/Utility/StringUtils.hpp>
#include <TestUtility/GTestUtils.hpp>

TEST(TestPacket, construct) {
	{
		cpv::Packet p(std::string_view("abc\x00""def", 7));
		ASSERT_EQ(cpv::joinString("", p), std::string_view("abc\x00""def", 7));
	}
	{
		seastar::temporary_buffer buf("123\x00""3210", 8);
		cpv::Packet p(std::string_view(buf.get(), buf.size()), buf.release());
		ASSERT_EQ(cpv::joinString("", p), std::string_view("123\x00""3210", 8));
	}
	{
		seastar::temporary_buffer buf("test123456", 10);
		cpv::Packet p(std::move(buf));
		ASSERT_EQ(cpv::joinString("", p), "test123456");
	}
	{
		cpv::Packet p(100);
		auto ptr = p.getIfMultiple();
		ASSERT_TRUE(ptr);
		ASSERT_GE(ptr->fragments.capacity(), 100U);
	}
}


TEST(TestPacket, append) {
	{
		cpv::Packet p;
		p.append("abc").append("123").append("def"); // single => multiple => multiple
		ASSERT_EQ(cpv::joinString("", p), "abc123def");
	}
	{
		cpv::Packet p;
		seastar::temporary_buffer bufA("def", 3);
		seastar::temporary_buffer bufB("qwert", 5);
		seastar::temporary_buffer bufC("asdfg", 5);
		p.append(std::string_view(bufA.get(), bufA.size()), bufA.release())
			.append(std::move(bufB))
			.append(std::move(bufC)); // single => multiple => multiple
		ASSERT_EQ(cpv::joinString("", p), "defqwertasdfg");
	}
	{
		cpv::Packet p;
		p.append(123).append(99999).append(-12345);
		ASSERT_EQ(cpv::joinString("", p), "12399999-12345");
	}
}

TEST(TestPacket, appendPacket) {
	{
		// append multiple to multiple
		cpv::Packet p;
		cpv::Packet q;
		p.append("abc").append(seastar::temporary_buffer("123", 3));
		q.append("def").append(seastar::temporary_buffer("321", 3));
		p.append(std::move(q));
		ASSERT_EQ(cpv::joinString("", p), "abc123def321");
		ASSERT_EQ(cpv::joinString("", q), "");
	}
	{
		// append single to multiple
		cpv::Packet p;
		cpv::Packet q("def");
		p.append("abc").append(seastar::temporary_buffer("123", 3));
		p.append(std::move(q));
		ASSERT_EQ(cpv::joinString("", p), "abc123def");
		ASSERT_EQ(cpv::joinString("", q), "");
	}
	{
		// append multiple to empty single
		cpv::Packet p;
		cpv::Packet q;
		q.append("def").append(seastar::temporary_buffer("321", 3));
		p.append(std::move(q));
		ASSERT_EQ(cpv::joinString("", p), "def321");
		ASSERT_EQ(cpv::joinString("", q), "");
	}
	{
		// append multiple to single
		cpv::Packet p("abc");
		cpv::Packet q;
		q.append("def").append(seastar::temporary_buffer("321", 3));
		p.append(std::move(q));
		ASSERT_EQ(cpv::joinString("", p), "abcdef321");
		ASSERT_EQ(cpv::joinString("", q), "");
	}
	{
		// append single to single
		cpv::Packet p("abc");
		cpv::Packet q("def");
		p.append(std::move(q));
		ASSERT_EQ(cpv::joinString("", p), "abcdef");
		ASSERT_EQ(cpv::joinString("", q), "");
	}
	{
		// replace valueless
		cpv::Packet p("abc");
		cpv::Packet p_(std::move(p));
		cpv::Packet q(seastar::temporary_buffer("def", 3));
		p.append(std::move(q));
		ASSERT_EQ(cpv::joinString("", p), "def");
		ASSERT_EQ(cpv::joinString("", p_), "abc");
		ASSERT_EQ(cpv::joinString("", q), "");
	}
}

TEST(TestPacket, size) {
	cpv::Packet p("abc");
	cpv::Packet p_(std::move(p));
	cpv::Packet q("def");
	q.append(-321);
	ASSERT_EQ(p.size(), 0U);
	ASSERT_EQ(p_.size(), 3U);
	ASSERT_EQ(q.size(), 7U);
}

TEST(TestPacket, empty) {
	cpv::Packet p;
	cpv::Packet p_(std::move(p));
	cpv::Packet q("abc");
	cpv::Packet w(100);
	ASSERT_TRUE(p.empty());
	ASSERT_TRUE(p_.empty());
	ASSERT_FALSE(q.empty());
	ASSERT_TRUE(w.empty());
}

TEST(TestPacket, writeToStream) {
	cpv::Packet p;
	p.append("abc").append(123).append(seastar::temporary_buffer("def", 3));
	ASSERT_EQ(cpv::joinString("", p), "abc123def");
}

TEST(TestPacket, release) {
	{
		// release single
		cpv::Packet p(seastar::temporary_buffer<char>("abc", 3));
		auto ptr = p.getIfSingle();
		ASSERT_TRUE(ptr);
		seastar::temporary_buffer<char> buf = ptr->release();
		ASSERT_TRUE(p.empty());
		ASSERT_EQ(std::string_view(buf.get(), buf.size()), "abc");
	}
	{
		// release multiple
		cpv::Packet p(seastar::temporary_buffer<char>("abc", 3));
		p.append(123).append("def");
		auto ptr = p.getIfMultiple();
		ASSERT_TRUE(ptr);
		seastar::net::packet p_ = ptr->release();
		ASSERT_EQ(p_.len(), 9U);
		auto vec = p_.release();
		ASSERT_EQ(vec.size(), 3U);
		ASSERT_EQ(std::string_view(vec.at(0).get(), vec.at(0).size()), "abc");
		ASSERT_EQ(std::string_view(vec.at(1).get(), vec.at(1).size()), "123");
		ASSERT_EQ(std::string_view(vec.at(2).get(), vec.at(2).size()), "def");
	}
}

