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

#ifndef SOAPDEBUG_H
#define SOAPDEBUG_H

#include "soapH.h"
#include <string>

// Debug
std::string RestrictionToString(const restrictTable *lpRestriction, unsigned int indent = 0);
std::string PropNameFromPropArray(unsigned int cValues, const propVal *lpPropArray);
std::string PropValueToString(const propVal *lpPropValue);
const char* RightsToString(unsigned int ulecRights);

#endif
