//=============================================================================
//
//   File : ChannelsJoinDialog.cpp
//   Creation date : Thu Nov 6 2001 12:41:18 by Juanjo �varez
//
//   This file is part of the KVIrc irc client distribution
//   Copyright (C) 2001-2010 Szymon Stefanek (pragma at kvirc dot net)
//
//   This program is FREE software. You can redistribute it and/or
//   modify it under the linkss of the GNU General Public License
//   as published by the Free Software Foundation; either version 2
//   of the License, or (at your opinion) any later version.
//
//   This program is distributed in the HOPE that it will be USEFUL,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program. If not, write to the Free Software Foundation,
//   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
//=============================================================================

#include "ChannelsJoinDialog.h"

#include "KviLocale.h"
#include "KviOptions.h"
#include "KviSelectors.h"
#include "KviApplication.h"
#include "KviCString.h"
#include "KviQString.h"
#include "KviIconManager.h"
#include "KviConsoleWindow.h"
#include "KviRegisteredChannel.h"
#include "KviRegisteredChannelDataBase.h"
#include "KviKvsScript.h"
#include "KviTalGroupBox.h"

#include <QLabel>
#include <QLineEdit>
#include <QLayout>
#include <QPushButton>
#include <QEvent>
#include <QCloseEvent>
#include <QMouseEvent>
#include <QHeaderView>
#include <QCheckBox>


extern ChannelsJoinDialog    * g_pChannelsWindow;
extern QRect                   g_rectChannelsJoinGeometry;
// KviApplication.cpp
extern KVIRC_API KviRegisteredChannelDataBase * g_pRegisteredChannelDataBase;


ChannelsJoinDialog::ChannelsJoinDialog(QWidget * par, const char * name)
: QDialog(par)
{
	setObjectName(name);
	setWindowTitle(__tr2qs("Join Channels"));
	setWindowIcon(*(g_pIconManager->getSmallIcon(KviIconManager::Channel)));

	m_pConsole = 0;

	QGridLayout * g = new QGridLayout(this);

	m_pTreeWidget = new ChannelsJoinDialogTreeWidget(this);
	m_pTreeWidget->setHeaderLabel(__tr2qs("Channel"));
	m_pTreeWidget->setRootIsDecorated(true);
	m_pTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	g->addWidget(m_pTreeWidget,0,0,1,2);
	connect(m_pTreeWidget,SIGNAL(itemDoubleClicked(QTreeWidgetItem *,int)),this,SLOT(itemDoubleClicked(QTreeWidgetItem *,int)));


	m_pGroupBox = new KviTalGroupBox(Qt::Horizontal,__tr2qs("Channel" ),this);
	QString szMsg = __tr2qs("Name");
	szMsg.append(":");

	new QLabel(szMsg,m_pGroupBox);

	m_pChannelEdit = new QLineEdit(m_pGroupBox);
	connect(m_pChannelEdit,SIGNAL(returnPressed()), this, SLOT(editReturnPressed()));
	connect(m_pChannelEdit,SIGNAL(textChanged(const QString &)),this,SLOT(editTextChanged(const QString &)));

	szMsg = __tr2qs("Password");
	szMsg.append(":");
	QLabel(szMsg,m_pGroupBox);

	m_pPass = new QLineEdit(m_pGroupBox);
	m_pPass->setEchoMode(QLineEdit::Password);

	g->addWidget(m_pGroupBox,1,0,1,2);

	KviTalHBox * hb = new KviTalHBox(this);
	hb->setSpacing(4);

	g->addWidget(hb,2,0,1,2,Qt::AlignHCenter);

	m_pJoinButton = new QPushButton(__tr2qs("&Join"),hb);
	// Join on return pressed
	m_pJoinButton->setDefault(true);
	connect(m_pJoinButton,SIGNAL(clicked()),this,SLOT(joinClicked()));

	m_pRegButton = new QPushButton(__tr2qs("&Register"),hb);
	// Join on return pressed
	connect(m_pRegButton,SIGNAL(clicked()),this,SLOT(regClicked()));

	m_pClearButton = new QPushButton(__tr2qs("Clear Recent"),hb);
	connect(m_pClearButton,SIGNAL(clicked()),this,SLOT(clearClicked()));

	m_pShowAtStartupCheck = new QCheckBox(__tr2qs("Show this window after connecting"),this);
	m_pShowAtStartupCheck->setChecked(KVI_OPTION_BOOL(KviOption_boolShowChannelsJoinOnIrc));
	g->addWidget(m_pShowAtStartupCheck,3,0);


	QPushButton * cancelButton = new QPushButton(__tr2qs("Close"),this);
	connect(cancelButton,SIGNAL(clicked()),this,SLOT(cancelClicked()));

	g->addWidget(cancelButton,3,1);

/*
	KviTalHBox * hb = new KviTalHBox(this);
	hb->setSpacing(4);

	QPushButton * namesButton = new QPushButton("/names", hb);
	connect(namesButton,SIGNAL(clicked()),this,SLOT(namesClicked()));
	QPushButton * whoButton = new QPushButton("/who", hb);
	connect(whoButton,SIGNAL(clicked()),this,SLOT(whoClicked()));

	g->addWidget(hb,2,0);
*/

	g->setRowStretch(0,1);
	g->setColumnStretch(0,1);

	fillListView();

	if(g_rectChannelsJoinGeometry.y() < 5)g_rectChannelsJoinGeometry.setY(5);

	resize(g_rectChannelsJoinGeometry.width(),g_rectChannelsJoinGeometry.height());
	move(g_rectChannelsJoinGeometry.x(),g_rectChannelsJoinGeometry.y());

	enableJoin();
}

