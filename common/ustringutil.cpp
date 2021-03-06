/*
 * Copyright 2005 - 2015  Zarafa B.V. and its licensors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
@file
Unicode String Utilities

@defgroup ustringutil Unicode String Utilities
@{

The Unicode String Utilities provide some common string utilities aimed to be compliant with
all (or at least most) of the Unicode quirks.

The provided functions are:
  - str_equals, wcs_equals, u8_equals: Check if two strings are equal.
  - str_iequals, wcs_iequals, u8_iequals: Check if two strings are equal ignoring case.
  - str_startswith, wcs_startswith, u8_startswith: Check if one string starts with another.
  - str_istartswith, wcs_istartswith, u8_istartswith: Check if one string starts with another ignoring case.
  - str_compare, wcs_compare, u8_compare: Compare two strings.
  - str_icompare, wcs_icompare, u8_icompare: Compare two strings ignoring case.
  - str_contains, wcs_contains, u8_contains: Check if one string contains the other.
  - str_icontains, wcs_icontains, u8_icontains: Check if one string contains the other ignoring case.

@par Normalization
In order to compare unicode strings, the data needs to be normailized first. This is needed because Unicode allows
different binary representations of the same data. The functions provide in this module make no assumptions about
the provided data and will always perform a normalization before doing a comparison.

@par Case mapping
The case insensitive functions need a way to match code points regardless of their case. ICU provides a few methods for
this, but they use a method called case-folding to avoid the need for a locale (changing case is dependant on a locale).
Since case-folding doesn't take a locale, it's a best guess method, which will produce wrong results in certain situations.
The functions in this library apply a method called case-mapping, which basically means we perform a to-upper on all
code-points with a provided locale.

@par Collation
The functions that try to match (sub)strings, have no interest in the order in which strings would appear if they would be
sorted. However, the compare functions do produce a result that could be used for sorting. Since sorting is dependant on a
locale as well, they would need a locale. However, ICU provides a Collator class that performs the actual comparison for a
particular locale. Since we don't want to construct a Collator class for every string comparison, the string comparison
functions take a Collator object as argument. This way the caller can reuse the Collator.

@par Performance
Performance of the current (21-05-2010) implementation is probably pretty bad. This is caused by all the conversion that are
performed on the complete strings before the actual comparison is even started.

At some point we need to rewqrite these functions to do all the conversion on the fly to minimize processing.
*/

#include "config.h"
#include <zarafa/platform.h>
#include <zarafa/ustringutil.h>
#include <zarafa/CommonUtil.h>
#include "utf8.h"

#include <cassert>

#ifdef ZCP_USES_ICU
#include <memory>
#include <unicode/unorm.h>
#include <unicode/coll.h>
#include <unicode/tblcoll.h>
#include <unicode/coleitr.h>
#include <unicode/normlzr.h>
#include <unicode/ustring.h>

#include "ustringutil/utfutil.h"

typedef UTF32Iterator	WCharIterator;
#if __cplusplus >= 201100L
typedef std::unique_ptr<Collator> unique_ptr_Collator;
#else
typedef std::auto_ptr<Collator> unique_ptr_Collator;
#endif

#else
#include <cstring>
#include <zarafa/charset/convert.h>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

#ifndef ZCP_USES_ICU
ECSortKey::ECSortKey(const unsigned char *lpSortData, unsigned int cbSortData)
	: m_lpSortData(lpSortData)
	, m_cbSortData(cbSortData)
{ }

ECSortKey::ECSortKey(const ECSortKey &other)
	: m_lpSortData((unsigned char*)memcpy(new unsigned char[other.m_cbSortData], other.m_lpSortData, other.m_cbSortData))
	, m_cbSortData(other.m_cbSortData)
{ }

ECSortKey::~ECSortKey() {
	delete m_lpSortData;
}

ECSortKey& ECSortKey::operator=(const ECSortKey &other) {
	if (this != &other) {
		delete[] m_lpSortData;

		m_lpSortData = (unsigned char*)memcpy(new unsigned char[other.m_cbSortData], other.m_lpSortData, other.m_cbSortData);
		m_cbSortData = other.m_cbSortData;
	}
	return *this;
}

int ECSortKey::compareTo(const ECSortKey &other) const {
	return compareSortKeys(m_cbSortData, m_lpSortData, other.m_cbSortData, other.m_lpSortData);
}
#endif

/** 
 * US-ASCII version to find a case-insensitive string part in a
 * haystack.
 * 
 * @param haystack search this haystack for a case-insensitive needle
 * @param needle search this needle in the case-insensitive haystack
 * 
 * @return pointer where needle is found or NULL
 */
const char* str_ifind(const char *haystack, const char *needle)
{
	locale_t loc = createlocale(LC_CTYPE, "C");
	const char *needlepos = needle;
	const char *needlestart = haystack;

	while(*haystack) {
		if (toupper_l(*haystack, loc) == toupper_l(*needlepos, loc)) {
			++needlepos;

			if(*needlepos == 0)
				goto exit;
		} else {
			haystack = needlestart++;
			needlepos = needle;
		}

		++haystack;
	}
	needlestart = NULL;

exit:
	freelocale(loc);

	return needlestart;
}

/**
 * Check if two strings are canonical equivalent.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	locale	The locale used to perform string collation.
 * 
 * @return	boolean
 * @retval	true	The strings are canonical equivalent
 * @retval	false	The strings are not canonical equivalent
 */
bool str_equals(const char *s1, const char *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);

#ifdef ZCP_USES_ICU
    UnicodeString a = StringToUnicode(s1);
    UnicodeString b = StringToUnicode(s2);

    return a.compare(b) == 0;
#else
	return strcmp(s1, s2) == 0;
#endif
}

/**
 * Check if two strings are canonical equivalent when ignoring the case.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	locale	The locale used to convert the case of the strings.
 * 
 * @return	boolean
 * @retval	true	The strings are canonical equivalent
 * @retval	false	The strings are not canonical equivalent
 */
bool str_iequals(const char *s1, const char *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);

#ifdef ZCP_USES_ICU
    UnicodeString a = StringToUnicode(s1);
    UnicodeString b = StringToUnicode(s2);

    return a.caseCompare(b, 0) == 0;
#else
	return strcasecmp_l(s1, s2, locale) == 0;
#endif
}

/**
 * Check if the string s1 starts with s2.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	locale	The locale used to perform string collation.
 * 
 * @return	boolean
 * @retval	true	The strings are canonical equivalent
 * @retval	false	The strings are not canonical equivalent
 */
bool str_startswith(const char *s1, const char *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);

#ifdef ZCP_USES_ICU
    UnicodeString a = StringToUnicode(s1);
    UnicodeString b = StringToUnicode(s2);

    return a.compare(0, b.length(), b) == 0;

#else
	size_t cb2 = strlen(s2);
	return strlen(s1) >= cb2 ? (strncmp(s1, s2, cb2) == 0) : false;
#endif
}

