#!/usr/bin/env python

# example of using Service class, in the form of an (incomplete) python version of zarafa-monitor

# usage: ./zarafa-monitor.py

import time
from datetime import date, timedelta, datetime

import zarafa
from zarafa import log_exc, Config, _bytes_to_human

import MAPI
from MAPI.Util import *

CONFIG = {
    'quota_check_interval': Config.integer(default=15),
    'mailquota_resend_interval': Config.integer(default=1),
    'servers': Config.string(),
    'userquota_warning_template': Config.path(default='/etc/zarafa/quotamail/userwarning.mail'),
    'userquota_soft_template': Config.path(default='/etc/zarafa/quotamail/usersoft.mail'),
    'userquota_hard_template': Config.path(default='/etc/zarafa/quotamail/userhard.mail'),
    'companyquota_warning_template': Config.path(default='/etc/zarafa/quotamail/companywarning.mail'),
}

""""
TODO:
- set sender
- Company quota
- Send email to system administrator? => who is this?

"""

class Monitor(zarafa.Service):
    def replace_template(self, mail, user):
        header = "X-Priority: 1\nTO: {} <{}>\nFROM:\n".format(user.fullname,user.email) 
        mail = header + mail
        mail = mail.replace('${ZARAFA_QUOTA_NAME}', user.name)
        mail = mail.replace('${ZARAFA_QUOTA_STORE_SIZE}', _bytes_to_human(user.store.size))
        mail = mail.replace('${ZARAFA_QUOTA_WARN_SIZE}', _bytes_to_human(user.quota.warn_limit))
        mail = mail.replace('${ZARAFA_QUOTA_SOFT_SIZE}', _bytes_to_human(user.quota.soft_limit))
        mail = mail.replace('${ZARAFA_QUOTA_HARD_SIZE}', _bytes_to_human(user.quota.hard_limit))
        return mail

    def check_quota_interval(self, user):
        interval = self.config['mailquota_resend_interval']
        config_item = user.store.config_item('Zarafa.Quota')
        try:
            mail_time = config_item.prop(PR_EC_QUOTA_MAIL_TIME)
        except MAPIErrorNotFound: # XXX use some kind of defaulting, so try/catch is not necessary
            mail_time = None # set default?
        if not mail_time or datetime.now() - mail_time.pyval > timedelta(days=1):
            mail_time.value = datetime.now()
            return True
        else:
            return False

    def main(self):
        server, config, log = self.server, self.config, self.log
        while True:
            with zarafa.log_exc(log):
                log.info('start check')
                for user in server.users():
                    for limit in ('hard', 'soft', 'warning'): # XXX add quota_status to API?
                        if 0 < getattr(user.quota, limit+'_limit') < u.store.size:
                            log.warning('Mailbox of user %s has exceeded its %s limit' % (u.name, limit))
                            if self.check_quota_interval(u):
                                mail = open(self.config['userquota_warning_template']).readlines()
                                mail = ''.join(mail)
                                mail = self.replace_template(mail, u)
                                for r in u.quota.recipients:
                                    r.store.inbox.create_item(eml=mail)
                log.info('check done')
            time.sleep(config['quota_check_interval']*60)

if __name__ == '__main__':
    Monitor('monitor', config=CONFIG).main()
