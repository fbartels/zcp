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

#include <zarafa/platform.h>
#include <cassert>
#include <zarafa/ECKeyTable.h> 
#include <zarafa/ustringutil.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

bool operator!=(const sObjectTableKey& a, const sObjectTableKey& b)
{
	return !(a.ulObjId==b.ulObjId && a.ulOrderId == b.ulOrderId);
}

bool operator==(const sObjectTableKey& a, const sObjectTableKey& b)
{
	return (a.ulObjId==b.ulObjId && a.ulOrderId == b.ulOrderId);
}

bool operator<(const sObjectTableKey& a, const sObjectTableKey& b)
{

	return a.ulObjId < b.ulObjId || (a.ulObjId==b.ulObjId && a.ulOrderId < b.ulOrderId);
}

bool operator>(const sObjectTableKey& a, const sObjectTableKey& b)
{
	return a.ulObjId > b.ulObjId || (a.ulObjId==b.ulObjId && a.ulOrderId > b.ulOrderId);
}

/*
 * Balanced binary tree node system
 *
 * To make the row system as fast as possible, we use a balanced binary tree algorithm. Main speed interests are:
 *
 * - Fast insertion
 * - Fast deletion by row ID
 * - Fast row update by row ID
 * - Fast seek by row number
 * - Fast current row number retrieval
 * 
 * The insertion and row number functions can be directly related to the balanced b-tree. This makes sure that
 * inserting data only requires updating parent nodes and is therefore a O(log n) operation. Deletion also needs
 * to update parent nodes, and is therefore also a O(log n) operation. 
 *
 * For the row ID retrieval, a simple hash map is used for almost constant-time retrieval of nodes. This makes row
 * update extremely fast, while getting the current row number is also O(log n)
 *
 * The b-tree is built up basically like a binary search tree, with the ordering being
 *
 *   2
 *  / \
 * 1   3
 *
 * .. in each node. This means that the natural order for the tree is the sorted order.
 *
 */

/*
 * A table row in the KeyTable contains only the row ID, used for the PR_INSTANCE_KEY in 
 * the client table, and any columns that are required for sorting. This class does *not*
 * communicate with the database and is a standalone KeyTable implementation. Any other data
 * must be collected by the caller.
 *
 * Rows are immutable, changes are done with a delete/add
 */

ECTableRow::ECTableRow(sObjectTableKey sKey, unsigned int ulSortCols,
    const unsigned int *lpSortLen, const unsigned char *lpFlags,
    unsigned char **lppSortData, bool fHidden)
{
	this->sKey = sKey;
	this->lpParent = NULL;
	this->lpLeft = NULL;
	this->lpRight = NULL;
	this->fLeft = 0;
	this->ulBranchCount = 0;
	this->fRoot = false;
	this->ulHeight = 0;
	this->fHidden = fHidden;

	initSortCols(ulSortCols, reinterpret_cast<const int *>(lpSortLen),
	             lpFlags, lppSortData);
}

void ECTableRow::initSortCols(unsigned int ulSortCols, const int *lpSortLen,
    const unsigned char *lpFlags, unsigned char **lppSortData)
{
	unsigned int i=0;
	int len = 0;

	this->ulSortCols = ulSortCols;
	
	if(lpFlags) {
		this->lpFlags = new unsigned char [ ulSortCols ];
		memcpy(this->lpFlags, lpFlags, ulSortCols * sizeof(this->lpFlags[0]));
	} else {
		this->lpFlags = NULL;
	}
	this->lpSortLen = new int [ulSortCols];
	this->lppSortKeys = new unsigned char *[ulSortCols];

	// Copy sort lengths
	assert(ulSortCols == 0 || lpSortLen != NULL);
	if (ulSortCols != 0)
		memcpy(this->lpSortLen, lpSortLen, sizeof(unsigned int) * ulSortCols);

	// Copy sort keys
	for (i = 0; i < ulSortCols; ++i) {
		len = lpSortLen[i];
		len = len < 0 ? -len : len;

		this->lppSortKeys[i] = new unsigned char[len];

		memcpy(this->lppSortKeys[i], lppSortData[i], len);
	}
}

ECTableRow::ECTableRow(const ECTableRow &other)
{
	this->sKey = other.sKey;
	this->lpParent = NULL;
	this->lpLeft = NULL;
	this->lpRight = NULL;
	this->fLeft = 0;
	this->ulBranchCount = 0;
	this->fRoot = false;
	this->ulHeight = 0;
	this->fHidden = other.fHidden;

	initSortCols(other.ulSortCols, other.lpSortLen, other.lpFlags, other.lppSortKeys);
}

ECTableRow& ECTableRow::operator=(const ECTableRow &other)
{
    if(this == &other)
        return *this;
        
    freeSortCols();
    initSortCols(other.ulSortCols, other.lpSortLen, other.lpFlags, other.lppSortKeys);
    
    return *this;
}

void ECTableRow::freeSortCols()
{
	unsigned int i=0;

	delete[] lpSortLen;
	if(lppSortKeys)
	{
		for (i = 0; i < ulSortCols; ++i)
			delete [] lppSortKeys[i];

		delete [] lppSortKeys;
	}
	delete[] lpFlags;
}

ECTableRow::~ECTableRow()
{
    freeSortCols();
}

/*
 * The sortkeys are stored as binary data, the caller must ensure that the ordering of this
 * binary data is the same as the ordering of the underlying fields; variable-size columns
 * are sorted like strings; ie "AAbb" comes before "AAbbcc". If two fields match exactly, the
 * next column is used for sorting.
 *
 * Columns that must be sorted in descending order have a NEGATIVE lpSortLen[] entry. We split
 * this into the ascending and descending cases below.
 */
bool ECTableRow::rowcompare(const ECTableRow *a, const ECTableRow *b)
{
	// The root node is before anything!
	if(a->fRoot && !b->fRoot)
		return true;
	if(b->fRoot)
		return false;
		
    return rowcompare(a->ulSortCols, a->lpSortLen, a->lppSortKeys, a->lpFlags, b->ulSortCols, b->lpSortLen, b->lppSortKeys, b->lpFlags);
}