ChannelsJoinDialog::~ChannelsJoinDialog()
{
	KVI_OPTION_BOOL(KviOption_boolShowChannelsJoinOnIrc) = m_pShowAtStartupCheck->isChecked();

	g_rectChannelsJoinGeometry = QRect(pos().x(),pos().y(),size().width(),size().height());
	g_pChannelsWindow = 0;
}

void ChannelsJoinDialog::setConsole(KviConsoleWindow  * pConsole)
{
	m_pConsole = pConsole;
	fillListView();
}

void ChannelsJoinDialog::fillListView()
{
	m_pTreeWidget->clear();

	m_pTreeWidget->header()->hide();

	// Registered channels go first

	QTreeWidgetItem * par = new QTreeWidgetItem(m_pTreeWidget, HeaderItem);
	par->setText(0,__tr2qs("Registered Channels"));
	par->setExpanded(true);

	QHash<QString,KviRegisteredChannelList *> * d = g_pRegisteredChannelDataBase->channelDict();
	if(d)
	{
		for(QHash<QString,KviRegisteredChannelList *>::Iterator it = d->begin();it != d->end();++it)
		{
			QTreeWidgetItem * chld = new QTreeWidgetItem(par, RegisteredChannelItem);
			chld->setText(0,it.key());
			chld->setIcon(0,*(g_pIconManager->getSmallIcon(KviIconManager::Channel)));
		}
	}

	par = new QTreeWidgetItem(m_pTreeWidget, HeaderItem);
	par->setText(0,__tr2qs("Recent Channels"));
	par->setExpanded(true);

	QTreeWidgetItem * chld;

	bool bGotChanOnCurrentNetwork = false;

	if(m_pConsole)
	{
		QStringList * pList = g_pApp->recentChannelsForNetwork(m_pConsole->currentNetworkName());
		if(pList)
		{
			if(pList->count() > 0)
			{
				bGotChanOnCurrentNetwork = true;

				par = new QTreeWidgetItem(par, HeaderItem);
				par->setText(0,__tr2qs("Current Network"));
				par->setExpanded(true);

				for(QStringList::Iterator it = pList->begin(); it != pList->end(); ++it)
				{
					chld = new QTreeWidgetItem(par, RecentChannelItem);
					chld->setText(0,*it);
					chld->setIcon(0,*(g_pIconManager->getSmallIcon(KviIconManager::Channel)));
				}
			}
		}
	}

	KviPointerHashTable<QString,QStringList> * pDict = g_pApp->recentChannels();
	if(!pDict)
		return;

	par = new QTreeWidgetItem(par, HeaderItem);
	par->setText(0,__tr2qs("All Networks"));

	if(!bGotChanOnCurrentNetwork)
		par->setExpanded(true); // expand this one instead

	QHash<QString,int> hNoDuplicates;

	for(QStringList * pChans = pDict->first();pChans;pChans = pDict->next())
	{
		for(QStringList::Iterator it = pChans->begin(); it != pChans->end(); ++it)
		{
			QString chan = *it;
			if(hNoDuplicates.contains(chan.toLower()))
				continue;
			hNoDuplicates.insert(chan.toLower(),1);
			chld = new QTreeWidgetItem(par, RecentChannelItem);
			chld->setText(0,chan);
			chld->setIcon(0,*(g_pIconManager->getSmallIcon(KviIconManager::Channel)));
		}
	}
}

