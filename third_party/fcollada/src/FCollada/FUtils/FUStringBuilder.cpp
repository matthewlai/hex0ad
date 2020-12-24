/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America

	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUString.h"
#include "FUStringBuilder.hpp"

#include <limits>

//
// FUString
//

const char* emptyCharString = "";
const fchar* emptyFCharString = FC("");
const fm::string emptyString(emptyCharString);
const fstring emptyFString(emptyFCharString);

//
// FUStringBuilder [parasitic]
//

template<> fstring FUStringBuilder::ToString() const
{
	return fstring(ToCharPtr());
}

template<> void FUStringBuilder::append(uint32 i)
{
	fchar sz[128];
	fsnprintf(sz, 128, FC("%u"), (unsigned int) i);
	append(sz);
}

template<> void FUStringBuilder::append(uint64 i)
{
	fchar sz[128];
	fsnprintf(sz, 128, FC("%u"), (unsigned int) i);
	append(sz);
}

template<> void FUStringBuilder::append(int32 i)
{
	fchar sz[128];
	fsnprintf(sz, 128, FC("%i"), (int) i);
	append(sz);
}

#ifdef UNICODE
template<> fm::string FUSStringBuilder::ToString() const
{
	return fm::string(ToCharPtr());
}

template<> void FUSStringBuilder::append(uint32 i)
{
	char sz[128];
	snprintf(sz, 128, "%u", (unsigned int) i);
	append(sz);
}

template<> void FUSStringBuilder::append(uint64 i)
{
	char sz[128];
	snprintf(sz, 128, "%u", (unsigned int) i);
	append(sz);
}

template<> void FUSStringBuilder::append(int32 i)
{
	char sz[128];
	snprintf(sz, 128, "%i", (int) i);
	append(sz);
}
#endif // UNICODE

template class FUStringBuilderT<char>;