// Does a normal row compare between two rows
bool ECTableRow::rowcompare(unsigned int ulSortColsA, const int *lpSortLenA,
    unsigned char **lppSortKeysA, const unsigned char *lpSortFlagsA,
    unsigned int ulSortColsB, const int *lpSortLenB,
    unsigned char **lppSortKeysB, const unsigned char *lpSortFlagsB,
    bool fIgnoreOrder)
{
	unsigned int i=0;
	bool ret = false;
	unsigned int ulSortCols;
	
	ulSortCols = ulSortColsA < ulSortColsB ? ulSortColsA : ulSortColsB;

	for (i = 0; i < ulSortCols; ++i) {
	    int cmp = 0;
	    
	    if(lpSortFlagsA && lpSortFlagsA[i] & TABLEROW_FLAG_FLOAT) {
	        if(lpSortLenA[i] != sizeof(double) || lpSortLenB[i] != sizeof(double)) {
	            cmp = 0;
            } else {
                // only good way to cast from unsigned char * -> double is via a pointer
    	        double *ad, *bd;
	            ad = (double *)lppSortKeysA[i];
	            bd = (double *)lppSortKeysB[i];
	            
	            if(*ad == *bd)
	                cmp = 0;
                else if(*ad < *bd)
                    cmp = -1;
                else cmp = 1;
            }
	    } else if (lpSortFlagsA && lpSortFlagsA[i] & TABLEROW_FLAG_STRING) {
			cmp = compareSortKeys(lpSortLenA[i], lppSortKeysA[i], lpSortLenB[i], lppSortKeysB[i]);
	    } else {
	        // Sort data is pre-constructed so a simple memcmp suffices for sorting
		    cmp = memcmp(lppSortKeysA[i], lppSortKeysB[i], lpSortLenA[i] < lpSortLenB[i] ? lpSortLenA[i] : lpSortLenB[i]);
        }
        
		if(cmp < 0) {
			ret = true;
			break;
		} else if (cmp == 0) {
			if(lpSortLenA[i] == lpSortLenB[i])
				continue;
			ret = lpSortLenA[i] < lpSortLenB[i];
			break;
		} else {
			ret = false;
			break;
		}
	}
 
	if(i == ulSortCols) {
	    if(ulSortColsA == ulSortColsB) {
    		// equal, always return false independent of asc/desc
	    	return false;
        }
        else {
            // unequal number of sort columns, the item with the least sort columns comes first, independent of asc/desc
            return ulSortColsA < ulSortColsB;
        }
	} else {
	    // Unequal, flip order if desc
		if(!fIgnoreOrder && lpSortFlagsA && (lpSortFlagsA[i] & TABLEROW_FLAG_DESC))
			return !ret;
		else
			return ret;
	}
}

// Compares a row by looking only at a certain prefix of sort columns
bool ECTableRow::rowcompareprefix(unsigned int ulPrefix,
    unsigned int ulSortColsA, const int *lpSortLenA,
    unsigned char **lppSortKeysA, const unsigned char *lpSortFlagsA,
    unsigned int ulSortColsB, const int *lpSortLenB,
    unsigned char **lppSortKeysB, const unsigned char *lpSortFlagsB)
{
    return ECTableRow::rowcompare(ulSortColsA > ulPrefix ? ulPrefix : ulSortColsA, lpSortLenA, lppSortKeysA, lpSortFlagsA,
                                  ulSortColsB > ulPrefix ? ulPrefix : ulSortColsB, lpSortLenB, lppSortKeysB, lpSortFlagsB);
}

bool ECTableRow::operator <(const ECTableRow &other) const
{
    return ECTableRow::rowcompare(ulSortCols, lpSortLen, lppSortKeys, lpFlags,
                                  other.ulSortCols, other.lpSortLen, other.lppSortKeys, other.lpFlags, true);
}

/**
 * Get object size
 *
 * @return Object size in bytes
 */
unsigned int ECTableRow::GetObjectSize(void) const
{
	unsigned int ulSize = sizeof(*this);

	if (ulSortCols > 0)
	{
		ulSize+= (sizeof(unsigned char) + sizeof(unsigned char) + sizeof(unsigned int)) * ulSortCols; // flag, SortKey, Sortlen
		for (unsigned int i = 0; i < ulSortCols; ++i)
			ulSize += lpSortLen[i];
	}

	return ulSize;
}

ECKeyTable::ECKeyTable()
{
	sObjectTableKey sKey;
	memset(&sKey, 0, sizeof(sObjectTableKey));

	this->lpRoot = new ECTableRow(sKey, 0, NULL, NULL, NULL, false);
	this->lpRoot->fRoot = true;
	this->lpCurrent = lpRoot;

	// The start of bookmark, the first 3 (0,1,2) are default
	m_ulBookmarkPosition = 3;


	// g++ doesn't like static initializers on existing variables
	pthread_mutexattr_t mattr;
	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mLock, &mattr);
}

ECKeyTable::~ECKeyTable()
{
	Clear();
	delete lpRoot;
	pthread_mutex_destroy(&mLock);
}

// Propagate this node's counts up to the root
ECRESULT ECKeyTable::UpdateCounts(ECTableRow *lpRow)
{
	ECRESULT er = erSuccess;
	unsigned int ulHeight = 0;

	while(lpRow != NULL) {
		if(lpRow == lpRoot) {
			lpRow->ulHeight = 0;
			lpRow->ulBranchCount = 0; // makes sure that lpRoot->ulBranchCount is the total size 
		} else if(lpRow->fHidden) {
		    lpRow->ulHeight = 1;		// Hidden rows count as 'height' for AVL ..
		    lpRow->ulBranchCount = 0;   // .. but not as a 'row' for rowcount
		} else {
			lpRow->ulHeight = 1;
			lpRow->ulBranchCount = 1;
		}

		if(lpRow->lpLeft)
			lpRow->ulBranchCount += lpRow->lpLeft->ulBranchCount;
		if(lpRow->lpRight)
			lpRow->ulBranchCount += lpRow->lpRight->ulBranchCount;

		// Get the height of the highest subtree
		ulHeight = 0;

		if(lpRow->lpLeft)
			ulHeight = lpRow->lpLeft->ulHeight;
		if(lpRow->lpRight)
			ulHeight = lpRow->lpRight->ulHeight > ulHeight ? lpRow->lpRight->ulHeight : ulHeight;

		// Our height is highest subtree plus one
		lpRow->ulHeight += ulHeight;

		lpRow = lpRow->lpParent;

	}

	return er;	
}