void ChannelsJoinDialogTreeWidget::mousePressEvent(QMouseEvent *e)
{
	QTreeWidgetItem * it = itemAt(e->pos());
	ChannelsJoinDialog *pDialog = (ChannelsJoinDialog*) parentWidget();
	if(!it || !pDialog)
		return;

	setCurrentItem(it);

	if(it->type() == ChannelsJoinDialog::HeaderItem)
		return;

	if(e->button() & Qt::RightButton)
	{
		if(!m_pJoinPopup)
		{
			m_pJoinPopup = new KviTalPopupMenu(this);
			m_pJoinPopup->insertItem(*(g_pIconManager->getSmallIcon(KviIconManager::Join)),__tr2qs("Join"),pDialog,SLOT(joinClicked()));
			m_pJoinPopup->insertItem(*(g_pIconManager->getSmallIcon(KviIconManager::Discard)),__tr2qs("Delete"),pDialog,SLOT(deleteClicked()));
		}

		m_pJoinPopup->popup(QCursor::pos());
	} else {
		pDialog->itemSelected();
	}
}

void ChannelsJoinDialogTreeWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
	QTreeWidgetItem * it = itemAt(e->pos());
	ChannelsJoinDialog *pDialog = (ChannelsJoinDialog*) parentWidget();
	if(!it || !pDialog || !(e->button() & Qt::LeftButton))
		return;

	if(it->type() == ChannelsJoinDialog::HeaderItem)
		return;

	setCurrentItem(it);
	pDialog->itemSelected();
	pDialog->joinClicked();
}

void ChannelsJoinDialog::itemSelected()
{
	QTreeWidgetItem * it = m_pTreeWidget->currentItem();
	if(!it)
		return;
	if(it->type() == HeaderItem)
		return;

	QString szTmp = it->text(0);
	m_pChannelEdit->setText(szTmp);
	enableJoin();
}

void ChannelsJoinDialog::editTextChanged(const QString &)
{
	enableJoin();
}

void ChannelsJoinDialog::enableJoin()
{
	QString szTmp = m_pChannelEdit->text();
	KviConsoleWindow * c = g_pApp->topmostConnectedConsole();
	if(c)
	{
		if(szTmp.isEmpty())
		{
			m_pJoinButton->setEnabled(false);
			m_pRegButton->setEnabled(false);
		} else {
			m_pJoinButton->setEnabled(true);
			m_pRegButton->setEnabled(true);
		}
	} else {
		m_pTreeWidget->setEnabled(false);
		m_pGroupBox->setEnabled(false);
		m_pJoinButton->setEnabled(false);
	}
}

void ChannelsJoinDialog::cancelClicked()
{
	delete this;
}

