﻿/*! @file */
/*
	Copyright (C) 2018-2021, Sakura Editor Organization

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
// 2008.11.10 変換ロジックを書き直す

#include "StdAfx.h"
#include "CUtf7.h"
#include "charset/charcode.h"
#include "charset/codechecker.h"
#include "convert/convert_util2.h"
#include "CEol.h"

/*!
	UTF-7 Set D 部分の読み込み。
*/
int CUtf7::_Utf7SetDToUni_block( const char* pSrc, const int nSrcLen, wchar_t* pDst )
{
	const char* pr = pSrc;
	wchar_t* pw = pDst;

	for( ; pr < pSrc+nSrcLen; ++pr ){
		if( IsUtf7Direct(*pr) ){
			*pw = *pr;
		}else{
			BinToText(reinterpret_cast<const unsigned char*>(pr), 1, reinterpret_cast<unsigned short*>(pw));
		}
		++pw;
	}
	return pw - pDst;
}

/*!
	UTF-7 Set B 部分の読み込み
*/
int CUtf7::_Utf7SetBToUni_block( const char* pSrc, const int nSrcLen, wchar_t* pDst, bool* pbError )
{
	char* pbuf = new (std::nothrow) char[nSrcLen];
	if (pbuf == NULL) {
		if( pbError ){
			*pbError = true;
		}
		return 0;
	}
	int ndecoded_len = _DecodeBase64( pSrc, nSrcLen, pbuf );
	int nModLen = ndecoded_len % sizeof(wchar_t);
	ndecoded_len = ndecoded_len - nModLen;
	CMemory::SwapHLByte( pbuf, ndecoded_len );  // UTF-16 BE を UTF-16 LE に直す
	memcpy( reinterpret_cast<char*>(pDst), pbuf, ndecoded_len );
	if( nModLen ){
		ndecoded_len += BinToText( reinterpret_cast<const unsigned char *>(pbuf) + ndecoded_len,
			nModLen, &reinterpret_cast<unsigned short*>(pDst)[ndecoded_len / sizeof(wchar_t)]) * sizeof(wchar_t);
		if( pbError ){
			*pbError = true;
		}
	}
	delete [] pbuf;
	return ndecoded_len / sizeof(wchar_t);
}

int CUtf7::Utf7ToUni( const char* pSrc, const int nSrcLen, wchar_t* pDst, bool* pbError )
{
	const char *pr, *pr_end;
	char *pr_next;
	wchar_t *pw;
	int nblocklen=0;
	bool berror_tmp, berror=false;

	pr = pSrc;
	pr_end = pSrc + nSrcLen;
	pw = pDst;

	do{
		// UTF-7 Set D 部分のチェック
		nblocklen = CheckUtf7DPart( pr, pr_end-pr, &pr_next, &berror_tmp );
		if( berror_tmp == true ){
			berror = true;
		}
		pw += _Utf7SetDToUni_block( pr, nblocklen, pw );

		pr = pr_next;  // 次の読み込み位置を取得
		if( pr_next >= pr_end ){
			break;
		}

		// UTF-7 Set B 部分のチェック
		nblocklen = CheckUtf7BPart( pr, pr_end-pr, &pr_next, &berror_tmp, UC_LOOSE );
		{
			// エラーがあってもできるところまでデコード
			if( berror_tmp ){
				berror = true;
			}
			if( nblocklen < 1 && *(pr_next-1) == '-' ){
				// +- → + 変換
				*pw = L'+';
				++pw;
			}else{
				pw += _Utf7SetBToUni_block( pr, nblocklen, pw, &berror_tmp );
				if( berror_tmp != false ){
					berror = true;
				}
			}
		}
		pr = pr_next;  // 次の読み込み位置を取得
	}while( pr_next < pr_end );

	if( pbError ){
		*pbError = berror;
	}

	return pw - pDst;
}

//! UTF-7→Unicodeコード変換
// 2007.08.13 kobake 作成
EConvertResult CUtf7::UTF7ToUnicode( const CMemory& cSrc, CNativeW* pDstMem )
{
	// エラー状態：
	bool bError;

	// データ取得
	int nDataLen = cSrc.GetRawLength();
	const char* pData = reinterpret_cast<const char*>( cSrc.GetRawPtr() );

	// 必要なバッファサイズを調べて確保
	wchar_t* pDst = new (std::nothrow) wchar_t[nDataLen + 1];
	if( pDst == NULL ){
		return RESULT_FAILURE;
	}

	// 変換
	int nDstLen = Utf7ToUni( pData, nDataLen, pDst, &bError );

	// pDstMem を設定
	pDstMem->_GetMemory()->SetRawDataHoldBuffer( pDst, nDstLen*sizeof(wchar_t) );

	delete [] pDst;

	if( bError == false ){
		return RESULT_COMPLETE;
	}else{
		return RESULT_LOSESOME;
	}
}