ECRESULT ECKeyTable::UpdateRow(UpdateType ulType,
    const sObjectTableKey *lpsRowItem, unsigned int ulSortCols,
    const unsigned int *lpSortLen, const unsigned char *lpFlags,
    unsigned char **lppSortData, sObjectTableKey *lpsPrevRow, bool fHidden,
    UpdateType *lpulAction)
{
	ECRESULT er = erSuccess;
	ECTableRow *lpRow = NULL;
	ECTableRowMap::const_iterator iterMap;
	ECTableRow *lpNewRow = NULL;
	unsigned int fLeft = 0;
	bool fRelocateCursor = false;

	pthread_mutex_lock(&mLock);

	switch(ulType) {
	case TABLE_ROW_DELETE:
		// Find the row by ID
		iterMap = mapRow.find(*lpsRowItem);

		if(iterMap != mapRow.end()) {
			// The row exists
			lpRow = iterMap->second;

			// Do the delete
			if(lpRow->lpLeft == NULL && lpRow->lpRight == NULL) {
				// This is a leaf node, just delete the leaf
				if(lpRow->fLeft)
					lpRow->lpParent->lpLeft = NULL;
				else
					lpRow->lpParent->lpRight = NULL;

				UpdateCounts(lpRow->lpParent);
				RestructureRecursive(lpRow->lpParent);

			} else if(lpRow->lpLeft != NULL && lpRow->lpRight == NULL) {
				// We have a left branch, put that branch in place of this node
				if(lpRow->fLeft)
					lpRow->lpParent->lpLeft = lpRow->lpLeft;
				else
					lpRow->lpParent->lpRight = lpRow->lpLeft;

				lpRow->lpLeft->lpParent = lpRow->lpParent;
				lpRow->lpLeft->fLeft = lpRow->fLeft;

				UpdateCounts(lpRow->lpParent);
				RestructureRecursive(lpRow->lpParent);

			} else if(lpRow->lpRight != NULL && lpRow->lpLeft == NULL) {
				// We have a right branch, put that branch in place of this node
				if(lpRow->fLeft)
					lpRow->lpParent->lpLeft = lpRow->lpRight;
				else
					lpRow->lpParent->lpRight = lpRow->lpRight;

				lpRow->lpRight->lpParent = lpRow->lpParent;
				lpRow->lpRight->fLeft = lpRow->fLeft;

				UpdateCounts(lpRow->lpParent);
				RestructureRecursive(lpRow->lpParent);
			} else {
				// We have two child nodes ..

				ECTableRow *lpPredecessor = lpRow->lpLeft;
				ECTableRow *lpPredecessorParent = NULL;

				while(lpPredecessor->lpRight)
					lpPredecessor = lpPredecessor->lpRight;

				// Remove the predecessor from the tree, optionally moving the left tree into place
				if(lpPredecessor->fLeft)
					lpPredecessor->lpParent->lpLeft = lpPredecessor->lpLeft;
				else
					lpPredecessor->lpParent->lpRight = lpPredecessor->lpLeft;

				if(lpPredecessor->lpLeft) {
					lpPredecessor->lpLeft->lpParent = lpPredecessor->lpParent;
					lpPredecessor->lpLeft->fLeft = lpPredecessor->fLeft;
				}

				// Remember the predecessor parent for later use
				lpPredecessorParent = lpPredecessor->lpParent;

				// Replace the row to be deleted with the predecessor
				if(lpRow->fLeft)
					lpRow->lpParent->lpLeft = lpPredecessor;
				else
					lpRow->lpParent->lpRight = lpPredecessor;

				lpPredecessor->lpParent = lpRow->lpParent;
				lpPredecessor->fLeft = lpRow->fLeft;
				lpPredecessor->lpLeft = lpRow->lpLeft;
				lpPredecessor->lpRight = lpRow->lpRight;

				if(lpPredecessor->lpLeft)
					lpPredecessor->lpLeft->lpParent = lpPredecessor;
				if(lpPredecessor->lpRight)
					lpPredecessor->lpRight->lpParent = lpPredecessor;

				// Do a recursive update of the counts in the branch of the predecessorparent, (where we removed the predecessor)
				UpdateCounts(lpPredecessorParent);
				// Do a recursive update of the counts in the branch we moved to predecessor to
				UpdateCounts(lpPredecessor);

				// Restructure 
				RestructureRecursive(lpPredecessor);
				if(lpPredecessorParent->sKey != *lpsRowItem)
					RestructureRecursive(lpPredecessorParent);
			}

			// Move cursor to next node (or previous if this was the last row)
			if(lpCurrent == lpRow) {
				this->SeekRow(EC_SEEK_CUR, -1, NULL);
				this->SeekRow(EC_SEEK_CUR, 1, NULL);
			}

			// Delete this uncoupled node
			InvalidateBookmark(lpRow); //ignore errors
			delete lpRow;

			// Remove the row from the id map
			mapRow.erase(*lpsRowItem);

			if(lpulAction)
				*lpulAction = TABLE_ROW_DELETE;
		} else {
			er = ZARAFA_E_NOT_FOUND;
			goto exit;
		}
		break;

	case TABLE_ROW_MODIFY:
	case TABLE_ROW_ADD:
		lpRow = lpRoot;

		// Find the row by id (see if we already have the row)
		iterMap = mapRow.find(*lpsRowItem);
	
		if(iterMap != mapRow.end()) {
			// Found the row

			// Indiciate that we are modifying an existing row
			if(lpulAction)
				*lpulAction = TABLE_ROW_MODIFY;

			// Create a new node
			lpNewRow = new ECTableRow(*lpsRowItem, ulSortCols, lpSortLen, lpFlags, lppSortData, fHidden);

			if(iterMap->second == lpCurrent) {
			    fRelocateCursor = true;
			}

			// If the exact same row is already in here, just look up the predecessor
			if( !ECTableRow::rowcompare(iterMap->second, lpNewRow) &&
				!ECTableRow::rowcompare(lpNewRow, iterMap->second)) {

				// Find the predecessor
				if(lpsPrevRow) {
					ECTableRow *lpPredecessor = NULL;

					if(iterMap->second->lpLeft) {
						lpPredecessor = iterMap->second->lpLeft;

						while(lpPredecessor && lpPredecessor->lpRight)
							lpPredecessor = lpPredecessor->lpRight;

					} else {
						lpPredecessor = iterMap->second;

						while(lpPredecessor && lpPredecessor->fLeft)
							lpPredecessor = lpPredecessor->lpParent;

						if(lpPredecessor && lpPredecessor->lpParent)
							lpPredecessor = lpPredecessor->lpParent;
						else
							lpPredecessor = NULL;
					}

					if(lpPredecessor) {
						*lpsPrevRow = lpPredecessor->sKey;
					} else {
						lpsPrevRow->ulObjId		= 0;
						lpsPrevRow->ulOrderId	= 0;
					}
				}
				
				// Delete the unused new node
				delete lpNewRow;
				goto exit;
			} else {
				// new row data is different, so delete the old row now
				er = UpdateRow(TABLE_ROW_DELETE, lpsRowItem, 0, NULL, NULL, NULL, NULL);

				if(er != erSuccess){
					// Delete the unused new node
					if(lpNewRow) delete lpNewRow;

					goto exit;
				}
			}

		} else {
			// Indicate that we are adding a new row
			if(lpulAction)
				*lpulAction = TABLE_ROW_ADD;
		}

		// Create the row that we will be inserting
		if(lpNewRow == NULL)
			lpNewRow = new ECTableRow(*lpsRowItem, ulSortCols, lpSortLen, lpFlags, lppSortData, fHidden);

		// Do a binary search in the tree
		while(1) {
			if(ECTableRow::rowcompare(lpNewRow, lpRow)) {
				if(lpRow->lpLeft == NULL) {
					fLeft = 1;
					break;
				}
				else
					lpRow = lpRow->lpLeft;
			} else {
				if(lpRow->lpRight == NULL) {
					fLeft = 0;
					break;
				}
				else
					lpRow = lpRow->lpRight;
			}
		}
			
		// lpRow now points to our parent, fLeft is whether we're the new left or right node of this parent

		// Get the predecesor ID
		if(lpsPrevRow) {
			if(fLeft) {
				// Our predecessor is the parent of our rightmost parent
				ECTableRow *lpPredecessor = lpRow;

				while(lpPredecessor && lpPredecessor->fLeft)
					lpPredecessor = lpPredecessor->lpParent;

				if(lpPredecessor == NULL || lpPredecessor->lpParent == NULL) {
					lpsPrevRow->ulObjId = 0;
					lpsPrevRow->ulOrderId = 0;
				} else
					*lpsPrevRow = lpPredecessor->lpParent->sKey;			
			} else {
				// Our predecessor is simply our parent
				*lpsPrevRow = lpRow->sKey;
			}
		}

		// Link the row into the table
		if(fLeft)
			lpRow->lpLeft = lpNewRow;
		else 
			lpRow->lpRight = lpNewRow;

		lpNewRow->lpParent = lpRow;
		lpNewRow->fLeft = fLeft;

		// Add it in the id map
		mapRow[*lpsRowItem] = lpNewRow;

		// Do a recursive update of the counts in the branch
		UpdateCounts(lpNewRow);
		RestructureRecursive(lpNewRow);
		
		// Reposition the cursor if it used to be on the old row
		if(fRelocateCursor)
		    lpCurrent = lpNewRow;

		break;

	// no action for: TABLE_CHANGE, TABLE_ERR, TABLE_SORT, TABLE_RESTRICT, TABLE_SETCOL, TABLE_DO_RELOAD
	default:
		break;
	}

exit:
	pthread_mutex_unlock(&mLock);

	return er;
}