/**
 * Check if the string s1 starts with s2 when ignoring the case.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	locale	The locale used to convert the case of the strings.
 * 
 * @return	boolean
 * @retval	true	The strings are canonical equivalent
 * @retval	false	The strings are not canonical equivalent
 */
bool str_istartswith(const char *s1, const char *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);

#ifdef ZCP_USES_ICU
    UnicodeString a = StringToUnicode(s1);
    UnicodeString b = StringToUnicode(s2);

    return a.caseCompare(0, b.length(), b, 0) == 0;
#else
	size_t cb2 = strlen(s2);
	return strlen(s1) >= cb2 ? (strncasecmp_l(s1, s2, cb2, locale) == 0) : false;
#endif
}

/**
 * Compare two strings using the collator to determine the sort order.
 * 
 * Both strings are expectes to be in the current locale.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	collator	The collator used to determine which string precedes the other.
 * 
 * @return		An integer.
 * @retval		-1	s1 is smaller than s2
 * @retval		0	s1 equals s2.
 * @retval		1	s1 is greater than s2
 */
int str_compare(const char *s1, const char *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);
	
#ifdef ZCP_USES_ICU
	UErrorCode status = U_ZERO_ERROR;
	unique_ptr_Collator ptrCollator(Collator::createInstance(locale, status));

	UnicodeString a = StringToUnicode(s1);
	UnicodeString b = StringToUnicode(s2);

	return ptrCollator->compare(a,b,status);
#else
	int r = strcmp(s1, s2);
	return (r < 0 ? -1 : (r > 0 ? 1 : 0));
#endif
}

/**
 * Compare two strings using the collator to determine the sort order.
 * 
 * Both strings are expectes to be in the current locale. The comparison is
 * case insensitive. Effectively this only changes behavior compared to strcmp_unicode
 * if the two strings are the same if the case is discarded. It doesn't effect the
 * sorting in any other way.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	collator	The collator used to determine which string precedes the other.
 * 
 * @return		An integer.
 * @retval		-1	s1 is smaller than s2
 * @retval		0	s1 equals s2.
 * @retval		1	s1 is greater than s2
 */
int str_icompare(const char *s1, const char *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);
	
#ifdef ZCP_USES_ICU
	UErrorCode status = U_ZERO_ERROR;
	unique_ptr_Collator ptrCollator(Collator::createInstance(locale, status));

	UnicodeString a = StringToUnicode(s1);
	UnicodeString b = StringToUnicode(s2);

	a.foldCase();
	b.foldCase();

	return ptrCollator->compare(a,b,status);
#else
	int r = strcasecmp_l(s1, s2, locale);
	return (r < 0 ? -1 : (r > 0 ? 1 : 0));
#endif
}

/**
 * Find a string in another string.
 *
 * @param[in]	haystack	The string to search in
 * @param[in]	needle		The string to search for.
 * @param[in]	locale		The locale used to perform string collation.
 *
 * @return boolean
 * @retval	true	The needle was found
 * @retval	false	The needle wasn't found
 *
 * @note This function behaves different than strstr in that it returns a
 *       a boolean instead of a pointer to the found substring. This is
 *       because we search on a transformed string. Getting the correct
 *       pointer would involve additional processing while we don't need
 *       the result anyway.
 */
bool str_contains(const char *haystack, const char *needle, const ECLocale &locale)
{
	assert(haystack);
	assert(needle);

#ifdef ZCP_USES_ICU
    UnicodeString a = StringToUnicode(haystack);
    UnicodeString b = StringToUnicode(needle);

    return u_strstr(a.getTerminatedBuffer(), b.getTerminatedBuffer());
#else
	return strstr(haystack, needle) != NULL;
#endif
}

/**
 * Find a string in another string while ignoreing case.
 *
 * @param[in]	haystack	The string to search in
 * @param[in]	needle		The string to search for.
 * @param[in]	locale		The locale used to convert the case of the strings.
 *
 * @return boolean
 * @retval	true	The needle was found
 * @retval	false	The needle wasn't found
 */
bool str_icontains(const char *haystack, const char *needle, const ECLocale &locale)
{
	assert(haystack);
	assert(needle);

#ifdef ZCP_USES_ICU
    UnicodeString a = StringToUnicode(haystack);
    UnicodeString b = StringToUnicode(needle);

    a.foldCase();
    b.foldCase();

    return u_strstr(a.getTerminatedBuffer(), b.getTerminatedBuffer());
#else
	return strcasestr(haystack, needle) != NULL;
#endif
}



/**
 * Check if two strings are canonical equivalent.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	locale	The locale used to perform string collation.
 * 
 * @return	boolean
 * @retval	true	The strings are canonical equivalent
 * @retval	false	The strings are not canonical equivalent
 */
bool wcs_equals(const wchar_t *s1, const wchar_t *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);

#ifdef ZCP_USES_ICU
    UnicodeString a = WCHARToUnicode(s1);
    UnicodeString b = WCHARToUnicode(s2);

    return a.compare(b) == 0;
#else
	return wcscmp(s1, s2) == 0;
#endif
}

/**
 * Check if two strings are canonical equivalent when ignoring the case.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	locale	The locale used to convert the case of the strings.
 * 
 * @return	boolean
 * @retval	true	The strings are canonical equivalent
 * @retval	false	The strings are not canonical equivalent
 */
bool wcs_iequals(const wchar_t *s1, const wchar_t *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);

#ifdef ZCP_USES_ICU
    UnicodeString a = WCHARToUnicode(s1);
    UnicodeString b = WCHARToUnicode(s2);

    return a.caseCompare(b, 0) == 0;
#else
	return wcscasecmp_l(s1, s2, locale) == 0;
#endif
}

/**
 * Check if s1 starts with s2.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	locale	The locale used to perform string collation.
 * 
 * @return	boolean
 * @retval	true	The strings are canonical equivalent
 * @retval	false	The strings are not canonical equivalent
 */
bool wcs_startswith(const wchar_t *s1, const wchar_t *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);

#ifdef ZCP_USES_ICU
    UnicodeString a = WCHARToUnicode(s1);
    UnicodeString b = WCHARToUnicode(s2);

    return a.compare(0, b.length(), b) == 0;
#else
	size_t cb2 = wcslen(s2);
	return wcslen(s1) >= cb2 ? (wcsncmp(s1, s2, cb2) == 0) : false;
#endif
}

/**
 * Check if s1 starts with s2 when ignoring the case.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	locale	The locale used to convert the case of the strings.
 * 
 * @return	boolean
 * @retval	true	The strings are canonical equivalent
 * @retval	false	The strings are not canonical equivalent
 */
bool wcs_istartswith(const wchar_t *s1, const wchar_t *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);

#ifdef ZCP_USES_ICU
    UnicodeString a = WCHARToUnicode(s1);
    UnicodeString b = WCHARToUnicode(s2);

    return a.caseCompare(0, b.length(), b, 0) == 0;
