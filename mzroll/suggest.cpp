#include "suggest.h"

SuggestPopup::SuggestPopup(QLineEdit *parent): QObject(parent), editor(parent)
{
		popup = new QTreeWidget;
		popup->setWindowFlags(Qt::Popup);
		popup->setFocusPolicy(Qt::NoFocus);
		popup->setFocusProxy(parent);
		popup->setMouseTracking(true);

        popup->setColumnCount(3);
		popup->setUniformRowHeights(true);
		popup->setRootIsDecorated(false);
		popup->setEditTriggers(QTreeWidget::NoEditTriggers);
		popup->setSelectionBehavior(QTreeWidget::SelectRows);
		popup->setFrameStyle(QFrame::Box | QFrame::Plain);
		popup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		popup->header()->hide();

		popup->installEventFilter(this);


		connect(popup, SIGNAL(itemClicked(QTreeWidgetItem*, int)), SLOT(doneCompletion()));

		timer = new QTimer(this);
		timer->setSingleShot(true);
		timer->setInterval(500);

		connect(editor, SIGNAL(textEdited(QString)), timer, SLOT(start()));
		connect(editor, SIGNAL(textEdited(QString)), this, SLOT(doSearch(QString)));
}

SuggestPopup::~SuggestPopup()
{
		delete popup;
}

void SuggestPopup::addToHistory(QString key, int count) {
//	qDebug() << "SuggestPopup: addToHistory() " << key << " " << count;
	searchHistory[key]=count;
}

void SuggestPopup::addToHistory(QString key) {
	if ( searchHistory.contains(key) ) {
	 	addToHistory(key, searchHistory.value(key)+1);
	} else  {
	 	addToHistory(key, 1);
	}
}

void SuggestPopup::setDatabase(QString db) {
    _currentDatabase =db;
}

void SuggestPopup::addToHistory() {
	 QString key = editor->text();
	 if(! key.isEmpty()) addToHistory(key);
}

QMap<QString,int> SuggestPopup::getHistory() { 
	return searchHistory;
}
bool SuggestPopup::eventFilter(QObject *obj, QEvent *ev)
{
		if (obj != popup)
				return false;

		if (ev->type() == QEvent::MouseButtonPress) {
				popup->hide();
				editor->setFocus();
				return true;
		}

		if (ev->type() == QEvent::KeyPress) {

				bool consumed = false;
				int key = static_cast<QKeyEvent*>(ev)->key();
				switch (key) {
						case Qt::Key_Enter:
						case Qt::Key_Return:
								doneCompletion();
								consumed = true;

						case Qt::Key_Escape:
								editor->setFocus();
								popup->hide();
								consumed = true;

						case Qt::Key_Up:
						case Qt::Key_Down:
						case Qt::Key_Home:
						case Qt::Key_End:
						case Qt::Key_PageUp:
						case Qt::Key_PageDown:
								break;

						default:
								popup->hide();
								editor->setFocus();
								editor->event(ev);
								break;
				}

				return consumed;
		}

		return false;
}

void SuggestPopup::doSearchCompounds(QString needle, int maxHitCount, QString dbLimit) {

    //construct regular expression
    QRegExp regexp("^" + needle, Qt::CaseInsensitive,QRegExp::RegExp);

    //bad regular expression
    if (!needle.isEmpty() && !regexp.isValid()) return;

    string currentDb = dbLimit.toStdString();
    for(unsigned int i=0;  i < DB.compoundsDB.size(); i++ ) {
        Compound* c = DB.compoundsDB[i];

        if(!c) continue;
        if(!dbLimit.isEmpty() and c->db != currentDb) continue;

        QString name(c->name.c_str() );
        QString formula(c->formula.c_str() );
        QString id(c->id.c_str() );

        if ( name.length()==0) continue;
        if ( compound_matches.count(name) ) continue;

        //location of match
        int index = regexp.indexIn(name);
        if ( index < 0) index = regexp.indexIn(id);
        if ( index < 0) continue;

        //score
        float c1=0;
        if ( searchHistory.contains(name)) c1 = searchHistory.value(name);
        if ( c->fragment_mzs.size() > 0 ) c1 += 10;
        if ( index == 0 ) c1 += 10;
        if ( c->db == currentDb ) c1 += 1000;
        if ( formula == needle)   c1 += 300;

        float c2 = 100 * regexp.matchedLength() / float(name.length()); //range from 0 to 1

        float c3=0;
        if ( c->expectedRt > 0 ) c3=10;

        float score=1+c1+c2+c3;
        //qDebug() << name << c1 << c2 << c3;

        if ( !scores.contains(name) || scores[name] < score ) {
            scores[name]=score;
            compound_matches[name]=c;
        }

        if (scores.size()>maxHitCount) break;
    }
}