/* 
 * Clears the KeyTable. This is done in linear time.
 */

ECRESULT ECKeyTable::Clear()
{
	ECTableRow *lpRow = NULL;
	ECTableRow *lpParent = NULL;

	pthread_mutex_lock(&mLock);

	lpRow = lpRoot;

	// Depth-first delete of all nodes (excluding root)
	while(lpRow) {
		if(lpRow->lpLeft) 
			lpRow = lpRow->lpLeft;
		else if(lpRow->lpRight)
			lpRow = lpRow->lpRight;
		else if(lpRow == lpRoot)
			break;
		else {
			// remember parent node
			lpParent = lpRow->lpParent;

			// decouple from parent
			if(lpRow->fLeft)
				lpParent->lpLeft = NULL;
			else
				lpParent->lpRight = NULL;

			// delete this node
			delete lpRow;

			// continue with parent
			lpRow = lpParent;
			
		}
	}

	lpCurrent = lpRoot;
	
	lpRoot->ulBranchCount = 0;

	mapRow.clear();

	// Remove all bookmarks
	m_mapBookmarks.clear();

	pthread_mutex_unlock(&mLock);

	return erSuccess;
}

ECRESULT ECKeyTable::SeekId(const sObjectTableKey *lpsRowItem)
{
	ECRESULT er = erSuccess;
	ECTableRowMap::iterator iterMap;

	pthread_mutex_lock(&mLock);

	iterMap = this->mapRow.find(*lpsRowItem);

	if(iterMap == mapRow.end()) {
		er = ZARAFA_E_NOT_FOUND;
		goto exit;
	}

	lpCurrent = iterMap->second;

exit:
	pthread_mutex_unlock(&mLock);

	return er;
}

ECRESULT ECKeyTable::GetBookmark(unsigned int ulbkPosition, int* lpbkPosition)
{
	ECRESULT er = erSuccess;
	ECBookmarkMap::iterator	iPosition;
	unsigned int ulCurrPosition = 0;
	
	pthread_mutex_lock(&mLock);

	iPosition = m_mapBookmarks.find(ulbkPosition);
	if (iPosition == m_mapBookmarks.end()) {
		er = ZARAFA_E_INVALID_BOOKMARK;
		goto exit;
	}

	er = CurrentRow(iPosition->second.lpPosition, &ulCurrPosition);
	if (er != erSuccess)
		goto exit;

	if (iPosition->second.ulFirstRowPosition != ulCurrPosition)
		er = ZARAFA_W_POSITION_CHANGED;

	*lpbkPosition = ulCurrPosition;

exit:
	pthread_mutex_unlock(&mLock);

	return er;
}

ECRESULT ECKeyTable::CreateBookmark(unsigned int* lpulbkPosition)
{
	ECRESULT		er = erSuccess;
	sBookmarkPosition	sbkPosition;
	unsigned int	ulbkPosition = 0;
	unsigned int	ulRowCount = 0;
	
	pthread_mutex_lock(&mLock);

	// Limit of bookmarks
	if (m_mapBookmarks.size() >= BOOKMARK_LIMIT) {
		er = ZARAFA_E_UNABLE_TO_COMPLETE;
		goto exit;
	}

	sbkPosition.lpPosition = lpCurrent;

	er = GetRowCount(&ulRowCount, &sbkPosition.ulFirstRowPosition);
	if (er != erSuccess)
		goto exit;

	// set unique bookmark id higher
	ulbkPosition = m_ulBookmarkPosition++;

	// insert into list
	m_mapBookmarks.insert( ECBookmarkMap::value_type(ulbkPosition, sbkPosition) );

	*lpulbkPosition = ulbkPosition;

exit:
	pthread_mutex_unlock(&mLock);

	return er;
}

ECRESULT ECKeyTable::FreeBookmark(unsigned int ulbkPosition)
{
	ECRESULT er = erSuccess;
	ECBookmarkMap::iterator	iPosition;

	pthread_mutex_lock(&mLock);

	iPosition = m_mapBookmarks.find(ulbkPosition);
	if (iPosition == m_mapBookmarks.end()) {
		er = ZARAFA_E_INVALID_BOOKMARK;
		goto exit;
	}

	m_mapBookmarks.erase(iPosition);
	
exit:
	pthread_mutex_unlock(&mLock);

	return er;
}

