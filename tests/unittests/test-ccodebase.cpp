﻿/*
	Copyright (C) 2021, Sakura Editor Organization

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

		1. The origin of this software must not be misrepresented;
		   you must not claim that you wrote the original software.
		   If you use this software in a product, an acknowledgment
		   in the product documentation would be appreciated but is
		   not required.

		2. Altered source versions must be plainly marked as such,
		   and must not be misrepresented as being the original software.

		3. This notice may not be removed or altered from any source
		   distribution.
*/

#include <gtest/gtest.h>
#include "charset/CCodeFactory.h"

#include <cstdlib>

TEST(CCodeBase, MIMEHeaderDecode)
{
	CMemory m;

	// Base64 JIS
	std::string source1("From: =?iso-2022-jp?B?GyRCJTUlLyVpGyhC?=");
	EXPECT_TRUE(CCodeBase::MIMEHeaderDecode(source1.c_str(), source1.length(), &m, CODE_JIS));
	EXPECT_STREQ(static_cast<char*>(m.GetRawPtr()), "From: $B%5%/%i(B");

	// Base64 UTF-8
	std::string source2("From: =?utf-8?B?44K144Kv44Op?=");
	EXPECT_TRUE(CCodeBase::MIMEHeaderDecode(source2.c_str(), source2.length(), &m, CODE_UTF8));
	EXPECT_STREQ(static_cast<char*>(m.GetRawPtr()), "From: \xe3\x82\xb5\xe3\x82\xaf\xe3\x83\xa9");

	// QP UTF-8
	std::string source3("From: =?utf-8?Q?=E3=82=B5=E3=82=AF=E3=83=A9!?=");
	EXPECT_TRUE(CCodeBase::MIMEHeaderDecode(source3.c_str(), source3.length(), &m, CODE_UTF8));
	EXPECT_STREQ(static_cast<char*>(m.GetRawPtr()), "From: \xe3\x82\xb5\xe3\x82\xaf\xe3\x83\xa9!");

	// 引数の文字コードとヘッダー内の文字コードが異なる場合は変換しない
	EXPECT_TRUE(CCodeBase::MIMEHeaderDecode(source1.c_str(), source1.length(), &m, CODE_UTF8));
	EXPECT_STREQ(static_cast<char*>(m.GetRawPtr()), source1.c_str());

	// 対応していない文字コードなら変換しない
	std::string source4("From: =?utf-7?B?+MLUwrzDp-");
	EXPECT_TRUE(CCodeBase::MIMEHeaderDecode(source4.c_str(), source4.length(), &m, CODE_UTF7));
	EXPECT_STREQ(static_cast<char*>(m.GetRawPtr()), source4.c_str());

	// 謎の符号化方式が指定されていたら何もしない
	std::string source5("From: =?iso-2022-jp?X?GyRCJTUlLyVpGyhC?=");
	EXPECT_TRUE(CCodeBase::MIMEHeaderDecode(source5.c_str(), source5.length(), &m, CODE_JIS));
	EXPECT_STREQ(static_cast<char*>(m.GetRawPtr()), source5.c_str());

	// 末尾の ?= がなければ変換しない
	std::string source6("From: =?iso-2022-jp?B?GyRCJTUlLyVpGyhC");
	EXPECT_TRUE(CCodeBase::MIMEHeaderDecode(source6.c_str(), source6.length(), &m, CODE_JIS));
	EXPECT_STREQ(static_cast<char*>(m.GetRawPtr()), source6.c_str());
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeSJis)
{
	const auto eCodeType = CODE_SJIS;
	auto pCodeBase = CCodeFactory::CreateCodeBase( eCodeType );

	// 7bit ASCII範囲（等価変換）
	constexpr const auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr const auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	bool bComplete1_1 = false;
	auto encoded1 = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(mbsAscii), _countof(mbsAscii) ), &bComplete1_1 );
	EXPECT_STREQ( wcsAscii, encoded1.GetStringPtr() );
	EXPECT_TRUE( bComplete1_1 );

	bool bComplete1_2 = false;
	auto decoded1 = pCodeBase->UnicodeToCode( encoded1, &bComplete1_2 );
	EXPECT_EQ( 0, memcmp( mbsAscii, decoded1.data(), decoded1.size() ) );
	EXPECT_TRUE( bComplete1_2 );

	// かな漢字の変換（Shift-JIS仕様）
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";
	constexpr const auto& mbsKanaKanji = "\xB6\xC5\x82\xA9\x82\xC8\x83\x4A\x83\x69\x8A\xBF\x8E\x9A";

	bool bComplete2_1 = false;
	auto encoded2 = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(mbsKanaKanji), _countof(mbsKanaKanji) ), &bComplete2_1 );
	ASSERT_STREQ( wcsKanaKanji, encoded2.GetStringPtr() );
	ASSERT_TRUE( bComplete2_1 );

	bool bComplete2_2 = false;
	auto decoded2 = pCodeBase->UnicodeToCode( encoded2, &bComplete2_2 );
	ASSERT_EQ( 0, memcmp( mbsKanaKanji, decoded2.data(), decoded2.size() ) );
	ASSERT_TRUE( bComplete2_2 );

	// Unicodeから変換できない文字（Shift-JIS仕様）
	// 1. SJIS⇒Unicode変換ができても、元に戻せない文字は変換失敗と看做す。
	//    該当するのは NEC選定IBM拡張文字 と呼ばれる約400字。
	// 2. 先行バイトが範囲外
	//    (ch1 >= 0x81 && ch1 <= 0x9F) ||
	//    (ch1 >= 0xE0 && ch1 <= 0xFC)
	// 3. 後続バイトが範囲外
	//    ch2 >= 0x40 &&  ch2 != 0xFC &&
	//    ch2 <= 0x7F
	constexpr const auto& mbsCantConvSJis =
		"\x87\x40\xED\x40\xFA\x40"					// "①纊ⅰ" NEC拡張、NEC選定IBM拡張、IBM拡張
		"\x80\x40\xFD\x40\xFE\x40\xFF\x40"			// 第1バイト不正
		"\x81\x0A\x81\x7F\x81\xFD\x81\xFE\x81\xFF"	// 第2バイト不正
		;
	constexpr const auto& wcsCantConvSJis =
		L"①\xDCED\xDC40ⅰ"
		L"\xDC80@\xDCFD@\xDCFE@\xDCFF@"
		L"\xDC81\n\xDC81\x7F\xDC81\xDCFD\xDC81\xDCFE\xDC81\xDCFF"
		;

	bool bComplete3_1 = true;
	auto encoded3 = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(mbsCantConvSJis), _countof(mbsCantConvSJis) ), &bComplete3_1 );
	ASSERT_STREQ( wcsCantConvSJis, encoded3.GetStringPtr() );
	ASSERT_TRUE( bComplete3_1 );	//👈 仕様バグ。変換できないので false が返るべき。

	// Unicodeから変換できない文字（Shift-JIS仕様）
	constexpr const auto& wcsOGuy = L"森鷗外";
	constexpr const auto& mbsOGuy = "\x90\x58\x3F\x8A\x4F"; //森?外

	bool bComplete4_2 = true;
	auto decoded4 = pCodeBase->UnicodeToCode( wcsOGuy, &bComplete4_2 );
	ASSERT_EQ( 0, memcmp( mbsOGuy, decoded4.data(), decoded4.size() ) );
	ASSERT_FALSE( bComplete4_2 );
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeEucJp)
{
	const auto eCodeType = CODE_EUC;
	auto pCodeBase = CCodeFactory::CreateCodeBase( eCodeType );

	// 7bit ASCII範囲（等価変換）
	constexpr const auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr const auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	bool bComplete1_1 = false;
	auto encoded1 = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(mbsAscii), _countof(mbsAscii) ), &bComplete1_1 );
	EXPECT_STREQ( wcsAscii, encoded1.GetStringPtr() );
	EXPECT_TRUE( bComplete1_1 );

	bool bComplete1_2 = false;
	auto decoded1 = pCodeBase->UnicodeToCode( encoded1, &bComplete1_2 );
	EXPECT_EQ( 0, memcmp( mbsAscii, decoded1.data(), decoded1.size() ) );
	EXPECT_TRUE( bComplete1_2 );

	// かな漢字の変換（EUC-JP仕様）
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";
	constexpr const auto& mbsKanaKanji = "\x8E\xB6\x8E\xC5\xA4\xAB\xA4\xCA\xA5\xAB\xA5\xCA\xB4\xC1\xBB\xFA";

	bool bComplete2_1 = false;
	auto encoded2 = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(mbsKanaKanji), _countof(mbsKanaKanji) ), &bComplete2_1 );
	ASSERT_STREQ( wcsKanaKanji, encoded2.GetStringPtr() );
	ASSERT_TRUE( bComplete2_1 );

	bool bComplete2_2 = false;
	auto decoded2 = pCodeBase->UnicodeToCode( encoded2, &bComplete2_2 );
	ASSERT_EQ( 0, memcmp( mbsKanaKanji, decoded2.data(), decoded2.size() ) );
	ASSERT_TRUE( bComplete2_2 );

	// Unicodeから変換できない文字（EUC-JP仕様）
	// （保留）
	constexpr const auto& mbsCantConvEucJp =
		""	// 第1バイト不正
		""	// 第2バイト不正
		;
	constexpr const auto& wcsCantConvEucJp =
		L""
		L""
		;

	bool bComplete3_1 = true;
	auto encoded3 = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(mbsCantConvEucJp), _countof(mbsCantConvEucJp) ), &bComplete3_1 );
	//ASSERT_STREQ( wcsCantConvEucJp, encoded3.GetStringPtr() );
	//ASSERT_FALSE( bComplete3_1 );

	// Unicodeから変換できない文字（EUC-JP仕様）
	constexpr const auto& wcsOGuy = L"森鷗外";
	constexpr const auto& mbsOGuy = "\xBF\xB9\x3F\xB3\xB0"; //森?外

	// 本来のEUC-JPは「森鷗外」を正確に表現できるため、不具合と考えられる。
	//constexpr const auto& wcsOGuy = L"森鷗外";
	//constexpr const auto& mbsOGuy = "\xBF\xB9\x8F\xEC\xBF\xB3\xB0";

	bool bComplete4_2 = true;
	auto decoded4 = pCodeBase->UnicodeToCode( wcsOGuy, &bComplete4_2 );
	ASSERT_EQ( 0, memcmp( mbsOGuy, decoded4.data(), decoded4.size() ) );
	ASSERT_FALSE( bComplete4_2 );
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeLatin1)
{
	const auto eCodeType = CODE_LATIN1;
	auto pCodeBase = CCodeFactory::CreateCodeBase( eCodeType );

	// 7bit ASCII範囲（等価変換）
	constexpr const auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr const auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	bool bComplete1_1 = false;
	auto encoded1 = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(mbsAscii), _countof(mbsAscii) ), &bComplete1_1 );
	EXPECT_STREQ( wcsAscii, encoded1.GetStringPtr() );
	EXPECT_TRUE( bComplete1_1 );

	bool bComplete1_2 = false;
	auto decoded1 = pCodeBase->UnicodeToCode( encoded1, &bComplete1_2 );
	EXPECT_EQ( 0, memcmp( mbsAscii, decoded1.data(), decoded1.size() ) );
	EXPECT_TRUE( bComplete1_2 );

	// Latin1はかな漢字変換非サポート
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeUtf8)
{
	const auto eCodeType = CODE_UTF8;
	auto pCodeBase = CCodeFactory::CreateCodeBase( eCodeType );

	// 7bit ASCII範囲（等価変換）
	constexpr const auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr const auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	bool bComplete1_1 = false;
	auto encoded1 = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(mbsAscii), _countof(mbsAscii) ), &bComplete1_1 );
	EXPECT_STREQ( wcsAscii, encoded1.GetStringPtr() );
	EXPECT_TRUE( bComplete1_1 );

	bool bComplete1_2 = false;
	auto decoded1 = pCodeBase->UnicodeToCode( encoded1, &bComplete1_2 );
	EXPECT_EQ( 0, memcmp( mbsAscii, decoded1.data(), decoded1.size() ) );
	EXPECT_TRUE( bComplete1_2 );

	// かな漢字の変換（UTF-8仕様）
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";
	constexpr const auto& mbsKanaKanji = u8"ｶﾅかなカナ漢字";

	bool bComplete2_1 = false;
	auto encoded2 = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(mbsKanaKanji), _countof(mbsKanaKanji) ), &bComplete2_1 );
	ASSERT_STREQ( wcsKanaKanji, encoded2.GetStringPtr() );
	ASSERT_TRUE( bComplete2_1 );

	bool bComplete2_2 = false;
	auto decoded2 = pCodeBase->UnicodeToCode( encoded2, &bComplete2_2 );
	ASSERT_EQ( 0, memcmp( mbsKanaKanji, decoded2.data(), decoded2.size() ) );
	ASSERT_TRUE( bComplete2_2 );
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeUtf8_OracleImplementation)
{
	const auto eCodeType = CODE_CESU8;
	auto pCodeBase = CCodeFactory::CreateCodeBase( eCodeType );

	// 7bit ASCII範囲（等価変換）
	constexpr const auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr const auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	bool bComplete1_1 = false;
	auto encoded1 = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(mbsAscii), _countof(mbsAscii) ), &bComplete1_1 );
	EXPECT_STREQ( wcsAscii, encoded1.GetStringPtr() );
	EXPECT_TRUE( bComplete1_1 );

	bool bComplete1_2 = false;
	auto decoded1 = pCodeBase->UnicodeToCode( encoded1, &bComplete1_2 );
	EXPECT_EQ( 0, memcmp( mbsAscii, decoded1.data(), decoded1.size() ) );
	EXPECT_TRUE( bComplete1_2 );

	// かな漢字の変換（UTF-8仕様）
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";
	constexpr const auto& mbsKanaKanji = u8"ｶﾅかなカナ漢字";

	bool bComplete2_1 = false;
	auto encoded2 = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(mbsKanaKanji), _countof(mbsKanaKanji) ), &bComplete2_1 );
	ASSERT_STREQ( wcsKanaKanji, encoded2.GetStringPtr() );
	ASSERT_TRUE( bComplete2_1 );

	bool bComplete2_2 = false;
	auto decoded2 = pCodeBase->UnicodeToCode( encoded2, &bComplete2_2 );
	ASSERT_EQ( 0, memcmp( mbsKanaKanji, decoded2.data(), decoded2.size() ) );
	ASSERT_TRUE( bComplete2_2 );
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeUtf16Le)
{
	const auto eCodeType = CODE_UNICODE;
	auto pCodeBase = CCodeFactory::CreateCodeBase( eCodeType );

	// 7bit ASCII範囲（等価変換）
	constexpr auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	// リトルエンディアンのバイナリを作成
	std::basic_string<uint16_t> bin;
	for( const auto ch : mbsAscii ){
		bin.append( 1, ch );
	}

	bool bComplete1_1 = false;
	auto encoded1 = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(bin.data()), bin.size() * sizeof(decltype(bin)::value_type) ), &bComplete1_1 );
	ASSERT_STREQ( wcsAscii, encoded1.GetStringPtr() );
	ASSERT_TRUE( bComplete1_1 );

	bool bComplete1_2 = false;
	auto decoded1 = pCodeBase->UnicodeToCode( encoded1, &bComplete1_2 );
	ASSERT_EQ( 0, memcmp( bin.data(), decoded1.data(), decoded1.size() ) );
	ASSERT_TRUE( bComplete1_2 );

	// かな漢字の変換（UTF-16LE仕様）
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";

	// リトルエンディアンのバイナリを作成
	bin.clear();
	for( const auto ch : wcsKanaKanji ){
		bin.append( 1, ch );
	}

	bool bComplete2_1 = false;
	auto encoded2 = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(bin.data()), bin.size() * sizeof(decltype(bin)::value_type) ), &bComplete2_1 );
	ASSERT_STREQ( wcsKanaKanji, encoded2.GetStringPtr() );
	ASSERT_TRUE( bComplete2_1 );

	bool bComplete2_2 = false;
	auto decoded2 = pCodeBase->UnicodeToCode( encoded2, &bComplete2_2 );
	ASSERT_EQ( 0, memcmp( bin.data(), decoded2.data(), decoded2.size() ) );
	ASSERT_TRUE( bComplete2_2 );
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeUtf16Be)
{
	const auto eCodeType = CODE_UNICODEBE;
	auto pCodeBase = CCodeFactory::CreateCodeBase( eCodeType );

	// 7bit ASCII範囲（等価変換）
	constexpr auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	// ビッグエンディアンのバイナリを作成
	std::basic_string<uint16_t> bin;
	for( const auto ch : mbsAscii ){
		bin.append( 1, ::_byteswap_ushort( ch ) );
	}

	bool bComplete1_1 = false;
	auto encoded = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(bin.data()), bin.size() * sizeof(decltype(bin)::value_type)), &bComplete1_1 );
	ASSERT_STREQ( wcsAscii, encoded.GetStringPtr() );
	ASSERT_TRUE( bComplete1_1 );

	bool bComplete1_2 = false;
	auto decoded = pCodeBase->UnicodeToCode( encoded, &bComplete1_2 );
	ASSERT_EQ( 0, memcmp( bin.data(), decoded.data(), decoded.size() ) );
	ASSERT_TRUE( bComplete1_2 );

	// かな漢字の変換（UTF-16BE仕様）
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";

	// ビッグエンディアンのバイナリを作成
	bin.clear();
	for( const auto ch : wcsKanaKanji ){
		bin.append( 1, ::_byteswap_ushort( ch ) );
	}

	bool bComplete2_1 = false;
	auto encoded2 = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(bin.data()), bin.size() * sizeof(decltype(bin)::value_type) ), &bComplete2_1 );
	ASSERT_STREQ( wcsKanaKanji, encoded2.GetStringPtr() );
	ASSERT_TRUE( bComplete2_1 );

	bool bComplete2_2 = false;
	auto decoded2 = pCodeBase->UnicodeToCode( encoded2, &bComplete2_2 );
	ASSERT_EQ( 0, memcmp( bin.data(), decoded2.data(), decoded2.size() ) );
	ASSERT_TRUE( bComplete2_2 );
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeUtf32Le)
{
	const auto eCodeType = (ECodeType)12000;
	auto pCodeBase = CCodeFactory::CreateCodeBase( eCodeType );

	// 7bit ASCII範囲（等価変換）
	constexpr auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	// リトルエンディアンのバイナリを作成
	std::basic_string<uint32_t> bin;
	for( const auto ch : mbsAscii ){
		bin.append( 1, ch );
	}

	bool bComplete1_1 = false;
	auto encoded = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(bin.data()), bin.size() * sizeof(decltype(bin)::value_type)), &bComplete1_1 );
	ASSERT_STREQ( wcsAscii, encoded.GetStringPtr() );
	ASSERT_TRUE( bComplete1_1 );

	bool bComplete1_2 = false;
	auto decoded = pCodeBase->UnicodeToCode( encoded, &bComplete1_2 );
	ASSERT_EQ( 0, memcmp( bin.data(), decoded.data(), decoded.size() ) );
	ASSERT_TRUE( bComplete1_2 );

	// かな漢字の変換（UTF-32LE仕様）
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";

	// リトルエンディアンのバイナリを作成
	bin.clear();
	for( const auto ch : wcsKanaKanji ){
		bin.append( 1, ch );
	}

	bool bComplete2_1 = false;
	auto encoded2 = pCodeBase->CodeToUnicode( BinarySequenceView( reinterpret_cast<const std::byte*>(bin.data()), bin.size() * sizeof(decltype(bin)::value_type) ), &bComplete2_1 );
	ASSERT_STREQ( wcsKanaKanji, encoded2.GetStringPtr() );
	ASSERT_TRUE( bComplete2_1 );

	bool bComplete2_2 = false;
	auto decoded2 = pCodeBase->UnicodeToCode( encoded2, &bComplete2_2 );
	ASSERT_EQ( 0, memcmp( bin.data(), decoded2.data(), decoded2.size() ) );
	ASSERT_TRUE( bComplete2_2 );
}