#else
	size_t cb2 = wcslen(s2);
	return wcslen(s1) >= cb2 ? (wcsncasecmp_l(s1, s2, cb2, locale) == 0) : false;
#endif
}

/**
 * Compare two strings using the collator to determine the sort order.
 * 
 * Both strings are expectes to be wide character strings.
 * 
 * @param[in]	s1			The string to compare s2 with.
 * @param[in]	s2			The string to compare s1 with.
 * @param[in]	collator	The collator used to determine which string precedes the other.
 * 
 * @return		An integer.
 * @retval		-1	s1 is smaller than s2
 * @retval		0	s1 equals s2.
 * @retval		1	s1 is greater than s2
 */
int wcs_compare(const wchar_t *s1, const wchar_t *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);
	
#ifdef ZCP_USES_ICU
	UErrorCode status = U_ZERO_ERROR;
	unique_ptr_Collator ptrCollator(Collator::createInstance(locale, status));

	UnicodeString a = UTF32ToUnicode((UChar32*)s1);
	UnicodeString b = UTF32ToUnicode((UChar32*)s2);

	return ptrCollator->compare(a,b,status);
#else
	int r = wcscmp(s1, s2);
	return (r < 0 ? -1 : (r > 0 ? 1 : 0));
#endif
}

/**
 * Compare two strings using the collator to determine the sort order.
 * 
 * Both strings are expectes to be in the current locale. The comparison is
 * case insensitive. Effectively this only changes behavior compared to strcmp_unicode
 * if the two strings are the same if the case is discarded. It doesn't effect the
 * sorting in any other way.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	collator	The collator used to determine which string precedes the other.
 * 
 * @return		An integer.
 * @retval		-1	s1 is smaller than s2
 * @retval		0	s1 equals s2.
 * @retval		1	s1 is greater than s2
 */
int wcs_icompare(const wchar_t *s1, const wchar_t *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);
	
#ifdef ZCP_USES_ICU
	UErrorCode status = U_ZERO_ERROR;
	unique_ptr_Collator ptrCollator(Collator::createInstance(locale, status));

	UnicodeString a = WCHARToUnicode(s1);
	UnicodeString b = WCHARToUnicode(s2);

	a.foldCase();
	b.foldCase();

	return ptrCollator->compare(a,b,status);
#else
	int r = wcscasecmp_l(s1, s2, locale);
	return (r < 0 ? -1 : (r > 0 ? 1 : 0));
#endif
}

/**
 * Find a string in another string.
 *
 * @param[in]	haystack	The string to search in
 * @param[in]	needle		The string to search for.
 * @param[in]	locale		The locale used to perform string collation.
 *
 * @return boolean
 * @retval	true	The needle was found
 * @retval	false	The needle wasn't found
 *
 * @note This function behaves different than strstr in that it returns a
 *       a boolean instead of a pointer to the found substring. This is
 *       because we search on a transformed string. Getting the correct
 *       pointer would involve additional processing while we don't need
 *       the result anyway.
 */
bool wcs_contains(const wchar_t *haystack, const wchar_t *needle, const ECLocale &locale)
{
	assert(haystack);
	assert(needle);

#ifdef ZCP_USES_ICU
    UnicodeString a = WCHARToUnicode(haystack);
    UnicodeString b = WCHARToUnicode(needle);

    return u_strstr(a.getTerminatedBuffer(), b.getTerminatedBuffer());
#else
	return wcsstr(haystack, needle) != NULL;
#endif
}

/**
 * Find a string in another string while ignoreing case.
 *
 * @param[in]	haystack	The string to search in
 * @param[in]	needle		The string to search for.
 * @param[in]	locale		The locale to use when converting case.
 *
 * @return boolean
 * @retval	true	The needle was found
 * @retval	false	The needle wasn't found
 *
 * @note This function behaves different than strstr in that it returns a
 *       a boolean instead of a pointer to the found substring. This is
 *       because we search on a transformed string. Getting the correct
 *       pointer would involve additional processing while we don't need
 *       the result anyway.
 */
bool wcs_icontains(const wchar_t *haystack, const wchar_t *needle, const ECLocale &locale)
{
	assert(haystack);
	assert(needle);

#ifdef ZCP_USES_ICU
    UnicodeString a = WCHARToUnicode(haystack);
    UnicodeString b = WCHARToUnicode(needle);

    a.foldCase();
    b.foldCase();

    return u_strstr(a.getTerminatedBuffer(), b.getTerminatedBuffer());
#else
	const size_t cbHaystack = wcslen(haystack);
	const size_t cbNeedle = wcslen(needle);
	const wchar_t *lpHay = haystack;
	while (cbHaystack - (lpHay - haystack) >= cbNeedle) {
		if (wcsncasecmp_l(lpHay, needle, cbNeedle, locale) == 0)
			return true;
		++lpHay;
	}
	return false;
#endif
}




/**
 * Check if two strings are canonical equivalent.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	locale	The locale used to perform string collation.
 * 
 * @return	boolean
 * @retval	true	The strings are canonical equivalent
 * @retval	false	The strings are not canonical equivalent
 */
bool u8_equals(const char *s1, const char *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);

#ifdef ZCP_USES_ICU
    UnicodeString a = UTF8ToUnicode(s1);
    UnicodeString b = UTF8ToUnicode(s2);

    return a.compare(b) == 0;
#else
	convert_context converter;
	const wchar_t *ws1 = converter.convert_to<WCHAR*>(s1, rawsize(s1), "UTF-8");
	const wchar_t *ws2 = converter.convert_to<WCHAR*>(s2, rawsize(s2), "UTF-8");
	return wcs_equals(ws1, ws2, locale);
#endif
}

/**
 * Check if two strings are canonical equivalent when ignoring the case.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	locale	The locale to use when converting case.
 * 
 * @return	boolean
 * @retval	true	The strings are canonical equivalent
 * @retval	false	The strings are not canonical equivalent
 */
bool u8_iequals(const char *s1, const char *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);

#ifdef ZCP_USES_ICU
    UnicodeString a = UTF8ToUnicode(s1);
    UnicodeString b = UTF8ToUnicode(s2);

    return a.caseCompare(b, 0) == 0;
#else
	convert_context converter;
	const wchar_t *ws1 = converter.convert_to<WCHAR*>(s1, rawsize(s1), "UTF-8");
	const wchar_t *ws2 = converter.convert_to<WCHAR*>(s2, rawsize(s2), "UTF-8");
	return wcs_iequals(ws1, ws2, locale);
#endif
}

/**
 * Check if s1 starts with s2.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	locale	The locale used to perform string collation.
 * 
 * @return	boolean
 * @retval	true	The strings are canonical equivalent
 * @retval	false	The strings are not canonical equivalent
 */
bool u8_startswith(const char *s1, const char *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);

#ifdef ZCP_USES_ICU
    UnicodeString a = UTF8ToUnicode(s1);
    UnicodeString b = UTF8ToUnicode(s2);

    return a.compare(0, b.length(), b) == 0;
