#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("Finance Tracker SQLite Edition");

    dbFileName = "finance.db";
    QStringList incomeList{"Salary", "Prize", "Other income"};
    QStringList expenseList{"Auto", "Products", "Utility payments", "Other expense"};

    ui->dateEdit->setDate(QDate::currentDate());

    connectDb(dbFileName);

    connect(ui->searchEdit, &QLineEdit::textChanged, this, [=](const QString &str){
        dbProxy->setFilterKeyColumn(-1);
        dbProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
        dbProxy->setFilterFixedString(str);

        double total = 0.0;

        for(int i = 0; i < dbProxy->rowCount(); ++i) {
            total += dbProxy->data(dbProxy->index(i, 4)).toDouble();
        }

        ui->statusbar->showMessage("Balance: " + QString::number(total, 'f', 2));
    });

    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &MainWindow::addEntry);
    connect(ui->searchEdit, &QLineEdit::returnPressed, this, &MainWindow::showChart);

    connect(ui->addButton, &QPushButton::clicked, this, [=](){
        addEntry();
    });

    connect(ui->deleteButton, &QPushButton::clicked, this, [=](){
        deleteEntry();
    });

    connect(ui->incomeRadioButton, &QRadioButton::toggled, this, [=](bool checked){
        if(checked) {
            ui->categoryComboBox->clear();
            ui->categoryComboBox->addItems(incomeList);
        }
    });

    connect(ui->expenseRadioButton, &QRadioButton::toggled, this, [=](bool checked){
        if(checked) {
            ui->categoryComboBox->clear();
            ui->categoryComboBox->addItems(expenseList);
        }
    });
}

void MainWindow::connectDb(const QString &f) {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbFileName);

    if(!db.open())
        qDebug() << db.lastError().text();

    QSqlQuery querySql;

    if(!querySql.exec("CREATE TABLE IF NOT EXISTS finance "
                                   "(id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                   "date TEXT, "
                                   "type_id INTEGER, "
                                   "category_id INTEGER, "
                                   "amount REAL)"))
        qDebug() << db.lastError().text();

    if(!querySql.exec("CREATE TABLE IF NOT EXISTS type "
                        "(id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "type TEXT NOT NULL)"))
        qDebug() << db.lastError().text();

    if(!querySql.exec("CREATE TABLE IF NOT EXISTS category "
                        "(id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "category TEXT NOT NULL)"))
        qDebug() << db.lastError().text();

    QSqlQuery queryCheck("SELECT COUNT(*) FROM type");
    if(queryCheck.next() && queryCheck.value(0).toInt() == 0) {
        querySql.exec("INSERT INTO type (type) VALUES ('Income'), ('Expense')");
        // И добавь стандартные категории в таблицу category
        querySql.exec("INSERT INTO category (category) VALUES ('Salary'), ('Prize'), ('Other income'), ('Products'), ('Auto') , ('Utility payments'), ('Other expense')"); //
    }

    auto *dbRelationModel = new QSqlRelationalTableModel(this);
    dbRelationModel->setTable("finance");
    dbRelationModel->setRelation(2, QSqlRelation("type", "id", "type"));
    dbRelationModel->setRelation(3, QSqlRelation("category", "id", "category"));

    dbModel = dbRelationModel;
    //dbModel->setTable("finance");

    dbProxy = new QSortFilterProxyModel(this);
    dbProxy->setSourceModel(dbModel);
    dbProxy->setDynamicSortFilter(true);

    ui->tableView->setModel(dbProxy);
    dbModel->select();

    ui->tableView->setColumnHidden(0, true);
    ui->tableView->setSortingEnabled(true);
    ui->tableView->sortByColumn(0, Qt::SortOrder::AscendingOrder);

    ui->statusbar->showMessage("Balance: " + QString::number(calculateBalance(), 'f', 2));
}

void MainWindow::addEntry() {
    QSqlQuery query;
    QDate date = ui->dateEdit->date();
    QString type = ui->incomeRadioButton->isChecked() ? "Income" : "Expense";
    QString category = ui->categoryComboBox->currentText();
    QString amountText = ui->lineEdit->text();

    if(amountText.isEmpty())
        return;

    double amount = amountText.toDouble();
    double balance = calculateBalance();

    if(ui->expenseRadioButton->isChecked())
        amount = -std::abs(amountText.toDouble());

    if(type == "Expense" && amount > balance) {
        ui->statusbar->showMessage("Insufficient funds. Balance: " + QString::number(balance, 'f', 2));
        ui->lineEdit->clear();
        return;
    }

    QString dateText = date.toString("dd.MM.yyyy");

    query.prepare("INSERT INTO finance (date, type_id, category_id, amount)"
                  "VALUES (:date, (SELECT id FROM type WHERE type=:type), (SELECT id FROM category WHERE category=:category), :amount)");
    query.bindValue(":date", dateText);
    query.bindValue(":type", type);
    query.bindValue(":category", category);
    query.bindValue(":amount", amount);

    if(!query.exec())
        qDebug() << db.lastError().text();

    dbModel->select();
    ui->statusbar->showMessage("Balance: " + QString::number(calculateBalance(), 'f', 2));

    ui->lineEdit->clear();
}

void MainWindow::deleteEntry() {
    if(!ui->tableView->currentIndex().isValid())
        return;

    QSqlQuery query;
    QModelIndex indexProxy = ui->tableView->currentIndex();

    QSortFilterProxyModel *modelProxy = qobject_cast<QSortFilterProxyModel*>(ui->tableView->model());
    QModelIndex index = modelProxy->mapToSource(indexProxy);

    int id = index.model()->data(index.model()->index(index.row(), 0)).toInt();

    qDebug() << index << id;

    query.prepare("DELETE FROM finance WHERE id = :id");
    query.bindValue(":id", id);

    if(!query.exec())
        qDebug() << db.lastError().text();
    else
        dbModel->select();

    ui->statusbar->showMessage("Balance: " + QString::number(calculateBalance(), 'f', 2));

    ui->lineEdit->clear();
}

double MainWindow::calculateBalance() const {
    QSqlQuery query;
    double total = 0.0;
    query.exec("SELECT SUM(amount) FROM finance");
    if(query.next())
        total = query.value(0).toDouble();

    return total;
}

void MainWindow::showChart() {
    QSqlQuery query;
    auto *series = new QPieSeries;

    /*for(int i = 0; i < dbProxy->rowCount(); ++i) {
        series->append(dbProxy->data(dbProxy->index(i, 3)).toString(), dbProxy->data(dbProxy->index(i, 4)).toDouble());
    }*/

    query.prepare("SELECT c.category, SUM(ABS(f.amount)) FROM finance f JOIN category c ON f.category_id = c.id WHERE f.amount < 0 GROUP BY category");
    if(query.exec()) {
        while(query.next())
            series->append(query.value(0).toString(), query.value(1).toDouble());
    } else
        qDebug() << query.lastError().text();

    for(auto *slice : series->slices()) {
        slice->setLabelPosition(QPieSlice::LabelOutside);

        connect(slice, &QPieSlice::hovered, [slice](bool hovered){
            slice->setExploded(hovered);
            slice->setLabelVisible(hovered);
        });
    }

    auto *chart = new QChart;
    chart->addSeries(series);
    chart->legend()->setAlignment(Qt::AlignLeft);

    auto *chartView = new QChartView(chart);
    chartView->resize(400, 400);
    chartView->setAttribute(Qt::WA_DeleteOnClose);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->show();
}

MainWindow::~MainWindow() {
    delete ui;
}
