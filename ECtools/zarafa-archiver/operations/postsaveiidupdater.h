/*
 * Copyright 2005 - 2015  Zarafa B.V. and its licensors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation with the following
 * additional terms according to sec. 7:
 *
 * "Zarafa" is a registered trademark of Zarafa B.V.
 * The licensing of the Program under the AGPL does not imply a trademark
 * license. Therefore any rights, title and interest in our trademarks
 * remain entirely with us.
 *
 * Our trademark policy (see TRADEMARKS.txt) allows you to use our trademarks
 * in connection with Propagation and certain other acts regarding the Program.
 * In any case, if you propagate an unmodified version of the Program you are
 * allowed to use the term "Zarafa" to indicate that you distribute the Program.
 * Furthermore you may use our trademarks where it is necessary to indicate the
 * intended purpose of a product or service provided you use it in accordance
 * with honest business practices. For questions please contact Zarafa at
 * trademark@zarafa.com.
 *
 * The interactive user interface of the software displays an attribution
 * notice containing the term "Zarafa" and/or the logo of Zarafa.
 * Interactive user interfaces of unmodified and modified versions must
 * display Appropriate Legal Notices according to sec. 5 of the GNU Affero
 * General Public License, version 3, when you propagate unmodified or
 * modified versions of the Program. In accordance with sec. 7 b) of the GNU
 * Affero General Public License, version 3, these Appropriate Legal Notices
 * must retain the logo of Zarafa or display the words "Initial Development
 * by Zarafa" if the display of the logo is not reasonably feasible for
 * technical reasons.
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

#ifndef postsaveiidupdater_INCLUDED
#define postsaveiidupdater_INCLUDED

#include "postsaveaction.h"
#include <zarafa/mapi_ptr.h>
#include "instanceidmapper_fwd.h"
#include "archiver-common.h"
#include <list>

namespace za { namespace operations {

class TaskBase {
public:
	TaskBase(const AttachPtr &ptrSourceAttach, const MessagePtr &ptrDestMsg, ULONG ulDestAttachIdx);

	HRESULT Execute(ULONG ulPropTag, const InstanceIdMapperPtr &ptrMapper);

private:
	HRESULT GetUniqueIDs(IAttach *lpAttach, LPSPropValue *lppServerUID, ULONG *lpcbInstanceID, LPENTRYID *lppInstanceID);
	virtual HRESULT DoExecute(ULONG ulPropTag, const InstanceIdMapperPtr &ptrMapper, const SBinary &sourceServerUID, ULONG cbSourceInstanceID, LPENTRYID lpSourceInstanceID, const SBinary &destServerUID, ULONG cbDestInstanceID, LPENTRYID lpDestInstanceID) = 0;

private:
	AttachPtr	m_ptrSourceAttach;
	MessagePtr	m_ptrDestMsg;
	ULONG 	m_ulDestAttachIdx;
};
typedef boost::shared_ptr<TaskBase> TaskPtr;
typedef std::list<TaskPtr> TaskList;


class TaskMapInstanceId : public TaskBase {
public:
	TaskMapInstanceId(const AttachPtr &ptrSourceAttach, const MessagePtr &ptrDestMsg, ULONG ulDestAttachNum);
	HRESULT DoExecute(ULONG ulPropTag, const InstanceIdMapperPtr &ptrMapper, const SBinary &sourceServerUID, ULONG cbSourceInstanceID, LPENTRYID lpSourceInstanceID, const SBinary &destServerUID, ULONG cbDestInstanceID, LPENTRYID lpDestInstanceID);
};


class TaskVerifyAndUpdateInstanceId : public TaskBase {
public:
	TaskVerifyAndUpdateInstanceId(const AttachPtr &ptrSourceAttach, const MessagePtr &ptrDestMsg, ULONG ulDestAttachNum, ULONG cbDestInstanceID, LPENTRYID lpDestInstanceID);
	HRESULT DoExecute(ULONG ulPropTag, const InstanceIdMapperPtr &ptrMapper, const SBinary &sourceServerUID, ULONG cbSourceInstanceID, LPENTRYID lpSourceInstanceID, const SBinary &destServerUID, ULONG cbDestInstanceID, LPENTRYID lpDestInstanceID);

private:
	entryid_t m_destInstanceID;
};


class PostSaveInstanceIdUpdater : public IPostSaveAction {
public:

	PostSaveInstanceIdUpdater(ULONG ulPropTag, const InstanceIdMapperPtr &ptrMapper, const TaskList &lstDeferred);
	HRESULT Execute();

private:
	ULONG m_ulPropTag;
	InstanceIdMapperPtr m_ptrMapper;
	TaskList m_lstDeferred;
};

}} // namespace operations, za

#endif // ndef postsaveiidupdater_INCLUDED