// Intern function, no locking
ECRESULT ECKeyTable::InvalidateBookmark(ECTableRow *lpRow)
{
	ECBookmarkMap::iterator	iPosition, iRemove;

	// Nothing todo
	if (m_mapBookmarks.empty())
		return erSuccess;

	for(iPosition = m_mapBookmarks.begin(); iPosition != m_mapBookmarks.end(); )
	{
		if (lpRow == iPosition->second.lpPosition) {
			iRemove = iPosition++;
			m_mapBookmarks.erase(iRemove);
		} else {
			++iPosition;
		}
	}
	return erSuccess;
}


ECRESULT ECKeyTable::SeekRow(unsigned int lbkOrgin, int lSeekTo, int *lplRowsSought)
{
	ECRESULT er = erSuccess;
	int lDestRow = 0;
	unsigned int ulCurrentRow = 0;
	unsigned int ulRowCount = 0;
	ECTableRow *lpRow = NULL;


	pthread_mutex_lock(&mLock);

	er = this->GetRowCount(&ulRowCount, &ulCurrentRow);

	if(er != erSuccess)
		goto exit;

	// Go to the correct position in the table
	switch(lbkOrgin) {
	case EC_SEEK_SET:
		// Go to first node
		lDestRow = lSeekTo;
		break;
	case EC_SEEK_CUR:
		lDestRow = ulCurrentRow + lSeekTo;
		break;
	case EC_SEEK_END:
		lDestRow = ulRowCount + lSeekTo;
		break;
	default:
		er = GetBookmark(lbkOrgin, &lDestRow);
		if (er != ZARAFA_W_POSITION_CHANGED && er != erSuccess)
			goto exit;

		lDestRow += lSeekTo;
		break;
	}

	if(lDestRow < 0)
		lDestRow = 0;

	if((ULONG)lDestRow >= ulRowCount)
		lDestRow = ulRowCount;

	// Go to the correct row

	if(lplRowsSought) {
		switch(lbkOrgin) {
		case EC_SEEK_SET:
			*lplRowsSought = lDestRow;
			break;
		case EC_SEEK_CUR:
			*lplRowsSought = lDestRow - ulCurrentRow;
			break;
		case EC_SEEK_END:
			*lplRowsSought = lDestRow - ulRowCount;
			break;
		default:
			*lplRowsSought = lDestRow - ulCurrentRow;
			break;
		}
	}

	if(ulRowCount == 0) {
		lpCurrent = lpRoot; // before front in empty table
		goto exit;
	}

	lpRow = lpRoot->lpRight;

	while(1) {
		if((!lpRow->lpLeft && lDestRow == 0) || (lpRow->lpLeft && lpRow->lpLeft->ulBranchCount == (ULONG)lDestRow))
			break; // found the row!

		if(lpRow->lpLeft == NULL && lpRow->lpRight == NULL) {
			lpRow = NULL;
			break;
		}

		else if(lpRow->lpLeft && lpRow->lpRight == NULL) {
			// Follow the left branche
			lpRow = lpRow->lpLeft;
		}
		else if(lpRow->lpLeft == NULL && lpRow->lpRight) {
			// Follow the right branche
			lDestRow -= lpRow->fHidden ? 0 : 1;
			lpRow = lpRow->lpRight;
		}
		else {
			// There is both a left and a right branch ...
			if(lpRow->lpLeft->ulBranchCount < (ULONG)lDestRow) {
				// ... in which our row doesn't exist, so go to the right branch
				lDestRow -= lpRow->lpLeft->ulBranchCount+ (lpRow->fHidden ? 0 : 1);
				lpRow = lpRow->lpRight;
			} else {
				// ... in which we should be looking
				lpRow = lpRow->lpLeft;
			}
		}
	}

	lpCurrent = lpRow; // may be NULL (after end of table)

exit:
	pthread_mutex_unlock(&mLock);

	return er;
}

ECRESULT ECKeyTable::GetRowCount(unsigned int *lpulRowCount, unsigned int *lpulCurrentRow)
{
	ECRESULT er = erSuccess;

	pthread_mutex_lock(&mLock);

	er = CurrentRow(lpCurrent, lpulCurrentRow);
	if(er != erSuccess)
		goto exit;

    *lpulRowCount = lpRoot->ulBranchCount;

exit:	
	pthread_mutex_unlock(&mLock);

	return er;
}

// Intern function, no locking
ECRESULT ECKeyTable::CurrentRow(ECTableRow *lpRow, unsigned int *lpulCurrentRow)
{
	unsigned int ulCurrentRow = 0;

	if (lpulCurrentRow == NULL)
		return ZARAFA_E_INVALID_PARAMETER;

	if(lpRow == NULL) {
		*lpulCurrentRow = lpRoot->ulBranchCount;
		return erSuccess;
	}

	if(lpRow == lpRoot) {
		*lpulCurrentRow = 0;
		return erSuccess;
	}

	if (lpRow->lpLeft)
		ulCurrentRow += lpRow->lpLeft->ulBranchCount;

	while (lpRow && lpRow->lpParent != NULL && lpRow->lpParent != lpRoot) {
		
		if (!lpRow->fLeft)
			ulCurrentRow += lpRow->lpParent->ulBranchCount - lpRow->ulBranchCount;

		lpRow = lpRow->lpParent;
	}

	*lpulCurrentRow = ulCurrentRow;
	return erSuccess;
}

/*
 * Reads the object IDs from the table; the sorting data is not returned.
 *
 * This reads from the current row, traversing the data. The current row iterator
 * is also updated.
 */
ECRESULT ECKeyTable::QueryRows(unsigned int ulRows, ECObjectTableList* lpRowList, bool bDirBackward, unsigned int ulFlags, bool bShowHidden)
{
	ECRESULT er = erSuccess;
	ECTableRow *lpOrig = NULL;

	pthread_mutex_lock(&mLock);

	lpOrig = lpCurrent;
	
	if(bDirBackward == true && lpCurrent == NULL) {
		SeekRow(EC_SEEK_CUR, -1, NULL);
	}else if(lpCurrent == lpRoot && lpRoot->ulBranchCount) {
		// Go to actual first row if still pre-first row
		SeekRow(EC_SEEK_SET, 0 , NULL);
	}

	// Cap to max. table length. (probably smaller due to cursor position not at start)
	ulRows = ulRows > lpRoot->ulBranchCount ? lpRoot->ulBranchCount : ulRows;

	while(ulRows && lpCurrent) {
		
		if(!lpCurrent->fHidden || bShowHidden) {
    		lpRowList->push_back(lpCurrent->sKey);
		--ulRows;
        }
		
		if(bDirBackward == true && lpCurrent == lpRoot->lpRight)
			break;
	
		if(bDirBackward == true) {
		    Prev();
		} else{// Go to next row
		    Next();
		}
	}

	if(ulFlags & EC_TABLE_NOADVANCE)
		lpCurrent = lpOrig;

	pthread_mutex_unlock(&mLock);

	return er;
}