void ChannelsJoinDialog::deleteClicked()
{
	//FIXME this actually removes channel without matching their network,
	// since the whole ChannelsJoinDialog does not consider networks
	QTreeWidgetItem * it = m_pTreeWidget->currentItem();
	if(!it)
		return;
	if(it->type() == HeaderItem)
		return;

	QString szChan = it->text(0);
	switch(it->type())
	{
		case RegisteredChannelItem:
		{
			KviRegisteredChannel * ch = g_pRegisteredChannelDataBase->find(szChan,QString("*"));
			if(ch)
				g_pRegisteredChannelDataBase->remove(ch);
			delete it;
			break;
		}
		case RecentChannelItem:
		{
			KviPointerHashTable<QString,QStringList> * pDict = g_pApp->recentChannels();
			if(!pDict)
				return;
			for(QStringList * pChans = pDict->first();pChans;pChans = pDict->next())
			{
				for(QStringList::Iterator item = pChans->begin(); item != pChans->end(); ++item)
				{
					if(*item==szChan)
					{
						pChans->removeAll(szChan);
						delete it;
						return;
					}
				}
			}
			break;
		}
	}
}

void ChannelsJoinDialog::joinClicked()
{
	QString szPass = m_pPass->text();
	QString szTmp = m_pChannelEdit->text();

	if(szTmp.isEmpty())
		return;

	KviQString::escapeKvs(&szTmp);
	KviQString::escapeKvs(&szPass);

	QString szCmd = "join ";
	szCmd += szTmp;
	szCmd += " ";
	szCmd += szPass;

	KviConsoleWindow * c = g_pApp->topmostConnectedConsole();
	if(!c)
		return; // no connection

	KviWindow * w = g_pActiveWindow;
	if(w->console() != c)
		w = c;

	KviKvsScript::run(szCmd,w);

	m_pChannelEdit->setText("");
	m_pPass->setText("");
}

void ChannelsJoinDialog::regClicked()
{
	QString szTmp = m_pChannelEdit->text();

	if(szTmp.isEmpty())
		return;

	KviQString::escapeKvs(&szTmp);
	QString szCmd = "regchan.add ";
	szCmd += szTmp;

	KviConsoleWindow * c = g_pApp->topmostConnectedConsole();
	if(!c)
		return; // no connection

	KviWindow * w = g_pActiveWindow;
	if(w->console() != c)
		w = c;

	KviKvsScript::run(szCmd,w);
	fillListView();

	//focus the new item (in reality, the first matching one)
	QList<QTreeWidgetItem*> items = m_pTreeWidget->findItems(szTmp, Qt::MatchRecursive | Qt::MatchExactly);
	if(!items.empty())
	{
		m_pTreeWidget->setCurrentItem(items.first());
		m_pTreeWidget->scrollToItem(items.first());
	}
}

void ChannelsJoinDialog::clearClicked()
{
	QString szCmd = "option stringlistRecentChannels";

	KviConsoleWindow * c = g_pApp->topmostConnectedConsole();
	if(!c)
		return; // no connection

	KviWindow * w = g_pActiveWindow;
	if(w->console() != c)
		w = c;

	KviKvsScript::run(szCmd,w);
	fillListView();
}

/*
void ChannelsJoinDialog::whoClicked()
{
	KviCString tmp = m_pChannelEdit->text();
	if(!tmp.isEmpty())doCmd("who", tmp.ptr());
}

void ChannelsJoinDialog::namesClicked()
{
	KviCString tmp = m_pChannelEdit->text();
	//FIXME: I must be a nice guy and implement /names in the core...
	if(!tmp.isEmpty())doCmd("raw names", tmp.ptr());
}

*/

void ChannelsJoinDialog::editReturnPressed()
{
	joinClicked();
}

void ChannelsJoinDialog::closeEvent(QCloseEvent * e)
{
	e->ignore();
	delete this;
}

#ifndef COMPILE_USE_STANDALONE_MOC_SOURCES
#include "m_ChannelsJoinDialog.moc"
#endif //COMPILE_USE_STANDALONE_MOC_SOURCES
