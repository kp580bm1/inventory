#include <QtDebug>
#include <QtCore/QSettings>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtGui/QMessageBox>
#include <QtGui/QTextEdit>
#include <QtGui/QFileDialog>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>

#include "mainwindow.h"

MainWindow::MainWindow(QWidget * parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	tabs = new QTabBar();
	tabIndex.MAIN    = tabs->addTab(tr("Main"));
	tabIndex.PRINT   = tabs->addTab(tr("Print"));
	tabIndex.TYPES   = tabs->addTab(tr("Item types"));
	tabIndex.PLACES  = tabs->addTab(tr("Places"));
	tabIndex.PERSONS = tabs->addTab(tr("Persons"));

	QBoxLayout * box = static_cast<QBoxLayout*>(ui.centralwidget->layout());
	if(box) {
		box->insertWidget(0, tabs);
	}
	connect(tabs, SIGNAL(currentChanged(int)), this, SLOT(setupTab(int)));

	// Settings.
	QSettings settings;
	resize(settings.value("mainwindow/size", size()).toSize());
	move(settings.value("mainwindow/pos", pos()).toPoint());
	if(settings.value("mainwindow/maximized", false).toBool())
		setWindowState(Qt::WindowMaximized);

	QString databaseLocation = settings.value("database/location", "").toString();
	if(databaseLocation.isEmpty()) {
		databaseLocation = QFileDialog::getSaveFileName(this, tr("Database location"),
				QDir(QApplication::applicationDirPath()).absoluteFilePath("inventory.sqlite"),
				tr("SQLite3 database files (*.sqlite);;All files (*.*)")
				); 
	}

	// App logic.
	Inventory::Database::setDatabaseName(databaseLocation);
	if(!Inventory::Database::reopen()) {
		QMessageBox::critical(this, tr("Database"), tr("Cannot open database at path '%1'!").arg(Inventory::Database::databaseName()));
		exit(1);
	}
	inventoryModel = new Inventory::InventoryModel();
	printableModel = new Inventory::PrintableInventoryModel();
	itemTypesModel = new Inventory::ReferenceModel(Inventory::ReferenceModel::ITEM_TYPES);
	placesModel    = new Inventory::ReferenceModel(Inventory::ReferenceModel::PLACES);
	personsModel   = new Inventory::ReferenceModel(Inventory::ReferenceModel::PERSONS);
	connect(inventoryModel, SIGNAL(modelReset()), this, SLOT(resetView()));
	connect(printableModel, SIGNAL(modelReset()), this, SLOT(resetView()));
	connect(itemTypesModel, SIGNAL(modelReset()), this, SLOT(resetView()));
	connect(placesModel,    SIGNAL(modelReset()), this, SLOT(resetView()));
	connect(personsModel,   SIGNAL(modelReset()), this, SLOT(resetView()));

	ui.listItemTypeFilter->setModel(itemTypesModel);
	ui.listPlaceFilter->setModel(placesModel);

	if(tabs->currentIndex() == tabIndex.MAIN) {
		setupTab(tabIndex.MAIN);
	} else {
		tabs->setCurrentIndex(tabIndex.MAIN);
	}

	// Settings again.
	ui.actionHideFilter           ->setChecked     (settings.value("filter/hidden",              ui.actionHideFilter           ->isChecked()).toBool());
	ui.buttonUseItemTypeFilter    ->setChecked     (settings.value("filter/useitemtype",         ui.buttonUseItemTypeFilter    ->isChecked()).toBool());
	ui.listItemTypeFilter         ->setCurrentIndex(settings.value("filter/itemtype",            ui.listItemTypeFilter         ->currentIndex()).toInt());
	ui.buttonUsePlaceFilter       ->setChecked     (settings.value("filter/useplace",            ui.buttonUsePlaceFilter       ->isChecked()).toBool());
	ui.listPlaceFilter            ->setCurrentIndex(settings.value("filter/place",               ui.listPlaceFilter            ->currentIndex()).toInt());
	ui.buttonUseWrittenOffFilter  ->setChecked     (settings.value("filter/usewrittenoff",       ui.buttonUseWrittenOffFilter  ->isChecked()).toBool());
	ui.listWrittenOffFilter       ->setCurrentIndex(settings.value("filter/writtenoff",          ui.listWrittenOffFilter       ->currentIndex()).toInt());

	QStringList hiddenColumnList = settings.value("ui/hiddencolumns").toString().split(",");
	foreach(QString column, hiddenColumnList) {
		QAction * action = columnAction(column.toInt() - 1);
		if(action) {
			action->setChecked(true);
		}
	}
}

