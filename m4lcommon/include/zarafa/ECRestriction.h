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

#ifndef ECRestrictionBuilder_INCLUDED
#define ECRestrictionBuilder_INCLUDED

#include <zarafa/zcdefs.h>
#include <mapidefs.h>

#include <list>

#include <boost/smart_ptr.hpp>

class ECRestrictionList;

/**
 * Base class for all other ECxxxRestriction classes.
 * It defines the interface needed to hook the various restrictions together.
 */
class ECRestriction {
public:
	enum {
		Cheap	= 1,	// Stores the passes LPSPropValue pointer.
		Shallow = 2		// Creates a new SPropValue, but point to the embedded data from the original structure.
	};

	virtual ~ECRestriction() {}

	/**
	 * Create an LPSRestiction object that represents the restriction on which CreateMAPIRestriction was called.
	 * @param[in]	lppRestriction	Pointer to an LPSRestriction pointer, that will be set to the address
	 * 								of the newly allocated restriction. This memory must be freed with
	 * 								MAPIFreeBuffer.
	 * @param[in]	ulFlags			When set to ECRestriction::Cheap, not all data will be copied to the 
	 * 								new MAPI restriction. This is useful if the ECRestriction will outlive
	 * 								the MAPI restriction.
	 */
	HRESULT CreateMAPIRestriction(LPSRestriction *lppRestriction, ULONG ulFlags = 0) const;
	
	/**
	 * Apply the restriction on a table.
	 * @param[in]	lpTable		The table on which to apply the restriction.
	 */
	HRESULT RestrictTable(LPMAPITABLE lpTable) const;

	/**
	 * Use the restriction to perform a FindRow on the passed table.
	 * @param[in]	lpTable		The table on which to perform the FindRow.
	 * @param[in]	BkOrigin	The location to start searching from. Directly passed to FindRow.
	 * @param[in]	ulFlags		Flags controlling search behaviour. Directly passed to FindRow.
	 */
	HRESULT FindRowIn(LPMAPITABLE lpTable, BOOKMARK BkOrigin, ULONG ulFlags) const;

	/**
	 * Populate an SRestriction structure based on the objects state.
	 * @param[in]		lpBase			Base pointer used for allocating additional memory.
	 * @param[in,out]	lpRestriction	Pointer to the SRestriction object that is to be populated.
	 */
	virtual HRESULT GetMAPIRestriction(LPVOID lpBase, LPSRestriction lpRestriction, ULONG ulFlags = 0) const = 0;
	
	/**
	 * Create a new ECRestriction derived class of the same type of the object on which this
	 * method is invoked.
	 * @return	A copy of the current object.
	 */
	virtual ECRestriction *Clone() const = 0;

	ECRestrictionList operator+ (const ECRestriction &other) const;

protected:
	typedef boost::shared_ptr<SPropValue>		PropPtr;
	typedef boost::shared_ptr<ECRestriction>	ResPtr;
	typedef std::list<ResPtr>					ResList;

	ECRestriction() { }
	HRESULT CopyProp(LPSPropValue lpPropSrc, LPVOID lpBase, ULONG ulFLags, LPSPropValue *lppPropDst) const;
	HRESULT CopyPropArray(ULONG cValues, LPSPropValue lpPropSrc, LPVOID lpBase, ULONG ulFLags, LPSPropValue *lppPropDst) const;

	static void DummyFree(LPVOID);
};

/**
 * An ECRestrictionList is a list of ECRestriction objects.
 * This class is used to allow a list of restrictions to be passed in the
 * constructors of the ECAndRestriction and the ECOrRestriction classes.
 * It's implicitly created by +-ing multiple ECRestriction objects.
 */
class ECRestrictionList _zcp_final {
public:
	ECRestrictionList(const ECRestriction &res1, const ECRestriction &res2) {
		m_list.push_back(ResPtr(res1.Clone()));
		m_list.push_back(ResPtr(res2.Clone()));
	}
	
	ECRestrictionList& operator+(const ECRestriction &restriction) {
		m_list.push_back(ResPtr(restriction.Clone()));
		return *this;
	}

private:
	typedef boost::shared_ptr<ECRestriction>	ResPtr;
	typedef std::list<ResPtr>					ResList;