void ECKeyTable::Next()
{
    if(lpCurrent == NULL)
        return; // Already at end
        
    if(lpCurrent->lpRight) {
        lpCurrent = lpCurrent->lpRight;
        // go to leftmost node in right tree
        while(lpCurrent->lpLeft)
            lpCurrent = lpCurrent->lpLeft;
    } else {
        // Find node to top right from here
        while(lpCurrent && !lpCurrent->fLeft)
            lpCurrent = lpCurrent->lpParent;

        if(lpCurrent)
            lpCurrent = lpCurrent->lpParent;

    }
}

void ECKeyTable::Prev()
{
    if(lpCurrent == NULL) {
        // Past end, seek back one row
        SeekRow(EC_SEEK_END, -1, NULL);
    } else {
        if(lpCurrent->lpLeft) {
            lpCurrent = lpCurrent->lpLeft;
            // Go to rightmost node in left tree
            while(lpCurrent->lpRight)
                lpCurrent = lpCurrent->lpRight;
        } else {
            // Find node to top left from here
            while(lpCurrent && lpCurrent->fLeft)
                lpCurrent = lpCurrent->lpParent;

            if(lpCurrent)
                lpCurrent = lpCurrent->lpParent;
        }
    }
}

ECRESULT ECKeyTable::GetPreviousRow(const sObjectTableKey *lpsRowItem, sObjectTableKey *lpsPrev)
{
    ECRESULT er = erSuccess;
    ECTableRow *lpPos = NULL;

	pthread_mutex_lock(&mLock);

	lpPos = lpCurrent;
    
    er = SeekId((sObjectTableKey *)lpsRowItem);
    if(er != erSuccess)
        goto exit;
    
    Prev();
    while (lpCurrent && lpCurrent->fHidden)
        Prev();
    
    if(lpCurrent) {
        *lpsPrev = lpCurrent->sKey;
    } else {
        er = ZARAFA_E_NOT_FOUND;
    }
    
    // Go back to the previous cursor position
    lpCurrent = lpPos;
    
exit:
	pthread_mutex_unlock(&mLock);

    return er;
}

// Do a Left rotation around lpPivot
void ECKeyTable::RotateL(ECTableRow *lpPivot)
{
	ECTableRow *lpLeft = lpPivot->lpLeft;

	// First, move left leg into position of pivot
	lpLeft->lpParent = lpPivot->lpParent;
	lpLeft->fLeft = lpPivot->fLeft;

	if(lpPivot->fLeft)
		lpPivot->lpParent->lpLeft = lpLeft;
	else
		lpPivot->lpParent->lpRight = lpLeft;

	// Pivot is now an orphan, with only a right leg

	// Now, move right leg of left leg to left leg of pivot
	lpPivot->lpLeft = lpLeft->lpRight;
	if(lpLeft->lpRight) {
		lpLeft->lpRight->fLeft = true;
		lpLeft->lpRight->lpParent = lpPivot;
	}

	// Now, pivot is orphan with left and right leg, and left leg has no right leg

	// Connect pivot as right leg
	lpLeft->lpRight = lpPivot;
	lpPivot->lpParent = lpLeft;
	lpPivot->fLeft = false;

	UpdateCounts(lpPivot);
	UpdateCounts(lpLeft);
}

// Do a Right rotation around lpPivot
void ECKeyTable::RotateR(ECTableRow *lpPivot)
{
	ECTableRow *lpRight = lpPivot->lpRight;

	// First, move right leg into position of pivot
	lpRight->lpParent = lpPivot->lpParent;
	lpRight->fLeft = lpPivot->fLeft;
	
	if(lpPivot->fLeft)
		lpPivot->lpParent->lpLeft = lpRight;
	else
		lpPivot->lpParent->lpRight = lpRight;

	// Pivot is now an orphan, with only a left leg

	// Now, move left leg of right leg to right leg of pivot
	lpPivot->lpRight = lpRight->lpLeft;
	if(lpRight->lpLeft) {
		lpRight->lpLeft->fLeft = false;
		lpRight->lpLeft->lpParent = lpPivot;
	}

	// Now, pivot is orphan with left and right leg, and right leg has no left leg

	// Connect pivot as left leg
	lpRight->lpLeft = lpPivot;
	lpPivot->lpParent = lpRight;
	lpPivot->fLeft = true;

	UpdateCounts(lpPivot);
	UpdateCounts(lpRight);
}

void ECKeyTable::RotateLR(ECTableRow *lpPivot)
{
	ECTableRow *lpParent = lpPivot->lpParent;

	RotateR(lpPivot);
	RotateL(lpParent);
}

void ECKeyTable::RotateRL(ECTableRow *lpPivot)
{
	ECTableRow *lpParent = lpPivot->lpParent;

	RotateL(lpPivot);
	RotateR(lpParent);
}

// Returns the height of a given tree
unsigned int ECKeyTable::GetHeight(ECTableRow *lpRoot)
{
	return lpRoot->ulHeight;
}

int ECKeyTable::GetBalance(ECTableRow *lpPivot)
{
	int balance = 0;

	if(lpPivot) {
		if(lpPivot->lpLeft)
			balance += lpPivot->lpLeft->ulHeight;
		if(lpPivot->lpRight)
			balance -= lpPivot->lpRight->ulHeight;
	}

	return balance;
}

void ECKeyTable::Restructure(ECTableRow *lpPivot)
{
	int balance = 0;

	balance = GetBalance(lpPivot);

	if(balance > 1) {
		// Unbalanced (too much on the left)
		balance = GetBalance(lpPivot->lpLeft);
		if(balance >= 0) {
			// Subtree unbalanced in same direction
			RotateL(lpPivot);
		} else {
			// Subtree unbalanced in the other direction
			RotateLR(lpPivot->lpLeft);
		}
	} else if(balance < -1) {
		// Unbalanced (too much on the right)
		balance = GetBalance(lpPivot->lpRight);
		if(balance <= 0) {
			// Subtree unbalanced in the same direction
			RotateR(lpPivot);
		} else {
			// Subtree unbalanced in the other direction
			RotateRL(lpPivot->lpRight);
		}
	}
}

void ECKeyTable::RestructureRecursive(ECTableRow *lpRow)
{
	while(lpRow != lpRoot && lpRow != NULL) {
		Restructure(lpRow);
		lpRow = lpRow->lpParent;
	}
}

/**
 * Get all rows with a certain sorting prefix
 *
 * @param[in] lpsRowItem Item with the prefix to search for
 * @param[out] lpRowList Items that have a sort key that starts with the prefix of lpsRowItem
 */
