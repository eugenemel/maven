#include "editpeakgroupingdialog.h"

EditPeakGroupDialog::EditPeakGroupDialog(QWidget *parent, MainWindow *mainwindow) : QDialog(parent) {
    setupUi(this);
    setModal(true);

    _mainwindow = mainwindow;

    updateAdductComboBox();

    //Issue 278: force window to top
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    brsSuggestions->setOpenLinks(false);
    brsSuggestions->setOpenExternalLinks(false);
    connect(brsSuggestions, SIGNAL(anchorClicked(QUrl)), this, SLOT(onAnchorClicked(QUrl)));
    connect(_mainwindow, SIGNAL(updatedAvailableAdducts()), this, SLOT(updateAdductComboBox()));
    connect(cmbSelectAdduct, SIGNAL(currentIndexChanged(int)), this, SLOT(updateAdductText()));
}

EditPeakGroupDialog::~EditPeakGroupDialog(){}

void EditPeakGroupDialog::show(){
    if (QDialog::isVisible()){
        QDialog::show();
        QDialog::raise();
        QDialog::activateWindow();
    } else {
        QDialog::show();
    }
}

void EditPeakGroupDialog::setPeakGroup(PeakGroup *selectedPeakGroup) {

    if (!selectedPeakGroup) return; //Probably a cluster

    if (!_mainwindow) {
        qDebug() << "EditPeakGroupDialog::setPeakGroup() called with _mainwindow = nullptr!";
        return;
    }

    vector<string> suggestionSet;
    QString suggestionString = QString();

    QString compoundString = QString();
    if (selectedPeakGroup->compound){
        compoundString.append(selectedPeakGroup->compound->name.c_str());
    }

    if (compoundString.isEmpty() && _mainwindow->getProjectWidget()->currentProject){
        //try to get original compound string from matches (mzkitchen workflow)

        if (_mainwindow->getProjectWidget()->currentProject->topMatch.find(selectedPeakGroup->groupId)
                != _mainwindow->getProjectWidget()->currentProject->topMatch.end()){

            shared_ptr<mzrollDBMatch> originalCompoundNameAndScore = _mainwindow->getProjectWidget()->currentProject->topMatch.at(selectedPeakGroup->groupId);

            compoundString.append(originalCompoundNameAndScore->compoundName.c_str());
        }

    }

    brsSuggestions->setText("");

    if (!compoundString.isEmpty()){

        //try to summarize lipids based on suggestions
        QString strippedCompoundName = compoundString.section(' ', 0, 0);

        string baseName = strippedCompoundName.toStdString();

        string strucDefSummarized = LipidSummarizationUtils::getStrucDefSummary(baseName);
        string snChainSummarized = LipidSummarizationUtils::getSnPositionSummary(baseName);
        string acylChainLengthSummarized = LipidSummarizationUtils::getAcylChainLengthSummary(baseName);
        string acylChainCompositionSummarized = LipidSummarizationUtils::getAcylChainCompositionSummary(baseName);
        string lipidClassSummarized = LipidSummarizationUtils::getLipidClassSummary(baseName);

        suggestionSet.push_back(baseName);

        if (std::find(suggestionSet.begin(), suggestionSet.end(), strucDefSummarized) == suggestionSet.end()) {
            suggestionSet.push_back(strucDefSummarized);
        }

        if (std::find(suggestionSet.begin(), suggestionSet.end(), snChainSummarized) == suggestionSet.end()) {
            suggestionSet.push_back(snChainSummarized);
        }

        if (std::find(suggestionSet.begin(), suggestionSet.end(), acylChainLengthSummarized) == suggestionSet.end()) {
            suggestionSet.push_back(acylChainLengthSummarized);
        }

        if (std::find(suggestionSet.begin(), suggestionSet.end(), acylChainCompositionSummarized) == suggestionSet.end()) {
            suggestionSet.push_back(acylChainCompositionSummarized);
        }

        if (std::find(suggestionSet.begin(), suggestionSet.end(), lipidClassSummarized) == suggestionSet.end()) {
            suggestionSet.push_back(lipidClassSummarized);
        }

        for (unsigned int i = 0; i < suggestionSet.size(); i++) {

            QString hyperLink =
                    QString::fromStdString("<a href = \"") +
                    QString::fromStdString(suggestionSet.at(i).c_str()) +
                    QString::fromStdString("\" >") +
                    QString::fromStdString(suggestionSet.at(i).c_str()) +
                    QString::fromStdString("</a>");

            brsSuggestions->append(hyperLink);

        }
    }

    QString adductString = QString();
    if (selectedPeakGroup->adduct){
        adductString.append(selectedPeakGroup->adduct->name.c_str());
    }

    brsPreviousID->setText(TableDockWidget::groupTagString(selectedPeakGroup));
    brsCompound->setText(compoundString);
    brsAdduct->setText(adductString);
    brsMz->setText(QString::number(selectedPeakGroup->meanMz, 'f', 4));
    brsRT->setText(QString::number(selectedPeakGroup->meanRt, 'f', 2));
    txtUpdateID->setText(QString());
    txtNotes->setText(QString(selectedPeakGroup->notes.c_str()));

    if (selectedPeakGroup->adduct) {
        for (int i = 0; i < this->cmbSelectAdduct->count(); i++) {
            QVariant v = this->cmbSelectAdduct->itemData(i);
            Adduct*  adduct =  v.value<Adduct*>();
            if (adduct && adduct->name == selectedPeakGroup->adduct->name) {
                this->cmbSelectAdduct->setCurrentIndex(i);
                break;
            }
        }
    } else {
        cmbSelectAdduct->setCurrentIndex(0); //first item is always empty string, indicating no adduct
    }
}

void EditPeakGroupDialog::onAnchorClicked(const QUrl &link){
    this->txtUpdateID->setText(link.path());
}

void EditPeakGroupDialog::updateAdductComboBox() {

    this->cmbSelectAdduct->clear();

    this->cmbSelectAdduct->addItem("", QVariant::fromValue(nullptr));

    for (auto adduct : DB.adductsDB) {
        cmbSelectAdduct->addItem(adduct->name.c_str(), QVariant::fromValue(adduct));
    }
}

void EditPeakGroupDialog::updateAdductText() {
    this->brsAdduct->setText(cmbSelectAdduct->currentText());
}