	ResList	m_list;

friend class ECAndRestriction;
friend class ECOrRestriction;
};

/**
 * Add two restrictions together and create an ECRestrictionList.
 * @param[in]	restriction		The restriction to add with the current.
 * @return		The ECRestrictionList with the two entries.
 */
inline ECRestrictionList ECRestriction::operator+ (const ECRestriction &other) const {
	return ECRestrictionList(*this, other);
}

class ECAndRestriction _zcp_final : public ECRestriction {
public:
	ECAndRestriction() { }
	ECAndRestriction(const ECRestrictionList &list);
	
	ECAndRestriction(const ECRestriction &restriction)
	: m_lstRestrictions(1, ResPtr(restriction.Clone())) 
	{ }

	HRESULT GetMAPIRestriction(LPVOID lpBase, LPSRestriction lpRestriction, ULONG ulFlags) const _zcp_override;
	ECRestriction *Clone() const _zcp_override;

	HRESULT append(const ECRestriction &restriction) {
		m_lstRestrictions.push_back(ResPtr(restriction.Clone()));
		return hrSuccess;
	}

	HRESULT append(const ECRestrictionList &list);

private:
	ResList	m_lstRestrictions;
};

class ECOrRestriction _zcp_final : public ECRestriction {
public:
	ECOrRestriction() { }
	ECOrRestriction(const ECRestrictionList &list);
	
	ECOrRestriction(const ECRestriction &restriction)
	: m_lstRestrictions(1, ResPtr(restriction.Clone())) 
	{ }
	
	HRESULT GetMAPIRestriction(LPVOID lpBase, LPSRestriction lpRestriction, ULONG ulFlags) const _zcp_override;
	ECRestriction *Clone() const _zcp_override;

	HRESULT append(const ECRestriction &restriction) {
		m_lstRestrictions.push_back(ResPtr(restriction.Clone()));
		return hrSuccess;
	}

	HRESULT append(const ECRestrictionList &list);

private:
	ResList	m_lstRestrictions;
};

class ECNotRestriction _zcp_final : public ECRestriction {
public:
	ECNotRestriction(const ECRestriction &restriction)
	: m_ptrRestriction(ResPtr(restriction.Clone())) 
	{ }

	HRESULT GetMAPIRestriction(LPVOID lpBase, LPSRestriction lpRestriction, ULONG ulFlags) const _zcp_override;
	ECRestriction *Clone() const _zcp_override;

private:
	ECNotRestriction(ResPtr ptrRestriction);

private:
	ResPtr	m_ptrRestriction;
};

class ECContentRestriction _zcp_final : public ECRestriction {
public:
	ECContentRestriction(ULONG ulFuzzyLevel, ULONG ulPropTag, LPSPropValue lpProp, ULONG ulFlags = 0);

	HRESULT GetMAPIRestriction(LPVOID lpBase, LPSRestriction lpRestriction, ULONG ulFlags) const _zcp_override;
	ECRestriction *Clone() const _zcp_override;

private:
	ECContentRestriction(ULONG ulFuzzyLevel, ULONG ulPropTag, PropPtr ptrProp);

private:
	ULONG	m_ulFuzzyLevel;
	ULONG	m_ulPropTag;
	PropPtr	m_ptrProp;
};

class ECBitMaskRestriction _zcp_final : public ECRestriction {
public:
	ECBitMaskRestriction(ULONG relBMR, ULONG ulPropTag, ULONG ulMask)
	: m_relBMR(relBMR)
	, m_ulPropTag(ulPropTag)
	, m_ulMask(ulMask) 
	{ }

	HRESULT GetMAPIRestriction(LPVOID lpBase, LPSRestriction lpRestriction, ULONG ulFlags) const _zcp_override;
	ECRestriction *Clone() const _zcp_override;

private:
	ULONG	m_relBMR;
	ULONG	m_ulPropTag;
	ULONG	m_ulMask;
};

class ECPropertyRestriction _zcp_final : public ECRestriction {
public:
	ECPropertyRestriction(ULONG relop, ULONG ulPropTag, LPSPropValue lpProp, ULONG ulFlags = 0);