ECRESULT ECKeyTable::GetRowsBySortPrefix(sObjectTableKey *lpsRowItem, ECObjectTableList *lpRowList)
{
    ECRESULT er = erSuccess;
    ECTableRow *lpCursor = NULL;
    unsigned int ulSortColPrefixLen = 0;
    unsigned char **lppSortData = 0;
    int *lpSortLen = NULL;
    unsigned char *lpFlags = NULL;
    
	pthread_mutex_lock(&mLock);
	
	lpCursor = lpCurrent;

	er = SeekId(lpsRowItem);
	if(er != erSuccess)
	    goto exit;
	    
    ulSortColPrefixLen = lpCurrent->ulSortCols;
    lppSortData = lpCurrent->lppSortKeys;
    lpSortLen = lpCurrent->lpSortLen;
    lpFlags = lpCurrent->lpFlags;
	
    while(lpCurrent) {
        // Stop when lpCurrent > prefix, so prefix < lpCurrent
        if(ECTableRow::rowcompareprefix(ulSortColPrefixLen, ulSortColPrefixLen, lpSortLen, lppSortData, lpFlags, lpCurrent->ulSortCols, lpCurrent->lpSortLen, lpCurrent->lppSortKeys, lpCurrent->lpFlags))
            break;
            
        lpRowList->push_back(lpCurrent->sKey);
            
        Next();
    }
    
    lpCurrent = lpCursor;
    
exit:
	pthread_mutex_unlock(&mLock);

    return er;
}

ECRESULT ECKeyTable::HideRows(sObjectTableKey *lpsRowItem, ECObjectTableList *lpHiddenList)
{
    ECRESULT er = erSuccess;
    BOOL fCursorHidden = false;
    ECTableRow *lpCursor = NULL;
    unsigned int ulSortColPrefixLen = 0;
    unsigned char **lppSortData = 0;
    int *lpSortLen = NULL;
    unsigned char *lpFlags = NULL;
    
	pthread_mutex_lock(&mLock);

	lpCursor = lpCurrent;

	er = SeekId(lpsRowItem);
	if(er != erSuccess)
	    goto exit;
	    
    ulSortColPrefixLen = lpCurrent->ulSortCols;
    lppSortData = lpCurrent->lppSortKeys;
    lpSortLen = lpCurrent->lpSortLen;
    lpFlags = lpCurrent->lpFlags;
	
    // Go to next row; we never hide the first row, as it is the header
    Next();
    
    while(lpCurrent) {
        // Stop hiding when lpCurrent > prefix, so prefix < lpCurrent
        if(ECTableRow::rowcompareprefix(ulSortColPrefixLen, ulSortColPrefixLen, lpSortLen, lppSortData, lpFlags, lpCurrent->ulSortCols, lpCurrent->lpSortLen, lpCurrent->lppSortKeys, lpCurrent->lpFlags))
            break;
            
        lpHiddenList->push_back(lpCurrent->sKey);
        lpCurrent->fHidden = true;
        
        UpdateCounts(lpCurrent); // FIXME SLOW

        if(lpCurrent == lpCursor) 
            fCursorHidden = true;
            
        Next();
    }
    
    // If the row pointed to by the cursor was not touched, leave it there, otherwise, put the cursor on the next unhidden row
    if(!fCursorHidden) {
        lpCurrent = lpCursor;
    } else {
        while(lpCurrent && lpCurrent->fHidden)
            Next();
    }
    
exit:
	pthread_mutex_unlock(&mLock);

    return er;
}

// @todo lpCurrent should stay pointing at the same row we started at?
ECRESULT ECKeyTable::UnhideRows(sObjectTableKey *lpsRowItem, ECObjectTableList *lpUnhiddenList)
{
    ECRESULT er = erSuccess;
    unsigned int ulSortColPrefixLen = 0;
    unsigned int ulFirstCols = 0;
    unsigned char **lppSortData = 0;
    int *lpSortLen = NULL;
    unsigned char *lpFlags = NULL;

	pthread_mutex_lock(&mLock);

	er = SeekId(lpsRowItem);
	if(er != erSuccess)
	    goto exit;
	    
    ulSortColPrefixLen = lpCurrent->ulSortCols;
    lppSortData = lpCurrent->lppSortKeys;
    lpSortLen = lpCurrent->lpSortLen;
    lpFlags = lpCurrent->lpFlags;
    
    if(lpCurrent->fHidden) {
        // You cannot expand a category whose header is hidden
        er = ZARAFA_E_NOT_FOUND;
        goto exit;
    }
    
    // Go to next row; we don't unhide the first row, as it is the header, 
    Next();

    if(lpCurrent == NULL)
        goto exit; // No more rows
            
    ulFirstCols = lpCurrent->ulSortCols;

    while(lpCurrent) {
        // Stop unhiding when lpCurrent > prefix, so prefix < lpCurrent
        if(ECTableRow::rowcompareprefix(ulSortColPrefixLen, ulSortColPrefixLen, lpSortLen, lppSortData, lpFlags, lpCurrent->ulSortCols, lpCurrent->lpSortLen, lpCurrent->lppSortKeys, lpCurrent->lpFlags))
            break;

        // Only unhide items with the same amount of sort columns as the first row (ensures we only expand the first layer)
        if(lpCurrent->ulSortCols == ulFirstCols) {
                
            lpUnhiddenList->push_back(lpCurrent->sKey);
            lpCurrent->fHidden = false;

            UpdateCounts(lpCurrent); // FIXME SLOW
        }
                
        Next();
    }
    
exit:
	pthread_mutex_unlock(&mLock);

    return er;
}
        
ECRESULT ECKeyTable::LowerBound(unsigned int ulSortCols, int *lpSortLen, unsigned char **lppSortData, unsigned char *lpFlags)
{
    ECRESULT er = erSuccess;
    
	pthread_mutex_lock(&mLock);

	// With B being the passed sort key, find the first item A, for which !(A < B), AKA B >= A
	
	lpCurrent = lpRoot->lpRight;

    // This is a standard binary search through the entire tree	
	while(lpCurrent) {
	    if(ECTableRow::rowcompare(lpCurrent->ulSortCols, lpCurrent->lpSortLen, lpCurrent->lppSortKeys, lpCurrent->lpFlags, ulSortCols, lpSortLen, lppSortData, lpFlags)) {
	        // Value we're looking for is right
	        if(lpCurrent->lpRight == NULL) {
                // Value is not equal, go to next value
   	            Next();
	            break;
            } else {
                lpCurrent = lpCurrent->lpRight;
            }
	    } else {
	        // Value we're looking for is left
	        if(lpCurrent->lpLeft == NULL) {
	            // Found it, if the value is left, then we're just 'right' of that value now.
	            break;
	        } else {
	            lpCurrent = lpCurrent->lpLeft;
            }
        }
	}

	pthread_mutex_unlock(&mLock);

	return er;
}