void SuggestPopup::doSearchHistory(QString needle)  {
		QRegExp regexp(needle,Qt::CaseInsensitive,QRegExp::RegExp);
		if (!needle.isEmpty() && !regexp.isValid()) return;

		foreach( QString name, searchHistory.keys() ) {
				if ( name.length()==0) continue;
				if ( scores.contains(name) ) continue;
				if ( name.contains(needle) || name.contains(regexp)){
						float c1= searchHistory.value(name);
						float c2= 1.00/name.length();
						float c3=0;
						float score=1+c1+c2+c3;
						scores[name]=score;
				}
		}
}

void SuggestPopup::doSearch(QString needle) 
{
        //minum length of needle in order to start matching
        if (needle.isEmpty() || needle.length() < 1 ) return;
        needle = needle.simplified();
        needle.replace(' ',".*");

		QRegExp regexp(needle,Qt::CaseInsensitive,QRegExp::RegExp);
		if (!needle.isEmpty() && !regexp.isValid()) return;

		QRegExp startsWithNumber("^\\d");
		if (startsWithNumber.indexIn(needle) == 0 ) return;

		popup->setUpdatesEnabled(false);
        popup->clear();

        //do compound ssearch
		scores.clear(); 
		compound_matches.clear();

        //first search selected database
        doSearchCompounds(needle,50,_currentDatabase);

        //no compound matches, try different datbase
        if (scores.size() < 20) {
            doSearchCompounds(needle,50,QString());
        }

		 //sort hit list by score
		 QList< QPair<float,QString> > list;
         foreach( QString name, scores.keys() ) { list.append(qMakePair(1/(scores[name]+1),name)); }
         qSort(list);


         //show popup
         int compoundMatches=0;
         int historyMatches=0;
         int fragmentsCount=0;
         QString dbName;
         QString mw;

         QList<QPair<float, QString> >::ConstIterator it = list.constBegin();
         while (it != list.constEnd()) {
             QString name = (*it).second;
             float  score = (*it).first;

             qDebug() << name << " " << score;

             NumericTreeWidgetItem *item=NULL;
             if( compound_matches.contains(name)) {
                 Compound* c = compound_matches[name];
                 item = new NumericTreeWidgetItem(popup,CompoundType);
                 item->setData(0,Qt::UserRole,QVariant::fromValue(c));
                 dbName=c->db.c_str();
                 fragmentsCount=c->fragment_mzs.size();
                 mw = QString::number(c->mass,'f',3);
             }

             if (item) {
                 if(fragmentsCount>0) item->setIcon(0,QIcon(rsrcPath + "/spectra.png"));
                 item->setText(0,mw);
                 item->setText(1,name.left(30));
                 item->setText(2,dbName.left(20));
                 item->setTextColor(0, Qt::gray);
             }
             ++it;
         }

		//const QPalette &pal = editor->palette();
		//QColor color = pal.color(QPalette::Disabled, QPalette::WindowText);
		popup->setUpdatesEnabled(true);
        popup->setSortingEnabled(false);
        //popup->sortByColumn(0,Qt::DescendingOrder);
        popup->resizeColumnToContents(0);
		popup->resizeColumnToContents(1);
		
        int h = popup->sizeHintForRow(0) * (compoundMatches+historyMatches);
        popup->resize(popup->width(), h);
        popup->setMinimumWidth(500);
        popup->setMinimumHeight(50);
        popup->adjustSize();

		if (popup->topLevelItemCount() > 0 ) {
			popup->show();
			popup->setCurrentItem( popup->itemAt(0,0));
			popup->update();
			popup->setFocus();
		}
		//popup->hideColumn(0);
		popup->move(editor->mapToGlobal(QPoint(0, editor->height())));
}



void SuggestPopup::doneCompletion()
{
		timer->stop();
		popup->hide();
		QTreeWidgetItem *item = popup->currentItem();
		if (!item) return;

		editor->setFocus();
		editor->setText(item->text(1));

		QVariant v = item->data(0,Qt::UserRole);
		if ( item->type() == CompoundType ) {
        	Compound*  com =  v.value<Compound*>();
			if(com) emit(compoundSelected(com)); 
		} else {
			QMetaObject::invokeMethod(editor, "returnPressed");
		}
		addToHistory(item->text(1));
}

void SuggestPopup::preventSuggest()
{
		timer->stop();
}