#else
	convert_context converter;
	const wchar_t *ws1 = converter.convert_to<WCHAR*>(s1, rawsize(s1), "UTF-8");
	const wchar_t *ws2 = converter.convert_to<WCHAR*>(s2, rawsize(s2), "UTF-8");
	return wcs_startswith(ws1, ws2, locale);
#endif
}

/**
 * Check if s1 starts with s2 when ignoring the case.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	locale	The locale to use when converting case.
 * 
 * @return	boolean
 * @retval	true	The strings are canonical equivalent
 * @retval	false	The strings are not canonical equivalent
 */
bool u8_istartswith(const char *s1, const char *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);

#ifdef ZCP_USES_ICU
    UnicodeString a = UTF8ToUnicode(s1);
    UnicodeString b = UTF8ToUnicode(s2);

    return a.caseCompare(0, b.length(), b, 0) == 0;
#else
	convert_context converter;
	const wchar_t *ws1 = converter.convert_to<WCHAR*>(s1, rawsize(s1), "UTF-8");
	const wchar_t *ws2 = converter.convert_to<WCHAR*>(s2, rawsize(s2), "UTF-8");
	return wcs_istartswith(ws1, ws2, locale);
#endif
}

/**
 * Compare two strings using the collator to determine the sort order.
 * 
 * Both strings are expectes to be encoded in UTF-8.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	collator	The collator used to determine which string precedes the other.
 * 
 * @return		An integer.
 * @retval		-1	s1 is smaller than s2
 * @retval		0	s1 equals s2.
 * @retval		1	s1 is greater than s2
 */
int u8_compare(const char *s1, const char *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);

#ifdef ZCP_USES_ICU
	UErrorCode status = U_ZERO_ERROR;
	unique_ptr_Collator ptrCollator(Collator::createInstance(locale, status));

	UnicodeString a = UTF8ToUnicode(s1);
	UnicodeString b = UTF8ToUnicode(s2);

	return ptrCollator->compare(a,b,status);
#else
	convert_context converter;
	const wchar_t *ws1 = converter.convert_to<WCHAR*>(s1, rawsize(s1), "UTF-8");
	const wchar_t *ws2 = converter.convert_to<WCHAR*>(s2, rawsize(s2), "UTF-8");
	return wcs_compare(ws1, ws2, locale);
#endif
}

/**
 * Compare two strings using the collator to determine the sort order.
 * 
 * Both strings are expectes to be encoded in UTF-8. The comparison is
 * case insensitive. Effectively this only changes behavior compared to strcmp_unicode
 * if the two strings are the same if the case is discarded. It doesn't effect the
 * sorting in any other way.
 * 
 * @param[in]	s1		The string to compare s2 with.
 * @param[in]	s2		The string to compare s1 with.
 * @param[in]	collator	The collator used to determine which string precedes the other.
 * 
 * @return		An integer.
 * @retval		-1	s1 is smaller than s2
 * @retval		0	s1 equals s2.
 * @retval		1	s1 is greater than s2
 */
int u8_icompare(const char *s1, const char *s2, const ECLocale &locale)
{
	assert(s1);
	assert(s2);
	
#ifdef ZCP_USES_ICU
	UErrorCode status = U_ZERO_ERROR;
	unique_ptr_Collator ptrCollator(Collator::createInstance(locale, status));

	UnicodeString a = UTF8ToUnicode(s1);
	UnicodeString b = UTF8ToUnicode(s2);
	
	a.foldCase();
	b.foldCase();

	return ptrCollator->compare(a,b,status);
#else
	convert_context converter;
	const wchar_t *ws1 = converter.convert_to<WCHAR*>(s1, rawsize(s1), "UTF-8");
	const wchar_t *ws2 = converter.convert_to<WCHAR*>(s2, rawsize(s2), "UTF-8");
	return wcs_icompare(ws1, ws2, locale);
#endif
}

/**
 * Find a string in another string.
 *
 * @param[in]	haystack	The string to search in
 * @param[in]	needle		The string to search for.
 * @param[in]	locale		The locale used to perform string collation.
 *
 * @return boolean
 * @retval	true	The needle was found
 * @retval	false	The needle wasn't found
 *
 * @note This function behaves different than strstr in that it returns a
 *       a boolean instead of a pointer to the found substring. This is
 *       because we search on a transformed string. Getting the correct
 *       pointer would involve additional processing while we don't need
 *       the result anyway.
 */
bool u8_contains(const char *haystack, const char *needle, const ECLocale &locale)
{
	assert(haystack);
	assert(needle);

#ifdef ZCP_USES_ICU
    UnicodeString a = UTF8ToUnicode(haystack);
    UnicodeString b = UTF8ToUnicode(needle);

    return u_strstr(a.getTerminatedBuffer(), b.getTerminatedBuffer());
#else
	convert_context converter;
	const wchar_t *ws1 = converter.convert_to<WCHAR*>(haystack, rawsize(haystack), "UTF-8");
	const wchar_t *ws2 = converter.convert_to<WCHAR*>(needle, rawsize(needle), "UTF-8");
	return wcs_contains(ws1, ws2, locale);
#endif
}

/**
 * Find a string in another string while ignoreing case.
 *
 * @param[in]	haystack	The string to search in
 * @param[in]	needle		The string to search for.
 * @param[in]	locale		The locale to use when converting case.
 *
 * @return boolean
 * @retval	true	The needle was found
 * @retval	false	The needle wasn't found
 */
bool u8_icontains(const char *haystack, const char *needle, const ECLocale &locale)
{
	assert(haystack);
	assert(needle);

#ifdef ZCP_USES_ICU
    UnicodeString a = UTF8ToUnicode(haystack);
    UnicodeString b = UTF8ToUnicode(needle);

    a.foldCase();
    b.foldCase();

    return u_strstr(a.getTerminatedBuffer(), b.getTerminatedBuffer());
#else
	convert_context converter;
	const wchar_t *ws1 = converter.convert_to<WCHAR*>(haystack, rawsize(haystack), "UTF-8");
	const wchar_t *ws2 = converter.convert_to<WCHAR*>(needle, rawsize(needle), "UTF-8");
	return wcs_icontains(ws1, ws2, locale);
#endif
}


/**
 * Copy at most n characters from the utf8 string src to lpstrDest.
 *
 * @param[in]	src			The UTF-8 source data to copy
 * @param[in]	n			The maximum amount of characters to copy
 * @param[out]	lpstrDest	The copied data.
 *
 * @return The amount of characters copied.
 */
unsigned u8_ncpy(const char *src, unsigned n, std::string *lpstrDest)
{
	const char *it = src;
	unsigned len = 0;
	while (true) {
		const char *tmp = it;
		utf8::uint32_t cp = utf8::unchecked::next(tmp);
		if (cp == 0)
			break;
		it = tmp;
		if (++len == n)
			break;
	}
	lpstrDest->assign(src, it);
	return len;
}