// Find an exact match for a sort key
ECRESULT ECKeyTable::Find(unsigned int ulSortCols, int *lpSortLen, unsigned char **lppSortData, unsigned char *lpFlags, sObjectTableKey *lpsKey)
{
    ECRESULT er = erSuccess;
	ECTableRow *lpCurPos = NULL;
    
	pthread_mutex_lock(&mLock);

	lpCurPos = lpCurrent;

    er = LowerBound(ulSortCols, lpSortLen, lppSortData, lpFlags);
    if(er != erSuccess)
        goto exit;
     
    // No item is *lpCurrent >= *search, so not found   
    if(lpCurrent == NULL) {
        er = ZARAFA_E_NOT_FOUND;
        goto exit;
    }
        
    // Lower bound has put us either on the first matching row, or at the first item which is *lpCurrent > *search, aka *lpCurrent >= *search
    if(ECTableRow::rowcompare(ulSortCols, lpSortLen, lppSortData, lpFlags, lpCurrent->ulSortCols, lpCurrent->lpSortLen, lpCurrent->lppSortKeys, lpCurrent->lpFlags)) {
        // *lpCurrent >= *search && *lpCurrent > *search, so *lpCurrent != *search
        er = ZARAFA_E_NOT_FOUND;
    } else
		*lpsKey = lpCurrent->sKey;
    
    // *lpCurrent >= *search && !(*lpCurrent > *search), so *lpCurrent >= *search && *lpCurrent <= *search, so *lpCurrent == *search
    
exit:
	lpCurrent = lpCurPos;

	pthread_mutex_unlock(&mLock);

    return er;
}

/**
 * Get object size
 *
 * @return Object size in bytes
 */
unsigned int ECKeyTable::GetObjectSize()
{
	unsigned int ulSize = sizeof(*this);
	ECTableRowMap::iterator iterRow;

	pthread_mutex_lock(&mLock);
	
	ulSize += MEMORY_USAGE_MAP(mapRow.size(), ECTableRowMap);

	for (iterRow = mapRow.begin(); iterRow != mapRow.end(); ++iterRow)
		ulSize += iterRow->second->GetObjectSize();

	ulSize += MEMORY_USAGE_MAP(m_mapBookmarks.size(), ECBookmarkMap);

	pthread_mutex_unlock(&mLock);

	return ulSize;
}

/**
 * Update a part of the sort data for a specific row
 *
 * This function updates only a part of the sort data for a certain row. All other 'columns' in the sort
 * data are left untouched. The change of the sort data may cause the row to be relocated.
 *
 * @param[in] lpsRowItem Row item to modify
 * @param[in] ulColumn Column id to modify (must be already present in the sort data, you cannot add a new column)
 * @param[in] lpSortData New sort data
 * @param[in] ulSortLen New sort length
 * @param[in] ulFlags New sort flags (note: you MUST match the previous DESCENDING flag, or you will get unpredictable results)
 * @param[out] lpsPrevRow Returns the 'previous row' of the new location of the row for notification
 * @param[out] lpfHidden Returns the hidden flag of the modified row
 * @param[out] lpulAction Returns the action of the modification (always TABLE_ROW_MODIFY). It's here only for convenience
 * @return result
 */
ECRESULT ECKeyTable::UpdatePartialSortKey(sObjectTableKey *lpsRowItem, unsigned int ulColumn, unsigned char *lpSortData, unsigned int ulSortLen, unsigned char ulFlags, sObjectTableKey *lpsPrevRow, bool *lpfHidden, ECKeyTable::UpdateType *lpulAction)
{
    ECRESULT er = erSuccess;
    ECTableRow *lpCursor = NULL;
    unsigned char **lppSortKeys = NULL;
    unsigned int *lpSortLen = NULL;
    unsigned char *lpFlags = NULL;
    
	pthread_mutex_lock(&mLock);

    er = GetRow(lpsRowItem, &lpCursor);
    if(er != erSuccess)
        goto exit;
        
    if(ulColumn >= lpCursor->ulSortCols) {
        er = ZARAFA_E_INVALID_PARAMETER;
        goto exit;
    }
    
    // Copy the sortkeys that we used to have
    lppSortKeys = new unsigned char *[lpCursor->ulSortCols];
    lpSortLen = new unsigned int[lpCursor->ulSortCols];
    lpFlags = new unsigned char[lpCursor->ulSortCols];

    // Note: we can just copy the pointers of the sort data here, since they are still valid, and are also valid
    // to pass into UpdateRow()        
    memcpy(lppSortKeys, lpCursor->lppSortKeys, sizeof(unsigned char *) * lpCursor->ulSortCols);
    memcpy(lpSortLen, lpCursor->lpSortLen, sizeof(unsigned int) * lpCursor->ulSortCols);
    memcpy(lpFlags, lpCursor->lpFlags, sizeof(unsigned char) * lpCursor->ulSortCols);
    
    // Modify the updated colum
    lppSortKeys[ulColumn] = lpSortData;
    lpSortLen[ulColumn] = ulSortLen;
    lpFlags[ulColumn] = ulFlags;
    
    if(lpfHidden)
        *lpfHidden = lpCursor->fHidden;

    // Update the row
    er = UpdateRow(TABLE_ROW_MODIFY, lpsRowItem, lpCursor->ulSortCols, lpSortLen, lpFlags, lppSortKeys, lpsPrevRow, lpCursor->fHidden, lpulAction);
    if(er != erSuccess)
        goto exit;
    
exit:
	pthread_mutex_unlock(&mLock);
	delete[] lppSortKeys;
	delete[] lpSortLen;
	delete[] lpFlags;
	return er;
}

/**
 * Get row sort data
 *
 * NOTE, returning reference to internal data. Make sure that the table is not modified
 * while handling the returned data. Also, caller does *not* need to free returned data.
 *
 * @param[in] lpsRowItem Row ID
 * @param[out] lpRow Location to return reference to table's sort data
 * @return result
 */
ECRESULT ECKeyTable::GetRow(sObjectTableKey *lpsRowItem, ECTableRow **lpRow)
{
    ECTableRow *lpCursor = NULL;
    ECRESULT er = erSuccess;

	pthread_mutex_lock(&mLock);

	lpCursor = lpCurrent;
    
    er = SeekId(lpsRowItem);
    if(er != erSuccess)
        goto exit;
    
    *lpRow = lpCurrent;
    
exit:
    lpCurrent = lpCursor;

	pthread_mutex_unlock(&mLock);
    
    return er;
}