	HRESULT GetMAPIRestriction(LPVOID lpBase, LPSRestriction lpRestriction, ULONG ulFlags) const _zcp_override;
	ECRestriction *Clone() const _zcp_override;

private:
	ECPropertyRestriction(ULONG relop, ULONG ulPropTag, PropPtr ptrProp);

private:
	ULONG	m_relop;
	ULONG	m_ulPropTag;
	PropPtr	m_ptrProp;
};

class ECComparePropsRestriction _zcp_final : public ECRestriction {
public:
	ECComparePropsRestriction(ULONG relop, ULONG ulPropTag1, ULONG ulPropTag2)
	: m_relop(relop)
	, m_ulPropTag1(ulPropTag1)
	, m_ulPropTag2(ulPropTag2)
	{ }

	HRESULT GetMAPIRestriction(LPVOID lpBase, LPSRestriction lpRestriction, ULONG ulFlags) const _zcp_override;
	ECRestriction *Clone() const _zcp_override;

private:
	ULONG	m_relop;
	ULONG	m_ulPropTag1;
	ULONG	m_ulPropTag2;
};

class ECSizeRestriction _zcp_final : public ECRestriction {
public:
	ECSizeRestriction(ULONG relop, ULONG ulPropTag, ULONG cb)
	: m_relop(relop)
	, m_ulPropTag(ulPropTag)
	, m_cb(cb)
	{ }

	HRESULT GetMAPIRestriction(LPVOID lpBase, LPSRestriction lpRestriction, ULONG ulFlags) const _zcp_override;
	ECRestriction *Clone() const _zcp_override;

private:
	ULONG	m_relop;
	ULONG	m_ulPropTag;
	ULONG	m_cb;
};

class ECExistRestriction _zcp_final : public ECRestriction {
public:
	ECExistRestriction(ULONG ulPropTag)
	: m_ulPropTag(ulPropTag) 
	{ }

	HRESULT GetMAPIRestriction(LPVOID lpBase, LPSRestriction lpRestriction, ULONG ulFlags) const _zcp_override;
	ECRestriction *Clone() const _zcp_override;

private:
	ULONG	m_ulPropTag;
};

class ECSubRestriction _zcp_final : public ECRestriction {
public:
	ECSubRestriction(ULONG ulSubObject, const ECRestriction &restriction)
	: m_ulSubObject(ulSubObject)
	, m_ptrRestriction(ResPtr(restriction.Clone()))
	{ }

	HRESULT GetMAPIRestriction(LPVOID lpBase, LPSRestriction lpRestriction, ULONG ulFlags) const _zcp_override;
	ECRestriction *Clone() const _zcp_override;

private:
	ECSubRestriction(ULONG ulSubObject, ResPtr ptrRestriction);

private:
	ULONG	m_ulSubObject;
	ResPtr	m_ptrRestriction;
};

class ECCommentRestriction _zcp_final : public ECRestriction {
public:
	ECCommentRestriction(const ECRestriction &restriction, ULONG cValues, LPSPropValue lpProp, ULONG ulFlags = 0);

	HRESULT GetMAPIRestriction(LPVOID lpBase, LPSRestriction lpRestriction, ULONG ulFlags) const _zcp_override;
	ECRestriction *Clone() const _zcp_override;

private:
	ECCommentRestriction(ResPtr ptrRestriction, ULONG cValues, PropPtr ptrProp);

private:
	ResPtr	m_ptrRestriction;
	ULONG	m_cValues;
	PropPtr	m_ptrProp;
};

/**
 * This is a special class, which encapsulates a raw SRestriction structure to allow
 * prebuild or obtained restriction structures to be used in the ECRestriction model.
 */
class ECRawRestriction _zcp_final : public ECRestriction {
public:
	ECRawRestriction(LPSRestriction lpRestriction, ULONG ulFlags = 0);

	HRESULT GetMAPIRestriction(LPVOID lpBase, LPSRestriction lpRestriction, ULONG ulFlags) const _zcp_override;
	ECRestriction *Clone() const _zcp_override;

private:
	typedef boost::shared_ptr<SRestriction>	RawResPtr;

	ECRawRestriction(RawResPtr ptrRestriction);

private:
	RawResPtr	m_ptrRestriction;
};

#endif // ndef ECRestrictionBuilder_INCLUDED