/**
 * Returns the length in bytes of the string s when capped to a maximum of
 * max characters.
 *
 * @param[in]	s		The UTF-8 string to process
 * @param[in]	max		The maximum amount of characters for which to return
 * 						the length in bytes.
 *
 * @return	The length in bytes of the capped string.
 */
unsigned u8_cappedbytes(const char *s, unsigned max)
{
	const char *it = s;
	unsigned len = 0;
	while (true) {
		const char *tmp = it;
		utf8::uint32_t cp = utf8::unchecked::next(tmp);
		if (cp == 0)
			break;
		it = tmp;
		if (++len == max)
			break;
	}
	return unsigned(it - s);
}

/**
 * Returns the length in characters of the passed UTF-8 string s
 *
 * @param[in]	s	The UTF-8 string to get length of.
 *
 * @return	The length in characters of string s
 */
unsigned u8_len(const char *s)
{
	unsigned len = 0;
	while (true) {
		utf8::uint32_t cp = utf8::unchecked::next(s);
		if (cp == 0)
			break;
		++len;
	}
	return len;

}

static const struct localemap {
	const char *lpszLocaleID;	/*< Posix locale id */
	ULONG ulLCID;				/*< Windows LCID */
	const char *lpszLocaleName;	/*< Windows locale name */
} localeMap[] = {
	{"af",54,"Afrikaans_South Africa"},
	{"af_NA",54,"Afrikaans_South Africa"},
	{"af_ZA",1078,"Afrikaans_South Africa"},
	{"ar",1,"Arabic_Saudi Arabia"},
	{"ar_BH",15361,"Arabic_Bahrain"},
	{"ar_DZ",5121,"Arabic_Algeria"},
	{"ar_EG",3073,"Arabic_Egypt"},
	{"ar_IQ",2049,"Arabic_Iraq"},
	{"ar_JO",11265,"Arabic_Jordan"},
	{"ar_KW",13313,"Arabic_Kuwait"},
	{"ar_LB",12289,"Arabic_Lebanon"},
	{"ar_LY",4097,"Arabic_Libya"},
	{"ar_MA",6145,"Arabic_Morocco"},
	{"ar_OM",8193,"Arabic_Oman"},
	{"ar_QA",16385,"Arabic_Qatar"},
	{"ar_SA",1025,"Arabic_Saudi Arabia"},
	{"ar_SD",1,"Arabic_Saudi Arabia"},
	{"ar_SY",10241,"Arabic_Syria"},
	{"ar_TN",7169,"Arabic_Tunisia"},
	{"ar_YE",9217,"Arabic_Yemen"},
	{"az",44,"Azeri (Latin)_Azerbaijan"},
	{"az_Cyrl_AZ",2092,"Azeri (Cyrillic)_Azerbaijan"},
	{"az_Latn_AZ",1068,"Azeri (Latin)_Azerbaijan"},
	{"be",35,"Belarusian_Belarus"},
	{"be_BY",1059,"Belarusian_Belarus"},
	{"bg",2,"Bulgarian_Bulgaria"},
	{"bg_BG",1026,"Bulgarian_Bulgaria"},
	{"ca",3,"Catalan_Spain"},
	{"ca_ES",1027,"Catalan_Spain"},
	{"cs",5,"Czech_Czech Republic"},
	{"cs_CZ",1029,"Czech_Czech Republic"},
	{"cy",82,"Welsh_United Kingdom"},
	{"cy_GB",1106,"Welsh_United Kingdom"},
	{"da",6,"Danish_Denmark"},
	{"da_DK",1030,"Danish_Denmark"},
	{"de",7,"German_Germany"},
	{"de_AT",3079,"German_Austria"},
	{"de_BE",7,"German_Germany"},
	{"de_CH",2055,"German_Switzerland"},
	{"de_DE",1031,"German_Germany"},
	{"de_LI",5127,"German_Liechtenstein"},
	{"de_LU",4103,"German_Luxembourg"},
	{"el",8,"Greek_Greece"},
	{"el_CY",8,"Greek_Greece"},
	{"el_GR",1032,"Greek_Greece"},
	{"en",9,"English_United States"},
	{"en_AU",3081,"English_Australia"},
	{"en_BE",9,"English_United States"},
	{"en_BW",9,"English_United States"},
	{"en_BZ",10249,"English_Belize"},
	{"en_CA",4105,"English_Canada"},
	{"en_GB",2057,"English_United Kingdom"},
	{"en_HK",9,"English_United States"},
	{"en_IE",6153,"English_Ireland"},
	{"en_JM",8201,"English_Jamaica"},
	{"en_MH",1033,"English_United States"},
	{"en_MT",9,"English_United States"},
	{"en_MU",9,"English_United States"},
	{"en_NA",9,"English_United States"},
	{"en_NZ",5129,"English_New Zealand"},
	{"en_PH",13321,"English_Republic of the Philippines"},
	{"en_PK",9,"English_United States"},
	{"en_TT",11273,"English_Trinidad and Tobago"},
	{"en_US",1033,"English_United States"},
	{"en_VI",9225,"English_Caribbean"},
	{"en_ZA",7177,"English_South Africa"},
	{"en_ZW",12297,"English_Zimbabwe"},
	{"es",10,"Spanish_Spain"},
	{"es_AR",11274,"Spanish_Argentina"},
	{"es_BO",16394,"Spanish_Bolivia"},
	{"es_CL",13322,"Spanish_Chile"},
	{"es_CO",9226,"Spanish_Colombia"},
	{"es_CR",5130,"Spanish_Costa Rica"},
	{"es_DO",7178,"Spanish_Dominican Republic"},
	{"es_EC",12298,"Spanish_Ecuador"},
	{"es_ES",3082,"Spanish_Spain"},
	{"es_GQ",10,"Spanish_Spain"},
	{"es_GT",4106,"Spanish_Guatemala"},
	{"es_HN",18442,"Spanish_Honduras"},
	{"es_MX",2058,"Spanish_Mexico"},
	{"es_NI",19466,"Spanish_Nicaragua"},
	{"es_PA",6154,"Spanish_Panama"},
	{"es_PE",10250,"Spanish_Peru"},
	{"es_PR",20490,"Spanish_Puerto Rico"},
	{"es_PY",15370,"Spanish_Paraguay"},
	{"es_SV",17418,"Spanish_El Salvador"},
	{"es_UY",14346,"Spanish_Uruguay"},
	{"es_VE",8202,"Spanish_Venezuela"},
	{"et",37,"Estonian_Estonia"},
	{"et_EE",1061,"Estonian_Estonia"},
	{"eu",45,"Basque_Spain"},
	{"eu_ES",1069,"Basque_Spain"},
	{"fa",41,"Farsi_Iran"},
	{"fa_IR",1065,"Farsi_Iran"},
	{"fi",11,"Finnish_Finland"},
	{"fi_FI",1035,"Finnish_Finland"},
	{"fil",100,"Filipino_Philippines"},
	{"fil_PH",1124,"Filipino_Philippines"},
	{"fo",56,"Faroese_Faroe Islands"},
	{"fo_FO",1080,"Faroese_Faroe Islands"},
	{"fr",12,"French_France"},
	{"fr_BE",2060,"French_Belgium"},
	{"fr_BL",12,"French_France"},
	{"fr_CA",3084,"French_Canada"},
	{"fr_CF",12,"French_France"},
	{"fr_CH",4108,"French_Switzerland"},
	{"fr_FR",1036,"French_France"},
	{"fr_GN",12,"French_France"},
	{"fr_GP",12,"French_France"},
	{"fr_LU",5132,"French_Luxembourg"},
	{"fr_MC",6156,"French_Principality of Monaco"},
	{"fr_MF",12,"French_France"},
	{"fr_MG",12,"French_France"},
	{"fr_MQ",12,"French_France"},
	{"fr_NE",12,"French_France"},
	{"ga_IE",2108,"Irish_Ireland"},
	{"gl",86,"Galician_Spain"},
	{"gl_ES",1110,"Galician_Spain"},
	{"gu",71,"Gujarati_India"},
	{"gu_IN",1095,"Gujarati_India"},
	{"he",13,"Hebrew_Israel"},
	{"he_IL",1037,"Hebrew_Israel"},
	{"hi",57,"Hindi_India"},
	{"hi_IN",1081,"Hindi_India"},
	{"hr",26,"Croatian_Croatia"},
	{"hr_HR",1050,"Croatian_Croatia"},
	{"hu",14,"Hungarian_Hungary"},
	{"hu_HU",1038,"Hungarian_Hungary"},
	{"hy",43,"Armenian_Armenia"},
	{"hy_AM",1067,"Armenian_Armenia"},
	{"id",33,"Indonesian_Indonesia"},
	{"id_ID",1057,"Indonesian_Indonesia"},
	{"is",15,"Icelandic_Iceland"},
	{"is_IS",1039,"Icelandic_Iceland"},
	{"it",16,"Italian_Italy"},
	{"it_CH",2064,"Italian_Switzerland"},
	{"it_IT",1040,"Italian_Italy"},
	{"ja",17,"Japanese_Japan"},
	{"ja_JP",1041,"Japanese_Japan"},
	{"ka",55,"Georgian_Georgia"},
	{"ka_GE",1079,"Georgian_Georgia"},
	{"kk",63,"Kazakh_Kazakhstan"},
	{"kk_Cyrl",63,"Kazakh_Kazakhstan"},
	{"kk_Cyrl_KZ",63,"Kazakh_Kazakhstan"},
	{"kn",75,"Kannada_India"},
	{"kn_IN",1099,"Kannada_India"},
	{"ko",18,"Korean_Korea"},
	{"ko_KR",1042,"Korean_Korea"},
	{"kok",87,"Konkani_India"},
	{"kok_IN",1111,"Konkani_India"},
	{"lt",39,"Lithuanian_Lithuania"},
	{"lt_LT",1063,"Lithuanian_Lithuania"},
	{"lv",38,"Latvian_Latvia"},
	{"lv_LV",1062,"Latvian_Latvia"},
	{"mk",47,"FYRO Macedonian_Former Yugoslav Republic of Macedonia"},
	{"mk_MK",1071,"FYRO Macedonian_Former Yugoslav Republic of Macedonia"},
	{"mr",78,"Marathi_India"},
	{"mr_IN",1102,"Marathi_India"},
	{"ms",62,"Malay_Malaysia"},
	{"ms_BN",2110,"Malay_Brunei Darussalam"},
	{"ms_MY",1086,"Malay_Malaysia"},
	{"mt",58,"Maltese_Malta"},
	{"mt_MT",1082,"Maltese_Malta"},
	{"nb_NO",1044,"Norwegian_Norway"},
	{"ne",97,"Nepali_Nepal"},
	{"ne_NP",1121,"Nepali_Nepal"},
	{"nl",19,"Dutch_Netherlands"},
	{"nl_BE",2067,"Dutch_Belgium"},
	{"nl_NL",1043,"Dutch_Netherlands"},
	{"nn_NO",2068,"Norwegian (Nynorsk)_Norway"},
	{"pa",70,"Punjabi_India"},
	{"pa_Arab",70,"Punjabi_India"},
	{"pa_Arab_PK",70,"Punjabi_India"},
	{"pa_Guru",70,"Punjabi_India"},
	{"pa_Guru_IN",70,"Punjabi_India"},
	{"pl",21,"Polish_Poland"},
	{"pl_PL",1045,"Polish_Poland"},
	{"ps",99,"Pashto_Afghanistan"},
	{"ps_AF",1123,"Pashto_Afghanistan"},
	{"pt",22,"Portuguese_Brazil"},
	{"pt_BR",1046,"Portuguese_Brazil"},
	{"pt_GW",22,"Portuguese_Brazil"},
	{"pt_MZ",22,"Portuguese_Brazil"},
	{"pt_PT",2070,"Portuguese_Portugal"},
	{"rm",23,"Romansh_Switzerland"},
	{"rm_CH",1047,"Romansh_Switzerland"},
	{"ro",24,"Romanian_Romania"},
	{"ro_MD",24,"Romanian_Romania"},
	{"ro_RO",1048,"Romanian_Romania"},
	{"ru",25,"Russian_Russia"},
	{"ru_MD",25,"Russian_Russia"},
	{"ru_RU",1049,"Russian_Russia"},
	{"ru_UA",25,"Russian_Russia"},
	{"sk",27,"Slovak_Slovakia"},
	{"sk_SK",1051,"Slovak_Slovakia"},
	{"sl",36,"Slovenian_Slovenia"},
	{"sl_SI",1060,"Slovenian_Slovenia"},
	{"sq",28,"Albanian_Albania"},
	{"sq_AL",1052,"Albanian_Albania"},
	{"sr_Cyrl_BA",7194,"Serbian (Cyrillic)_Bosnia and Herzegovina"},
	{"sr_Latn_BA",6170,"Serbian (Latin)_Bosnia and Herzegovina"},
	{"sv",29,"Swedish_Sweden"},
	{"sv_FI",2077,"Swedish_Finland"},
	{"sv_SE",1053,"Swedish_Sweden"},
	{"sw",65,"Swahili_Kenya"},
	{"sw_KE",1089,"Swahili_Kenya"},
	{"sw_TZ",65,"Swahili_Kenya"},
	{"ta",73,"Tamil_India"},
	{"ta_IN",1097,"Tamil_India"},
	{"ta_LK",73,"Tamil_India"},
	{"te",74,"Telugu_India"},
	{"te_IN",1098,"Telugu_India"},
	{"th",30,"Thai_Thailand"},
	{"th_TH",1054,"Thai_Thailand"},
	{"tr",31,"Turkish_Turkey"},
	{"tr_TR",1055,"Turkish_Turkey"},
	{"uk",34,"Ukrainian_Ukraine"},
	{"uk_UA",1058,"Ukrainian_Ukraine"},
	{"ur",32,"Urdu_Islamic Republic of Pakistan"},
	{"ur_PK",1056,"Urdu_Islamic Republic of Pakistan"},
	{"uz",67,"Uzbek (Latin)_Uzbekistan"},
	{"uz_Arab",67,"Uzbek (Latin)_Uzbekistan"},
	{"uz_Arab_AF",67,"Uzbek (Latin)_Uzbekistan"},
	{"uz_Cyrl_UZ",2115,"Uzbek (Cyrillic)_Uzbekistan"},
	{"uz_Latn_UZ",1091,"Uzbek (Latin)_Uzbekistan"},
	{"vi",42,"Vietnamese_Viet Nam"},
	{"vi_VN",1066,"Vietnamese_Viet Nam"},
	{"zh_Hans",4,"Chinese_Taiwan"},
	{"zh_Hans_CN",2052,"Chinese_People's Republic of China"},
	{"zh_Hans_HK",4,"Chinese_Taiwan"},
	{"zh_Hans_MO",4,"Chinese_Taiwan"},
	{"zh_Hans_SG",4100,"Chinese_Singapore"},
	{"zh_Hant_TW",1028,"Chinese_Taiwan"},
	{"zu",53,"Zulu_South Africa"},
	{"zu_ZA",1077,"Zulu_South Africa"},
};