MainWindow::~MainWindow()
{
	// Settings.
	QSettings settings;
	settings.setValue("mainwindow/maximized",
			windowState().testFlag(Qt::WindowMaximized));
	if(!windowState().testFlag(Qt::WindowMaximized))
	{
		settings.setValue("mainwindow/size", size());
		settings.setValue("mainwindow/pos", pos());
	}
	settings.setValue("database/location", Inventory::Database::databaseName());

	settings.setValue("filter/hidden",              ui.actionHideFilter           ->isChecked());
	settings.setValue("filter/useitemtype",         ui.buttonUseItemTypeFilter    ->isChecked());
	settings.setValue("filter/itemtype",            ui.listItemTypeFilter         ->currentIndex());
	settings.setValue("filter/useplace",            ui.buttonUsePlaceFilter       ->isChecked());
	settings.setValue("filter/place",               ui.listPlaceFilter            ->currentIndex());
	settings.setValue("filter/usewrittenoff",       ui.buttonUseWrittenOffFilter  ->isChecked());
	settings.setValue("filter/writtenoff",          ui.listWrittenOffFilter       ->currentIndex());

	QStringList hiddenColumnList;
	foreach(int column, hiddenColumns) {
		hiddenColumnList << QString::number(column + 1);
	}
	settings.setValue("ui/hiddencolumns", hiddenColumnList.join(","));

	// Database.
	Inventory::Database::close();
}

QAction * MainWindow::columnAction(int column)
{
	switch(column) {
		case 0 : return ui.actionHideItemType;
		case 1 : return ui.actionHideItemPlace;
		case 2 : return ui.actionHideResponsiblePerson;
		case 3 : return ui.actionHideItemName;
		case 4 : return ui.actionHideINN;
		case 5 : return ui.actionHideWritingOff;
		case 6 : return ui.actionHideRepairState;
		case 7 : return ui.actionHideCheckedItemState;
		case 8 : return ui.actionHideNote;
		default: break;
	}
	return NULL;
}

void MainWindow::on_actionHideItemType_toggled(bool hidden)
{
	hideColumn(0, hidden);
}

void MainWindow::on_actionHideItemPlace_toggled(bool hidden)
{
	hideColumn(1, hidden);
}

void MainWindow::on_actionHideResponsiblePerson_toggled(bool hidden)
{
	hideColumn(2, hidden);
}

void MainWindow::on_actionHideItemName_toggled(bool hidden)
{
	hideColumn(3, hidden);
}

void MainWindow::on_actionHideINN_toggled(bool hidden)
{
	hideColumn(4, hidden);
}

void MainWindow::on_actionHideWritingOff_toggled(bool hidden)
{
	hideColumn(5, hidden);
}

void MainWindow::on_actionHideRepairState_toggled(bool hidden)
{
	hideColumn(6, hidden);
}

void MainWindow::on_actionHideCheckedItemState_toggled(bool hidden)
{
	hideColumn(7, hidden);
}

void MainWindow::on_actionHideNote_toggled(bool hidden)
{
	hideColumn(8, hidden);
}

void MainWindow::hideColumn(int column, bool hidden)
{
	if(hidden) {
		hiddenColumns += column;
	} else {
		hiddenColumns -= column;
	}
	ui.view->setColumnHidden(column, hidden);
}

