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

#ifndef postsaveaction_INCLUDED
#define postsaveaction_INCLUDED

#include <boost/smart_ptr.hpp>

namespace za { namespace operations {

/**
 * This interface defines an object that performs arbitrary operations
 * once a particular object has been saved.
 */
class IPostSaveAction {
public:
	virtual ~IPostSaveAction() {};
	virtual HRESULT Execute() = 0;
};
typedef boost::shared_ptr<IPostSaveAction> PostSaveActionPtr;

}} // namespace operations, za

#endif // ndef postsaveaction_INCLUDED