ECLocale createLocaleFromName(const char *lpszLocale)
{
#ifdef ZCP_USES_ICU
	return Locale::createFromName(lpszLocale);
#else
	if (lpszLocale == NULL)
		lpszLocale = "";
	// We only need LC_CTYPE and LC_COLLATE, but createlocale doesn't support that
	return ECLocale(localemask(LC_ALL), lpszLocale);
#endif
}

ECRESULT LocaleIdToLCID(const char *lpszLocaleID, ULONG *lpulLcid)
{
	const struct localemap *lpMapEntry = NULL;

	ASSERT(lpszLocaleID != NULL);
	ASSERT(lpulLcid != NULL);

	for (unsigned i = 0; !lpMapEntry && i < arraySize(localeMap); ++i)
		if (stricmp(localeMap[i].lpszLocaleID, lpszLocaleID) == 0)
			lpMapEntry = &localeMap[i];

	if (lpMapEntry == NULL)
		return ZARAFA_E_NOT_FOUND;
	*lpulLcid = lpMapEntry->ulLCID;
	return erSuccess;
}

ECRESULT LCIDToLocaleId(ULONG ulLcid, const char **lppszLocaleID)
{
	const struct localemap *lpMapEntry = NULL;

	ASSERT(lppszLocaleID != NULL);

	for (unsigned i = 0; !lpMapEntry && i < arraySize(localeMap); ++i)
		if (localeMap[i].ulLCID == ulLcid)
			lpMapEntry = &localeMap[i];

	if (lpMapEntry == NULL)
		return ZARAFA_E_NOT_FOUND;
	*lppszLocaleID = lpMapEntry->lpszLocaleID;
	return erSuccess;
}