void MainWindow::setupTab(int index)
{
	ui.actionShowHistory ->setEnabled(index == tabIndex.MAIN);
	ui.actionAddMultiline->setEnabled(index == tabIndex.TYPES || index == tabIndex.PLACES || index == tabIndex.PERSONS);
	ui.actionAdd         ->setEnabled(index != tabIndex.PRINT);
	ui.actionRemove      ->setEnabled(index != tabIndex.PRINT);
	ui.actionHideFilter  ->setEnabled(index == tabIndex.MAIN || index == tabIndex.PRINT);
	ui.actionUndo        ->setEnabled(index == tabIndex.MAIN);

	ui.filterBox->setVisible(ui.actionHideFilter->isEnabled() && !ui.actionHideFilter->isChecked());

	if     (index == tabIndex.MAIN)    ui.view->setModel(inventoryModel);
	else if(index == tabIndex.PRINT)   ui.view->setModel(printableModel);
	else if(index == tabIndex.TYPES)   ui.view->setModel(itemTypesModel);
	else if(index == tabIndex.PLACES)  ui.view->setModel(placesModel);
	else if(index == tabIndex.PERSONS) ui.view->setModel(personsModel);

	resetView(true);

	for(int column = 0; column < ui.view->model()->columnCount(); ++column) {
		if(index == tabIndex.MAIN && hiddenColumns.contains(column)) {
			ui.view->setColumnHidden(column, true);
		} else {
			ui.view->setColumnHidden(column, false);
		}
	}
	if(index == tabIndex.MAIN) {
		ui.view->setItemDelegate(new Inventory::InventoryDelegate(this));
	} else {
		ui.view->setItemDelegate(new QItemDelegate(this));
	}
}

void MainWindow::resetView(bool update)
{
	ui.view->resizeColumnsToContents();
	ui.view->horizontalHeader()->setStretchLastSection(true);
	if(update && ui.view->model()) {
		Inventory::AbstractUpdatableTableModel * model = qobject_cast<Inventory::AbstractUpdatableTableModel *>(ui.view->model());
		if(model) {
			model->update();
		}
	}
}