int CUtf7::_UniToUtf7SetD_block( const wchar_t* pSrc, const int nSrcLen, char* pDst )
{
	int i;

	if( nSrcLen < 1 ){
		return 0;
	}

	for( i = 0; i < nSrcLen; ++i ){
		pDst[i] = static_cast<char>( pSrc[i] & 0x00ff );
	}

	return i;
}

int CUtf7::_UniToUtf7SetB_block( const wchar_t* pSrc, const int nSrcLen, char* pDst )
{
	char* pw;

	if( nSrcLen < 1 ){
		return 0;
	}

	wchar_t* psrc = new (std::nothrow) wchar_t[nSrcLen];
	if( psrc == NULL ){
		return 0;
	}

	// // UTF-16 LE → UTF-16 BE
	wcsncpy( &psrc[0], pSrc, nSrcLen );
	CMemory::SwapHLByte( reinterpret_cast<char*>(psrc), nSrcLen*sizeof(wchar_t) );

	// 書き込み
	pw = pDst;
	pw[0] = '+';
	++pw;
	pw += _EncodeBase64( reinterpret_cast<char*>(psrc), nSrcLen*sizeof(wchar_t), pw );
	pw[0] = '-';
	++pw;

	delete [] psrc;

	return pw - pDst;
}

int CUtf7::UniToUtf7( const wchar_t* pSrc, const int nSrcLen, char* pDst )
{
	const wchar_t *pr, *pr_base;
	const wchar_t* pr_end;
	char* pw;

	pr = pSrc;
	pr_base = pSrc;
	pr_end = pSrc + nSrcLen;
	pw = pDst;

	do{
		for( ; pr < pr_end; ++pr ){
			if( !IsUtf7SetD(*pr) ){
				break;
			}
		}
		pw += _UniToUtf7SetD_block( pr_base, pr-pr_base, pw );
		pr_base = pr;

		if( *pr == L'+' ){
			// '+' → "+-"
			pw[0] = '+';
			pw[1] = '-';
			++pr;
			pw += 2;
		}else{
			for( ; pr < pr_end; ++pr ){
				if( IsUtf7SetD(*pr) ){
					break;
				}
			}
			pw += _UniToUtf7SetB_block( pr_base, pr-pr_base, pw );
		}
		pr_base = pr;
	}while( pr_base < pr_end );

	return pw - pDst;
}

/*! コード変換 Unicode→UTF-7
	@date 2002.10.25 Moca UTF-7で直接エンコードできる文字をRFCに合わせて制限した
*/
EConvertResult CUtf7::UnicodeToUTF7( const CNativeW& cSrc, CMemory* pDstMem )
{
	// データ取得
	const wchar_t* pSrc = cSrc.GetStringPtr();
	int nSrcLen = cSrc.GetStringLength();

	// 出力先バッファの確保
	// 最大で、変換元のデータ長の５倍。
	char *pDst = new (std::nothrow) char[ nSrcLen * 5 + 1 ];  // * → +ACo-
	if( pDst == NULL ){
		return RESULT_FAILURE;
	}

	// 変換
	int nDstLen = UniToUtf7( pSrc, nSrcLen, pDst );

	// pMem にデータをセット
	pDstMem->SetRawDataHoldBuffer( pDst, nDstLen );

	delete [] pDst;

	return RESULT_COMPLETE;
}

//! BOMデータ取得
void CUtf7::GetBom(CMemory* pcmemBom)
{
	static const BYTE UTF7_BOM[]= {'+','/','v','8','-'};
	pcmemBom->SetRawData(UTF7_BOM, sizeof(UTF7_BOM));
}

void CUtf7::GetEol(CMemory* pcmemEol, EEolType eEolType)
{
	static const struct{
		const char* szData;
		int nLen;
	}
	aEolTable[EOL_TYPE_NUM] = {
		{ "",			0 },	// EEolType::none
		{ "\x0d\x0a",	2 },	// EEolType::cr_and_lf
		{ "\x0a",		1 },	// EEolType::line_feed
		{ "\x0d",		1 },	// EEolType::carriage_return
		{ "+AIU-",		5 },	// EEolType::next_line
		{ "+ICg-",		5 },	// EEolType::line_separator
		{ "+ICk-",		5 },	// EEolType::paragraph_separator
	};
	auto& data = aEolTable[static_cast<size_t>(eEolType)];
	pcmemEol->SetRawData(data.szData, data.nLen);
}