ECRESULT LocaleIdToLocaleName(const char *lpszLocaleID, const char **lppszLocaleName)
{
	const struct localemap *lpMapEntry = NULL;

	ASSERT(lpszLocaleID != NULL);
	ASSERT(lppszLocaleName != NULL);

	for (unsigned i = 0; !lpMapEntry && i < arraySize(localeMap); ++i)
		if (stricmp(localeMap[i].lpszLocaleID, lpszLocaleID) == 0)
			lpMapEntry = &localeMap[i];

	if (lpMapEntry == NULL)
		return ZARAFA_E_NOT_FOUND;
	*lppszLocaleName = lpMapEntry->lpszLocaleName;
	return erSuccess;
}

#ifdef ZCP_USES_ICU
/**
 * Create a locale independant blob that can be used to sort
 * strings fast. This is used when a string would be compared
 * multiple times.
 *
 * @param[in]	s			The string to compare.
 * @param[in]	nCap		Base the key on the first nCap characters of s (if larger than 0).
 * @param[in]	locale		The locale used to create the sort key.
 *
 * @returns		ECSortKey object containing the blob
 */
static ECSortKey createSortKey(UnicodeString s, int nCap,
    const ECLocale &locale)
{
	if (nCap > 1)
		s.truncate(nCap);

	// Quick workaround for sorting items starting with ' (like From and To) and ( and '(
	if (s.startsWith("'") || s.startsWith("("))
		s.remove(0, 1);

	CollationKey key;
	UErrorCode status = U_ZERO_ERROR;
	unique_ptr_Collator ptrCollator(Collator::createInstance(locale, status));
	ptrCollator->getCollationKey(s, key, status);	// Create a collation key for sorting

	return key;
}

/**
 * Create a locale independant blob that can be used to sort
 * strings fast. This is used when a string would be compared
 * multiple times.
 *
 * @param[in]	s			The string to compare.
 * @param[in]	nCap		Base the key on the first nCap characters of s (if larger than 0).
 * @param[in]	locale		The locale used to create the sort key.
 * @param[out]	lpcbKeys	The size in bytes of the returned key.
 * @param[ou]t	lppKey		The returned key.
 */
static void createSortKeyData(const UnicodeString &s, int nCap, const ECLocale &locale, unsigned int *lpcbKey, unsigned char **lppKey)
{
	unsigned char *lpKey = NULL;

	CollationKey key = createSortKey(s, nCap, locale);

	int32_t 		cbKeyData = 0;
	const uint8_t	*lpKeyData = key.getByteArray(cbKeyData);

	lpKey = new unsigned char[cbKeyData];
	memcpy(lpKey, lpKeyData, cbKeyData);		

	*lpcbKey = cbKeyData;
	*lppKey = lpKey;
}
#endif

/**
 * Create a locale independant blob that can be used to sort
 * strings fast. This is used when a string would be compared
 * multiple times.
 *
 * @param[in]	s			The string to compare.
 * @param[in]	nCap		Base the key on the first nCap characters of s (if larger than 0).
 * @param[in]	locale		The locale used to create the sort key.
 * @param[out]	lpcbKeys	The size in bytes of the returned key.
 * @param[ou]t	lppKey		The returned key.
 */
void createSortKeyData(const char *s, int nCap, const ECLocale &locale, unsigned int *lpcbKey, unsigned char **lppKey)
{
	ASSERT(s != NULL);
	ASSERT(lpcbKey != NULL);
	ASSERT(lppKey != NULL);

#ifdef ZCP_USES_ICU
	createSortKeyData(UnicodeString(s), nCap, locale, lpcbKey, lppKey);
#else
	std::wstring wstrTmp = convert_to<std::wstring>(s);
	createSortKeyData(wstrTmp.c_str(), nCap, locale, lpcbKey, lppKey);
#endif
}

/**
 * Create a locale independant blob that can be used to sort
 * strings fast. This is used when a string would be compared
 * multiple times.
 *
 * @param[in]	s			The string to compare.
 * @param[in]	locale		The locale used to create the sort key.
 * @param[out]	lpcbKeys	The size in bytes of the returned key.
 * @param[ou]t	lppKey		The returned key.
 */