void MainWindow::on_actionUndo_triggered()
{
	if(ui.view->model() != inventoryModel)
		return;

	int row = ui.view->currentIndex().isValid() ? ui.view->currentIndex().row() : -1;
	if(row < 0)
		return;

	Inventory::Id id = inventoryModel->idAt(row);
	QScopedPointer<Inventory::HistoryModel> model(new Inventory::HistoryModel(id));
	if(model->rowCount() == 0)
		return;

	QString message = tr("Are you sure want to undo this change?") + "\n";
	int lastChangeRow = model->rowCount() - 1;
	message += tr("Change time: %1").arg(model->data(model->index(lastChangeRow, 0)).toString()) + "\n";
	message += tr("Field name: %1").arg(model->data(model->index(lastChangeRow, 1)).toString()) + "\n";
	message += tr("Old value: %1").arg(model->data(model->index(lastChangeRow, 2)).toString()) + "\n";
	message += tr("New value: %1").arg(model->data(model->index(lastChangeRow, 3)).toString()) + "\n";

	if(QMessageBox::question(this, tr("Undo change"), message, QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes) {
		bool ok = model->removeRow(lastChangeRow);
		if(ok) {
			inventoryModel->update();
		}
	}
}

void MainWindow::on_actionShowHistory_triggered()
{
	if(ui.view->model() != inventoryModel)
		return;

	int row = ui.view->currentIndex().isValid() ? ui.view->currentIndex().row() : -1;
	if(row < 0)
		return;

	Inventory::Id id = inventoryModel->idAt(row);
	QScopedPointer<Inventory::HistoryModel> model(new Inventory::HistoryModel(id));

	QDialog dialog(this);
		QVBoxLayout * vbox = new QVBoxLayout();
			QTableView * view = new QTableView();
				view->setModel(&(*model));
			vbox->addWidget(view);
			QDialogButtonBox * buttons = new QDialogButtonBox(QDialogButtonBox::Close);
				connect(buttons, SIGNAL(accepted()), &dialog, SLOT(accept()));
				connect(buttons, SIGNAL(rejected()), &dialog, SLOT(reject()));
			vbox->addWidget(buttons);
	dialog.setLayout(vbox);
	dialog.setSizeGripEnabled(true);
	dialog.exec();
}

void MainWindow::on_actionAddMultiline_triggered()
{
	if(tabs->currentIndex() != tabIndex.TYPES && tabs->currentIndex() != tabIndex.PLACES && tabs->currentIndex() != tabIndex.PERSONS)
		return;

	QDialog dialog(this);
		QVBoxLayout * vbox = new QVBoxLayout();
			QTextEdit * edit = new QTextEdit();
			vbox->addWidget(edit);
			QDialogButtonBox * buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
				connect(buttons, SIGNAL(accepted()), &dialog, SLOT(accept()));
				connect(buttons, SIGNAL(rejected()), &dialog, SLOT(reject()));
			vbox->addWidget(buttons);
	dialog.setLayout(vbox);
	dialog.setSizeGripEnabled(true);
	if(dialog.exec() == QDialog::Accepted) {
		QStringList lines = edit->toPlainText().split('\n');

		Inventory::ReferenceModel * model = qobject_cast<Inventory::ReferenceModel *>(ui.view->model());
		if(model) {
			bool success = model->addMultiline(lines);
			if(!success) {
				QMessageBox::critical(this, tr("Add lines"), tr("Adding of lines failed!"));
			}
		}
	}
}

void MainWindow::on_actionPrintCSV_triggered()
{
	if(!ui.view->model())
		return;

	QString csvFileName = QFileDialog::getSaveFileName(this, tr("Save to CSV..."), 0, tr("CSV files (*.csv);;All files (*.*)"));
	if(csvFileName.isEmpty())
		return;

	QFile file(csvFileName);
	if(!file.open(QFile::Text | QFile::WriteOnly)) {
		QMessageBox::critical(this, tr("Save to CSV"), tr("Cannot open file '%1' for write!").arg(file.fileName()));
		return;
	}

	QTextStream out(&file);
	out.setCodec("UTF-8");
	QAbstractItemModel * model = ui.view->model();

	QStringList header;
	for(int col = 0; col < model->columnCount(); ++col) {
		QString text = model->headerData(col, Qt::Horizontal).toString();
		text.replace("\"", "\\\"");
		text.append('"').prepend('"');
		header << text;
	}
	out << header.join(", ") << endl;

	for(int row = 0; row < model->rowCount(); ++row) {
		QStringList cells;
		for(int col = 0; col < model->columnCount(); ++col) {
			QString text = model->data(model->index(row, col)).toString();
			text.replace("\"", "\\\"");
			text.append('"').prepend('"');
			cells << text;
		}
		out << cells.join(", ") << endl;
	}
}

void MainWindow::on_actionAdd_triggered()
{
	ui.view->model()->insertRow(ui.view->model()->rowCount());
}

void MainWindow::on_actionRemove_triggered()
{
	if(!ui.view->model())
		return;

	int row = ui.view->currentIndex().isValid() ? ui.view->currentIndex().row() : -1;
	if(row < 0)
		return;

	bool removed = !ui.view->model()->removeRow(row);
	if(removed) {
		if(tabs->currentIndex() == tabIndex.TYPES || tabs->currentIndex() == tabIndex.PLACES || tabs->currentIndex() == tabIndex.PERSONS) {
			QMessageBox::information(this, tr("Remove record"), tr("Cannot remove record. Probably, there are items that are using it."));
		}
	}
}

void MainWindow::on_buttonUseItemTypeFilter_toggled(bool value)
{
	inventoryModel->switchItemTypeFilter(value);
	printableModel->switchItemTypeFilter(value);
}

void MainWindow::on_listItemTypeFilter_currentIndexChanged(int index)
{
	Inventory::Id value = itemTypesModel->idAt(index);
	inventoryModel->setItemTypeFilter(value);
	printableModel->setItemTypeFilter(value);
}

void MainWindow::on_buttonUsePlaceFilter_toggled(bool value)
{
	inventoryModel->switchPlaceFilter(value);
	printableModel->switchPlaceFilter(value);
}

void MainWindow::on_listPlaceFilter_currentIndexChanged(int index)
{
	Inventory::Id value = placesModel->idAt(index);
	inventoryModel->setPlaceFilter(value);
	printableModel->setPlaceFilter(value);
}

void MainWindow::on_buttonUseWrittenOffFilter_toggled(bool value)
{
	inventoryModel->switchWrittenOffFilter(value);
	printableModel->switchWrittenOffFilter(value);
}

void MainWindow::on_listWrittenOffFilter_currentIndexChanged(int index)
{
	bool value = (index == 1);
	inventoryModel->setWrittenOffFilter(value);
	printableModel->setWrittenOffFilter(value);
}