void createSortKeyData(const wchar_t *s, int nCap, const ECLocale &locale, unsigned int *lpcbKey, unsigned char **lppKey)
{
	ASSERT(s != NULL);
	ASSERT(lpcbKey != NULL);
	ASSERT(lppKey != NULL);

#ifdef ZCP_USES_ICU
	UnicodeString ustring;
	ustring = UTF32ToUnicode((const UChar32*)s);
	createSortKeyData(ustring, nCap, locale, lpcbKey, lppKey);
#else
	ASSERT((locale_t)locale != NULL);

	unsigned int cbKey = 1 + wcsxfrm_l(NULL, s, 0, locale);
	wchar_t *lpKey = new wchar_t[cbKey];

	wcsxfrm_l(lpKey, s, cbKey, locale);

	*lpcbKey = cbKey * sizeof(wchar_t);
	*lppKey = (unsigned char*)lpKey;
#endif
}

/**
 * Create a locale independant blob that can be used to sort
 * strings fast. This is used when a string would be compared
 * multiple times.
 *
 * @param[in]	s			The string to compare.
 * @param[in]	nCap		Base the key on the first nCap characters of s (if larger than 0).
 * @param[in]	locale		The locale used to create the sort key.
 * @param[out]	lpcbKeys	The size in bytes of the returned key.
 * @param[ou]t	lppKey		The returned key.
 */
void createSortKeyDataFromUTF8(const char *s, int nCap, const ECLocale &locale, unsigned int *lpcbKey, unsigned char **lppKey)
{
	ASSERT(s != NULL);
	ASSERT(lpcbKey != NULL);
	ASSERT(lppKey != NULL);

#ifdef ZCP_USES_ICU
	createSortKeyData(UTF8ToUnicode(s), nCap, locale, lpcbKey, lppKey);
#else
	std::wstring wstrTmp = convert_to<std::wstring>(s, rawsize(s), "UTF-8");
	createSortKeyData(wstrTmp.c_str(), nCap, locale, lpcbKey, lppKey);
#endif
}


/**
 * Create a locale independant blob that can be used to sort
 * strings fast. This is used when a string would be compared
 * multiple times.
 *
 * @param[in]	s			The string to compare.
 * @param[in]	nCap		Base the key on the first nCap characters of s (if larger than 0).
 * @param[in]	locale		The locale used to create the sort key.
 *
 * @returns		The ECSortKey containing the blob.
 */
ECSortKey createSortKeyFromUTF8(const char *s, int nCap, const ECLocale &locale)
{
	ASSERT(s != NULL);

#ifdef ZCP_USES_ICU
	return createSortKey(UTF8ToUnicode(s), nCap, locale);
#else
	unsigned int cbKey = 0;
	unsigned char *lpKey = NULL;

	createSortKeyDataFromUTF8(s, nCap, locale, &cbKey, &lpKey);
	return ECSortKey(lpKey, cbKey);
#endif
}

/**
 * Compare two sort keys previously created with createSortKey.
 * 
 * @param[in]	cbKey1		The size i nbytes of key 1.
 * @param[in]	lpKey1		Key 1.
 * @param[in]	cbKey2		The size i nbytes of key 2.
 * @param[in]	lpKey2		Key 2.
 *
 * @retval	<0	Key1 is smaller than key2
 * @retval	0	Key1 equals key2
 * @retval	>0	Key1 is greater than key2
 */
int compareSortKeys(unsigned int cbKey1, const unsigned char *lpKey1, unsigned int cbKey2, const unsigned char *lpKey2)
{
	ASSERT(!(cbKey1 != 0 && lpKey1 == NULL));
	ASSERT(!(cbKey2 != 0 && lpKey2 == NULL));

#ifdef ZCP_USES_ICU
	CollationKey ckA(lpKey1, cbKey1);
	CollationKey ckB(lpKey2, cbKey2);

	int cmp = 1;
	UErrorCode status = U_ZERO_ERROR;
	switch (ckA.compareTo(ckB, status)) {
		case UCOL_LESS:		cmp = -1; break;
		case UCOL_EQUAL:	cmp =  0; break;
		case UCOL_GREATER:	cmp =  1; break;
	}
	return cmp;
#else
	if (cbKey1 == 0)
		return (cbKey2 == 0 ? 0 : -1);
	else if (cbKey2 == 0)
		return 1;

	ASSERT(wcslen((wchar_t*)lpKey1) == (cbKey1 / sizeof(wchar_t)) - 1);
	ASSERT(wcslen((wchar_t*)lpKey2) == (cbKey2 / sizeof(wchar_t)) - 1);

	return wcscmp((wchar_t*)lpKey1, (wchar_t*)lpKey2);	
#endif
}

#ifndef ZCP_USES_ICU
ECLocale::ECLocale()
: m_locale(createlocale(LC_ALL, ""))
, m_category(LC_ALL)
{
	ASSERT(m_locale != NULL);
}

ECLocale::ECLocale(int category, const char *locale)
: m_locale(createlocale_real(category, locale))
, m_category(category)
, m_localeid(locale)
{
	if (!m_locale) {
		/* Three things might have happened here:
		 * 1. The passed locale makes no sence or is not installed on the system
		 * 2. No charset was specified, and the default charset for a locale is
		 *    not installed while the utf-8 charset is.
		 * 3. A specific charset is specified, which is not installed while the
		 *    utf-8 charset is.
		 *
		 * 2 and 3 seem to be Debian (and derivate) issues.
		 * We'll ignore option 1 and see if we can get the locale with the charset
		 * forced to utf-8.
		 */
		std::string::size_type idx = m_localeid.find('.');
		if (idx == std::string::npos || m_localeid.compare(idx + 1, strlen("utf-8"), "utf-8") != 0) {
			if (idx != std::string::npos)
				m_localeid.resize(idx);
			m_localeid.append(".utf-8");
			m_locale = createlocale_real(category, m_localeid.c_str());
		}

		if (!m_locale) {
			// Apparently option 1 happened. Go with the default locale.
			m_localeid.clear();
			m_locale = createlocale_real(category, m_localeid.c_str());
		}
	}
	ASSERT(m_locale != NULL);
}

ECLocale::ECLocale(const ECLocale &other)
: m_locale(createlocale_real(other.m_category, other.m_localeid.c_str()))
, m_category(other.m_category)
, m_localeid(other.m_localeid)
{
	ASSERT(m_locale != NULL);
}

ECLocale::~ECLocale() {
	if (m_locale)
		freelocale(m_locale);
}

ECLocale &ECLocale::operator=(const ECLocale &other) {
	if (this != &other) {
		ECLocale tmp(other);
		swap(tmp);
	}
	return *this;
}

void ECLocale::swap(ECLocale &other) {
	std::swap(m_locale, other.m_locale);
	std::swap(m_category, other.m_category);
	std::swap(m_localeid, other.m_localeid);
}
#endif

/** @} */
